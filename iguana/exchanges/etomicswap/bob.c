//
// Created by artem on 24.01.18.
//
#include <stdio.h>
#include <curl/curl.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include "etomiclib.h"
#include "etomiccurl.h"

char* bobContractAddress = "0x9387Fd3a016bB0205e4e131Dde886B9d2BC000A2";
char* aliceAddress = "0x485d2cc2d13a9e12E4b53D606DB1c8adc884fB8a";
char* bobAddress = "0xA7EF3f65714AE266414C9E58bB4bAa4E6FB82B41";
char* tokenAddress = "0xc0eb7AeD740E1796992A08962c15661bDEB58003";

int main(int argc, char** argv)
{
    enum {
        BOB_ETH_DEPOSIT,
        BOB_ERC20_DEPOSIT,
        BOB_CLAIMS_DEPOSIT,
        ALICE_CLAIMS_DEPOSIT,
        BOB_ETH_PAYMENT,
        BOB_ERC20_PAYMENT,
        BOB_CLAIMS_PAYMENT,
        ALICE_CLAIMS_PAYMENT,
        BOB_APPROVES_ERC20,
        BOB_ETH_BALANCE,
        BOB_ERC20_BALANCE,
        TX_RECEIPT
    };
    if (argc < 2) {
        return 1;
    }
    int action = atoi(argv[1]);
    BasicTxData txData;
    char* result;
    switch (action)
    {
        case BOB_ETH_DEPOSIT:
            strcpy(txData.amount, "1000000000000000000");
            strcpy(txData.from, bobAddress);
            strcpy(txData.to, bobContractAddress);
            strcpy(txData.secretKey, getenv("BOB_PK"));

            BobSendsEthDepositInput input;

            strcpy(input.aliceAddress, aliceAddress);
            strcpy(input.depositId, argv[2]);
            strcpy(input.bobHash, argv[3]);

            result = bobSendsEthDeposit(input, txData);
            printf("%s\n", result);
            free(result);
            break;
        case BOB_ERC20_DEPOSIT:
            strcpy(txData.amount, "0");
            strcpy(txData.from, bobAddress);
            strcpy(txData.to, bobContractAddress);
            strcpy(txData.secretKey, getenv("BOB_PK"));

            BobSendsErc20DepositInput input1 = {
                .amount = "1000000000000000000"
            };

            strcpy(input1.depositId, argv[2]);
            strcpy(input1.aliceAddress, aliceAddress);
            strcpy(input1.bobHash, argv[3]);
            strcpy(input1.tokenAddress, tokenAddress);

            result = bobSendsErc20Deposit(input1, txData);
            printf("%s\n", result);
            free(result);
            break;
        case BOB_CLAIMS_DEPOSIT:
            strcpy(txData.amount, "0");
            strcpy(txData.from, bobAddress);
            strcpy(txData.to, bobContractAddress);
            strcpy(txData.secretKey, getenv("BOB_PK"));

            BobRefundsDepositInput input2;
            strcpy(input2.depositId, argv[2]);
            strcpy(input2.amount, "1000000000000000000");
            strcpy(input2.aliceAddress, aliceAddress);
            strcpy(input2.tokenAddress, argv[3]);
            strcpy(input2.aliceCanClaimAfter, argv[4]);
            strcpy(input2.bobSecret, argv[5]);

            result = bobRefundsDeposit(input2, txData);
            printf("%s\n", result);
            free(result);
            break;
        case ALICE_CLAIMS_DEPOSIT:
            strcpy(txData.amount, "0");
            strcpy(txData.from, aliceAddress);
            strcpy(txData.to, bobContractAddress);
            strcpy(txData.secretKey, getenv("ALICE_PK"));

            AliceClaimsBobDepositInput input3;
            strcpy(input3.depositId, argv[2]);
            strcpy(input3.amount, "1000000000000000000");
            strcpy(input3.bobAddress, bobAddress);
            strcpy(input3.tokenAddress, argv[3]);
            strcpy(input3.aliceCanClaimAfter, argv[4]);
            strcpy(input3.bobHash, argv[5]);

            result = aliceClaimsBobDeposit(input3, txData);
            printf("%s\n", result);
            free(result);
            break;
        case BOB_ETH_PAYMENT:
            strcpy(txData.amount, "1000000000000000000");
            strcpy(txData.from, bobAddress);
            strcpy(txData.to, bobContractAddress);
            strcpy(txData.secretKey, getenv("BOB_PK"));

            BobSendsEthPaymentInput input4;
            strcpy(input4.paymentId, argv[2]);
            strcpy(input4.aliceHash, argv[3]);
            strcpy(input4.aliceAddress, aliceAddress);

            result = bobSendsEthPayment(input4, txData);
            printf("%s\n", result);
            free(result);
            break;
        case BOB_ERC20_PAYMENT:
            strcpy(txData.amount, "0");
            strcpy(txData.from, bobAddress);
            strcpy(txData.to, bobContractAddress);
            strcpy(txData.secretKey, getenv("BOB_PK"));

            BobSendsErc20PaymentInput input5;

            strcpy(input5.paymentId, argv[2]);
            strcpy(input5.amount, "1000000000000000000");
            strcpy(input5.tokenAddress, tokenAddress);
            strcpy(input5.aliceAddress, aliceAddress);
            strcpy(input5.aliceHash, argv[3]);

            result = bobSendsErc20Payment(input5, txData);
            printf("%s\n", result);
            free(result);
            break;
        case BOB_CLAIMS_PAYMENT:
            strcpy(txData.amount, "0");
            strcpy(txData.from, bobAddress);
            strcpy(txData.to, bobContractAddress);
            strcpy(txData.secretKey, getenv("BOB_PK"));

            BobReclaimsBobPaymentInput input6;

            strcpy(input6.paymentId, argv[2]);
            strcpy(input6.aliceAddress, aliceAddress);
            strcpy(input6.amount, "1000000000000000000");
            strcpy(input6.tokenAddress, argv[3]);
            strcpy(input6.bobCanClaimAfter, argv[4]);
            strcpy(input6.aliceHash, argv[5]);

            result = bobReclaimsBobPayment(input6, txData);
            printf("%s\n", result);
            free(result);
            break;
        case ALICE_CLAIMS_PAYMENT:
            strcpy(txData.amount, "0");
            strcpy(txData.from, aliceAddress);
            strcpy(txData.to, bobContractAddress);
            strcpy(txData.secretKey, getenv("ALICE_PK"));

            AliceSpendsBobPaymentInput input7;

            strcpy(input7.paymentId, argv[2]);
            strcpy(input7.bobAddress, bobAddress);
            strcpy(input7.amount, "1000000000000000000");
            strcpy(input7.tokenAddress, argv[3]);
            strcpy(input7.bobCanClaimAfter, argv[4]);
            strcpy(input7.aliceSecret, argv[5]);

            result = aliceSpendsBobPayment(input7, txData);
            printf("%s\n", result);
            free(result);
            break;
        case BOB_APPROVES_ERC20:
            result = approveErc20(
                    "10000000000000000000",
                    "0xA7EF3f65714AE266414C9E58bB4bAa4E6FB82B41",
                    getenv("BOB_PK")

            );
            printf("%s\n", result);
            free(result);
            break;
        case BOB_ETH_BALANCE:
            printf("%" PRIu64 "\n", getEthBalance(bobAddress));
            break;
        case BOB_ERC20_BALANCE:
            printf("%" PRIu64 "\n", getErc20Balance(bobAddress, tokenAddress));
            break;
        case TX_RECEIPT:
            printf("getTxReceipt\n");
            EthTxReceipt txReceipt;
            txReceipt = getEthTxReceipt("0x82afa1b00f8a63e1a91430162e5cb2d4ebe915831ffd56e6e3227814913e23e6");
            printf("%" PRIu64 "\n", txReceipt.blockNumber);
            printf("%s\n", txReceipt.blockHash);
            break;
        default:
            return 1;
    }
    char *pubkey = getPubKeyFromPriv(getenv("BOB_PK"));
    printf("pubkey: %s\n", pubkey);
    free(pubkey);

    uint64_t satoshis = 100000000;
    char weiBuffer[100];
    satoshisToWei(weiBuffer, satoshis);
    printf("wei: %s\n", weiBuffer);
    return 0;
}
