/* File name: NegotiationTest.c
 * Original File: Negotiation.c
 *      Written By: Nelson Yip
 *
 * Test Harness Written By: Jared Chin
 * University of California, Santa Cruz
 * Computer Engineering 13
 */

// **** Include libraries here ****
// Standard libraries
#include <stdio.h>
#include <string.h>
#include <stdint.h>

//CMPE13 Support Library
#include "BOARD.h"

// Microchip libraries
#include <xc.h>
#include <sys/attribs.h>

//CE13 standard libraries:
#include "Buttons.h"
#include "Uart1.h"
#include "Oled.h"

// Battleboats Libraries:
#include "Negotiation.h"

int main(void)
{
    BOARD_Init();

    printf("Now starting Negotiations.c Test harness.  Compiled on %s %s\n", __DATE__, __TIME__);

    int testPass = 0;
    int funcPass = 0;

    NegotiationData expectedHash;
    NegotiationData resultHash;

    /**********************************************************************************************/
    // TEST NEGOTIATION HASH FUNCTION
    printf("Now testing: NegotiationHash()\n");

    // case 1: 12345
    expectedHash = 43182;
    resultHash = NegotiationHash(12345);
    if (resultHash == expectedHash) {
        testPass++;
    } else {
        printf("Test1 error: ");
        printf("NegotiationHash(%d) did not return expected hash(%d)\n", resultHash, expectedHash);
    }

    // case 2: 16
    expectedHash = 16;
    resultHash = NegotiationHash(4);
    if (resultHash == expectedHash) {
        testPass++;
    } else {
        printf("Test2 error: ");
        printf("NegotiationHash(%d) did not return expected hash(%d)\n", resultHash, expectedHash);
    }

    // case 3: 4291
    expectedHash = 34177;
    resultHash = NegotiationHash(4291);
    if (resultHash == expectedHash) {
        testPass++;
    } else {
        printf("Test3 error: ");
        printf("NegotiationHash(%d) did not return expected hash(%d)\n", resultHash, expectedHash);
    }

    // case 4: 152
    expectedHash = 23104;
    resultHash = NegotiationHash(152);
    if (resultHash == expectedHash) {
        testPass++;
    } else {
        printf("Test4 error: ");
        printf("NegotiationHash(%d) did not return expected hash(%d)\n", resultHash, expectedHash);
    }

    // case 5: 844
    expectedHash = 28030;
    resultHash = NegotiationHash(844);
    if (resultHash == expectedHash) {
        testPass++;
    } else {
        printf("Test5 error: ");
        printf("NegotiationHash(%d) did not return expected hash(%d)\n", resultHash, expectedHash);
    }

    if (testPass == 5) {
        funcPass++;
    }
    printf("Function NegotiationHash() passed %d/5 tests\n", testPass);
    /**********************************************************************************************/
    testPass = 0;
    /**********************************************************************************************/
    // TEST NEGOTIATION VERIFY FUNCTION
    printf("Now testing: NegotiationVerify()\n");

    // case 1: 844
    expectedHash = 28030;
    if (NegotiationVerify(844, expectedHash)) {
        testPass++;
    } else {
        printf("Test1 error: ");
        printf("NegotiationVerify failed!\n");
    }

    // case 2: 152
    expectedHash = 23104;
    if (NegotiationVerify(152, expectedHash)) {
        testPass++;
    } else {
        printf("Test2 error: ");
        printf("NegotiationVerify failed!\n");
    }

    // case 3: 4291
    expectedHash = 34177;
    if (NegotiationVerify(4291, expectedHash)) {
        testPass++;
    } else {
        printf("Test3 error: ");
        printf("NegotiationVerify failed!\n");
    }

    // case 4: 16
    expectedHash = 16;
    if (NegotiationVerify(4, expectedHash)) {
        testPass++;
    } else {
        printf("Test4 error: ");
        printf("NegotiationVerify failed!\n");
    }

    // case 5: 12345
    expectedHash = 43182;
    if (NegotiationVerify(12345, expectedHash)) {
        testPass++;
    } else {
        printf("Test5 error: ");
        printf("NegotiationVerify failed!\n");
    }
    if (testPass == 5) {
        funcPass++;
    }
    printf("Function NegotiationVerify() passed %d/5 tests\n", testPass);

    /**********************************************************************************************/
    testPass = 0;
    /**********************************************************************************************/
    // TEST NEGOTIATE COIN FLIP FUNCTION
    printf("Now testing: NegotiateCoinFlip()\n");
    
    // TAILS IS EVEN
    // HEADS IS ODD
    
    NegotiationData a = 0b0101010101010101;
    NegotiationData b = 0b1010101010101010;
    NegotiationOutcome result = TAILS;
    
    // case 1: 0b 0101 0101 0101 0101
    //         0b 1010 1010 1010 1010
    if (result == NegotiateCoinFlip(a,b)){
        testPass++;
    } else {
        printf("Test1 error: ");
        printf("NegotiateCoinFlip did not result in tails\n");
    }
    
    // case 2: 0b 0101 0101 0101 0101
    //         0b 0111 0000 0011 1011
    b = 0b0111000000111011;
    if (result == NegotiateCoinFlip(a,b)){
        testPass++;
    } else {
        printf("Test2 error: ");
        printf("NegotiateCoinFlip did not result in heads\n");
    }
    
    // case 3: 0b 1100 1011 1110 1000
    //         0b 0111 0000 0011 1011
    result = HEADS;
    a = 0b1100101111101000;
    if (result == NegotiateCoinFlip(a,b)){
        testPass++;
    } else {
        printf("Test3 error: ");
        printf("NegotiateCoinFlip did not result in heads\n");
    }
    
    // case 4: 0b 0001 0001 0111 1110
    //         0b 0111 1100 0101 1011
    a = 0b0001000101111110;
    b = 0b0111110001011011;
    result = TAILS;
    if (result == NegotiateCoinFlip(a,b)){
        testPass++;
    } else {
        printf("Test4 error: ");
        printf("NegotiateCoinFlip did not result in tails\n");
    }
    
    // case 5: 0b 1111 1111 1111 1111
    //         0b 1111 1111 1111 1111
    a = 0b1111111111111111;
    b = 0b1111111111111111;
    if (result == NegotiateCoinFlip(a,b)){
        testPass++;
    } else {
        printf("Test5 error: ");
        printf("NegotiateCoinFlip did not result in tails\n");
    }
    
    if (testPass == 5) {
        funcPass++;
    }
    printf("Function NegotiationVerify() passed %d/5 tests\n", testPass);
    /**********************************************************************************************/

    if (funcPass == 3) {
        printf("Negotiation.c passed all test cases!");
    }
    
    while(1);
}