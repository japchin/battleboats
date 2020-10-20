/*
 * File:   FieldTest.c
 * Author: Nelson
 *
 * Created on June 3, 2019, 4:29 PM
 */


#include <xc.h>
#include "Field.h"
#include <stdio.h>
#include <stdlib.h>
#include "BOARD.h"

// function prototypes
void printOutLives(Field* f);

int main(void)
{
    BOARD_Init();
    
    // declare index vars outside of conditionals
    int row, col, i;
//    ******************************************************************
//    * Testing init()                                       
//    ******************************************************************
    Field* ownF = malloc(sizeof(Field));
    Field* oppF = malloc(sizeof(Field));
    FieldInit(ownF, oppF);
    
    printf("Testing contents of intialized fields:\n");
    printf("Testing own field first:\n");
    
    uint8_t notFailed = TRUE;
    
    for (row = 0; row < FIELD_ROWS; row++) {
        for (col = 0; col < FIELD_COLS; col++) {
            if(ownF->grid[row][col] != FIELD_SQUARE_EMPTY && notFailed){
                // if any of the tiles weren't init.ed right
                printf("***failed ownF init() test\n");
                notFailed = FALSE;
            }
        }
    }
    if(notFailed){
        printf("  PASSED ownF init() test\n");
    }
//    ******************************************************************
//    * Testing get() and set()                                   
//    ******************************************************************
    printf("\n\nTesting get() and set() square status:\n");
    SquareStatus oldStatus = FieldGetSquareStatus(ownF, 0, 0);
    if(oldStatus == FIELD_SQUARE_EMPTY){
        if(FieldSetSquareStatus(ownF, 0, 0, FIELD_SQUARE_HUGE_BOAT)
                == oldStatus){
            if(FieldGetSquareStatus(ownF, 0, 0) == FIELD_SQUARE_HUGE_BOAT){
                FieldSetSquareStatus(ownF, 0, 0, FIELD_SQUARE_INVALID);
                // this next set() func. should fail
                if(FieldSetSquareStatus(ownF, 0, 0, FIELD_SQUARE_EMPTY)){
                    if(FieldGetSquareStatus(ownF, 0, 0) == FIELD_SQUARE_INVALID){
                        printf("  PASSED get() and set() function testing\n");
                        // reset it for future tests
                        ownF->grid[0][0] = FIELD_SQUARE_EMPTY;
                    }
                }
            }
        }
    }
//    ******************************************************************
//    * Testing addBoat() and getState()                                       
//    ******************************************************************
    printf("\n\nTesting addBoat() function:\n");
    FieldAddBoat(ownF, 0, 0, FIELD_DIR_EAST, FIELD_BOAT_TYPE_SMALL);
    FieldAddBoat(ownF, 1, 0, FIELD_DIR_EAST, FIELD_BOAT_TYPE_MEDIUM);
    FieldAddBoat(ownF, 2, 0, FIELD_DIR_EAST, FIELD_BOAT_TYPE_LARGE);
    FieldAddBoat(ownF, 3, 0, FIELD_DIR_EAST, FIELD_BOAT_TYPE_HUGE);
    printf("Expected: lives should be default: 3, 4, 5, 6\n");
    printf("Actual: \n");
    printOutLives(ownF);
    
    printf("Testing getState() func: \n");
    printf("Expected: F\n");
    printf("Actual: %01X\n", FieldGetBoatStates(ownF));
    
    // try and add a boat at a tile that isn't empty
    if(FieldAddBoat(ownF, 3, 1, FIELD_DIR_SOUTH, FIELD_BOAT_TYPE_SMALL)
            == STANDARD_ERROR){
        printf("  PASSED error checking test 1\n");
        // reset the square
        ownF->grid[0][0] = FIELD_SQUARE_SMALL_BOAT;;
    }
    else{
        printf("  **FAILED error checking 1\n");
    }
    
    // try and add a boat with a midway-collision
    // the following call should collide with a large boat on row index 2
    if(FieldAddBoat(ownF, 0, 4, FIELD_DIR_SOUTH, FIELD_BOAT_TYPE_HUGE)
            == STANDARD_ERROR){
        printf("  PASSED error checking test 2: mid-collision adding\n");
    }
    else{
        printf("  **FAILED error checking test 2\n");
    }
//    ******************************************************************
//    * Testing registerAttack()                                       
//    ******************************************************************
    // init. the guess data to pass into later func.
    GuessData* gD = malloc(sizeof(GuessData));
    gD->col = 0;
    gD->row = 0;
    
    SquareStatus status = FieldRegisterEnemyAttack(ownF, gD);
    if(status == FIELD_SQUARE_SMALL_BOAT){
        printf("  PASSED registerAttack() test 1\n");
    }
    else{
        printf(" *FAILED error checking 1:\nReturned:%d\n", status);
    }
    
    printf("Expected state: F\n");
    printf("Actual state: %01X\n", FieldGetBoatStates(ownF));
    // now destroy the rest of the small boat and check state
    
    // change the gD to attack the rest of the small
    gD->col = 1;
    FieldRegisterEnemyAttack(ownF, gD);
    gD->col = 2;
    FieldRegisterEnemyAttack(ownF, gD);
    
    printf("Expected state: E\n");
    printf("Actual state: %01X\n", FieldGetBoatStates(ownF));
//    ******************************************************************
//    * Testing updateKnowledge() : I'm putting this off for now, it looks good
//    ******************************************************************
    
//    ******************************************************************
//    * Testing FieldAIPlaceAllBoats()
//    ******************************************************************
    // reset the fields for testing
    FieldInit(ownF, oppF);
    printf("\n\nNow Testing AI_PlaceAllBoats() func: \n");
    if(FieldAIPlaceAllBoats(ownF) == SUCCESS){
        printf("  PASSED first placeAll() test\n");
    }
    else{
        printf("**FAILED first placeAll() test\n");
    }
    
    printf("Now testing the function iteratively: set at 10 tests\n");
    int passes = 0;
//     see how many times the function fails after 10 calls
    for(i = 0; i < 10; i++){
        // reset field
        FieldInit(ownF, oppF);
        if(FieldAIPlaceAllBoats(ownF) == SUCCESS){
            passes++;
        }
    }
    printf("PASSED: %d tests\n", passes);
    if(passes != 10){
        printf("**FAILED: %d tests", 10-passes);
    }
    
    printf("Now looking at specific placement testing:");
    // reset the fields for testing
    FieldInit(ownF, oppF);
    if(FieldAIPlaceAllBoats(ownF) == SUCCESS){
        printf("  PASSED first specific placeAll() test\n");
    }
    else{
        printf("**FAILED first specific placeAll() test\n");
    }
    
    
    while(1);
}

void printOutLives(Field* f){
    printf("small lives:%d\n", f->smallBoatLives);
    printf("medium lives:%d\n", f->mediumBoatLives);
    printf("large lives:%d\n", f->largeBoatLives);
    printf("huge lives:%d\n\n", f->hugeBoatLives);
}