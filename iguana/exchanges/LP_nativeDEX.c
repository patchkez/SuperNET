
/******************************************************************************
 * Copyright © 2014-2017 The SuperNET Developers.                             *
 *                                                                            *
 * See the AUTHORS, DEVELOPER-AGREEMENT and LICENSE files at                  *
 * the top-level directory of this distribution for the individual copyright  *
 * holder information and the developer policies on copyright and licensing.  *
 *                                                                            *
 * Unless otherwise agreed in a custom licensing agreement, no part of the    *
 * SuperNET software, including this file may be copied, modified, propagated *
 * or distributed except according to the terms contained in the LICENSE file *
 *                                                                            *
 * Removal or modification of this copyright notice is prohibited.            *
 *                                                                            *
 ******************************************************************************/
//
//  LP_nativeDEX.c
//  marketmaker
//
// swap cancel should cleanly cancel
// as much as possible on a unidirectional basis
// stats
// auto-utxo creation
// multiple smartaddresses? enable/disable causes? 250 KMD bounty
// verify bid volumes

#include <stdio.h>
#include "LP_include.h"
portable_mutex_t LP_peermutex,LP_UTXOmutex,LP_utxomutex,LP_commandmutex,LP_cachemutex,LP_swaplistmutex,LP_forwardmutex,LP_pubkeymutex,LP_networkmutex,LP_psockmutex,LP_coinmutex;
int32_t LP_canbind;

struct LP_utxoinfo  *LP_utxoinfos[2],*LP_utxoinfos2[2];
struct LP_peerinfo  *LP_peerinfos,*LP_mypeer;
struct LP_forwardinfo *LP_forwardinfos;
struct iguana_info *LP_coins;
#include "LP_network.c"

char *activecoins[] = { "BTC", "KMD" };
char GLOBAL_DBDIR[] = { "DB" };
char USERPASS[65],USERPASS_WIFSTR[64],LP_myipaddr[64],LP_publicaddr[64],USERHOME[512] = { "/root" };
char LP_gui[16] = { "cli" };

char *default_LPnodes[] = { "5.9.253.195", "5.9.253.196", "5.9.253.197", "5.9.253.198", "5.9.253.199", "5.9.253.200", "5.9.253.201", "5.9.253.202", "5.9.253.203", };//"5.9.253.204" }; //

//uint32_t LP_deadman_switch;
uint16_t LP_fixed_pairport,LP_publicport;
int32_t LP_mybussock = -1;
int32_t LP_mypubsock = -1;
int32_t LP_mypullsock = -1;
int32_t USERPASS_COUNTER,IAMLP = 0;
double LP_profitratio = 1.;
bits256 LP_mypub25519,LP_mypriv25519;

// stubs

void tradebot_swap_balancingtrade(struct basilisk_swap *swap,int32_t iambob)
{
    
}

void tradebot_pendingadd(cJSON *tradejson,char *base,double basevolume,char *rel,double relvolume)
{
    // add to trades
}

char *LP_getdatadir()
{
    return(USERHOME);
}

char *blocktrail_listtransactions(char *symbol,char *coinaddr,int32_t num,int32_t skip)
{
    return(0);
}

#include "LP_secp.c"
#include "LP_bitcoin.c"
#include "LP_coins.c"
#include "LP_rpc.c"
#include "LP_prices.c"
#include "LP_scan.c"
#include "LP_transaction.c"
#include "LP_remember.c"
#include "LP_swap.c"
#include "LP_peers.c"
#include "LP_utxos.c"
#include "LP_forwarding.c"
#include "LP_ordermatch.c"
#include "LP_commands.c"

char *LP_command_process(void *ctx,char *myipaddr,int32_t pubsock,cJSON *argjson,uint8_t *data,int32_t datalen,double profitmargin)
{
    char *retstr=0;
    if ( jobj(argjson,"result") != 0 || jobj(argjson,"error") != 0 )
        return(0);
    if ( LP_tradecommand(ctx,myipaddr,pubsock,argjson,data,datalen,profitmargin) <= 0 )
    {
        if ( (retstr= stats_JSON(ctx,myipaddr,pubsock,profitmargin,argjson,"127.0.0.1",0)) != 0 )
        {
            //printf("%s PULL.[%d]-> (%s)\n",myipaddr != 0 ? myipaddr : "127.0.0.1",datalen,retstr);
            //if ( pubsock >= 0 ) //strncmp("{\"error\":",retstr,strlen("{\"error\":")) != 0 &&
                //LP_send(pubsock,retstr,(int32_t)strlen(retstr)+1,0);
        }
    }
    return(retstr);
}

