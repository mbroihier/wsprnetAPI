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
bool JSON::executeAction(ACTIONS currentAction, char currentInput, char * inputBuffer) {
  switch (currentAction) {
  case NOTHING: {
    fprintf(stderr, "Doing nothing with a %c\n", currentInput);
    break;
  }
  case RECORD_OBJECT_START: {
    if (thisIsA == UNKNOWN) {
      thisIsA = JSON_OBJECT;
      fprintf(stderr, "Recording an object start with %c\n", currentInput);
      symbolTableReference[stackPointer] = NULL;
      token[stackPointer++] = LEFT_BRACE_;
      snprintf(currentlyBuilding, sizeof(currentlyBuilding), "building object");
    } else {
      fprintf(stderr, "Making a child JSON object\n");
      JSON * child = new(JSON);
      if (child->parse(inputBuffer+inputIndex-1)) {
        symbolTableReference[stackPointer] = child;
        token[stackPointer++] = OBJECT_;
        for (size_t i = inputIndex; i < strlen(inputBuffer); i++) {
          if (inputBuffer[i] == '}') {
            inputIndex = i + 1;
            break;
          }
        }
        fprintf(stderr, "recording JSON child object, parsing continues at %d\n", inputIndex);
      } else {
        fprintf(stderr, "Child object failed to parse - stopping\n");
        return(false);
      }
    }
    break;
  }
  case RECORD_OBJECT_END: {
    fprintf(stderr, "Recording an object end with a %c\n", currentInput);
    symbolTableReference[stackPointer] = NULL;
    token[stackPointer++] = RIGHT_BRACE_;
    snprintf(currentlyBuilding, sizeof(currentlyBuilding), "doing nothing");
    returnToParent = true;
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
    token[stackPointer++] = STRING_;
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
    symbolTableReference[stackPointer] = NULL;
    token[stackPointer++] = COLON_;
    break;
  }
  case RECORD_NUMBER: {
    fprintf(stderr, "Recording a number with a %c\n", currentInput);
    workspace = endNumber(workspace);
    token[stackPointer++] = NUMBER_;
    if (currentInput == ',') {
      symbolTableReference[stackPointer] = NULL;
      token[stackPointer++] = COMMA_;
      fprintf(stderr, "Recording a comma - lambda transition\n");
    } else if (currentInput == '}') {
      symbolTableReference[stackPointer] = NULL;
      token[stackPointer++] = RIGHT_BRACE_;
      fprintf(stderr, "Recording an object end with right curly bracket - lambda transition\n");
      returnToParent = true;
    }
    break;
  }
  case RECORD_KEYWORD: {
    fprintf(stderr, "Recording a key word with a %c\n", currentInput);
    workspace = endKeyWord(workspace);
    if (strcmp(workspace, "true") == 0) {
      token[stackPointer++] = BOOLEAN_;
    } else if (strcmp(workspace, "false") == 0) {
      token[stackPointer++] = BOOLEAN_;
    } else if (strcmp(workspace, "null") == 0) {
      token[stackPointer++] = NULL_;
    } else {
      fprintf(stderr, "Unrecognized key word %s\n", workspace);
    }
    if (currentInput == ',') {
      symbolTableReference[stackPointer] = NULL;
      token[stackPointer++] = COMMA_;
      fprintf(stderr, "Recording a comma - lambda transition\n");
    } else if (currentInput == '}') {
      symbolTableReference[stackPointer] = NULL;
      token[stackPointer++] = RIGHT_BRACE_;
      fprintf(stderr, "Recording an object end with right curly bracket - lambda transition\n");
      returnToParent = true;
    }
    break;
  }
  case RECORD_LB: {
    if (thisIsA == UNKNOWN) {
      thisIsA = JSON_ARRAY;
      symbolTableReference[stackPointer] = NULL;
      token[stackPointer++] = LEFT_BRACKET_;
      fprintf(stderr, "Recording an array start with a %c\n", currentInput);
      snprintf(currentlyBuilding, sizeof(currentlyBuilding), "building array");
    } else {
      fprintf(stderr, "Making a child ARRAY object\n");
      JSON * child = new(JSON);
      if (child->parse(inputBuffer+inputIndex-1)) {
        symbolTableReference[stackPointer] = child;
        token[stackPointer++] = OBJECT_;
        for (size_t i = inputIndex; i < strlen(inputBuffer); i++) {
          if (inputBuffer[i] == '}') {
            inputIndex = i + 1;
            break;
          }
        }
        fprintf(stderr, "recording JSON child array object, parsing continues at %d\n", inputIndex);
      } else {
        fprintf(stderr, "Child array object failed to parse - stopping\n");
        return(false);
      }
    }
    break;
  }
  case RECORD_RB: {
    symbolTableReference[stackPointer] = NULL;
    token[stackPointer++] = RIGHT_BRACKET_;
    fprintf(stderr, "Recording an array object end with a %c\n", currentInput);
    snprintf(currentlyBuilding, sizeof(currentlyBuilding), "doing nothing");
    returnToParent = true;
    break;
  }
  case RECORD_COMMA: {
    symbolTableReference[stackPointer] = NULL;
    token[stackPointer++] = COMMA_;
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
  char lexChar;
  EQUIVALENCE_CLASSES inputClass;
  ACTIONS action;
  fprintf(stderr, "Buffer to parse:\n%s input index is %d\n", inputBuffer, inputIndex);
  while (inputBuffer[inputIndex] != 0 && !returnToParent) {
    lexChar = inputBuffer[inputIndex++];
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
    if (!executeAction(action, lexChar, inputBuffer)) {
      fprintf(stderr, "Parsing error detected - terminating early\n");
      return(false);
    }
    state = nextState;
    printStack();
    checkAndReduce();
  }
  fprintf(stdout, "Parsing complete without error after processing %d characters\n", inputIndex);
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
  symbolTableReference[stackPointer] = buffer;
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
  symbolTableReference[stackPointer] = buffer;
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
  symbolTableReference[stackPointer] = buffer;
  return(NULL);
}
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
/*
 *      checkAndReduce - checks the stack, and if there is a name value
 *                       pattern, reduce it
 *
 *      Copyright (C) 2021
 *          Mark Broihier
 *
 */

/* ---------------------------------------------------------------------- */

void JSON::checkAndReduce() {
  if (thisIsA == JSON_OBJECT) {
    if (stackPointer > 4 && token[stackPointer - 1] == COMMA_ && token[stackPointer - 2] == STRING_ &&
        token[stackPointer - 3] == COLON_ && token[stackPointer - 4] == STRING_) {
      fprintf(stderr, "A reduction can occur that creates a JSON entry that is a string\n");
      stringElements[std::string((char *)symbolTableReference[stackPointer - 4])] =
        (char *)symbolTableReference[stackPointer - 2];
      free(symbolTableReference[stackPointer - 4]);  // free dangling cstr rest will be done in destructor
      stackPointer -= 4;
      printStack();
    } else if (stackPointer > 4 && token[stackPointer - 1] == COMMA_ && token[stackPointer - 2] == NUMBER_ &&
               token[stackPointer - 3] == COLON_ && token[stackPointer - 4] == STRING_) {
      fprintf(stderr, "A reduction can occur that creates a JSON entry that is a number\n");
      numberElements[std::string((char *)symbolTableReference[stackPointer - 4])] =
        (char *)symbolTableReference[stackPointer - 2];
      free(symbolTableReference[stackPointer - 4]);  // free dangling cstr rest will be done in destructor
      stackPointer -= 4;
      printStack();
    } else if (stackPointer > 4 && token[stackPointer - 1] == COMMA_ && token[stackPointer - 2] == BOOLEAN_ &&
               token[stackPointer - 3] == COLON_ && token[stackPointer - 4] == STRING_) {
      fprintf(stderr, "A reduction can occur that creates a JSON entry that is a boolean\n");
      boolElements[std::string((char *)symbolTableReference[stackPointer - 4])] =
        strcmp((char *)symbolTableReference[stackPointer - 2], "true") == 0;
      free(symbolTableReference[stackPointer - 4]);  // free dangling cstr
      stackPointer -= 4;
      printStack();
    } else if (stackPointer > 4 && token[stackPointer - 1] == COMMA_ && token[stackPointer - 2] == NULL_ &&
               token[stackPointer - 3] == COLON_ && token[stackPointer - 4] == STRING_) {
      fprintf(stderr, "A reduction can occur that creates a JSON entry that is null\n");
      boolElements[std::string((char *)symbolTableReference[stackPointer - 4])] = true;
      free(symbolTableReference[stackPointer - 4]);  // free dangling cstr
      stackPointer -= 4;
    } else if (stackPointer > 4 && token[stackPointer - 1] == COMMA_ && token[stackPointer - 2] == OBJECT_ &&
               token[stackPointer - 3] == COLON_ && token[stackPointer - 4] == STRING_) {
      fprintf(stderr, "A reduction can occur that creates a JSON entry that is another JSON object\n");
      jsonElements[std::string((char *)symbolTableReference[stackPointer - 4])] =
        (JSON *)symbolTableReference[stackPointer - 2];
      free(symbolTableReference[stackPointer - 4]);  // free dangling cstr rest will be done in destructor
      stackPointer -= 4;
      printStack();
    } else if (stackPointer > 4 && token[stackPointer - 1] == RIGHT_BRACE_ && token[stackPointer - 2] == STRING_ &&
               token[stackPointer - 3] == COLON_ && token[stackPointer - 4] == STRING_) {
      fprintf(stderr, "A reduction can occur that creates a JSON entry that is a string\n");
      stringElements[std::string((char *)symbolTableReference[stackPointer - 4])] =
        (char *)symbolTableReference[stackPointer - 2];
      free(symbolTableReference[stackPointer - 4]);  // free dangling cstr rest will be done in destructor
      stackPointer -= 4;
      printStack();
    } else if (stackPointer > 4 && token[stackPointer - 1] == RIGHT_BRACE_ && token[stackPointer - 2] == NUMBER_ &&
               token[stackPointer - 3] == COLON_ && token[stackPointer - 4] == STRING_) {
      fprintf(stderr, "A reduction can occur that creates a JSON entry that is a number\n");
      numberElements[std::string((char *)symbolTableReference[stackPointer - 4])] =
        (char *)symbolTableReference[stackPointer - 2];
      free(symbolTableReference[stackPointer - 4]);  // free dangling cstr rest will be done in destructor
      stackPointer -= 4;
      printStack();
    } else if (stackPointer > 4 && token[stackPointer - 1] == RIGHT_BRACE_ && token[stackPointer - 2] == BOOLEAN_ &&
               token[stackPointer - 3] == COLON_ && token[stackPointer - 4] == STRING_) {
      fprintf(stderr, "A reduction can occur that creates a JSON entry that is a boolean\n");
      boolElements[std::string((char *)symbolTableReference[stackPointer - 4])] =
        strcmp((char *)symbolTableReference[stackPointer - 2], "true") == 0;
      free(symbolTableReference[stackPointer - 4]);  // free dangling cstr
      stackPointer -= 4;
      printStack();
    } else if (stackPointer > 4 && token[stackPointer - 1] == RIGHT_BRACE_ && token[stackPointer - 2] == NULL_ &&
               token[stackPointer - 3] == COLON_ && token[stackPointer - 4] == STRING_) {
      fprintf(stderr, "A reduction can occur that creates a JSON entry that is null\n");
      boolElements[std::string((char *)symbolTableReference[stackPointer - 4])] = true;
      free(symbolTableReference[stackPointer - 4]);  // free dangling cstr
      stackPointer -= 4;
      printStack();
    } else if (stackPointer > 4 && token[stackPointer - 1] == RIGHT_BRACE_ && token[stackPointer - 2] == OBJECT_ &&
               token[stackPointer - 3] == COLON_ && token[stackPointer - 4] == STRING_) {
      fprintf(stderr, "A reduction can occur that creates a JSON entry that is another JSON object\n");
      jsonElements[std::string((char *)symbolTableReference[stackPointer - 4])] =
        (JSON *)symbolTableReference[stackPointer - 2];
      free(symbolTableReference[stackPointer - 4]);  // free dangling cstr rest will be done in destructor
      stackPointer -= 4;
      printStack();
    } else {
      fprintf(stderr, "No reduction occurred\n");
    }
  } else if (thisIsA == JSON_ARRAY) {
    if (stackPointer > 2 && token[stackPointer - 1] == OBJECT_ && token[stackPointer - 2] == COMMA_) {
      fprintf(stderr, "A reduction can occur that creates a JSON entry that is a array element\n");
      aJsonElements[arrayIndex++] = (JSON *)symbolTableReference[stackPointer - 1];
      stackPointer -= 2;
      printStack();
    } else if (stackPointer > 2 && token[stackPointer - 1] == OBJECT_ && token[stackPointer - 2] == RIGHT_BRACKET_) {
      fprintf(stderr, "A reduction can occur that creates a JSON entry that is a array element\n");
      aJsonElements[arrayIndex++] = (JSON *)symbolTableReference[stackPointer - 1];
      stackPointer -= 2;
      printStack();
    } else if (stackPointer > 2 && token[stackPointer - 1] == STRING_ && token[stackPointer - 2] == COMMA_) {
      fprintf(stderr, "A reduction can occurs that creates a STRING that is a array element\n");
      aStringElements[arrayIndex++] = (char *)symbolTableReference[stackPointer - 1];
      stackPointer -= 2;
      printStack();
    } else if (stackPointer > 2 && token[stackPointer - 1] == STRING_ && token[stackPointer - 2] == RIGHT_BRACKET_) {
      fprintf(stderr, "A reduction can occur that creates a STRING that is a array element\n");
      aStringElements[arrayIndex++] = (char *)symbolTableReference[stackPointer - 1];
      stackPointer -= 2;
      printStack();
    } else if (stackPointer > 2 && token[stackPointer - 1] == NUMBER_ && token[stackPointer - 2] == COMMA_) {
      fprintf(stderr, "A reduction can occur that creates a STRING that is a array element\n");
      aNumberElements[arrayIndex++] = (char *)symbolTableReference[stackPointer - 1];
      stackPointer -= 2;
      printStack();
    } else if (stackPointer > 2 && token[stackPointer - 1] == NUMBER_ && token[stackPointer - 2] == RIGHT_BRACKET_) {
      fprintf(stderr, "A reduction can occur that creates a STRING that is a array element\n");
      aNumberElements[arrayIndex++] = (char *)symbolTableReference[stackPointer - 1];
      stackPointer -= 2;
      printStack();
    } else if (stackPointer > 2 && token[stackPointer - 1] == BOOLEAN_ && token[stackPointer - 2] == COMMA_) {
      fprintf(stderr, "A reduction can occur that creates a BOOLEAN that is a array element\n");
      aBoolElements[arrayIndex++] = strcmp((char *)symbolTableReference[stackPointer - 1], "true") == 0;
      stackPointer -= 2;
      printStack();
    } else if (stackPointer > 2 && token[stackPointer - 1] == BOOLEAN_ && token[stackPointer - 2] == RIGHT_BRACKET_) {
      fprintf(stderr, "A reduction can occur that creates a STRING that is a array element\n");
      aBoolElements[arrayIndex++] = strcmp((char *)symbolTableReference[stackPointer - 1], "true") == 0;
      stackPointer -= 2;
      printStack();
    } else if (stackPointer > 2 && token[stackPointer - 1] == NULL_ && token[stackPointer - 2] == COMMA_) {
      fprintf(stderr, "A reduction can occur that creates a BOOLEAN that is a array element\n");
      aNullElements[arrayIndex++] = true;
      stackPointer -= 2;
      printStack();
    } else if (stackPointer > 2 && token[stackPointer - 1] == NULL_ && token[stackPointer - 2] == RIGHT_BRACKET_) {
      fprintf(stderr, "A reduction can occur that creates a STRING that is a array element\n");
      aNullElements[arrayIndex++] = true;
      stackPointer -= 2;
      printStack();
    } else {
      fprintf(stderr, "No reduction occurred\n");
    }
  } else {
    fprintf(stderr, "Shouldn't be checking for a reduction if unknown object type\n");
  }
}
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
/*
 *      printStack - print the contents of the current parse stack
 *
 *      Copyright (C) 2021
 *          Mark Broihier
 *
 */

/* ---------------------------------------------------------------------- */

void JSON::printStack() {
  fprintf(stdout, "Currently %s in state %s\n", currentlyBuilding, printState(state));
  for (int index = 0; index < stackPointer; index++) {
    fprintf(stdout, " %4d : %s %p\n", index, printToken(token[index]), symbolTableReference[index]);
  }
}
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
/*
 *      printToken - print the text of a token
 *
 *      Copyright (C) 2021
 *          Mark Broihier
 *
 */

/* ---------------------------------------------------------------------- */

const char * JSON::printToken(TOKENS token) {
  switch (token) {
  case OBJECT_ : return("OBJECT_");
  case ARRAY_ : return("ARRAY_");
  case STRING_ : return("STRING_");
  case COLON_ : return("COLON_");
  case COMMA_ : return("COMMA_");
  case NUMBER_ : return("NUMBER_");
  case LEFT_BRACE_ : return("LEFT_BRACE_");
  case RIGHT_BRACE_ : return("RIGHT_BRACE_");
  case LEFT_BRACKET_ : return("LEFT_BRACKET_");
  case RIGHT_BRACKET_ : return("RIGHT_BRACKET_");
  case BOOLEAN_ : return("BOOLEAN_");
  case NULL_ : return("NULL_");
  default: fprintf(stderr, "Token not mapped - %d\n", token);
  }
  return("?");
}
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
/*
 *      printState - print the text of a state
 *
 *      Copyright (C) 2021
 *          Mark Broihier
 *
 */

/* ---------------------------------------------------------------------- */

const char * JSON::printState(STATES state) {
  switch (state) {
  case INITIAL : return("INITIAL");
  case STRING : return("STRING");
  case NUMBER : return("NUMBER");
  case KEYWORD : return("KEYWORD");
  case ERROR : return("ERROR");
  default: fprintf(stderr, "State not mapped - %d\n", state);
  }
  return("?");
}
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
/*
 *      print - print the Object
 *
 *      Copyright (C) 2021
 *          Mark Broihier
 *
 */

/* ---------------------------------------------------------------------- */

void JSON::print(bool parent=false) {
  bool printedSomething = false;
  if (thisIsA == JSON_OBJECT) {
    fprintf(stdout, "{");
    {
      std::map<std::string, char *>::iterator iterator;
      for (iterator = stringElements.begin(); iterator != stringElements.end(); iterator++) {
        if (printedSomething) fprintf(stdout, ", ");
        fprintf(stdout, "\"%s\" : \"%s\"", (iterator->first).c_str(), iterator->second);
        printedSomething = true;
      }
      for (iterator = numberElements.begin(); iterator != numberElements.end(); iterator++) {
        if (printedSomething) fprintf(stdout, ", ");
        fprintf(stdout, "\"%s\" : %s", (iterator->first).c_str(), iterator->second);
        printedSomething = true;
      }
    }
    {
      std::map<std::string, bool>::iterator iterator;
      for (iterator = boolElements.begin(); iterator != boolElements.end(); iterator++) {
        if (printedSomething) fprintf(stdout, ", ");
        fprintf(stdout, "\"%s\" : %s", (iterator->first).c_str(), iterator->second ? "true" : "false");
        printedSomething = true;
      }
      for (iterator = nullElements.begin(); iterator != nullElements.end(); iterator++) {
        if (printedSomething) fprintf(stdout, ", ");
        fprintf(stdout, "\"%s\" : %s", (iterator->first).c_str(), "null");
        printedSomething = true;
      }
    }
    {
      std::map<std::string, JSON *>::iterator iterator;
      for (iterator = jsonElements.begin(); iterator != jsonElements.end(); iterator++) {
        if (printedSomething) fprintf(stdout, ", ");
        fprintf(stdout, "\"%s\" : ", (iterator->first).c_str());
        (iterator->second)->print();
        printedSomething = true;
      }
    }
    if (parent) {
      fprintf(stdout, "}\n");
    } else {
      fprintf(stdout, "}");
    }
  } else {  // this is a JSON array object
    fprintf(stdout, "[");
    std::map<int, char*>::iterator sit;
    std::map<int, char*>::iterator numit;
    std::map<int, bool>::iterator bit;
    std::map<int, bool>::iterator nullit;
    std::map<int, JSON *>::iterator jit;
    
    for (int arrayObjectIndex = 0; arrayObjectIndex < arrayIndex; arrayObjectIndex++) {
      sit = aStringElements.find(arrayObjectIndex);
      if (sit == aStringElements.end()) {
        numit = aNumberElements.find(arrayObjectIndex);
        if (numit == aNumberElements.end()) {
          bit = aBoolElements.find(arrayObjectIndex);
          if (bit == aBoolElements.end()) {
            nullit = aNullElements.find(arrayObjectIndex);
            if (nullit == aNullElements.end()) {
              if (printedSomething) fprintf(stdout, ", ");
              jit = aJsonElements.find(arrayObjectIndex);
              (jit->second)->print();
              printedSomething = true;
            } else {  //null
              if (printedSomething) fprintf(stdout, ", ");
              fprintf(stdout, "%s", "null");
              printedSomething = true;
            }
          } else {  //boolean
            if (printedSomething) fprintf(stdout, ", ");
            fprintf(stdout, "%s", bit->second ? "true" : "false");
            printedSomething = true;
          }            
        } else {  // number
          if (printedSomething) fprintf(stdout, ", ");
          fprintf(stdout, "%s", numit->second);
          printedSomething = true;
        } 
      } else {  // string
        if (printedSomething) fprintf(stdout, ", ");
        fprintf(stdout, "%s", sit->second);
        printedSomething = true;
      }
    }
    if (parent) {
      fprintf(stdout, "]\n");
    } else {
      fprintf(stdout, "]");
    }
      
  }
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
  memset(currentlyBuilding, 0, sizeof(currentlyBuilding));
  snprintf(currentlyBuilding, sizeof(currentlyBuilding), "doing nothing");
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
  if (thisIsA == JSON_OBJECT) {
    {
      std::map<std::string, char *>::iterator iterator;
      for (iterator = stringElements.begin(); iterator != stringElements.end(); iterator++) {
        free(iterator->second);
      }
      for (iterator = numberElements.begin(); iterator != numberElements.end(); iterator++) {
        free(iterator->second);
      }
    }
    {
      std::map<std::string, JSON *>::iterator iterator;
      for (iterator = jsonElements.begin(); iterator != jsonElements.end(); iterator++) {
        delete(iterator->second);
      }
    }
    stringElements.clear();
    numberElements.clear();
    boolElements.clear();
    nullElements.clear();
    jsonElements.clear();
  } else {
    std::map<int, char*>::iterator sit;
    std::map<int, char*>::iterator numit;
    std::map<int, JSON *>::iterator jit;
    for (int arrayObjectIndex = 0; arrayObjectIndex < arrayIndex; arrayObjectIndex++) {
      sit = aStringElements.find(arrayObjectIndex);
      if (sit == aStringElements.end()) {
        numit = aNumberElements.find(arrayObjectIndex);
        if (numit == aNumberElements.end()) {
          jit = aJsonElements.find(arrayObjectIndex);
          if (jit != aJsonElements.end()) {
            delete(jit->second);  // delete the JSON object being referenced
          }
        } else {  // free character memory
          free(numit->second);
        } 
      } else {  // free character memory
        free(sit->second);
      }
    }
    aStringElements.clear();
    aNumberElements.clear();
    aBoolElements.clear();
    aNullElements.clear();
    aJsonElements.clear();
  }
}
/* ---------------------------------------------------------------------- */
