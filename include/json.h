#ifndef INCLUDE_JSON_H_
#define INCLUDE_JSON_H_
/*
 *      json.h - header for json objects
 *
 *      Copyright (C) 2021
 *          Mark Broihier
 *
 */
/* ---------------------------------------------------------------------- */
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
/* ---------------------------------------------------------------------- */
class JSON {
 private:
  bool debug = false;
  enum STATES {INITIAL, STRING, NUMBER, KEYWORD, ERROR, NUMBER_OF_STATES };
  STATES state;
  STATES nextState;
  enum EQUIVALENCE_CLASSES { LEFT_BRACE, RIGHT_BRACE, QUOTE, ALPHA, NUMERIC, COLON, COMMA, LEFT_BRACKET,
                             RIGHT_BRACKET, WHITE, NUMBER_OF_EQUIVALENCE_CLASSES };
  //  INITIAL        STRING           NUMBER     KEYWORD   ERROR
  STATES StateTransitionMatrix[NUMBER_OF_EQUIVALENCE_CLASSES][NUMBER_OF_STATES] =
    { INITIAL,       STRING,          ERROR,     ERROR,    ERROR,
      INITIAL,       STRING,          INITIAL,   ERROR,    ERROR,
      STRING,        INITIAL,         ERROR,     ERROR,    ERROR,
      KEYWORD,       STRING,          NUMBER,    KEYWORD,  ERROR,
      NUMBER,        STRING,          NUMBER,    ERROR,    ERROR,
      INITIAL,       STRING,          ERROR,     ERROR,    ERROR,
      INITIAL,       STRING,          INITIAL,   INITIAL,  ERROR,
      INITIAL,       STRING,          ERROR,     ERROR,    ERROR,
      INITIAL,       STRING,          INITIAL,   INITIAL,  ERROR,
      INITIAL,       STRING,          INITIAL,   INITIAL,  ERROR };

  enum ACTIONS
    { NOTHING, RECORD_OBJECT_START, RECORD_OBJECT_END, RECORD_STRING_START, RECORD_STRING, BUILD_STRING, BUILD_KEYWORD,
      BUILD_NUMBER, RECORD_COLON, RECORD_NUMBER, RECORD_KEYWORD, RECORD_LB, RECORD_RB, RECORD_COMMA };

  //  INITIAL              STRING           NUMBER         KEYWORD         ERROR
  ACTIONS ActionTransitionMatrix[NUMBER_OF_EQUIVALENCE_CLASSES][NUMBER_OF_STATES] =
    { RECORD_OBJECT_START, BUILD_STRING,    NOTHING,       NOTHING,        NOTHING,
      RECORD_OBJECT_END,   BUILD_STRING,    RECORD_NUMBER, NOTHING,        NOTHING,
      RECORD_STRING_START, RECORD_STRING,   NOTHING,       NOTHING,        NOTHING,
      BUILD_KEYWORD,       BUILD_STRING,    BUILD_NUMBER,  BUILD_KEYWORD,  NOTHING,
      BUILD_NUMBER,        BUILD_STRING,    BUILD_NUMBER,  NOTHING,        NOTHING,
      RECORD_COLON,        BUILD_STRING,    NOTHING,       NOTHING,        NOTHING,
      RECORD_COMMA,        BUILD_STRING,    RECORD_NUMBER, RECORD_KEYWORD, NOTHING,
      RECORD_LB,           BUILD_STRING,    NOTHING,       NOTHING,        NOTHING,
      RECORD_RB,           BUILD_STRING,    RECORD_NUMBER, RECORD_KEYWORD, NOTHING,
      NOTHING,             BUILD_STRING,    RECORD_NUMBER, RECORD_KEYWORD, NOTHING };

  enum TOKENS
    { OBJECT_, ARRAY_, STRING_, COLON_, COMMA_, NUMBER_, LEFT_BRACE_, RIGHT_BRACE_, LEFT_BRACKET_, RIGHT_BRACKET_,
      BOOLEAN_ };

  static const size_t TOKEN_STACK_SIZE = 1024;
  TOKENS tokens[TOKEN_STACK_SIZE];
  void * symbolTableReference[TOKEN_STACK_SIZE];
  size_t stackPointer = 0;
  
  regex_t isAlpha;
  regex_t isNumber;
  regex_t isLCB;
  regex_t isRCB;
  regex_t isLB;
  regex_t isRB;
  regex_t isColon;
  regex_t isComma;
  regex_t isQuote;
  regex_t isWhite;

  char * workspace;
  
  EQUIVALENCE_CLASSES determineEquivalenceClass(char inputCharacter);
  bool executeAction(ACTIONS currentAction, char currentInput);
  bool buildString(char * buffer, char newCharacter);
  char * startString(void);
  char * endString(char * buffer);
  bool buildNumber(char * buffer, char newCharacter);
  char * startNumber(void);
  char * endNumber(char * buffer);
  bool buildKeyWord(char * buffer, char newCharacter);
  char * startKeyWord(void);
  char * endKeyWord(char * buffer);
  
 public:
  bool parse(char * inputBuffer);
  char * getValue(const char * name);
  bool setValue(const char * name, char * value);
  JSON();
  ~JSON();
};
#endif  // INCLUDE_JSON_H_