int32_t LP_crc32find(int32_t *duplicatep,int32_t ind,uint32_t crc32)
{
    static uint32_t crcs[8192]; static unsigned long dup,total;
    int32_t i;
    *duplicatep = 0;
    if ( ind < 0 )
    {
        total++;
        for (i=0; i<sizeof(crcs)/sizeof(*crcs); i++)
        {
            if ( crc32 == crcs[i] )
            {
                if ( i > 0 )
                {
                    crcs[i] = crcs[i >> 1];
                    crcs[i >> 1] = crc32;
                    dup++;
                    printf("duplicate %08x in slot %d -> slot %d (%lu / %lu)\n",crc32,i,i>>1,dup,total);
                }
                *duplicatep = 1;
                break;
            }
            else if ( crcs[i] == 0 )
                break;
        }
        if ( i >= sizeof(crcs)/sizeof(*crcs) )
            i = (rand() % (sizeof(crcs)/sizeof(*crcs)));
        return(i);
    }
    else
    {
        crcs[ind] = crc32;
        return(ind);
    }
}

char *LP_process_message(void *ctx,char *typestr,char *myipaddr,int32_t pubsock,double profitmargin,uint8_t *ptr,int32_t recvlen,int32_t recvsock)
{
    int32_t i,len,datalen=0,duplicate=0,encrypted=0; char *retstr=0,*jsonstr=0; cJSON *argjson; uint32_t crc32; uint8_t decoded[LP_ENCRYPTED_MAXSIZE + crypto_box_ZEROBYTES];
    crc32 = calc_crc32(0,&ptr[2],recvlen-2);
    if ( (crc32 & 0xff) == ptr[0] && ((crc32>>8) & 0xff) == ptr[1] )
        encrypted = 1;
    printf("recv.%u\n",crc32);
    portable_mutex_lock(&LP_commandmutex);
    i = LP_crc32find(&duplicate,-1,crc32);
    if ( duplicate == 0 )
    {
        LP_crc32find(&duplicate,i,crc32);
        if ( encrypted != 0 )
        {
            uint8_t *nonce,*cipher; int32_t cipherlen;
            nonce = &ptr[2];
            cipher = &ptr[2 + crypto_box_NONCEBYTES];
            cipherlen = recvlen - (2 + crypto_box_NONCEBYTES);
            if ( cipherlen > 0 && cipherlen < sizeof(decoded) )
            {
                if ( (jsonstr= (char *)_SuperNET_decipher(nonce,cipher,decoded,cipherlen,GENESIS_PUBKEY,LP_mypriv25519)) != 0 )
                {
                    recvlen = (cipherlen - crypto_box_ZEROBYTES);
                    if ( strlen(jsonstr)+1 != recvlen )
                    {
                        printf("unexpected len %d vs recvlen.%d\n",(int32_t)strlen(jsonstr)+1,recvlen);
                        jsonstr = 0;
                    } else printf("decrypted (%s)\n",jsonstr);
                }
            } else printf("cipher.%d too big for %d\n",cipherlen,(int32_t)sizeof(decoded));
        }
        else if ( (datalen= is_hexstr((char *)ptr,0)) > 0 )
        {
            datalen >>= 1;
            jsonstr = malloc(datalen + 1);
            decode_hex((void *)jsonstr,datalen,(char *)ptr);
            jsonstr[datalen] = 0;
        } else jsonstr = (char *)ptr;
        // "encrypted" "cipherstr" case!
        if ( jsonstr != 0 && (argjson= cJSON_Parse(jsonstr)) != 0 )
        {
            len = (int32_t)strlen(jsonstr) + 1;
            /*if ( jstr(argjson,"method") != 0 && strcmp(jstr(argjson,"method"),"forwardhex") == 0 )
             {
             //printf("got forwardhex\n");
             if ( (retstr= LP_forwardhex(ctx,pubsock,jbits256(argjson,"pubkey"),jstr(argjson,"hex"))) != 0 )
             {
             }
             }
             else if ( jstr(argjson,"method") != 0 && strcmp(jstr(argjson,"method"),"publish") == 0 )
             {
             printf("got publish\n");
             if ( jobj(argjson,"method2") != 0 )
             jdelete(argjson,"method2");
             jaddstr(argjson,"method2","broadcast");
             if ( pubsock >= 0 && (reqjson= LP_dereference(argjson,"publish")) != 0 )
             {
             msg = jprint(reqjson,1);
             /LP_send(pubsock,msg,(int32_t)strlen(msg)+1,1);
             }
             }
             else*/ if ( (retstr= LP_command_process(ctx,myipaddr,pubsock,argjson,&((uint8_t *)ptr)[len],recvlen - len,profitmargin)) != 0 )
             {
             }
            /*if ( LP_COMMAND_RECVSOCK == NN_REP )
             {
             if ( retstr != 0 )
             {
             if ( strcmp("PULL",typestr) == 0 )
             {
             printf("%d got REQ.(%s) -> (%s)\n",recvsock,jprint(argjson,0),retstr);
             /LP_send(recvsock,retstr,(int32_t)strlen(retstr)+1,0);
             }
             }
             else if ( strcmp("PULL",typestr) == 0 )
             {
             printf("%d got REQ.(%s) -> null\n",recvsock,jprint(argjson,0));
             /LP_send(recvsock,"{\"result\":null}",(int32_t)strlen("{\"result\":null}")+1,0);
             }
             }*/
            free_json(argjson);
        } else printf("error parsing(%s)\n",jsonstr);
    } else printf("DUPLICATE.(%s)\n",(char *)ptr);
    portable_mutex_unlock(&LP_commandmutex);
    if ( jsonstr != 0 && (void *)jsonstr != (void *)ptr && (void *)jsonstr != (void *)decoded )
        free(jsonstr);
    if ( ptr != 0 )
        nn_freemsg(ptr), ptr = 0;
    return(retstr);
}

