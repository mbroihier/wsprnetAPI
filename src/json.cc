/*
 *      JSON - Class for building JSON objects
 *
 *      Copyright (C) 2021
 *          Mark Broihier
 *
 */
/* ---------------------------------------------------------------------- */
#include "../include/json.h"
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
/*
 *      determineEquivalenceClass.cc -- determine the equivalence class of
 *        an input character
 *
 *      Copyright (C) 2021
 *          Mark Broihier
 *
 */

/* ---------------------------------------------------------------------- */
JSON::EQUIVALENCE_CLASSES JSON::determineEquivalenceClass(char inputCharacter) {
  char ic[2];
  EQUIVALENCE_CLASSES returnValue = NUMBER_OF_EQUIVALENCE_CLASSES;
  ic[0] = inputCharacter;
  ic[1] = 0;
  fprintf(stderr, "Classifying %s ", ic);
  if (!regexec(&isAlpha, ic, 0, 0, 0)) {
    returnValue = ALPHA;
  } else if (!regexec(&isNumber, ic, 0, 0, 0)) {
    returnValue = NUMERIC;
  } else if (!regexec(&isQuote, ic, 0, 0, 0)) {
    returnValue = QUOTE;
  } else if (!regexec(&isComma, ic, 0, 0, 0)) {
    returnValue = COMMA;
  } else if (!regexec(&isColon, ic, 0, 0, 0)) {
    returnValue = COLON;
  } else if (!regexec(&isLCB, ic, 0, 0, 0)) {
    returnValue = LEFT_BRACE;
  } else if (!regexec(&isRCB, ic, 0, 0, 0)) {
    returnValue = RIGHT_BRACE;
  } else if (!regexec(&isLB, ic, 0, 0, 0)) {
    returnValue = LEFT_BRACKET;
  } else if (!regexec(&isRB, ic, 0, 0, 0)) {
    returnValue = RIGHT_BRACKET;
  } else if (isspace(inputCharacter)) {
    returnValue = WHITE;
  } else {
    fprintf(stderr, "error, can't classify %s\n", ic);
  }
  fprintf(stderr, "as %d\n", returnValue);
  return(returnValue);
}
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
/*
 *      executeAction.cc -- perform actions implied by this state transition
 *
 *      Copyright (C) 2021
 *          Mark Broihier
 *
 */

/* ---------------------------------------------------------------------- */
bool JSON::executeAction(ACTIONS currentAction, char currentInput) {
  switch (currentAction) {
  case NOTHING: {
    fprintf(stderr, "Doing nothing with a %c\n", currentInput);
    break;
  }
  case RECORD_OBJECT_START: {
    fprintf(stderr, "Recording an object start with %c\n", currentInput);
    break;
  }
  case RECORD_OBJECT_END: {
    fprintf(stderr, "Recording an object end with a %c\n", currentInput);
    break;
  }
  case RECORD_STRING_START: {
    fprintf(stderr, "Recording a string start with a %c\n", currentInput);
    workspace = startString();
    break;
  }
  case RECORD_STRING: {
    fprintf(stderr, "Recording a string end with a %c\n", currentInput);
    workspace = endString(workspace);
    break;
  }
  case BUILD_STRING: {
    fprintf(stderr, "Building a string with a %c\n", currentInput);
    if (!buildString(workspace, currentInput)) {
      fprintf(stderr, "Ran out of space for the string being built\n");
      return(false);
    }
    break;
  }
  case BUILD_KEYWORD: {
    if (workspace == 0) {
      workspace = startKeyWord();
    }
    if (!buildKeyWord(workspace, currentInput)) {
      fprintf(stderr, "Improperly formatted key word\n");
      return(false);
    }
    fprintf(stderr, "Building a key word with a %c\n", currentInput);
    break;
  }
  case BUILD_NUMBER: {
    if (workspace == 0) {
      workspace = startNumber();
    }
    if (!buildNumber(workspace, currentInput)) {
      fprintf(stderr, "Ran out of space for the number being built\n");
      return(false);
    }
    fprintf(stderr, "Building a number with a %c\n", currentInput);
    break;
  }
  case RECORD_COLON: {
    fprintf(stderr, "Recording a colon with a %c\n", currentInput);
    break;
  }
  case RECORD_NUMBER: {
    fprintf(stderr, "Recording a number with a %c\n", currentInput);
    workspace = endNumber(workspace);
    if (currentInput == ',') {
      fprintf(stderr, "Recording a comma - lambda transition\n");
    } else if (currentInput == '}') {
      fprintf(stderr, "Recording an object end with right curly bracket - lambda transition\n");
    }
    break;
  }
  case RECORD_KEYWORD: {
    fprintf(stderr, "Recording a key word with a %c\n", currentInput);
    workspace = endKeyWord(workspace);
    if (currentInput == ',') {
      fprintf(stderr, "Recording a comma - lambda transition\n");
    }
    break;
  }
  case RECORD_LB: {
    fprintf(stderr, "Recording a left bracket with a %c\n", currentInput);
    break;
  }
  case RECORD_RB: {
    fprintf(stderr, "Recording a right bracket with a %c\n", currentInput);
    break;
  }
  case RECORD_COMMA: {
    fprintf(stderr, "Recording a comma with a %c\n", currentInput);
    break;
  }
  default:
    fprintf(stderr, "Unimplemented action %d\n", currentAction);
    return(false);
  }
  return(true);
}
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
/*
 *      parse.cc -- parse the input buffer into a JSON object
 *
 *      Copyright (C) 2021
 *          Mark Broihier
 *
 */

