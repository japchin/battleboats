/*
 * File:   AgentTest.c
 * Author: Nelson
 *
 * Created on June 5, 2019, 5:51 PM
 */


#include <xc.h>
#include "Message.h"
#include <stdio.h>
#include <stdlib.h>
#include "BOARD.h"
#include "Agent.h"
#include "Negotiation.h"

// macros
#define RESET_BUFFS() resetBuff(buff);resetBuff(buff2)

// function prototypes
static void printBBEV(BB_Event b);
static void resetBuff(char* a);


int main(void)
{
//    srand(SEED);
    BOARD_Init();
    
    printf("==============Running Agent Test==============\n\n");
    
    //init agent
    AgentInit();
    BB_Event bbEv;
    bbEv.param0 = 0;
    bbEv.param0 = 0;
    bbEv.param0 = 0;
    bbEv.type = 0;
    
    Message tempMsg;
    
    
    char* MESSAGES[] = {
        "$CHA,43182*5A\n", // 0
        "$REV,12345*5C\n",
        "$RES,4,8,1*55\n",
        "$SHO,2,2*54\n", // 3
        "$RES,3,3,1*59\n",
        "$SHO,0,0*54\n", // 5
    };
    const int MESSAGES_LEN = (sizeof(MESSAGES) / sizeof(char*));
    
    char buff[MESSAGE_MAX_LEN];
    int i;
    resetBuff(buff);
    
    char* s;
    // decode and run all messages provided above
    for(i = 0; i < MESSAGES_LEN; i++){
        s = MESSAGES[i];

        // decode the message
        while(*s){
            if(Message_Decode(*s, &bbEv) == STANDARD_ERROR){
                printf("decoding error, char:%c\n", *s);
            }
            s++;
        }
        
        printf("\n\nFinished decoding \"%s\", here are the results:\n", MESSAGES[i]);
        printBBEV(bbEv);
        
        printf("Now running agentSM on: %s", MESSAGES[i]);
        tempMsg = AgentRun(bbEv);
        printf("new agent state: %d\n", AgentGetState());
        
        printf("\nNow encoding returned msg and printing it\n");
        Message_Encode(buff, tempMsg);
        printf("---->\t\t%s\n\n", buff);
        
        // after a shot event, feed a message sent event into the SM
        if(i == 3){
            bbEv.type = BB_EVENT_MESSAGE_SENT;
            tempMsg = AgentRun(bbEv);
            printf("\nNow encoding returned msg and printing it\n");
            Message_Encode(buff, tempMsg);
            printf("---->\t\t%s\n\n", buff);
        }
        else if(i == 5){
            // start second stage of testing: test CHA generation
//            AgentInit();
            printf("\n\n================Resetting the SM for second phase "
                    "of testing================\n\n");
            // resetting by feeding a reset button event
            bbEv.type = BB_EVENT_RESET_BUTTON;
            AgentRun(bbEv);
            
            printf("Now feeding it a start btn event, expecting a CHA msg in return:\n");
            // now feed it a start btn event
            bbEv.type = BB_EVENT_START_BUTTON;
            tempMsg = AgentRun(bbEv);
            Message_Encode(buff, tempMsg);
            printf("---->\t\t%s\n\n", buff);
        }
    }
    while(1);
}

static void resetBuff(char* a){
    while(*a){
        *a = '\0';
        a++;
    }
}

static void printBBEV(BB_Event b){
//    printf("Printing battle boat state:\n");
    printf("Type : %d\n", b.type);
    printf("arg 0: %d\n", b.param0);
    printf("arg 1: %d\n", b.param1);
    printf("arg 2: %d\n", b.param2);
}
