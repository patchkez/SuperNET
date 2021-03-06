//
// Created by artem on 24.01.18.
//
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
    
#define ETOMIC_TESTNET
#ifdef ETOMIC_TESTNET
#define ETOMIC_ALICECONTRACT "0xe1D4236C5774D35Dc47dcc2E5E0CcFc463A3289c"
#define ETOMIC_BOBCONTRACT "0x9387Fd3a016bB0205e4e131Dde886B9d2BC000A2"
#define ETOMIC_GASMULT 100
#else
#define ETOMIC_ALICECONTRACT "0x9bC5418CEdED51dB08467fc4b62F32C5D9EBdA55"
#define ETOMIC_BOBCONTRACT "0xB1Ad803ea4F57401639c123000C75F5B66E4D123"
#define ETOMIC_GASMULT 4
#endif
    
#define ETOMIC_SATOSHICAT "0000000000"

typedef struct {
    char from[65];
    char to[65];
    char amount[100];
    char secretKey[70];
} BasicTxData;

typedef struct {
    char dealId[70];
    char bobAddress[65];
    char aliceHash[65];
    char bobHash[65];
} AliceSendsEthPaymentInput;

typedef struct {
    char dealId[70];
    char amount[100];
    char tokenAddress[65];
    char bobAddress[65];
    char aliceHash[65];
    char bobHash[65];
} AliceSendsErc20PaymentInput;

typedef struct {
    char dealId[70];
    char amount[100];
    char tokenAddress[65];
    char bobAddress[65];
    char aliceHash[65];
    char bobSecret[70];
} AliceReclaimsAlicePaymentInput;

typedef struct {
    char dealId[70];
    char amount[100];
    char tokenAddress[65];
    char aliceAddress[65];
    char aliceSecret[70];
    char bobHash[65];
} BobSpendsAlicePaymentInput;

typedef struct {
    char depositId[70];
    char aliceAddress[65];
    char bobHash[65];
} BobSendsEthDepositInput;

typedef struct {
    char depositId[70];
    char amount[100];
    char tokenAddress[65];
    char aliceAddress[65];
    char bobHash[65];
} BobSendsErc20DepositInput;

typedef struct {
    char depositId[70];
    char amount[100];
    char tokenAddress[65];
    char aliceAddress[65];
    char bobSecret[70];
    char aliceCanClaimAfter[100];
} BobRefundsDepositInput;

typedef struct {
    char depositId[70];
    char amount[100];
    char tokenAddress[65];
    char bobAddress[65];
    char bobHash[65];
    char aliceCanClaimAfter[100];
} AliceClaimsBobDepositInput;

typedef struct {
    char paymentId[70];
    char aliceAddress[65];
    char aliceHash[65];
} BobSendsEthPaymentInput;

typedef struct {
    char paymentId[70];
    char amount[100];
    char tokenAddress[65];
    char aliceAddress[65];
    char aliceHash[65];
} BobSendsErc20PaymentInput;

typedef struct {
    char paymentId[70];
    char amount[100];
    char tokenAddress[65];
    char aliceAddress[65];
    char aliceHash[65];
    char bobCanClaimAfter[100];
} BobReclaimsBobPaymentInput;

typedef struct {
    char paymentId[70];
    char amount[100];
    char tokenAddress[65];
    char aliceSecret[70];
    char bobAddress[65];
    char bobCanClaimAfter[100];
} AliceSpendsBobPaymentInput;

char* approveErc20(char amount[100], char* from, char* secret);
char* aliceSendsEthPayment(AliceSendsEthPaymentInput input, BasicTxData txData);
char* aliceSendsErc20Payment(AliceSendsErc20PaymentInput input, BasicTxData txData);
char* aliceReclaimsAlicePayment(AliceReclaimsAlicePaymentInput input, BasicTxData txData);
char* bobSpendsAlicePayment(BobSpendsAlicePaymentInput input, BasicTxData txData);
char* bobSendsEthDeposit(BobSendsEthDepositInput input, BasicTxData txData);
char* bobSendsErc20Deposit(BobSendsErc20DepositInput input, BasicTxData txData);
char* bobRefundsDeposit(BobRefundsDepositInput input, BasicTxData txData);
char* aliceClaimsBobDeposit(AliceClaimsBobDepositInput input, BasicTxData txData);
char* bobSendsEthPayment(BobSendsEthPaymentInput input, BasicTxData txData);
char* bobSendsErc20Payment(BobSendsErc20PaymentInput input, BasicTxData txData);
char* bobReclaimsBobPayment(BobReclaimsBobPaymentInput input, BasicTxData txData);
char* aliceSpendsBobPayment(AliceSpendsBobPaymentInput input, BasicTxData txData);
char* privKey2Addr(char* privKey);
char* pubKey2Addr(char* pubKey);
char* getPubKeyFromPriv(char* privKey);
uint64_t getEthBalance(char* address);
uint64_t getErc20Balance(char* address, char tokenAddress[65]);
void uint8arrayToHex(char *dest, uint8_t *input, int len);
void satoshisToWei(char *dest, uint64_t input);
// Your prototype or Definition
#ifdef __cplusplus
}
#endif