/* ---------------------------------------------------------------------- */
bool JSON::parse(char * inputBuffer) {
  int index = 0;
  char lexChar;
  EQUIVALENCE_CLASSES inputClass;
  ACTIONS action;
  fprintf(stderr, "Buffer to parse:\n%s\n", inputBuffer);
  while (inputBuffer[index] != 0) {
    lexChar = inputBuffer[index++];
    inputClass = determineEquivalenceClass(lexChar);
    if (inputClass == NUMBER_OF_EQUIVALENCE_CLASSES) {
      fprintf(stderr, "Stopping parse do to unexpected input\n");
      return(false);
    }
    nextState = StateTransitionMatrix[inputClass][state];
    if (nextState == ERROR) {
      fprintf(stderr, "Lexical parsing error - previous state %d, equivalence class %d\n", state,
              determineEquivalenceClass(lexChar));
      return(false);
    }
    action = ActionTransitionMatrix[inputClass][state];
    if (!executeAction(action, lexChar)) {
      fprintf(stderr, "Parsing error detected - terminating early\n");
      return(false);
    }
    state = nextState;
  }
  fprintf(stdout, "Parsing complete without error after processing %d characters\n", index);
  return(true);
}
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
/*
 *      getValue.cc -- get the JSON object value pointed to by name
 *
 *      Copyright (C) 2021
 *          Mark Broihier
 *
 */

/* ---------------------------------------------------------------------- */
char * JSON::getValue(const char * name) {
  char * result;
  return(result);
}
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
/*
 *      setValue.cc -- set the JSON object value pointed to by name
 *
 *      Copyright (C) 2021
 *          Mark Broihier
 *
 */

/* ---------------------------------------------------------------------- */
bool JSON::setValue(const char * name, char * value) {
  return(true);
}
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
/*
 *      startString.cc -- do processing to start the construction of
 *                        a string
 *
 *      Copyright (C) 2021
 *          Mark Broihier
 *
 */

/* ---------------------------------------------------------------------- */
char * JSON::startString(void) {
  char * buffer = reinterpret_cast<char *>(malloc(1024));
  memset(buffer, 0, 1024);
  return(buffer);
}
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
/*
 *      buildString.cc -- do processing to build a string 
 *                        character by character
 *
 *      Copyright (C) 2021
 *          Mark Broihier
 *
 */

/* ---------------------------------------------------------------------- */
bool JSON::buildString(char * buffer, char newCharacter) {
  bool status = false;
  buffer[strlen(buffer)] = newCharacter;
  if (strlen(buffer) < 1024) {
    status = true;
  }
  return(status);
}
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
/*
 *      endString.cc -- do processing to complete a string
 *
 *      Copyright (C) 2021
 *          Mark Broihier
 *
 */

/* ---------------------------------------------------------------------- */
char * JSON::endString(char * buffer) {
  fprintf(stderr, "a string has been built - it's value is:\n%s\n", buffer);
  free(buffer);
  return(NULL);
}
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
/*
 *      startNumber.cc -- do processing to start the construction of
 *                        a number
 *
 *      Copyright (C) 2021
 *          Mark Broihier
 *
 */

/* ---------------------------------------------------------------------- */
char * JSON::startNumber(void) {
  char * buffer = reinterpret_cast<char *>(malloc(1024));
  memset(buffer, 0, 1024);
  return(buffer);
}
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
/*
 *      buildNumber.cc -- do processing to build a number character by 
 *                        character
 *
 *      Copyright (C) 2021
 *          Mark Broihier
 *
 */

/* ---------------------------------------------------------------------- */
bool JSON::buildNumber(char * buffer, char newCharacter) {
  bool status = false;
  buffer[strlen(buffer)] = newCharacter;
  if (strlen(buffer) < 1024) {
    status = true;
  }
  return(status);
}
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
/*
 *      endNumber.cc -- do processing to finish a number
 *
 *      Copyright (C) 2021
 *          Mark Broihier
 *
 */

