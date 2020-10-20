#include "Message.h"
#include "BOARD.h"
#include <stdio.h>
#include <string.h>

// macros
// note this macro only checks capital letters
#define IS_ALPHANUM(c) ((c >= '0') && c <= '9') || ((c >= 'A') && (c <= 'F'))

// 3 chars + null term
#define MSG_ID_LEN 4

// 5 possible msg_id's
#define NUM_TEMPLATES 5

// new types
typedef enum {
    WAITING_FOR_DELIM = 0,
    RECORDING_PAYLOAD,
    RECORDING_CHECKSUM,
} DecodeState;

// module-level variables
const static char* TEMPLATES[] = {"CHA", "ACC", "REV", "SHO", "RES"};
static DecodeState decState = WAITING_FOR_DELIM;
static char buff[MESSAGE_MAX_LEN];
static char *checkPt;
static int decInd = 0;
static int checkLen = 0;

//helper functions
static uint8_t checksum_xtoi(const char* s);

/** 
 * NEMA0183 messages wrap the payload with a start delimiter, 
 * a checksum to verify the contents of   
 * the message in case of transmission errors, and an end delimiter.
 * This template defines the wrapper.
 * Note that it uses printf-style tokens so that it can be used with sprintf().
 * 
 * Here is an example message:
 *                 
 * 1 start delimiter    (a literal $)
 * 2 payload            (any string, represented by %s in the template)
 * 3 checksum delimiter (a literal *)
 * 4 checksum			(two ascii characters representing hex digits, %02x in the template)
 * 5 end delimiter      (a literal \n)
 * 
 * example message:      1       3  5      
 *						 v       v  vv
 *                       $SHO,2,3*A4\n
 *                        ^^^^^^^ ^^      
 *                          2     4      
 * 
 * Note that 2 and 4 correspond to %s and %02x in the template.
 * 
 * Also note that valid BattleBoats messages use 
 * strictly upper-case letters, so $SHO,2,3*a4\n is an invalid message.
 */

/**
 * Given a payload string, calculate its checksum
 * 
 * @param payload       //the string whose checksum we wish to calculate
 * @return   //The resulting 8-bit checksum 
 */
uint8_t Message_CalculateChecksum(const char* payload){
    uint8_t checksum = 0;
    while(*payload){
        checksum ^= *payload;
        payload++;
    }
    return checksum;
}

/**
 * Given a payload and its checksum, verify that the payload matches the checksum
 * 
 * @param payload       //the payload of a message
 * @param checksum      //the checksum (in string form) of  a message,
 *                          should be exactly 2 chars long, plus a null char
 * @param message_event //An event corresponding to the parsed message.
 *                      //If the message could be parsed successfully,
 *                          message_event's type will correspond to the message type and 
 *                          its parameters will match the message's data fields.
 *                      //If the message could not be parsed,
 *                          message_events type will be BB_EVENT_ERROR
 * 
 * @return STANDARD_ERROR if:
 *              the payload does not match the checksum
 *              the checksum string is not two characters long
 *              the message does not match any message template
 *          SUCCESS otherwise
 * 
 * Please note!  sscanf() has a couple compiler bugs that make it a very
 * unreliable tool for implementing this function. * 
 */
int Message_ParseMessage(const char* payload,
        const char* checksum_string, BB_Event * message_event){
    // +1 to skip the '$'
    if(checksum_xtoi(checksum_string) != Message_CalculateChecksum(payload+1)){
        return STANDARD_ERROR;
    }
    else if(strlen(checksum_string) != 2){
        return STANDARD_ERROR;
    }
    
    // get msg id and compare them to templates
    char msg_id[MSG_ID_LEN];
    int i;
    // copies only 3 chars because we need null term at end
    // and also because the first 3 chars (after $) are the message id
    for(i = 0; i < MSG_ID_LEN - 1; i++){
        // +1 because we want to skip the dollar sign in the front
        msg_id[i] = *(payload + i + 1);
    }
    // add null term at end of msg id
    msg_id[MSG_ID_LEN-1] = '\0';
    
    // this loop checks to make sure the message id is valid
    for(i = 0; i < NUM_TEMPLATES; i++){
        // if any of the templates match the id, then there's no error
        if(!strcmp(msg_id, TEMPLATES[i])){
            break;
        }
        // else if last iteration and none matched, error
        else if(i == NUM_TEMPLATES-1){
            return STANDARD_ERROR;
        }
    }
    
    int args = 0;
    switch(message_event->type){
        case BB_EVENT_CHA_RECEIVED:
        case BB_EVENT_ACC_RECEIVED: 
        case BB_EVENT_REV_RECEIVED:
            args = 1;
            break;
        case BB_EVENT_SHO_RECEIVED: 
            args = 2;
            break;
        case BB_EVENT_RES_RECEIVED:
            args = 3;
            break;
        default:
//            printf("\n****Message_parse() error at cases\n");
            return STANDARD_ERROR;
    }
    
    const char delim[] = ",";
    // call strtok() initially to skip the msg ID
    char* token = strtok((char*)payload, delim);
    // should store the right amount of args and stop at the delim '*'
    for(i = 0; i < args; i++){
        // grab the next token
        token = strtok(NULL, delim);
        // convert and store
        switch(i){
            case 0:
                message_event->param0 = atoi(token);
                break;
            case 1:
                message_event->param1 = atoi(token);
                break;
            case 2:
                message_event->param2 = atoi(token);
                break;
            default:
                return STANDARD_ERROR;
        }
    }
    return SUCCESS;
}

