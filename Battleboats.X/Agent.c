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
#include "Agent.h"
#include "Field.h"
#include "Negotiation.h"
#include "FieldOled.h"

// declare macros
#define ZERO 0

// define module-level variables
static Field ownField, opponentField;
static AgentState state;
static NegotiationData a;
static NegotiationData b;
static NegotiationData hash;
//static uint8_t stateMachineStart = TRUE;
static uint8_t turnCount;
static FieldOledTurn turn;

// declare helper functions
static void ResetButton(void);

void AgentInit(void)
{
    FieldInit(&ownField, &opponentField);
    FieldAIPlaceAllBoats(&ownField);
    state = AGENT_STATE_START;
    turn = FIELD_OLED_TURN_NONE;
    turnCount = ZERO;
}

Message AgentRun(BB_Event event)
{
    /**
     * I'm making a list of questions here:
     * - is there a new game notif? if so is it at the start state?
     *      New game notif will be added with reset button, and initial game is implemented
     *      in main
     *      - ADDED
     * - we still need to add the turn count? people say it should be in agent.c
     *      - ADDED
     * - im gonna make FieldAIGuess(), I think everything compiles except that
     *      To Be Added...
     */
    // we should really optimize the variable assignment, but im lazy rn
    Message msg;
    msg.type = MESSAGE_NONE;
    msg.param0 = ZERO;
    msg.param1 = ZERO;
    msg.param2 = ZERO;
    if (event.type == BB_EVENT_RESET_BUTTON) {
        // if reset button is pressed, set state to START
        ResetButton();
    }
    if (event.type == BB_EVENT_ERROR) {
        OledDrawString("Some error occurred!");
        OledUpdate();
    }
    switch (state) {
    case AGENT_STATE_START:
        // for AGENT_STATE_START:
        // if button is pressed, then player is challenging, else player is challenged
        // decide what to do based on events triggered and send appropriate message
        if (event.type == BB_EVENT_START_BUTTON) {
            // if start button is pressed, then player is sending challenge
            // generate a, hash it and then init fields and send message with hash
            a = rand(); // generate a
            hash = NegotiationHash(a); // generate hash

            // NOTE: added this if to stop the initial start from
            // resetting the boats that were added in agentInit()
            //            if(!stateMachineStart){
            //                FieldInit(&ownField, &opponentField); // initiate fields
            //            }
            //            else{
            //                stateMachineStart = FALSE;
            //            }


            // set message to challenge with param0 as hash and send
            msg.type = MESSAGE_CHA;
            msg.param0 = hash;
            state = AGENT_STATE_CHALLENGING; // set state to challenging
        }
        if (event.type == BB_EVENT_CHA_RECEIVED) {
            // if receiving challenge, then player is receiving
            // generate b, init fields and then send b
            b = rand(); // generate b
            hash = event.param0;
            //            FieldInit(&ownField, &opponentField); // initiate fields
            // set message to accepted, and send b as param0
            msg.type = MESSAGE_ACC;
            msg.param0 = b;
            state = AGENT_STATE_ACCEPTING; // set state to accepting
        }
        //        return msg;
        break;
    case AGENT_STATE_CHALLENGING:
        // for AGENT_STATE_CHALLENGING:
        // obtain b from other agent and then send a to other player
        // calculate coin flip and then move to appropriate state
        // then send message containing a
        if (event.type == BB_EVENT_ACC_RECEIVED) {
            // if accept is received, obtain b from event and then calculate coin flip
            // and move to appropriate state
            b = event.param0; // obtain b
            NegotiationOutcome result = NegotiateCoinFlip(a, b); // calculate coin flip
            FieldOledDrawScreen(&ownField, &opponentField, turn, turnCount);
            if (result == HEADS) {
                // if result is HEADS, then set state to WAITING_TO_SEND and wait
                state = AGENT_STATE_WAITING_TO_SEND;
            } else if (result == TAILS) {
                // if result is TAILS, set state to defending and wait
                state = AGENT_STATE_DEFENDING;
            }
            // set message type to 'REVEAL' and set param0 to a to send to other player
            msg.type = MESSAGE_REV;
            msg.param0 = a;
        }
        //        return msg;
        break;
    case AGENT_STATE_ACCEPTING:
        // for AGENT_STATE_ACCEPTING:
        // obtain 'A' from other player to calculate result of coin flip
        // move to appropriate state after based on result and then send message
        // if cheating is detected, return ERROR message
        if (event.type == BB_EVENT_REV_RECEIVED) {
            // if receiving 'A', calculate coin flip
            a = event.param0;
            int notCheating = NegotiationVerify(a, hash);
            if (notCheating == FALSE) {
                state = AGENT_STATE_END_SCREEN;
            } else {
                NegotiationOutcome result = NegotiateCoinFlip(a, b);
                FieldOledDrawScreen(&ownField, &opponentField, turn, turnCount);
                if (result == HEADS) {
                    // if heads, then player is defending
                    state = AGENT_STATE_DEFENDING;
                    // set message to NONE as no message is to be sent, only received
                } else if (result == TAILS) {
                    // if tails, then player starts with attacking
                    // make first guess
                    GuessData shot = FieldAIDecideGuess(&opponentField);
                    // set message to SHOT FIRED!
                    msg.type = MESSAGE_SHO;
                    msg.param0 = shot.row;
                    msg.param1 = shot.col;
                    state = AGENT_STATE_ATTACKING;
                }
            }
        }
        //        return msg;
        break;
    case AGENT_STATE_WAITING_TO_SEND:
        // for AGENT_STATE_WAITING_TO_SEND:
        // if message is sent, then make guess and send shot
        if (event.type == BB_EVENT_MESSAGE_SENT) {
            // if event is message sent, increment turn count, decide guess, and send SHOT
            turnCount++;
            GuessData shot = FieldAIDecideGuess(&opponentField);
            msg.type = MESSAGE_SHO;
            msg.param0 = shot.row;
            msg.param1 = shot.col;
            state = AGENT_STATE_ATTACKING;
        }
        //        return msg;
        break;
    case AGENT_STATE_ATTACKING:
        // for AGENT_STATE_ATTACKING:
        // wait to receive the result of shot then update the enemy field accordingly
        if (event.type == BB_EVENT_RES_RECEIVED) {
            // take data obtained and extract data to formulate guess data
            GuessData guess;
            guess.row = event.param0;
            guess.col = event.param1;
            guess.result = event.param2;
            // update own knowledge
            FieldUpdateKnowledge(&opponentField, &guess);
            // update field
            FieldOledDrawScreen(&ownField, &opponentField, turn, turnCount);
            //            SquareStatus old = FieldUpdateKnowledge(&opponentField, &guess);
            if (FieldGetBoatStates(&opponentField) == ZERO) {
                state = AGENT_STATE_END_SCREEN; // victory
            } else {
                state = AGENT_STATE_DEFENDING; // change turn to defending
                turn = FIELD_OLED_TURN_THEIRS;
            }
            // set message to none and params to 0 as no message is to be sent
            // it was already sent in the previous state (WAITING_TO_SEND)
        }
        //        return msg;
        break;
    case AGENT_STATE_DEFENDING:
        // for AGENT_STATE_DEFENDING:
        // wait to receive the shot and then update player field accordingly
        // record result of guess and then return the result
        if (event.type == BB_EVENT_SHO_RECEIVED) {
            // take data obtained and extract data to formulate guess data
            GuessData guess;
            guess.row = event.param0; // row as param0
            guess.col = event.param1; // col as param1
            guess.result = event.param2; // result of attack as param2
            // register enemy attack
            FieldRegisterEnemyAttack(&ownField, &guess);
            // update field
            FieldOledDrawScreen(&ownField, &opponentField, turn, turnCount);
            //            SquareStatus old = FieldRegisterEnemyAttack(&ownField, &guess);
            // return message containing result of guess shot
            msg.type = MESSAGE_RES;
            msg.param0 = guess.row; // param0 as row
            msg.param1 = guess.col; // param1 as col
            msg.param2 = guess.result; // param2 as result
            if (FieldGetBoatStates(&ownField) == ZERO) {
                state = AGENT_STATE_END_SCREEN; // defeat
            } else {
                state = AGENT_STATE_WAITING_TO_SEND; // set state
                turn = FIELD_OLED_TURN_MINE;
            }
        }
        //        return msg;
        break;
    case AGENT_STATE_END_SCREEN:
        // for AGENT_STATE_END_SCREEN:
        // if victory (ships left for opponent is 0), display a victory message
        // if defeat (ships left for you is 0), display a defeat message
        OledClear(OLED_COLOR_BLACK);
        OledUpdate();
        if (FieldGetBoatStates(&ownField) == ZERO) {
            OledDrawString("DEFEAT!\nThe opponent\nSUNK\nall your boats :(");
            OledUpdate();
        } else if (FieldGetBoatStates(&opponentField) == ZERO) {
            OledDrawString("VICTORY!\nYou SUNK\nall your opponent's\nboats :)");
            OledUpdate();
        }
        break;
    case AGENT_STATE_SETUP_BOATS:
        AgentInit();
        break;
    }
    return msg;
}

AgentState AgentGetState(void)
{
    return state;
}

void AgentSetState(AgentState newState)
{
    state = newState;
}

/* Helper function that resets all the fields and static variables used in Agent.c.
 * Called when reset button is pressed in any state.
 * 
 * Reset
 * 
 * No parameters or returns
 */
void ResetButton(void)
{
    FieldInit(&ownField, &opponentField);
    FieldAIPlaceAllBoats(&ownField);
    turnCount = 0;
    turn = FIELD_OLED_TURN_NONE;
    state = AGENT_STATE_START;
    OledClear(OLED_COLOR_BLACK);
    OledDrawString("Reset Complete!\nPress BTN4 to\nchallenge, or wait\nfor opponent.");
    OledUpdate();
}