/* ---------------------------------------------------------------------- */
char * JSON::endNumber(char * buffer) {
  double number;
  sscanf(buffer, "%lf", &number);
  fprintf(stderr, "a number has been built - it's value is:\n%lf\n", number);
  fprintf(stderr, "buffer contents: %s\n", buffer);
  free(buffer);
  return(NULL);
}
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
/*
 *      startKeyWord.cc -- do processing to start the construction of
 *                         a key word
 *
 *      Copyright (C) 2021
 *          Mark Broihier
 *
 */

/* ---------------------------------------------------------------------- */
char * JSON::startKeyWord(void) {
  char * buffer = reinterpret_cast<char *>(malloc(64));
  memset(buffer, 0, 64);
  return(buffer);
}
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
/*
 *      buildKeyWord.cc -- do processing to build a key word character by 
 *                         character
 *
 *      Copyright (C) 2021
 *          Mark Broihier
 *
 */

/* ---------------------------------------------------------------------- */
bool JSON::buildKeyWord(char * buffer, char newCharacter) {
  bool status = false;
  buffer[strlen(buffer)] = newCharacter;
  if (strlen(buffer) < 64 && (strncmp(buffer, "true", strlen(buffer)) == 0 ||
                              strncmp(buffer, "false", strlen(buffer)) == 0 ||
                              strncmp(buffer, "null", strlen(buffer)) == 0)) {
    status = true;
  }
  return(status);
}
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
/*
 *      endKeyWord.cc -- do processing to finish a key word
 *
 *      Copyright (C) 2021
 *          Mark Broihier
 *
 */

/* ---------------------------------------------------------------------- */
char * JSON::endKeyWord(char * buffer) {
  fprintf(stderr, "a key word has been built - it's value is:\n%s\n", buffer);
  free(buffer);
  return(NULL);
}
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
/*
 *      JSON - constructor
 *
 *      Copyright (C) 2021
 *          Mark Broihier
 *
 */

/* ---------------------------------------------------------------------- */

JSON::JSON() {
  char buf[1024];
  int result;
  regcomp(&isAlpha, "[a-zA-Z_&//@]", REG_NOSUB);
  regcomp(&isNumber, "[0-9.+-]", REG_NOSUB);
  regcomp(&isLCB, "[\{]", REG_NOSUB);
  regcomp(&isRCB, "[}]", REG_NOSUB);
  regcomp(&isLB, "[\[]", REG_NOSUB);
  regcomp(&isRB, "[]]", REG_NOSUB);
  regcomp(&isColon, "[:]", REG_NOSUB);
  regcomp(&isComma, "[,]", REG_NOSUB);
  regcomp(&isQuote, "\"", REG_NOSUB);
  if (debug) {
    result = regexec(&isAlpha, "b", 0, 0, 0);
    regerror(result, &isAlpha, buf, 1024);
    fprintf(stderr, "test of b as alpha result value is %d: %s\n", result, buf);
    result = regexec(&isNumber, "5", 0, 0, 0);
    regerror(result, &isNumber, buf, 1024);
    fprintf(stderr, "test of 5 as number result value is %d: %s\n", result, buf);
    result = regexec(&isLCB, "{", 0, 0, 0);
    regerror(result, &isLCB, buf, 1024);
    fprintf(stderr, "test of { as left curly bracket result value is %d: %s\n", result, buf);
    result = regexec(&isRCB, "}", 0, 0, 0);
    regerror(result, &isRCB, buf, 1024);
    fprintf(stderr, "test of } as right curly bracket result value is %d: %s\n", result, buf);
    result = regexec(&isLB, "[", 0, 0, 0);
    regerror(result, &isLB, buf, 1024);
    fprintf(stderr, "test of [ as left bracket result value is %d: %s\n", result, buf);
    result = regexec(&isRB, "]", 0, 0, 0);
    regerror(result, &isRB, buf, 1024);
    fprintf(stderr, "test of ] as right bracket result value is %d: %s\n", result, buf);
    result = regexec(&isColon, ":", 0, 0, 0);
    regerror(result, &isColon, buf, 1024);
    fprintf(stderr, "test of : as colon result value is %d: %s\n", result, buf);
    result = regexec(&isComma, ",", 0, 0, 0);
    regerror(result, &isComma, buf, 1024);
    fprintf(stderr, "test of , as comma result value is %d: %s\n", result, buf);
    result = regexec(&isQuote, "\"", 0, 0, 0);
    regerror(result, &isQuote, buf, 1024);
    fprintf(stderr, "test of \" as quote result value is %d: %s\n", result, buf);
  }
  state = INITIAL;
}
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
/*
 *      JSON - destructor
 *
 *      Copyright (C) 2021 
 *          Mark Broihier
 *
 */
/* ---------------------------------------------------------------------- */

JSON::~JSON() {
}
/* ---------------------------------------------------------------------- */