/*int32_t LP_pullsock_check(void *ctx,char **retstrp,char *myipaddr,int32_t pubsock,int32_t pullsock,double profitmargin)
{
    void *ptr; int32_t recvlen=-1,nonz = 0;
    *retstrp = 0;
    if ( pullsock >= 0 )
    {
        while ( (recvlen= nn_recv(pullsock,&ptr,NN_MSG,0)) > 0 )
        {
            nonz++;
            *retstrp = LP_process_message(ctx,"PULL",myipaddr,pubsock,profitmargin,ptr,recvlen,pullsock);
        }
    }
    return(nonz);
}*/

void LP_utxo_spentcheck(int32_t pubsock,struct LP_utxoinfo *utxo,double profitmargin)
{
    struct _LP_utxoinfo u; char str[65]; uint32_t now = (uint32_t)time(NULL);
    //printf("%s lag.%d\n",bits256_str(str,utxo->txid),now-utxo->lastspentcheck);
    if ( utxo->T.spentflag == 0 && now > utxo->T.lastspentcheck+60 )
    {
        u = (utxo->iambob != 0) ? utxo->deposit : utxo->fee;
        utxo->T.lastspentcheck = now;
        if ( LP_txvalue(0,utxo->coin,utxo->payment.txid,utxo->payment.vout) == 0 )
        {
            printf("txid.%s %s/v%d %.8f has been spent\n",utxo->coin,bits256_str(str,utxo->payment.txid),utxo->payment.vout,dstr(utxo->payment.value));
            LP_spentnotify(utxo,0);
        }
        else if ( LP_txvalue(0,utxo->coin,u.txid,u.vout) == 0 )
        {
            printf("txid2.%s %s/v%d %.8f has been spent\n",utxo->coin,bits256_str(str,u.txid),u.vout,dstr(u.value));
            LP_spentnotify(utxo,1);
        }
    }
}

