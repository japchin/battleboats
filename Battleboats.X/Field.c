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
#include "Field.h"

// macros
#define ZERO 0
#define ONE 1
#define RAND_TIMEOUT 50
#define NUM_BOAT_TYPES 4
#define NUM_DIRECTIONS 4
#define MID_ROW_IND 2
#define MID_COL_IND 4

// set this one to 1 to turn on smart ai
#define SMARTAI 0

// declare new types here

typedef enum {
    GUESS_UP,
    GUESS_RIGHT,
    GUESS_DOWN,
    GUESS_LEFT
} GuessDir;

typedef struct {
    //    GuessDir dir;
    int row;
    int col;
    uint8_t dist;
} FieldAIGuessData;

typedef struct {
    BoatType type;
    GuessDir dir;
    int row;
    int col;
} BoatObj;

typedef enum {
    GUESS,
    TARGET
} AIGuessStates;

// module level variables
//static FieldAIGuessData currGuess;
//static BoatObj enemyBoats[NUM_BOAT_TYPES];
//static uint8_t row_delta;
//static uint8_t col_delta;
#if SMARTAI
static GuessData previousGuess;
static GuessData startingLocation;
static AIGuessStates state;
static GuessDir currDir;
static uint8_t boatStates;
#endif

// declare helper functions

void FieldPrint_UART(Field *own_field, Field * opp_field)
{
    int i, k;
    for (i = 0; i < FIELD_ROWS; i++) {
        for (k = 0; k < FIELD_COLS; k++) {
            printf("|%d| ", own_field->grid[i][k]);
        }
        printf("\n");
    }
}

void FieldInit(Field *own_field, Field * opp_field)
{
    int row, col;
    // set the opponents boat lives
    opp_field->hugeBoatLives = FIELD_BOAT_SIZE_HUGE;
    opp_field->largeBoatLives = FIELD_BOAT_SIZE_LARGE;
    opp_field->mediumBoatLives = FIELD_BOAT_SIZE_MEDIUM;
    opp_field->smallBoatLives = FIELD_BOAT_SIZE_SMALL;

    // init. our lives
    own_field->hugeBoatLives = 0;
    own_field->largeBoatLives = 0;
    own_field->mediumBoatLives = 0;
    own_field->smallBoatLives = 0;

    for (row = ZERO; row < FIELD_ROWS; row++) {
        for (col = ZERO; col < FIELD_COLS; col++) {
            // set the spot to be unknown
            own_field->grid[row][col] = FIELD_SQUARE_EMPTY;
            opp_field->grid[row][col] = FIELD_SQUARE_UNKNOWN;
        }
    }

    //    // init. static guess data struct for AI:
    //    // init. the distance intervals to smallest for best probability
    //    currGuess.dist = FIELD_BOAT_SIZE_SMALL;
    //    // inits the row and col such that we're pointing to one of the four corners
    //    currGuess.row = ((rand() & 0x1) ? 0 : (FIELD_ROWS - 1));
    //    currGuess.col = ((rand() & 0x1) ? 0 : (FIELD_COLS - 1));
    //    // now init. the delta's which drive the direction of the AI
    //    row_delta = ((currGuess.row) ? -1 : 1);
    //    col_delta = ((currGuess.col) ? -1 : 1);
    //
    //    // now init. the BoatObj struct[]
    //    int i;
    //    for (i = 0; i < NUM_BOAT_TYPES; i++) {
    //        BoatObj boat = enemyBoats[i];
    //        // assign them neg. values to know they haven't been found yet
    //        boat.row = boat.col = -1;
    //        // init. one of every boat type
    //        boat.type = (BoatType) (i);
    //    }
}

SquareStatus FieldGetSquareStatus(const Field *f, uint8_t row, uint8_t col)
{
    // if either the row or col are invalid (greater than max or less than 0), return invalid
    if (row >= FIELD_ROWS || row < ZERO) {
        return FIELD_SQUARE_INVALID;
    }
    if (col >= FIELD_COLS || col < ZERO) {
        return FIELD_SQUARE_INVALID;
    }
    // else return the square
    return f->grid[row][col];
}

SquareStatus FieldSetSquareStatus(Field *f, uint8_t row, uint8_t col, SquareStatus p)
{
    // call FieldGetSquareStatus to get whether the square was invalid or not
    SquareStatus oldStatus = FieldGetSquareStatus(f, row, col);
    if (oldStatus != FIELD_SQUARE_INVALID) {
        // if the square was not invalid, then set the square to be 'p'
        f->grid[row][col] = p;
    }
    // return the old status
    return oldStatus;
}