/**
 * Encodes the coordinate data for a guess into the string `message`. This string must be big
 * enough to contain all of the necessary data. The format is specified in PAYLOAD_TEMPLATE_COO,
 * which is then wrapped within the message as defined by MESSAGE_TEMPLATE. 
 * 
 * The final length of this
 * message is then returned. There is no failure mode for this function as there is no checking
 * for NULL pointers.
 * 
 * @param message            The character array used for storing the output. 
 *                              Must be long enough to store the entire string,
 *                              see MESSAGE_MAX_LEN.
 * @param message_to_encode  A message to encode
 * @return                   The length of the string stored into 'message_string'.
                             Return 0 if message type is MESSAGE_NONE.
 */
int Message_Encode(char *message_string, Message message_to_encode){
    // add the init. '$'
    printf("encoding buffer:%s\n", message_string);
    *message_string = '$';
    
    // this block handles choosing the right template
    char* template;
    switch(message_to_encode.type){
        case MESSAGE_NONE: 
            return 0;
        case MESSAGE_CHA:
            template = (char*)PAYLOAD_TEMPLATE_CHA;
            break;
        case MESSAGE_ACC:
            template = (char*)PAYLOAD_TEMPLATE_ACC;
            break;
        case MESSAGE_REV:
            template = (char*)PAYLOAD_TEMPLATE_REV;
            break;
        case MESSAGE_SHO:
            template = (char*)PAYLOAD_TEMPLATE_SHO;
            break;
        case MESSAGE_RES:
            template = (char*)PAYLOAD_TEMPLATE_RES;
            break;
        case MESSAGE_ERROR:
            return -1;
    }
    
//    switch(message_to_encode.type){
//        case MESSAGE_NONE: 
//            return 0;
//        case MESSAGE_CHA:
//        case MESSAGE_ACC:
//        case MESSAGE_REV:
//        case MESSAGE_SHO:
//        case MESSAGE_RES:
//            sprintf(template)
//            break;
//        case MESSAGE_ERROR:
//            return -1;
//    }
    
    // keep a temp pointer for sprintf() return
    char* temp = message_string;
    // now call sprintf with the correct amount of args.
    switch(message_to_encode.type){
        // 1 arg
        case MESSAGE_CHA:
        case MESSAGE_ACC:
        case MESSAGE_REV:
            temp += sprintf(message_string, template, 
                            message_to_encode.param0);
            break;
        // 2 args
        case MESSAGE_SHO:
            temp += sprintf(message_string, template, 
                    message_to_encode.param0,
                    message_to_encode.param1);
            break;
        // 3 args
        case MESSAGE_RES:
            temp += sprintf(message_string, template, 
                    message_to_encode.param0,
                    message_to_encode.param1,
                    message_to_encode.param2);
            break;
        default:
            return -1;
    }
    // use the temp pointer to find the length of payload and allocate an array for payload
    int pl_len = (temp - message_string);
    char payload[pl_len+1];
    // add a null term at the end of payload to temporarily
    // use the function below to calc. the checksum and also use strcpy
    *(temp+1) = '\0';
    uint8_t checksum = Message_CalculateChecksum(message_string);
    strcpy(payload, message_string);
    // add the asterisk back because we can't have a null char there
    *(temp+1) = '*';
    
    // sprintf returns the length
    return sprintf(message_string, MESSAGE_TEMPLATE, payload, checksum);
}