void LP_myutxo_updates(void *ctx,int32_t pubsock,char *passphrase,double profitmargin)
{
    //LP_utxopurge(0); not good to disrupt existing pointers
    LP_privkey_updates(ctx,pubsock,passphrase,0);
}

int32_t LP_peer_utxosquery(struct LP_peerinfo *mypeer,uint16_t myport,int32_t pubsock,struct LP_peerinfo *peer,uint32_t now,double profitmargin,int32_t interval)
{
    int32_t lastn,n = -1;
    if ( peer->lastutxos < now-interval )
    {
        //lastn = peer->numutxos - mypeer->numutxos + LP_PROPAGATION_SLACK;
        //if ( lastn < LP_PROPAGATION_SLACK * 2 )
        lastn = LP_PROPAGATION_SLACK * 2;
        if ( mypeer == 0 || strcmp(peer->ipaddr,mypeer->ipaddr) != 0 )
        {
            peer->lastutxos = now;
            //printf("query utxos from %s\n",peer->ipaddr);
            n = LP_utxosquery(mypeer,pubsock,peer->ipaddr,peer->port,"",lastn,mypeer != 0 ? mypeer->ipaddr : "127.0.0.1",myport,profitmargin);
        }
    } //else printf("LP_peer_utxosquery skip.(%s) %u\n",peer->ipaddr,peer->lastutxos);
    return(n);
}

int32_t LP_sock_check(char *typestr,void *ctx,char *myipaddr,int32_t pubsock,int32_t sock,double profitmargin)
{
    int32_t recvlen=1,nonz = 0; void *ptr; char *retstr; struct nn_pollfd pfd;
    if ( sock >= 0 )
    {
        while ( nonz < 10 && recvlen > 0 )
        {
            memset(&pfd,0,sizeof(pfd));
            pfd.fd = sock;
            pfd.events = NN_POLLIN;
            //if ( nn_poll(&pfd,1,1) != 1 )
            //    break;
            if ( (recvlen= nn_recv(sock,&ptr,NN_MSG,0)) > 0 )
            {
                nonz++;
                if ( (retstr= LP_process_message(ctx,typestr,myipaddr,pubsock,profitmargin,ptr,recvlen,sock)) != 0 )
                    free(retstr);
            }
        }
    }
    return(nonz);
}

void command_rpcloop(void *myipaddr)
{
    int32_t nonz = 0; char *origipaddr; struct iguana_info *coin,*ctmp; struct LP_peerinfo *peer,*tmp; void *ctx;
    ctx = bitcoin_ctx();
    if ( (origipaddr= myipaddr) == 0 )
        origipaddr = "127.0.0.1";
    while ( 1 )
    {
        nonz = 0;
        HASH_ITER(hh,LP_peerinfos,peer,tmp)
        {
            if ( peer->errors >= LP_MAXPEER_ERRORS )
            {
                if ( (rand() % 10000) == 0 )
                    peer->errors--;
                else continue;
            }
            nonz += LP_sock_check("PULL",ctx,origipaddr,LP_mypubsock,peer->subsock,LP_profitratio - 1.);
        }
        HASH_ITER(hh,LP_coins,coin,ctmp) // firstrefht,firstscanht,lastscanht
        {
            if ( coin->bussock >= 0 )
                nonz += LP_sock_check(coin->symbol,ctx,origipaddr,-1,coin->bussock,LP_profitratio - 1.);
        }
        if ( LP_mypullsock >= 0 )
            nonz += LP_sock_check("SUB",ctx,origipaddr,-1,LP_mypullsock,LP_profitratio - 1.);
        if ( LP_mybussock >= 0 )
            nonz += LP_sock_check("BUS",ctx,origipaddr,-1,LP_mybussock,LP_profitratio - 1.);
        if ( nonz == 0 )
            usleep(10000);
    }
}