uint8_t FieldAddBoat(Field *f, uint8_t row, uint8_t col, BoatDirection dir, BoatType boat_type)
{
    // if either the row or col are invalid (greater than max or less than 0), return invalid
    if (row < ZERO || row > FIELD_ROWS - ONE) {
        return STANDARD_ERROR;
    }
    if (col < ZERO || col > FIELD_COLS - ONE) {
        return STANDARD_ERROR;
    }
    BoatSize lives;
    SquareStatus boat;
    // set the boat and square to be the correct type based on given boat type
    if (boat_type == FIELD_BOAT_TYPE_SMALL) {
        // if boat type given is SMALL, set boat to be SMALL and lives to be 3
        f->smallBoatLives = FIELD_BOAT_SIZE_SMALL;
        lives = f->smallBoatLives;
        boat = FIELD_SQUARE_SMALL_BOAT;
    } else if (boat_type == FIELD_BOAT_TYPE_MEDIUM) {
        // if boat type given is MEDIUM, set boat to be MEDIUM and lives to be 4
        f->mediumBoatLives = FIELD_BOAT_SIZE_MEDIUM;
        lives = f->mediumBoatLives;
        boat = FIELD_SQUARE_MEDIUM_BOAT;
    } else if (boat_type == FIELD_BOAT_TYPE_LARGE) {
        // if boat type given is LARGE, set boat to be LARGE and lives to be 5
        f->largeBoatLives = FIELD_BOAT_SIZE_LARGE;
        lives = f->largeBoatLives;
        boat = FIELD_SQUARE_LARGE_BOAT;
    } else if (boat_type == FIELD_BOAT_TYPE_HUGE) {
        // if boat type given is HUGE, then set boat to be HUGE and lives to be 6
        f->hugeBoatLives = FIELD_BOAT_SIZE_HUGE;
        lives = f->hugeBoatLives;
        boat = FIELD_SQUARE_HUGE_BOAT;
    }

    // this block checks if all squares we need are valid
    BoatSize oldLives = lives;
    int oldRow = row, oldCol = col;
    while (lives > 0) {
        if (FieldGetSquareStatus(f, row, col) == FIELD_SQUARE_EMPTY) {
            if (dir == FIELD_DIR_EAST) {
                col++;
            } else if (dir == FIELD_DIR_SOUTH) {
                row++;
            }
            lives--;
        } else {
            return STANDARD_ERROR;
        }
    }
    //reset all the vars
    lives = oldLives;
    row = oldRow;
    col = oldCol;

    // this block actually assigns them since we know it's possible now
    while (lives > 0) {
        f->grid[row][col] = boat;
        if (dir == FIELD_DIR_EAST) {
            col++;
        } else if (dir == FIELD_DIR_SOUTH) {
            row++;
        }
        lives--;
    }
    return SUCCESS;
}

SquareStatus FieldRegisterEnemyAttack(Field *f, GuessData * gData)
{
    uint8_t row = gData->row;
    uint8_t col = gData->col;
    SquareStatus previous = f->grid[row][col];
    if (f->grid[row][col] == FIELD_SQUARE_EMPTY) {
        // if the spot guessed is empty, return a miss
        gData->result = RESULT_MISS;
        f->grid[row][col] = FIELD_SQUARE_MISS;
    } else if (f->grid[row][col] == FIELD_SQUARE_SMALL_BOAT) {
        // if the spot guessed is a SMALL boat...
        if (f->smallBoatLives > ONE) {
            // if the lives are greater than one, then decrement it by 1 and return hit
            f->smallBoatLives--;
            gData->result = RESULT_HIT;
        } else if (f->smallBoatLives == ONE) {
            // if the lives are equal to one, then decrement and then return SUNK
            f->smallBoatLives--;
            gData->result = RESULT_SMALL_BOAT_SUNK;
        }
        // update grid to be HIT
        f->grid[row][col] = FIELD_SQUARE_HIT;
    } else if (f->grid[row][col] == FIELD_SQUARE_MEDIUM_BOAT) {
        // if the spot guessed is a MEDIUM boat...
        if (f->mediumBoatLives > ONE) {
            // if the lives are greater than one, then decrement it by 1 and return hit
            f->mediumBoatLives--;
            gData->result = RESULT_HIT;
        } else if (f->mediumBoatLives == ONE) {
            // if the lives are equal to one, then decrement and return SUNK
            f->mediumBoatLives--;
            gData->result = RESULT_MEDIUM_BOAT_SUNK;
        }
        // update grid to be HIT
        f->grid[row][col] = FIELD_SQUARE_HIT;
    } else if (f->grid[row][col] == FIELD_SQUARE_LARGE_BOAT) {
        // if the spot guessed is a LARGE boat...
        if (f->largeBoatLives > ONE) {
            // if the lives are greater than one, then decrement it by 1 and return hit
            f->largeBoatLives--;
            gData->result = RESULT_HIT;
        } else if (f->largeBoatLives == ONE) {
            // if the lives are equal to one, then decrement and return SUNK
            f->largeBoatLives--;
            gData->result = RESULT_LARGE_BOAT_SUNK;
        }
        // update grid to be HIT
        f->grid[row][col] = FIELD_SQUARE_HIT;
    } else if (f->grid[row][col] == FIELD_SQUARE_HUGE_BOAT) {
        // if the spot guessed is a HUGE boat...
        if (f->hugeBoatLives > ONE) {
            // if the lives are greater than one, then decrement it by 1 and return hit
            f->hugeBoatLives--;
            gData->result = RESULT_HIT;
        } else if (f->hugeBoatLives == ONE) {
            // if the lives are equal to one, then decrement and return SUNK
            f->hugeBoatLives--;
            gData->result = RESULT_HUGE_BOAT_SUNK;
        }
        // update grid to be HIT
        f->grid[row][col] = FIELD_SQUARE_HIT;
    }
    // return the previous SS
    return previous;
}

