/* Original File: Message.c
 * File:   MessageTest.c
 * Author: Jared
 *
 */


#include <xc.h>
#include "Message.h"
#include <stdio.h>
#include <stdlib.h>
#include "BOARD.h"

// macros

// note that most tests rely on other tests
// testing flags; set them to 1 for testing, else 0
#define TEST_CHECKSUM 0
#define TEST_PARSE 0
#define TEST_ENC 0
#define TEST_DEC 1
#define TRY_CHECKSUM_CALC 0
#define MESSAGE_CHECKSUM_LEN 2

// function prototypes

int main(void)
{
    BOARD_Init();
//    ******************************************************************
//    * Testing CalcChecksum()                                       
//    ******************************************************************
#if TEST_CHECKSUM
    printf("\nTesting CalcChecksum():\n\n");
    // CASE ONE:
    char check_plOne[] = "1,2,3";
    // 48 in decimal = 0x30
    char expecCheckOne[] = "30";
    uint8_t checkOne = Message_CalculateChecksum(check_plOne);
    printf("Test 1:\nExpected:%s \nActual:%02X\n\n", expecCheckOne, checkOne);
    
    // CASE TWO:
    char check_plTwo[] = "718,2,0";
    // 60 in decimal = 0x3C
    char expecCheckTwo[] = "3C";
    uint8_t checkTwo = Message_CalculateChecksum(check_plTwo);
    printf("Test 2:\nExpected:%s \nActual:%02X\n\n", expecCheckTwo, checkTwo);
    
    // CASE THREE:
    char check_plThree[] = "76381";
    // 59 in decimal = 0x3B
    char expecCheckThree[] = "3B";
    uint8_t checkThree = Message_CalculateChecksum(check_plThree);
    printf("Test 3:\nExpected:%s \nActual:%02X\n", expecCheckThree, checkThree);
#endif
    
    // made a section for testing checksums calculations so I don't have to
    // calc. by hand for hard ocded test values
#if TRY_CHECKSUM_CALC
    char testPayload[] = "SHO,8,6";
    printf("\nTESTING PL VALUE:%02X\n", Message_CalculateChecksum(testPayload));
#endif
//    ******************************************************************
//    * Testing ParseMessage()                                       
//    ******************************************************************
#if TEST_PARSE
    printf("\nTesting parse() function:\n\n");
    // CASE ONE:
    // all the setup:
    char parse_plOne[] = "$CHA,1";
    // +1 for null term
    char expecParseOne[MESSAGE_CHECKSUM_LEN+1];
    // +1 to addr. to skip the dollar
    sprintf(expecParseOne, "%02X", Message_CalculateChecksum(parse_plOne+1));
    printf("Test case one:\n  ***SETUP: Payload:%s, Checksum:%s\n",
            parse_plOne, expecParseOne);
    BB_Event bbEv;
    bbEv.type = BB_EVENT_CHA_RECEIVED;
    
    // actual test
    if(Message_ParseMessage(parse_plOne, expecParseOne, &bbEv) == STANDARD_ERROR){
        printf("  ***FAILED test one: parseMessage() returned error\n");
    }else if(bbEv.param0 == 1){
        printf("  PASSED parse() test one: all args right\n");
    }
    else{
        printf(" **FAILED parse() args parsed wrong\n");
    }
    
    // **************************************************************
    // **************************************************************
    // CASE TWO:
    char parse_plTwo[] = "$RES,2,4,0";
    char expecParseTwo[MESSAGE_CHECKSUM_LEN+1];
    
    // setup
    sprintf(expecParseTwo, "%02X", Message_CalculateChecksum(parse_plTwo+1));
    printf("Test case two:\n  ***SETUP: Payload:%s, Checksum:%s\n",
            parse_plTwo, expecParseTwo);
    
    // overwrite old BBev to pass in
    bbEv.type = BB_EVENT_RES_RECEIVED;
    
    // actual testing
    if(Message_ParseMessage(parse_plTwo, expecParseTwo, &bbEv) == STANDARD_ERROR){
        printf("  ***FAILED test two: parseMessage() returned error\n");
    }else if(bbEv.param0 == 2 && bbEv.param1 == 4 && bbEv.param2 == 0){
        printf("  PASSED parse() test two: all args right\n");
    }
    else{
        printf(" **FAILED parse() test two: args parsed wrong\n");
    }
    
    // **************************************************************
    // **************************************************************
    // CASE THREE: error checking
    char parse_plThree[] = "$SHO,0,9";
    char expecParseThree[MESSAGE_CHECKSUM_LEN+1];
    
    // setup: pass in an incorrect checksum to test parse()
    sprintf(expecParseThree, "%02X", (Message_CalculateChecksum(parse_plThree+1) + 1));
    printf("Test case three:\n  ***SETUP: Payload:%s, Checksum:passing "
            "in an incorrect checksum\n", parse_plThree);
    
    // overwrite old BBev to pass in
    bbEv.type = BB_EVENT_SHO_RECEIVED;
    
    // actual testing
    if(Message_ParseMessage(parse_plThree, expecParseThree, &bbEv) == STANDARD_ERROR){
        printf("  PASSED parse() test three: correctly caught checksum error\n");
    } else{
        printf(" **FAILED parse() test three: didn't catch error\n");
    }
    
    // **************************************************************
    // **************************************************************
    // CASE FOUR: error checking 2
    char parse_plFour[] = "$XD,3";
    char expecParseFour[MESSAGE_CHECKSUM_LEN+1];
    
    // setup
    sprintf(expecParseFour, "%02X", Message_CalculateChecksum(parse_plFour+1));
    printf("Test case four: pass in a message id that doesn't exist \n", parse_plFour);
    
    // overwrite old BBev to pass in
    bbEv.type = BB_EVENT_NO_EVENT;
    
    // actual testing
    if(Message_ParseMessage(parse_plFour, expecParseFour, &bbEv) == STANDARD_ERROR){
        printf("  PASSED parse() test four: correctly caught non-existent msg id\n");
    } else{
        printf(" **FAILED parse() test four: didn't catch error\n");
    }
#endif
    
//    ******************************************************************
//    * Testing MessageEncode()                                       
//    ******************************************************************
#if TEST_ENC
    printf("\nTesting encode() function:\n\n");
    Message msg;
    
    // **************************************************************
    // **************************************************************
    // CASE ONE:
    char encode_expectedOne[] = "$CHA,1*57\n";
    char encode_buffOne[strlen(encode_expectedOne)+1];
    printf("Test case one: encode \"%s\"\n", encode_expectedOne);
    
    // set type and param
    msg.type = MESSAGE_CHA;
    msg.param0 = 1;
    
    if(Message_Encode(encode_buffOne, msg) == -1){
        printf("  **ERROR RECEIVED: MsgEncode() returned an error\n");
    }
    
    if(strcmp(encode_buffOne, encode_expectedOne)){
        printf("  **FAILED: encoded string didn't match\n  ->str:%s\n", encode_buffOne);
    }
    else{
        printf("  PASSED: encoded string correctly\n");
    }
    
    // **************************************************************
    // **************************************************************
    // CASE TWO:
    char encode_expectedTwo[] = "$SHO,8,6*5A\n";
    char encode_buffTwo[strlen(encode_expectedTwo)+1];
    printf("Test case two: encode \"%s\"\n", encode_expectedTwo);
    
    // set type and param
    msg.type = MESSAGE_SHO;
    msg.param0 = 8;
    msg.param1 = 6;
    
    if(Message_Encode(encode_buffTwo, msg) == -1){
        printf("  **ERROR RECEIVED: MsgEncode() returned an error\n");
    }
    
    if(strcmp(encode_buffTwo, encode_expectedTwo)){
        printf("  **FAILED: encoded string didn't match\n  ->str:%s\n", encode_buffTwo);
    }
    else{
        printf("  PASSED: encoded string correctly\n");
    }
#endif
    
//    ******************************************************************
//    * Testing MessageDecode()                                       
//    ******************************************************************
#if TEST_DEC
    // **************************************************************
    // **************************************************************
    // CASE ONE:
    char decode_testOne[] = "$CHA,1*57\n", *dec_ptOne = decode_testOne;
    BB_Event decode_bbEv;
    while(*dec_ptOne){
        if(Message_Decode(*dec_ptOne, &decode_bbEv) == STANDARD_ERROR){
            printf("  **ERROR: returned from decode(%c)\n", *dec_ptOne);
        }
        
        // increm. the pointer
        dec_ptOne++;
    }
    if(decode_bbEv.param0 == 1 && decode_bbEv.type == BB_EVENT_CHA_RECEIVED){
        printf("  PASSED: decode() test one, successfully decoded the string\n");
    }
    else{
        printf("  **FAILED: either wrong arg or type: c:%c, type:%d\n",
                decode_bbEv.param0, decode_bbEv.type);
    }
#endif
    // END OF TESTING
    while(1);
}