int32_t LP_mainloop_iter(void *ctx,char *myipaddr,struct LP_peerinfo *mypeer,int32_t pubsock,char *pushaddr,uint16_t myport,char *passphrase,double profitmargin)
{
    static uint32_t counter,numpeers; //lastforward
    struct LP_utxoinfo *utxo,*utmp; struct iguana_info *coin,*ctmp; char *retstr,*origipaddr; struct LP_peerinfo *peer,*tmp; uint32_t now; int32_t nonz = 0,n=0,lastn=-1;
    now = (uint32_t)time(NULL);
    if ( (origipaddr= myipaddr) == 0 )
        origipaddr = "127.0.0.1";
    if ( mypeer == 0 )
        myipaddr = "127.0.0.1";
    //if ( LP_canbind == 0 ) printf("counter.%d canbind.%d peers\n",counter,LP_canbind);
    numpeers = LP_numpeers();
    HASH_ITER(hh,LP_peerinfos,peer,tmp)
    {
        if ( peer->errors >= LP_MAXPEER_ERRORS )
        {
            if ( (rand() % 10000) == 0 )
                peer->errors--;
            else continue;
        }
        if ( now > peer->lastpeers+60 && peer->numpeers > 0 && (peer->numpeers != numpeers || (rand() % 10000) == 0) )
        {
            if ( IAMLP != 0 )
                printf("numpeers.%d updatepeer.%s lag.%d\n",numpeers,peer->ipaddr,now-peer->lastpeers);
            peer->lastpeers = now;
            if ( IAMLP != 0 && peer->numpeers != numpeers )
                printf("%s num.%d vs %d\n",peer->ipaddr,peer->numpeers,numpeers);
            if ( strcmp(peer->ipaddr,myipaddr) != 0 )
                LP_peersquery(mypeer,pubsock,peer->ipaddr,peer->port,myipaddr,myport,profitmargin);
        }
        if ( peer->diduquery == 0 )
        {
            if ( lastn != n || n < 20 )
            {
                lastn = n;
                n = LP_peer_utxosquery(mypeer,myport,pubsock,peer,now,profitmargin,60);
            }
            LP_peer_pricesquery(peer->ipaddr,peer->port);
            peer->diduquery = now;
        }
    }
    if ( (counter % 600) == 10 )
    {
        LP_myutxo_updates(ctx,pubsock,passphrase,profitmargin);
        /*if ( lastforward < now-3600 )
        {
            if ( (retstr= LP_registerall(0)) != 0 )
                free(retstr);
            //LP_forwarding_register(LP_mypubkey,pushaddr,pushport,10);
            lastforward = now;
        }*/
        HASH_ITER(hh,LP_utxoinfos[0],utxo,utmp)
        {
            LP_utxo_spentcheck(pubsock,utxo,profitmargin);
        }
        HASH_ITER(hh,LP_utxoinfos[1],utxo,utmp)
        {
            LP_utxo_spentcheck(pubsock,utxo,profitmargin);
            if ( utxo->T.spentflag == 0 && utxo->T.lasttime == 0 )
                LP_utxo_clientpublish(utxo);
        }
    }
    //if ( IAMLP != 0 && (counter % 600) == 42 )
    //    LP_hellos();
    /*if ( 0 && LP_canbind == 0 && (counter % (PSOCK_KEEPALIVE*MAINLOOP_PERSEC/2)) == 13 )
    {
        char keepalive[128];
        sprintf(keepalive,"{\"method\":\"keepalive\"}");
        //printf("send keepalive to %s pullsock.%d\n",pushaddr,pullsock);
        if ( /LP_send(pullsock,keepalive,(int32_t)strlen(keepalive)+1,0) < 0 )
        {
            //LP_deadman_switch = 0;
        }
    }*/
    HASH_ITER(hh,LP_coins,coin,ctmp) // firstrefht,firstscanht,lastscanht
    {
        cJSON *obj; int32_t height; bits256 zero;
        //printf("%s ref.%d scan.%d to %d, longest.%d\n",coin->symbol,coin->firstrefht,coin->firstscanht,coin->lastscanht,coin->longestchain);
        if ( coin->inactive != 0 )
            continue;
        memset(zero.bytes,0,sizeof(zero));
        if ( time(NULL) > coin->lastgetinfo+LP_GETINFO_INCR )
        {
            if ( (obj= LP_getinfo(coin->symbol)) != 0 )
            {
                if ( (height= jint(obj,"blocks")) > coin->longestchain )
                {
                    coin->longestchain = height;
                    if ( coin->firstrefht != 0 )
                        printf(">>>>>>>>>> set %s longestchain %d (ref.%d [%d, %d])\n",coin->symbol,height,coin->firstrefht,coin->firstscanht,coin->lastscanht);
                } else LP_mempoolscan(coin->symbol,zero);
                free_json(obj);
            } else printf("error getting info.%s\n",coin->symbol);
            coin->lastgetinfo = (uint32_t)time(NULL);
        }
        if ( coin->firstrefht == 0 )
            continue;
        else if ( coin->firstscanht == 0 )
            coin->lastscanht = coin->firstscanht = coin->firstrefht;
        else if ( coin->firstrefht < coin->firstscanht )
        {
            printf("detected %s firstrefht.%d < firstscanht.%d\n",coin->symbol,coin->firstrefht,coin->firstscanht);
            coin->lastscanht = coin->firstscanht = coin->firstrefht;
        }
        if ( coin->lastscanht == coin->longestchain+1 )
            continue;
        else if ( coin->lastscanht > coin->longestchain+1 )
        {
            printf("detected chain rewind lastscanht.%d vs longestchain.%d, first.%d ref.%d\n",coin->lastscanht,coin->longestchain,coin->firstscanht,coin->firstrefht);
            LP_undospends(coin,coin->longestchain-1);
            LP_mempoolscan(coin->symbol,zero);
            coin->lastscanht = coin->longestchain - 1;
            if ( coin->firstscanht < coin->lastscanht )
                coin->lastscanht = coin->firstscanht;
            continue;
        }
        printf("%s ref.%d scan.%d to %d, longest.%d\n",coin->symbol,coin->firstrefht,coin->firstscanht,coin->lastscanht,coin->longestchain);
        if ( LP_blockinit(coin,coin->lastscanht) < 0 )
        {
            printf("blockinit.%s %d error\n",coin->symbol,coin->lastscanht);
            continue;
        }
        coin->lastscanht++;
        break;
    }
    if ( (counter % 6000) == 60 )
    {
        if ( (retstr= basilisk_swapentry(0,0)) != 0 )
        {
            //printf("SWAPS.(%s)\n",retstr);
            free(retstr);
        }
    }
    counter++;
    return(nonz);
}