/**
 * Message_Decode reads one character at a time.  If it detects a full NMEA message,
 * it translates that message into a BB_Event struct, which can be passed to other 
 * services.
 * 
 * @param char_in - The next character in the NMEA0183 message to be decoded.
 * @param decoded_message - a pointer to a message struct, used to "return" a message
 *                          if char_in is the last character of a valid message, 
 *                              then decoded_message
 *                              should have the appropriate message type.
 *                              otherwise, it should have type NO_EVENT.
 * @return SUCCESS if no error was detected
 *         STANDARD_ERROR if an error was detected
 * 
 * note that ANY call to Message_Decode may modify decoded_message.
 */
int Message_Decode(unsigned char char_in, BB_Event * decoded_message_event){
    switch(decState){
        case WAITING_FOR_DELIM:
            if(char_in == '$'){
                decState = RECORDING_PAYLOAD;
                buff[decInd] = char_in;
                decInd++;
            }
            break;
        case RECORDING_PAYLOAD:
            // if at max len,
            if(decInd >= MESSAGE_MAX_LEN){
                decState = WAITING_FOR_DELIM;
                return STANDARD_ERROR;
            }
            
            // breaks unecess. b/c they all return unconditionally
            switch(char_in){
                case '$':
                case '\n':
                    decState = WAITING_FOR_DELIM;
                    return STANDARD_ERROR;
                case '*':
                    // record the char and keep a temp pt. there
                    buff[decInd] = char_in;
                    // note the pt points to the '*'
                    checkPt = buff + decInd;
                    decInd++;
                    
                    // reset the checksum len, change state and ret
                    checkLen = 0;
                    decState = RECORDING_CHECKSUM;
                    return SUCCESS;
                // if none of the cases, record char
                default:
                    buff[decInd] = char_in;
                    decInd++;
                    return SUCCESS;
            }
            break;
        case RECORDING_CHECKSUM:
            // checksum should only ever be <= 2
            if(checkLen > 2){
                decState = WAITING_FOR_DELIM;
                return STANDARD_ERROR;
            }
            
            // if valid hex char, add it
            if(IS_ALPHANUM(char_in)){
                buff[decInd] = char_in;
                decInd++;
                checkLen++;
            }
            
            // if at end of message
            else if(char_in == '\n'){
                // SETUP:
                // temporarily add null term so the payload and the
                // checksum portion of the messages so they behave like normal strings:
                *checkPt = '\0';
                // this is the end of the checksum part
                buff[decInd] = '\0';
                
                // this block assigns the correct BB_eventType given the respective payload
                int i;
                // add a null term after the msg id to use strcmp()
                buff[MSG_ID_LEN] = '\0';
                for(i = 0; i < NUM_TEMPLATES; i++){
                    // buff+1 b/c we skip the '$' in the payload
                    if(!strcmp((buff+1), TEMPLATES[i])){
                        // NOTE: the acceptable enum values should be within the
                        // range of (3 to 7) (Look at BB_EventType enum in BB.h)
                        decoded_message_event->type = (BB_EventType)(i+3);
                        break;
                    }
                    // else if at last iteration and none matched, ret err
                    else if(i == NUM_TEMPLATES-1){
                        return STANDARD_ERROR;
                    }
                }
                // change the char back since we're done with strcmp()
                buff[MSG_ID_LEN] = ',';
                
                // now parse the payload and checksum with func.
                if(Message_ParseMessage(buff, (checkPt+1), decoded_message_event)
                        == STANDARD_ERROR){
                    return STANDARD_ERROR;
                }
                // NOTE: REMEMBER to change null terms back to right chars
                *checkPt = '*';
                buff[decInd] = '\n';
                decInd = 0;
                decState = WAITING_FOR_DELIM;
                return SUCCESS;
            }
            else{
                decState = WAITING_FOR_DELIM;
                return STANDARD_ERROR;
            }
    }
    return SUCCESS;
}

// function only expects a checksum_string from an NMEA message,
// so it must be 2 chars long and be alphanumeric
static uint8_t checksum_xtoi(const char* s){
    if(strlen(s) != 2){
        return 0;
    }
    
    char c;
    int i = 0;
    uint8_t result = 0;
    while((c = *s)){
        // find the hex value first
        uint8_t hexValue;
        if((c >= 'A') && (c <= 'F')){
            hexValue = (c - 55);
        }
        else if((c >= '0') && (c <= '9')){
            hexValue = (c - 48);
        }
        
        // now shift accordingly for corresponding hex position
        if(i == 0){
            // if parsing the (16^1) position; (sh left 4) = (mult. by 16)
            result += hexValue << 4;
        }
        else if(i == 1){
            result += hexValue;
        }
        s++; 
        i++;
    }
    
    return result;
}