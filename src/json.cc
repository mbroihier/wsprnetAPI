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
  if (debug) fprintf(stderr, "Classifying %s ", ic);
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
  if (debug) fprintf(stderr, "as %d\n", returnValue);
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
    if (debug) fprintf(stderr, "Doing nothing with a %c\n", currentInput);
    break;
  }
  case RECORD_OBJECT_START: {
    if (thisIsA == UNKNOWN) {
      thisIsA = JSON_OBJECT;
      if (debug) fprintf(stderr, "Recording an object start with %c\n", currentInput);
      symbolTableReference[stackPointer] = NULL;
      token[stackPointer++] = LEFT_BRACE_;
      snprintf(currentlyBuilding, sizeof(currentlyBuilding), "building object");
    } else {
      if (debug) fprintf(stderr, "Making a child JSON object\n");
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
        if (debug) fprintf(stderr, "recording JSON child object, parsing continues at %d\n", inputIndex);
      } else {
        fprintf(stderr, "Child object failed to parse - stopping\n");
        return(false);
      }
    }
    break;
  }
  case RECORD_OBJECT_END: {
    if (debug) fprintf(stderr, "Recording an object end with a %c\n", currentInput);
    symbolTableReference[stackPointer] = NULL;
    token[stackPointer++] = RIGHT_BRACE_;
    snprintf(currentlyBuilding, sizeof(currentlyBuilding), "doing nothing");
    returnToParent = true;
    break;
  }
  case RECORD_STRING_START: {
    if (debug) fprintf(stderr, "Recording a string start with a %c\n", currentInput);
    workspace = startString();
    break;
  }
  case RECORD_STRING: {
    if (debug) fprintf(stderr, "Recording a string end with a %c\n", currentInput);
    workspace = endString(workspace);
    token[stackPointer++] = STRING_;
    break;
  }
  case BUILD_STRING: {
    if (debug) fprintf(stderr, "Building a string with a %c\n", currentInput);
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
    if (debug) fprintf(stderr, "Building a key word with a %c\n", currentInput);
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
    if (debug) fprintf(stderr, "Building a number with a %c\n", currentInput);
    break;
  }
  case RECORD_COLON: {
    if (debug) fprintf(stderr, "Recording a colon with a %c\n", currentInput);
    symbolTableReference[stackPointer] = NULL;
    token[stackPointer++] = COLON_;
    break;
  }
  case RECORD_NUMBER: {
    if (debug) fprintf(stderr, "Recording a number with a %c\n", currentInput);
    workspace = endNumber(workspace);
    token[stackPointer++] = NUMBER_;
    if (currentInput == ',') {
      symbolTableReference[stackPointer] = NULL;
      token[stackPointer++] = COMMA_;
      if (debug) fprintf(stderr, "Recording a comma - lambda transition\n");
    } else if (currentInput == '}') {
      symbolTableReference[stackPointer] = NULL;
      token[stackPointer++] = RIGHT_BRACE_;
      if (debug) fprintf(stderr, "Recording an object end with right curly bracket - lambda transition\n");
      returnToParent = true;
    }
    break;
  }
  case RECORD_KEYWORD: {
    if (debug) fprintf(stderr, "Recording a key word with a %c\n", currentInput);
    endKeyWord(workspace);  // don't null out workspace yet
    if (strcmp(workspace, "true") == 0) {
      token[stackPointer++] = BOOLEAN_;
    } else if (strcmp(workspace, "false") == 0) {
      token[stackPointer++] = BOOLEAN_;
    } else if (strcmp(workspace, "null") == 0) {
      token[stackPointer++] = NULL_;
    } else {
      fprintf(stderr, "Unrecognized key word %s\n", workspace);
    }
    if (workspace) {  // we can now delete unused cstr
      free(workspace);
      workspace = 0;
    }
    if (currentInput == ',') {
      symbolTableReference[stackPointer] = NULL;
      token[stackPointer++] = COMMA_;
      if (debug) fprintf(stderr, "Recording a comma - lambda transition\n");
    } else if (currentInput == '}') {
      symbolTableReference[stackPointer] = NULL;
      token[stackPointer++] = RIGHT_BRACE_;
      if (debug) fprintf(stderr, "Recording an object end with right curly bracket - lambda transition\n");
      returnToParent = true;
    }
    break;
  }
  case RECORD_LB: {
    if (thisIsA == UNKNOWN) {
      thisIsA = JSON_ARRAY;
      symbolTableReference[stackPointer] = NULL;
      token[stackPointer++] = LEFT_BRACKET_;
      if (debug) fprintf(stderr, "Recording an array start with a %c\n", currentInput);
      snprintf(currentlyBuilding, sizeof(currentlyBuilding), "building array");
    } else {
      if (debug) fprintf(stderr, "Making a child ARRAY object\n");
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
        if (debug) fprintf(stderr, "recording JSON child array object, parsing continues at %d\n", inputIndex);
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
    if (debug) fprintf(stderr, "Recording an array object end with a %c\n", currentInput);
    snprintf(currentlyBuilding, sizeof(currentlyBuilding), "doing nothing");
    returnToParent = true;
    break;
  }
  case RECORD_COMMA: {
    symbolTableReference[stackPointer] = NULL;
    token[stackPointer++] = COMMA_;
    if (debug) fprintf(stderr, "Recording a comma with a %c\n", currentInput);
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
  if (debug) fprintf(stderr, "Buffer to parse:\n%s input index is %d\n", inputBuffer, inputIndex);
  while (inputBuffer[inputIndex] != 0 && !returnToParent) {
    lexChar = inputBuffer[inputIndex++];
    inputClass = determineEquivalenceClass(lexChar);
    if (inputClass == NUMBER_OF_EQUIVALENCE_CLASSES) {
      fprintf(stderr, "Stopping parse do to unexpected input, %c, at location %d\n", lexChar, inputIndex - 1);
      fprintf(stderr, "inputBuffer:\n%s\n", inputBuffer);
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
    if (debug) fprintf(stdout, "Before Stack\n");
    if (debug) printStack();
    checkAndReduce();
    if (debug) fprintf(stdout, "After Stack\n");
    if (debug) printStack();
  }
  if (debug) fprintf(stderr, "Parsing complete without error after processing %d characters\n", inputIndex);
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
  char * copyOfName = strdup(name);
  result = getValue(copyOfName);
  free(copyOfName);
  return(result);
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
char * JSON::getValue(char * name) {
  const size_t resultSize = 128;
  char * result;
  char * copyOfName = strdup(name);
  bool isArrayIndex = name[0] == '[';
  if (debug) fprintf(stderr, "strdup addresses (before): %p, size: %d\n", copyOfName, strlen(copyOfName));
  char * index = getFirstIndex(copyOfName);
  if (debug) fprintf(stderr, "first index: %s, remainder: %s\n", index, copyOfName);
  if (isArrayIndex) {
    std::map<int, char*>::iterator sit;
    std::map<int, char*>::iterator numit;
    std::map<int, bool>::iterator bit;
    std::map<int, bool>::iterator nullit;
    std::map<int, JSON *>::iterator jit;
    int arrayIndex;
    sscanf(index, "%d", &arrayIndex);
    // return what is at that array index
    sit = aStringElements.find(arrayIndex);
    if (sit == aStringElements.end()) {
      numit = aNumberElements.find(arrayIndex);
      if (numit == aNumberElements.end()) {
        bit = aBoolElements.find(arrayIndex);
        if (bit == aBoolElements.end()) {
          nullit = aNullElements.find(arrayIndex);
          if (nullit == aNullElements.end()) {
            jit = aJsonElements.find(arrayIndex);
            if (jit == aJsonElements.end()) {
              // index was not found - return null
              result = NULL;
            } else {
              result = (jit->second)->getValue(copyOfName);
            }
          } else {  // is null
            result = reinterpret_cast<char *>(malloc(resultSize));
            memset(result, 0, resultSize);
            snprintf(result, resultSize, "null");
          }
        } else {  // is boolean
          result = reinterpret_cast<char *>(malloc(resultSize));
          memset(result, 0, resultSize);
          snprintf(result, resultSize, "%s", bit->second ? "true" : "false");
        }
      } else {  // is number
        result = reinterpret_cast<char *>(malloc(resultSize));
        memset(result, 0, resultSize);
        snprintf(result, resultSize, "%s", numit->second);
      }
    } else {  // is string
      result = reinterpret_cast<char *>(malloc(resultSize));
      memset(result, 0, resultSize);
      snprintf(result, resultSize, "%s", sit->second);
    }
  } else {  // JSON Object
    std::map<std::string, char*>::iterator sit;
    std::map<std::string, char*>::iterator numit;
    std::map<std::string, bool>::iterator bit;
    std::map<std::string, bool>::iterator nullit;
    std::map<std::string, JSON *>::iterator jit;
    sit = stringElements.find(std::string(index));
    if (sit == stringElements.end()) {
      numit = numberElements.find(std::string(index));
      if (numit == numberElements.end()) {
        bit = boolElements.find(std::string(index));
        if (bit == boolElements.end()) {
          nullit = nullElements.find(std::string(index));
          if (nullit == nullElements.end()) {
            jit = jsonElements.find(std::string(index));
            if (jit == jsonElements.end()) {
              // element not found, return null
              result = NULL;
            } else {
              result = (jit->second)->getValue(copyOfName);
            }
          } else {  // is null
            result = reinterpret_cast<char *>(malloc(resultSize));
            memset(result, 0, resultSize);
            snprintf(result, resultSize, "null");
          }
        } else {  // is boolean
          result = reinterpret_cast<char *>(malloc(resultSize));
          memset(result, 0, resultSize);
          snprintf(result, resultSize, "%s", bit->second ? "true" : "false");
        }
      } else {  // is number
        result = reinterpret_cast<char *>(malloc(resultSize));
        memset(result, 0, resultSize);
        snprintf(result, resultSize, "%s", numit->second);
      }
    } else {  // is string
      result = reinterpret_cast<char *>(malloc(resultSize));
      memset(result, 0, resultSize);
      snprintf(result, resultSize, "%s", sit->second);
    }
  }
  if (debug) fprintf(stderr, "strdup addresses (after): %p, size: %d\n", copyOfName, strlen(copyOfName));
  if (copyOfName) {
    free(copyOfName);
  } else {
    fprintf(stderr, "getValue failed to free copyOfName which should be allocated\n");
  }
  if (index) {
    free(index);
  } else {
    fprintf(stderr, "getValue failed to free index which should be allocated\n");
  }
  return(result);
}
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
/*
 *      getFirstIndex.cc -- get an index for this object
 *
 *      Copyright (C) 2021
 *          Mark Broihier
 *
 */

/* ---------------------------------------------------------------------- */
char * JSON::getFirstIndex(char * name) {
  char * result = reinterpret_cast<char *>(malloc(strlen(name) + 1));
  char ic[2];
  size_t index = 0;
  size_t nameSize = strlen(name);
  regex_t * pattern;
  ic[1] = 0;
  if (name[0] == '[') {  // this is an array index
    pattern = &isNumberOnly;
    index = 1;  // skip this character
  } else {
    pattern = &isAlphaNumber;
  }
  size_t indexIndex = 0;
  ic[0] = name[index];
  while (!regexec(pattern, ic, 0, 0, 0)) {
    result[indexIndex++] = name[index++];
    ic[0] = name[index];
  }
  result[indexIndex] = 0;
  index++;
  size_t backFillIndex = 0;
  while (index < nameSize) {
    name[backFillIndex++] = name[index++];
  }
  name[backFillIndex] = 0;
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
  if (debug) fprintf(stderr, "a string has been built - it's value is:\n%s\n", buffer);
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
  if (debug) fprintf(stderr, "a number has been built - it's value is:\n%lf\n", number);
  if (debug) fprintf(stderr, "buffer contents: %s\n", buffer);
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
  if (debug) fprintf(stderr, "a key word has been built - it's value is:\n%s\n", buffer);
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
      if (debug) fprintf(stderr, "A reduction can occur that creates a JSON entry that is a string\n");
      stringElements[std::string(reinterpret_cast<char *>(symbolTableReference[stackPointer - 4]))] =
        reinterpret_cast<char *>(symbolTableReference[stackPointer - 2]);
      free(symbolTableReference[stackPointer - 4]);  // free dangling cstr rest will be done in destructor
      stackPointer -= 4;
    } else if (stackPointer > 4 && token[stackPointer - 1] == COMMA_ && token[stackPointer - 2] == NUMBER_ &&
               token[stackPointer - 3] == COLON_ && token[stackPointer - 4] == STRING_) {
      if (debug) fprintf(stderr, "A reduction can occur that creates a JSON entry that is a number\n");
      numberElements[std::string(reinterpret_cast<char *>(symbolTableReference[stackPointer - 4]))] =
        reinterpret_cast<char *>(symbolTableReference[stackPointer - 2]);
      free(symbolTableReference[stackPointer - 4]);  // free dangling cstr rest will be done in destructor
      stackPointer -= 4;
    } else if (stackPointer > 4 && token[stackPointer - 1] == COMMA_ && token[stackPointer - 2] == BOOLEAN_ &&
               token[stackPointer - 3] == COLON_ && token[stackPointer - 4] == STRING_) {
      if (debug) fprintf(stderr, "A reduction can occur that creates a JSON entry that is a boolean\n");
      boolElements[std::string(reinterpret_cast<char *>(symbolTableReference[stackPointer - 4]))] =
        strcmp(reinterpret_cast<char *>(symbolTableReference[stackPointer - 2]), "true") == 0;
      free(symbolTableReference[stackPointer - 4]);  // free dangling cstr
      stackPointer -= 4;
    } else if (stackPointer > 4 && token[stackPointer - 1] == COMMA_ && token[stackPointer - 2] == NULL_ &&
               token[stackPointer - 3] == COLON_ && token[stackPointer - 4] == STRING_) {
      if (debug) fprintf(stderr, "A reduction can occur that creates a JSON entry that is null\n");
      boolElements[std::string(reinterpret_cast<char *>(symbolTableReference[stackPointer - 4]))] = true;
      free(symbolTableReference[stackPointer - 4]);  // free dangling cstr
      stackPointer -= 4;
    } else if (stackPointer > 4 && token[stackPointer - 1] == COMMA_ && token[stackPointer - 2] == OBJECT_ &&
               token[stackPointer - 3] == COLON_ && token[stackPointer - 4] == STRING_) {
      if (debug) fprintf(stderr, "A reduction can occur that creates a JSON entry that is another JSON object\n");
      jsonElements[std::string(reinterpret_cast<char *>(symbolTableReference[stackPointer - 4]))] =
        reinterpret_cast<JSON *>(symbolTableReference[stackPointer - 2]);
      free(symbolTableReference[stackPointer - 4]);  // free dangling cstr rest will be done in destructor
      stackPointer -= 4;
    } else if (stackPointer > 4 && token[stackPointer - 1] == RIGHT_BRACE_ && token[stackPointer - 2] == STRING_ &&
               token[stackPointer - 3] == COLON_ && token[stackPointer - 4] == STRING_) {
      if (debug) fprintf(stderr, "A reduction can occur that creates a JSON entry that is a string\n");
      stringElements[std::string(reinterpret_cast<char *>(symbolTableReference[stackPointer - 4]))] =
        reinterpret_cast<char *>(symbolTableReference[stackPointer - 2]);
      free(symbolTableReference[stackPointer - 4]);  // free dangling cstr rest will be done in destructor
      stackPointer -= 4;
    } else if (stackPointer > 4 && token[stackPointer - 1] == RIGHT_BRACE_ && token[stackPointer - 2] == NUMBER_ &&
               token[stackPointer - 3] == COLON_ && token[stackPointer - 4] == STRING_) {
      if (debug) fprintf(stderr, "A reduction can occur that creates a JSON entry that is a number\n");
      numberElements[std::string(reinterpret_cast<char *>(symbolTableReference[stackPointer - 4]))] =
        reinterpret_cast<char *>(symbolTableReference[stackPointer - 2]);
      free(symbolTableReference[stackPointer - 4]);  // free dangling cstr rest will be done in destructor
      stackPointer -= 4;
    } else if (stackPointer > 4 && token[stackPointer - 1] == RIGHT_BRACE_ && token[stackPointer - 2] == BOOLEAN_ &&
               token[stackPointer - 3] == COLON_ && token[stackPointer - 4] == STRING_) {
      if (debug) fprintf(stderr, "A reduction can occur that creates a JSON entry that is a boolean\n");
      boolElements[std::string(reinterpret_cast<char *>(symbolTableReference[stackPointer - 4]))] =
        strcmp(reinterpret_cast<char *>(symbolTableReference[stackPointer - 2]), "true") == 0;
      free(symbolTableReference[stackPointer - 4]);  // free dangling cstr
      stackPointer -= 4;
    } else if (stackPointer > 4 && token[stackPointer - 1] == RIGHT_BRACE_ && token[stackPointer - 2] == NULL_ &&
               token[stackPointer - 3] == COLON_ && token[stackPointer - 4] == STRING_) {
      if (debug) fprintf(stderr, "A reduction can occur that creates a JSON entry that is null\n");
      boolElements[std::string(reinterpret_cast<char *>(symbolTableReference[stackPointer - 4]))] = true;
      free(symbolTableReference[stackPointer - 4]);  // free dangling cstr
      stackPointer -= 4;
    } else if (stackPointer > 4 && token[stackPointer - 1] == RIGHT_BRACE_ && token[stackPointer - 2] == OBJECT_ &&
               token[stackPointer - 3] == COLON_ && token[stackPointer - 4] == STRING_) {
      if (debug) fprintf(stderr, "A reduction can occur that creates a JSON entry that is another JSON object\n");
      jsonElements[std::string(reinterpret_cast<char *>(symbolTableReference[stackPointer - 4]))] =
        reinterpret_cast<JSON *>(symbolTableReference[stackPointer - 2]);
      free(symbolTableReference[stackPointer - 4]);  // free dangling cstr rest will be done in destructor
      stackPointer -= 4;
    } else {
      if (debug) fprintf(stderr, "No reduction occurred, stack pointer was %d, processing a %s\n", stackPointer,
              "JSON OBJECT");
    }
  } else if (thisIsA == JSON_ARRAY) {
    if (stackPointer > 2 && token[stackPointer - 2] == OBJECT_ && token[stackPointer - 1] == COMMA_) {
      if (debug) fprintf(stderr, "A reduction can occur that creates a JSON entry that is a array element\n");
      aJsonElements[arrayIndex++] = reinterpret_cast<JSON *>(symbolTableReference[stackPointer - 2]);
      stackPointer -= 2;
    } else if (stackPointer > 2 && token[stackPointer - 2] == OBJECT_ && token[stackPointer - 1] == RIGHT_BRACKET_) {
      if (debug) fprintf(stderr, "A reduction can occur that creates a JSON entry that is a array element\n");
      aJsonElements[arrayIndex++] = reinterpret_cast<JSON *>(symbolTableReference[stackPointer - 2]);
      stackPointer -= 2;
    } else if (stackPointer > 2 && token[stackPointer - 2] == STRING_ && token[stackPointer - 1] == COMMA_) {
      if (debug) fprintf(stderr, "A reduction can occurs that creates a STRING that is a array element\n");
      aStringElements[arrayIndex++] = reinterpret_cast<char *>(symbolTableReference[stackPointer - 2]);
      stackPointer -= 2;
    } else if (stackPointer > 2 && token[stackPointer - 2] == STRING_ && token[stackPointer - 1] == RIGHT_BRACKET_) {
      if (debug) fprintf(stderr, "A reduction can occur that creates a STRING that is a array element\n");
      aStringElements[arrayIndex++] = reinterpret_cast<char *>(symbolTableReference[stackPointer - 2]);
      stackPointer -= 2;
    } else if (stackPointer > 2 && token[stackPointer - 2] == NUMBER_ && token[stackPointer - 1] == COMMA_) {
      if (debug) fprintf(stderr, "A reduction can occur that creates a STRING that is a array element\n");
      aNumberElements[arrayIndex++] = reinterpret_cast<char *>(symbolTableReference[stackPointer - 2]);
      stackPointer -= 2;
    } else if (stackPointer > 2 && token[stackPointer - 2] == NUMBER_ && token[stackPointer - 1] == RIGHT_BRACKET_) {
      if (debug) fprintf(stderr, "A reduction can occur that creates a STRING that is a array element\n");
      aNumberElements[arrayIndex++] = reinterpret_cast<char *>(symbolTableReference[stackPointer - 2]);
      stackPointer -= 2;
    } else if (stackPointer > 2 && token[stackPointer - 2] == BOOLEAN_ && token[stackPointer - 1] == COMMA_) {
      if (debug) fprintf(stderr, "A reduction can occur that creates a BOOLEAN that is a array element\n");
      aBoolElements[arrayIndex++] = strcmp(reinterpret_cast<char *>(symbolTableReference[stackPointer - 2]), "true")
        == 0;
      stackPointer -= 2;
    } else if (stackPointer > 2 && token[stackPointer - 2] == BOOLEAN_ && token[stackPointer - 1] == RIGHT_BRACKET_) {
      if (debug) fprintf(stderr, "A reduction can occur that creates a STRING that is a array element\n");
      aBoolElements[arrayIndex++] = strcmp(reinterpret_cast<char *>(symbolTableReference[stackPointer - 2]), "true")
        == 0;
      stackPointer -= 2;
    } else if (stackPointer > 2 && token[stackPointer - 2] == NULL_ && token[stackPointer - 1] == COMMA_) {
      if (debug) fprintf(stderr, "A reduction can occur that creates a BOOLEAN that is a array element\n");
      aNullElements[arrayIndex++] = true;
      stackPointer -= 2;
    } else if (stackPointer > 2 && token[stackPointer - 2] == NULL_ && token[stackPointer - 1] == RIGHT_BRACKET_) {
      if (debug) fprintf(stderr, "A reduction can occur that creates a STRING that is a array element\n");
      aNullElements[arrayIndex++] = true;
      stackPointer -= 2;
    } else {
      if (debug) fprintf(stderr, "No reduction occurred, stack pointer was %d, processing a %s\n", stackPointer,
              "JSON ARRAY");
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
  char display[128];
  fprintf(stdout, "Currently %s in state %s\n", currentlyBuilding, printState(state));
  for (int index = 0; index < stackPointer; index++) {
    if (token[index] == STRING_) {
      snprintf(display, sizeof(display), "%s", (char *)symbolTableReference[index]);
    } else {
      snprintf(display, sizeof(display), " ");
    }
    fprintf(stdout, " %4d : %s %p - %s\n", index, printToken(token[index]), symbolTableReference[index], display);
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

void JSON::print(bool parent = false) {
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
            } else {  // null
              if (printedSomething) fprintf(stdout, ", ");
              fprintf(stdout, "%s", "null");
              printedSomething = true;
            }
          } else {  // boolean
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
 *      initialize - initialize the object
 *
 *      Copyright (C) 2021
 *          Mark Broihier
 *
 */

/* ---------------------------------------------------------------------- */
void JSON::initialize(char * jsonText) {
  char buf[1024];
  int result;
  regcomp(&isAlpha, "[a-zA-Z_&\\/@]", REG_NOSUB);
  regcomp(&isNumber, "[0-9.+-]", REG_NOSUB);
  regcomp(&isAlphaNumber, "[a-zA-Z_0-9]", REG_NOSUB);
  regcomp(&isNumberOnly, "[0-9]", REG_NOSUB);
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
  if (strlen(jsonText) > 0) parse(jsonText);
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
JSON::JSON(void) {
  textCopy = strdup("");
  initialize(textCopy);
  free(textCopy);
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
JSON::JSON(const char * jsonText) {
  textCopy = strdup(jsonText);
  initialize(textCopy);
  free(textCopy);
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
JSON::JSON(char * jsonText) {
  textCopy = 0;
  initialize(jsonText);
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
  if (debug) fprintf(stderr, "in JSON destructor\n");
  if (thisIsA == JSON_OBJECT) {
    {
      std::map<std::string, char *>::iterator iterator;
      for (iterator = stringElements.begin(); iterator != stringElements.end(); iterator++) {
        if (debug) fprintf(stderr, "freeing memory that holds: %s\n", (char *)iterator->second);
        free(iterator->second);
      }
      for (iterator = numberElements.begin(); iterator != numberElements.end(); iterator++) {
        if (debug) fprintf(stderr, "freeing memory that holds: %s\n", (char *)iterator->second);
        free(iterator->second);
      }
    }
    {
      std::map<std::string, JSON *>::iterator iterator;
      for (iterator = jsonElements.begin(); iterator != jsonElements.end(); iterator++) {
        if (debug) fprintf(stderr, "deleting an object from an object\n");
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
            if (debug) fprintf(stderr, "deleting an object from an array\n");
            delete(jit->second);  // delete the JSON object being referenced
          }
        } else {  // free character memory
          if (debug) fprintf(stderr, "array object freeing memory that holds: %s\n", (char *)numit->second);
          free(numit->second);
        }
      } else {  // free character memory
        if (debug) fprintf(stderr, "array object freeing memory that holds: %s\n", (char *)sit->second);
        free(sit->second);
      }
    }
    aStringElements.clear();
    aNumberElements.clear();
    aBoolElements.clear();
    aNullElements.clear();
    aJsonElements.clear();
  }
  if (debug) fprintf(stderr, "done with JSON destructor\n");
}
/* ---------------------------------------------------------------------- */