void LP_initcoins(void *ctx,int32_t pubsock,cJSON *coins,char *passphrase)
{
    int32_t i,n; cJSON *item;
    for (i=0; i<sizeof(activecoins)/sizeof(*activecoins); i++)
    {
        fprintf(stderr,"%s ",activecoins[i]);
        LP_coinfind(activecoins[i]);
        LP_priceinfoadd(activecoins[i]);
    }
    if ( (n= cJSON_GetArraySize(coins)) > 0 )
    {
        for (i=0; i<n; i++)
        {
            item = jitem(coins,i);
            fprintf(stderr,"%s ",jstr(item,"coin"));
            LP_coincreate(item);
            LP_priceinfoadd(jstr(item,"coin"));
        }
    }
    fprintf(stderr,"privkey updates\n");
    LP_privkey_updates(ctx,pubsock,passphrase,1);
}

void LP_initpeers(int32_t pubsock,struct LP_peerinfo *mypeer,char *myipaddr,uint16_t myport,char *seednode,double profitmargin)
{
    int32_t i,j; uint32_t r;
    if ( IAMLP != 0 )
    {
        LP_mypeer = mypeer = LP_addpeer(mypeer,pubsock,myipaddr,myport,0,0,profitmargin,0,0);
        if ( myipaddr == 0 || mypeer == 0 )
        {
            printf("couldnt get myipaddr or null mypeer.%p\n",mypeer);
            exit(-1);
        }
        if ( seednode == 0 || seednode[0] == 0 )
        {
            for (i=0; i<sizeof(default_LPnodes)/sizeof(*default_LPnodes); i++)
            {
                if ( (rand() % 100) > 25 )
                    continue;
                LP_peersquery(mypeer,pubsock,default_LPnodes[i],myport,mypeer->ipaddr,myport,profitmargin);
            }
        } else LP_peersquery(mypeer,pubsock,seednode,myport,mypeer->ipaddr,myport,profitmargin);
    }
    else
    {
        if ( myipaddr == 0 )
        {
            printf("couldnt get myipaddr\n");
            exit(-1);
        }
        if ( seednode == 0 || seednode[0] == 0 )
        {
            OS_randombytes((void *)&r,sizeof(r));
            for (j=0; j<sizeof(default_LPnodes)/sizeof(*default_LPnodes); j++)
            {
                i = (r + j) % (sizeof(default_LPnodes)/sizeof(*default_LPnodes));
                LP_peersquery(mypeer,pubsock,default_LPnodes[i],myport,"127.0.0.1",myport,profitmargin);
            }
        } else LP_peersquery(mypeer,pubsock,seednode,myport,"127.0.0.1",myport,profitmargin);
    }
}

