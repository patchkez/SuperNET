#!/bin/bash
path="../"
data="${path}/methods"


URL="https://raw.githubusercontent.com/jl777/komodo/beta/src/assetchains"

cd $path

if [ "$1" != "list" ];then
  echo "USAGE:"
  echo -e "  - pass 'list' argument to script to display converted to YAML e.g.:\n"
  echo -e "      $0 list\n"
  echo -e "  - use '>' redirection to file if needed."
  exit 1
fi


IFS='
'

SUPPLIES=(`curl $URL -s | grep "^komodo_asset" | awk '{print $2 " " $3 }'`)
SEED_IP=`curl $URL -s | grep "^seed_ip" | sed 's/^.*hosts \(.[^ ]*\).*$/\1/g'`

echo "seed_ip: ${SEED_IP}"
echo "check_my_ip: checkip.amazonaws.com"
echo "misc_methods:"
echo "  supernet_myip":

cat ../m_notary_run | grep SuperNET | grep myipaddr | awk -F "--data " '{print $2}' | sed -e 's/\\"//g;s/"{//g;s/}"//g;s/,/\n    /g;s/^/    /g;s/:/: /g'


# Notaries IPs
echo "  notaries:"
INDEX=1
for notary in `cat ../tests/addnotarys_7776 | grep -v ^# | awk -F '--data "{' '{print $2}' | sed 's/\\"//g;s/}"//g'`;do
  echo "    notary${INDEX}:"
  echo $notary | sed 's/\\//g;s/}//g;s/:/: /g;s/,/\n      /g;s/^/      /g'
  INDEX=$((INDEX+1))
done


# WP7776
echo "  encrypt_wallet:"
echo "{\"agent\":\"bitcoinrpc\",\"method\":\"encryptwallet\",\"passphrase\":\"\$passphrase\"}" | sed 's/{"//g;s/"//g;s/}//g;s/:/: /g;s/,/\n    /g;s/^/    /g'

echo "  wallet_passphrase:"
#echo "{\"method\":\"walletpassphrase\",\"params\":[\"\$passphrase\", 9999999]}" | sed 's/{"//g;s/"//g;s/}//g;s/:/ : /g;s/, /  /g;s/,/\n    /g;s/^/    /g'
echo "    method: walletpassphrase
    params:
      - \$passphrase
      - 9999999"

echo "  dpow_general:"
cat ../m_notary_run| grep dpow | awk -F '--data ' '{print $2}' | sed 's/{//g;s/"//g;s/}//g;s/:/: /g;s/\\//g;s/KMD/$assetchain/g;  s/,/\n    /g;s/^/    /g'

echo "assetchains:"
for coin in `ls -1 *_7776`; do
    # echo  "Processing ==== $coin ===="
    coin_name=`echo ${coin} | sed 's/_7776//g' | tr '[:lower:]' '[:upper:]'`
    echo "  ${coin_name}:"
    echo "    iguana_payload:"
    cat $coin | \
	sed -n 's/^.*data \(.*\)$/\1/gp' | \
	sed -e 's/,/\n/g' | \
	sed -e :a -e '$!N;s/,/,\n/;ta' -e 'P;D' | \
	sed -e 's/\\"//g' \
	    -e 's/:/: /g' \
	    -e 's/"{//g' \
	    -e 's/}"//g' \
	    -e 's/"\/"/\//g' \
	    -e 's/^/      /g' | \
  sed -e '/genesis: /{
      $!{N;N;N;N;
      s/\n/\n  /g
      s/ {version: \(.*\)/\n        version: \1/
      s/}//g
      t label-yes
      :label-no
      P
      D
      :label-yes
      }
    }'| \
	sed '/genesisblock: /{s/genesisblock: \(.[^\n]*\)/genesisblock:\n        "\1/g; s/.\{90\}/&\\\n        /g;s/$/"/g}'

    LIST="" 
    for SUPP in `echo "${SUPPLIES[*]}"`;do
	ASE=`echo ${SUPP} | awk '{print $1}'`
	SUPPLY=`echo ${SUPP} | awk '{print $2}'`
	if [ "${ASE}" = "${coin_name}" ];then
	    LIST="${SUPPLY}"
	fi
    done
    if [ -n "$LIST" ];then
	echo "    supply: $LIST"
    else
	echo "    supply:"
  echo
fi
done


#	    -e 's/"\/"/\//g' \