SquareStatus FieldUpdateKnowledge(Field *f, const GuessData * gData)
{
    uint8_t row = gData->row;
    uint8_t col = gData->col;
    SquareStatus previous = f->grid[row][col];

    if (gData->result == RESULT_HIT) {
        f->grid[row][col] = FIELD_SQUARE_HIT;
    } else if (gData->result == RESULT_MISS) {
        f->grid[row][col] = FIELD_SQUARE_EMPTY;
    } else {
        if (gData->result == RESULT_SMALL_BOAT_SUNK) {
            f->smallBoatLives = 0;
        } else if (gData->result == RESULT_MEDIUM_BOAT_SUNK) {
            f->mediumBoatLives = 0;
        } else if (gData->result == RESULT_LARGE_BOAT_SUNK) {
            f->largeBoatLives = 0;
        } else if (gData->result == RESULT_HUGE_BOAT_SUNK) {
            f->hugeBoatLives = 0;
        }
        // also set hit square
        f->grid[row][col] = FIELD_SQUARE_HIT;
    }
    return previous;
}

uint8_t FieldGetBoatStates(const Field * f)
{
    uint8_t status = ZERO;
    if (f->hugeBoatLives) {
        status |= FIELD_BOAT_STATUS_HUGE;
    }
    if (f->largeBoatLives) {
        status |= FIELD_BOAT_STATUS_LARGE;
    }
    if (f->mediumBoatLives) {
        status |= FIELD_BOAT_STATUS_MEDIUM;
    }
    if (f->smallBoatLives) {
        status |= FIELD_BOAT_STATUS_SMALL;
    }
    return status;
}

// TODO: add srand() of some type

uint8_t FieldAIPlaceAllBoats(Field * f)
{
    int i, j;
    for (i = 0; i < NUM_BOAT_TYPES; i++) {

        BoatType type = (BoatType) (i);
        // try and add a boat, timeout if failed the 'RAND_TIMEOUT' times
        for (j = 0; j < RAND_TIMEOUT; j++) {
            // keep generating random coords. until the grid there is empty
            uint8_t row, col;
            do {
                row = rand() % FIELD_ROWS;
                col = rand() % FIELD_COLS;
            } while (f->grid[row][col] != FIELD_SQUARE_EMPTY);

            // make random dir.
            BoatDirection dir = (BoatDirection) (rand() & 0x1);

            // try and add it, if it worked, move to next boat type
            if (FieldAddBoat(f, row, col, dir, type) == SUCCESS) {
                break;
            }
        }
        // if we timed out and didn't add the boat,
        if (j == (RAND_TIMEOUT - 1)) {
            // reset the field
            int k, l;
            for (k = 0; k < FIELD_ROWS; k++) {
                for (l = 0; l < FIELD_COLS; l++) {
                    f->grid[k][l] = FIELD_SQUARE_EMPTY;
                }
            }

            // "reset" by decrementing the loop var
            i--;
        }
    }
    return SUCCESS;

    //    int i, j;
    //    // if fails placing 25 times, reset the field and
    //    // try to re-place
    //    for (i = 0; i < RAND_TIMEOUT; i++) {
    //        // row and col use mod because they're not pows of 2
    //        uint8_t row = rand() % FIELD_ROWS;
    //        uint8_t col = rand() % FIELD_COLS;
    //        // 2 possiblities
    //        BoatDirection dir = (BoatDirection) (rand() & 0x1);
    //
    //        // this block handles finding out which boat to place
    //        // 4 possibilities of boat type, check what hasn't been placed
    //        BoatType type = -1;
    //        uint8_t states = FieldGetBoatStates(f);
    //        for (j = 0; j < NUM_BOAT_TYPES; j++) {
    //            // if current state already placed, move on with new
    //            if (states & 0x01) {
    //                states = states >> 1;
    //            } else {
    //                // assign the correctly corresp. boat type
    //                type = (BoatType) (j);
    //                break;
    //            }
    //        }
    //        // if all boats were placed, done
    //        if (type == -1) {
    //            return SUCCESS;
    //        }
    //
    //        if (FieldAddBoat(f, row, col, dir, type) == SUCCESS) {
    //            // if success restart timeout var
    //            i = 0;
    //        }
    //    }
    //    // return error if timeout reached
    //    return STANDARD_ERROR;
}