void LPinit(uint16_t myport,uint16_t mypullport,uint16_t mypubport,uint16_t mybusport,double profitmargin,char *passphrase,int32_t amclient,char *userhome,cJSON *argjson)
{
    char *myipaddr=0; long filesize,n; int32_t timeout,pubsock=-1; struct LP_peerinfo *mypeer=0; char pushaddr[128],subaddr[128],bindaddr[128]; void *ctx = bitcoin_ctx();
    if ( passphrase == 0 || passphrase[0] == 0 )
    {
        printf("jeezy says we cant use the nullstring as passphrase and I agree\n");
        exit(-1);
    }
    IAMLP = !amclient;
#ifndef __linux__
    if ( IAMLP != 0 )
    {
        printf("must run a unix node for LP node\n");
        exit(-1);
    }
#endif
    LP_profitratio += profitmargin;
    OS_randombytes((void *)&n,sizeof(n));
    if ( jobj(argjson,"gui") != 0 )
        safecopy(LP_gui,jstr(argjson,"gui"),sizeof(LP_gui));
    if ( jobj(argjson,"canbind") == 0 )
    {
#ifndef __linux__
        LP_canbind = IAMLP;
#else
        LP_canbind = IAMLP;
#endif
    }
    else
    {
        LP_canbind = jint(argjson,"canbind");
        printf(">>>>>>>>>>> set LP_canbind.%d\n",LP_canbind);
    }
    if ( LP_canbind > 1000 && LP_canbind < 65536 )
        LP_fixed_pairport = LP_canbind;
    if ( LP_canbind != 0 )
        LP_canbind = 1;
    srand((int32_t)n);
    if ( userhome != 0 && userhome[0] != 0 )
    {
        safecopy(USERHOME,userhome,sizeof(USERHOME));
#ifdef __APPLE__
        strcat(USERHOME,"/Library/Application Support");
#endif
    }
    portable_mutex_init(&LP_peermutex);
    portable_mutex_init(&LP_utxomutex);
    portable_mutex_init(&LP_UTXOmutex);
    portable_mutex_init(&LP_commandmutex);
    portable_mutex_init(&LP_swaplistmutex);
    portable_mutex_init(&LP_cachemutex);
    portable_mutex_init(&LP_networkmutex);
    portable_mutex_init(&LP_forwardmutex);
    portable_mutex_init(&LP_psockmutex);
    portable_mutex_init(&LP_coinmutex);
    portable_mutex_init(&LP_pubkeymutex);
    if ( profitmargin == 0. || profitmargin == 0.01 )
    {
        profitmargin = 0.01 + (double)(rand() % 100)/100000;
        printf("default profit margin %f\n",profitmargin);
    }
    printf("getting myipaddr\n");
    if ( system("curl -s4 checkip.amazonaws.com > /tmp/myipaddr") == 0 )
    {
        if ( (myipaddr= OS_filestr(&filesize,"/tmp/myipaddr")) != 0 && myipaddr[0] != 0 )
        {
            n = strlen(myipaddr);
            if ( myipaddr[n-1] == '\n' )
                myipaddr[--n] = 0;
            strcpy(LP_myipaddr,myipaddr);
        } else printf("error getting myipaddr\n");
    } else printf("error issuing curl\n");
    if ( IAMLP != 0 )
    {
        pubsock = -1;
        nanomsg_transportname(0,subaddr,myipaddr,mypubport);
        nanomsg_transportname(1,bindaddr,myipaddr,mypubport);
        if ( (pubsock= nn_socket(AF_SP,NN_PUB)) >= 0 )
        {
            if ( nn_bind(pubsock,bindaddr) >= 0 )
            {
                timeout = 10;
                nn_setsockopt(pubsock,NN_SOL_SOCKET,NN_SNDTIMEO,&timeout,sizeof(timeout));
            }
            else
            {
                printf("error binding to (%s).%d\n",subaddr,pubsock);
                if ( pubsock >= 0 )
                    nn_close(pubsock), pubsock = -1;
            }
        } else printf("error getting pubsock %d\n",pubsock);
        printf(">>>>>>>>> myipaddr.%s (%s) pullsock.%d\n",myipaddr,subaddr,pubsock);
        LP_mypubsock = pubsock;
    }
    printf("got %s, initpeers\n",myipaddr);
    LP_initpeers(pubsock,mypeer,myipaddr,myport,jstr(argjson,"seednode"),profitmargin);
    printf("get public socket\n");
    LP_mypullsock = LP_initpublicaddr(ctx,&mypullport,pushaddr,myipaddr,mypullport,0);
    strcpy(LP_publicaddr,pushaddr);
    LP_publicport = mypullport;
    LP_mybussock = LP_coinbus(mybusport);
    //LP_deadman_switch = (uint32_t)time(NULL);
    printf("canbind.%d my command address is (%s) pullsock.%d pullport.%u\n",LP_canbind,pushaddr,LP_mypullsock,mypullport);
    printf("initcoins\n");
    LP_initcoins(ctx,pubsock,jobj(argjson,"coins"),passphrase);
    if ( IAMLP != 0 && OS_thread_create(malloc(sizeof(pthread_t)),NULL,(void *)LP_psockloop,(void *)&myipaddr) != 0 )
    {
        printf("error launching LP_psockloop for (%s)\n",myipaddr);
        exit(-1);
    }
    if ( OS_thread_create(malloc(sizeof(pthread_t)),NULL,(void *)stats_rpcloop,(void *)&myport) != 0 )
    {
        printf("error launching stats rpcloop for port.%u\n",myport);
        exit(-1);
    }
    if ( OS_thread_create(malloc(sizeof(pthread_t)),NULL,(void *)command_rpcloop,(void *)&myipaddr) != 0 )
    {
        printf("error launching stats rpcloop for port.%u\n",myport);
        exit(-1);
    }
    if ( OS_thread_create(malloc(sizeof(pthread_t)),NULL,(void *)queue_loop,(void *)&myipaddr) != 0 )
    {
        printf("error launching stats rpcloop for port.%u\n",myport);
        exit(-1);
    }
    //if ( (retstr= basilisk_swapentry(0,0)) != 0 )
    //    free(retstr);
    while ( 1 )
    {
        //fprintf(stderr,".");
        if ( LP_mainloop_iter(ctx,myipaddr,mypeer,pubsock,pushaddr,myport,passphrase,profitmargin) == 0 )
            usleep(1000000 / MAINLOOP_PERSEC);
        /*if ( LP_canbind == 0 )
        {
            //printf("check deadman %u vs %u\n",LP_deadman_switch,(uint32_t)time(NULL));
            if ( LP_deadman_switch < time(NULL)-PSOCK_KEEPALIVE )
            {
                printf("DEAD man's switch %u activated at %u lag.%d, register forwarding again\n",LP_deadman_switch,(uint32_t)time(NULL),(uint32_t)(time(NULL) - LP_deadman_switch));
                if ( pullsock >= 0 )
                    nn_close(pullsock);
                pullsock = LP_initpublicaddr(ctx,&mypullport,pushaddr,myipaddr,mypullport,0);
                LP_deadman_switch = (uint32_t)time(NULL);
                strcpy(LP_publicaddr,pushaddr);
                LP_publicport = mypullport;
                LP_forwarding_register(LP_mypubkey,pushaddr,mypullport,MAX_PSOCK_PORT);
            }
        }*/
    }
}



