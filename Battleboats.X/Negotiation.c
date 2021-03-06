// **** Include libraries here ****
// Standard libraries
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

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

/**
 * This function implements a one-way hash.  It maps its input, A, 
 * into an image, #a, in a way that is hard to reverse, but easy 
 * to reproduce.
 * @param secret        //A number that a challenger commits to
 * @return hash         //the hashed value of the secret commitment.
 *
 * This function implements the "Beef Hash," a variant of a Rabin hash.
 * The result is ((the square of the input) modulo the constant key 0xBEEF).
 * So, for example, 
 * 
 * NegotiationHash(3) == 9
 * NegotiationHash(12345) == 42182
 */
NegotiationData NegotiationHash(NegotiationData secret)
{
    // don't know if needed?
    // uint64_t secret_exp = (unit64_t)(secret*secret);
    return (NegotiationData) ((secret * secret) % PUBLIC_KEY);
}

/**
 * Detect cheating.  An accepting agent will receive both a commitment hash
 * and a secret number from the challenging agent.  This function
 * verifies that the secret and the commitment hash agree, hopefully
 * detecting cheating by the challenging agent.
 *
 * @param secret        //the previously secret number that the challenging agent has revealed
 * @param commitment    //the hash of the secret number
 * @return TRUE if the commitment validates the revealed secret, FALSE otherwise
 */
int NegotiationVerify(NegotiationData secret, NegotiationData commitment)
{
    return (commitment == (NegotiationHash(secret)));
}

/**
 * The coin-flip protocol uses random numbers generated by both
 * agents to determine the outcome of the coin flip.
 *
 * The parity of a bitstring is 1 if there are an odd number of one bits,
 *   and 0 otherwise.
 * So, for example, the number 0b01101011 has 5 ones.  If the parity of
 * A XOR B is 1, then the outcome is HEADS.  Otherwise, the outcome is TAILS.
 */
NegotiationOutcome NegotiateCoinFlip(NegotiationData A, NegotiationData B)
{
    uint8_t bitCount = 0;
    uint16_t result = (A^B);
    while (result) {
        // if rightmost bit is 1
        if (result & 0x01) {
            bitCount++;
        }
        // shift the bitstring once to check the next bit
        result = result >> 1;
    }
    // mod 2 to find parity
    if (bitCount % 2 == 0) {
        return TAILS;
    } else {
        return HEADS;
    }
}


/**
 * Extra credit: 
 * Use either or both of these two functions if you want to generate a "cheating" agent.  
 *
 * To get extra credit, define these functions 
 * and use these functions in agent.c to generate A and/or B
 * 
 * Your agent only needs to be able to cheat at one role for extra credit.  They must result in 
 * your agent going first more than 75% of the time in that role when
 * competing against a fair agent (that is, an agent that uses purely random A and B).
 *
 * You must state that you did this at the top of your README, and describe your 
 * strategy thoroughly.
 */
// note: in other words, you want B to be attacking given hash_a, you want TAILS

NegotiationData NegotiateGenerateBGivenHash(NegotiationData hash_a)
{
    // note: hash_a = (A^2) xor 0xBEEF
    // 0xBEEF = 0b 1011 1110 1110 1111
    // (A^2) = hash_a xor 0xBEEF
    //    NegotiaionData A = sqrt(hash_a ^ PUBLIC_KEY);
    // B is heads if the parity of A^B is 1
    // since we know A^A = 0, make B have only one bit different
    //    uint8_t firstBit =  A & 0x01;
    // essentially inverts the first bit
    //    return B = (firstBit==1 ? (A-1) : (A+1));
    /***************************************/
    // find A and return it because A^A = 0
    NegotiationData a = hash_a ^ PUBLIC_KEY;
    a = (NegotiationData) sqrt(a);
    return a;
}

//  NOT IMPLEMENTED!!!!!
NegotiationData NegotiateGenerateAGivenB(NegotiationData B)
{
    return B;
}