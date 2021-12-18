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
#include <map>
#include <string>
/* ---------------------------------------------------------------------- */
class JSON {
 private:
  char * textCopy;
  bool debug = false;
  bool returnToParent = false;
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
      BOOLEAN_, NULL_ };

  static const size_t TOKEN_STACK_SIZE = 1024;
  TOKENS token[TOKEN_STACK_SIZE];
  void * symbolTableReference[TOKEN_STACK_SIZE];
  size_t stackPointer = 0;
  size_t inputIndex = 0;
  size_t arrayIndex = 0;
  char currentlyBuilding[1024];

  regex_t isAlpha;
  regex_t isNumber;
  regex_t isAlphaNumber;
  regex_t isNumberOnly;
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
  bool executeAction(ACTIONS currentAction, char currentInput, char * inputBuffer);
  bool buildString(char * buffer, char newCharacter);
  char * startString(void);
  char * endString(char * buffer);
  bool buildNumber(char * buffer, char newCharacter);
  char * startNumber(void);
  char * endNumber(char * buffer);
  bool buildKeyWord(char * buffer, char newCharacter);
  char * startKeyWord(void);
  char * endKeyWord(char * buffer);
  void checkAndReduce();
  bool parse(char * inputBuffer);
  void initialize(char * inputBuffer);
  char * getFirstIndex(char * name);

  std::map<std::string, char *> stringElements;
  std::map<std::string, char *> numberElements;
  std::map<std::string, bool> boolElements;
  std::map<std::string, bool> nullElements;
  std::map<std::string, JSON *> jsonElements;
  std::map<int, char *> aStringElements;
  std::map<int, char *> aNumberElements;
  std::map<int, bool> aBoolElements;
  std::map<int, bool> aNullElements;
  std::map<int, JSON *> aJsonElements;
  enum OBJECT_TYPES { UNKNOWN, JSON_OBJECT, JSON_ARRAY };

 public:
  OBJECT_TYPES thisIsA = UNKNOWN;
  const char * printToken(TOKENS token);
  const char * printState(STATES state);
  void printStack(void);
  void print(bool parent);
  const char * getValue(const char * name);
  const char * getValue(char * name);
  JSON();
  explicit JSON(char * jsonText);
  explicit JSON(const char * jsonText);
  ~JSON();
};
#endif  // INCLUDE_JSON_H_