GuessData FieldAIDecideGuess(const Field * f)
{
    GuessData nextGuess;
    do {
        nextGuess.row = rand() % FIELD_ROWS;
        nextGuess.col = rand() % FIELD_COLS;
    } while (FieldGetSquareStatus(f, nextGuess.row, nextGuess.col) != FIELD_SQUARE_UNKNOWN);

#if SMARTAI
    GuessData nextGuess;
    nextGuess.col = 0;
    nextGuess.row = 0;

    switch (state) {
    case GUESS:
        // guess by iterating through the grid by 3
        // if resulting in a hit, switch to hunt
        // set up next guess coordinates
        nextGuess.col = previousGuess.col + FIELD_BOAT_SIZE_SMALL;
        if (nextGuess.col >= FIELD_COLS) {
            nextGuess.col = nextGuess.col - FIELD_COLS;
            nextGuess.row = previousGuess.row + ONE;
        } else {
            nextGuess.row = previousGuess.row;
        }
        // if the previous guess was a hit, store the coordinates as the center and enter
        // TARGET mode
        if (f->grid[previousGuess.row][previousGuess.col] == FIELD_SQUARE_HIT) {
            // 
            startingLocation.row = previousGuess.row;
            startingLocation.col = previousGuess.col;
            currDir = GUESS_UP;
            boatStates = FieldGetBoatStates(f);
            state = TARGET;
        } else if (f->grid[nextGuess.row][nextGuess.col] != FIELD_SQUARE_UNKNOWN) {
            // if next guess results in an already guessed spot, then keep iterating through
            // the grid until it is an unguessed spot
            while (f->grid[nextGuess.row][nextGuess.col] != FIELD_SQUARE_UNKNOWN) {
                nextGuess.col = nextGuess.col + FIELD_BOAT_SIZE_SMALL;
                if (nextGuess.col >= FIELD_COLS) {
                    nextGuess.col = nextGuess.col - FIELD_COLS;
                    nextGuess.row = nextGuess.row + ONE;
                    if (nextGuess.row >= FIELD_ROWS) {
                        nextGuess.row = ZERO;
                    }
                }
            }
            previousGuess.row = nextGuess.row;
            previousGuess.col = nextGuess.col;
            return nextGuess;
        } else {
            break;
        }
    case TARGET:
        // when guess results in a hit, switch to TARGET mode to signal a hunt
        if (FieldGetBoatStates(f) != boatStates) {
            state = GUESS;
            nextGuess = FieldAIDecideGuess(f);
            break;
        }
        char guessLocked = FALSE; // tells whether the next guess is confirmed/locked in
        while (guessLocked == FALSE) {
            // while the guess is not confirmed...
            if (currDir == GUESS_UP) {
                // check 1 square up (row - 1)
                nextGuess.row = previousGuess.row - ONE;
                if (nextGuess.row < ZERO) {
                    currDir = GUESS_DOWN;
                    previousGuess.row = startingLocation.row;
                    previousGuess.col = startingLocation.col;
                    continue;
                }
                nextGuess.col = previousGuess.col;
                if (f->grid[nextGuess.row][nextGuess.col] != FIELD_SQUARE_UNKNOWN) {
                    // if it is already guessed or invalid, set new direction and start over
                    currDir = GUESS_LEFT;
                    previousGuess.row = startingLocation.row;
                    previousGuess.col = startingLocation.col;
                    continue;
                }
                if (f->grid[previousGuess.row][previousGuess.col] == FIELD_SQUARE_HIT) {
                    // if the previous guess resulting in a hit, confirm guess
                    guessLocked = TRUE;
                } else if (f->grid[previousGuess.row][previousGuess.col] == FIELD_SQUARE_MISS) {
                    // else if the previous guess resulted in a miss, switch directions and start
                    // over from center. Start over in loop
                    currDir = GUESS_LEFT;
                    previousGuess.row = startingLocation.row;
                    previousGuess.col = startingLocation.col;
                    continue;
                }
            } else if (currDir == GUESS_LEFT) {
                // check one square to the left (col - 1)
                nextGuess.row = previousGuess.row;
                nextGuess.col = previousGuess.col - ONE;
                if (nextGuess.col < ZERO) {
                    currDir = GUESS_RIGHT;
                    previousGuess.row = startingLocation.row;
                    previousGuess.col = startingLocation.col;
                    continue;
                }
                if (f->grid[nextGuess.row][nextGuess.col] != FIELD_SQUARE_UNKNOWN) {
                    // if the next guess is invalid, check next square in different direction
                    currDir = GUESS_DOWN;
                    previousGuess.row = startingLocation.row;
                    previousGuess.col = startingLocation.col;
                    continue;
                }
                if (f->grid[previousGuess.row][previousGuess.col] == FIELD_SQUARE_HIT) {
                    // if the previous guess resulted in a hit, confirm guess
                    guessLocked = TRUE;
                } else if (f->grid[previousGuess.row][previousGuess.col] == FIELD_SQUARE_MISS) {
                    // else if the previous guess resulted in a miss, switch directions and start
                    // over from the center. Start over in loop
                    currDir = GUESS_LEFT;
                    previousGuess.row = startingLocation.row;
                    previousGuess.col = startingLocation.col;
                    continue;
                }
            } else if (currDir == GUESS_DOWN) {
                // check one square down (row + 1)
                nextGuess.row = previousGuess.row + ONE;
                if (nextGuess.row >= FIELD_ROWS) {
                    currDir = GUESS_UP;
                    previousGuess.row = startingLocation.row;
                    previousGuess.col = startingLocation.col;
                    continue;
                }
                nextGuess.col = previousGuess.col;
                if (f->grid[nextGuess.row][nextGuess.col] != FIELD_SQUARE_UNKNOWN) {
                    // if the next guess is invalid, check next square in different direction
                    currDir = GUESS_RIGHT;
                    previousGuess.row = startingLocation.row;
                    previousGuess.col = startingLocation.col;
                    continue;
                }
                if (f->grid[previousGuess.row][previousGuess.col] == FIELD_SQUARE_HIT) {
                    // if the previous guess resulted in a hit, confirm guess
                    guessLocked = TRUE;
                } else if (f->grid[previousGuess.row][previousGuess.col] == FIELD_SQUARE_MISS) {
                    // else if the previous guess resulted in a miss, switch directions and start
                    // over from the center. Start over in loop
                    currDir = GUESS_LEFT;
                    previousGuess.row = startingLocation.row;
                    previousGuess.col = startingLocation.col;
                    continue;
                }
            } else if (currDir == GUESS_RIGHT) {
                // check on square to the right (col + 1)
                nextGuess.row = previousGuess.row;
                nextGuess.col = previousGuess.col + ONE;
                if (nextGuess.col >= FIELD_COLS) {
                    currDir = GUESS_LEFT;
                    previousGuess.row = startingLocation.row;
                    previousGuess.col = startingLocation.col;
                    continue;
                }
                if (f->grid[nextGuess.row][nextGuess.col] != FIELD_SQUARE_UNKNOWN) {
                    // if the next guess is invalid, check next square in different direction
                    currDir = GUESS_UP;
                    previousGuess.row = startingLocation.row;
                    previousGuess.col = startingLocation.col;
                    continue;
                }
                if (f->grid[previousGuess.row][previousGuess.col] == FIELD_SQUARE_HIT) {
                    // if the previous guess resulted in a hit, confirm guess
                    guessLocked = TRUE;
                } else if (f->grid[previousGuess.row][previousGuess.col] == FIELD_SQUARE_MISS) {
                    // else if the previous guess resulted in a miss, switch directions and start
                    // over from the center. Start over in loop
                    currDir = GUESS_LEFT;
                    previousGuess.row = startingLocation.row;
                    previousGuess.col = startingLocation.col;
                    continue;
                }
            }
        }
        // set previous guess to next guess and return it
        previousGuess.row = nextGuess.row;
        previousGuess.col = nextGuess.col;
        return nextGuess;
    }
    return nextGuess;
#endif
    return nextGuess;

}