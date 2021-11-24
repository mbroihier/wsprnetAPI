/*
 *      wsprnetAPI
 *
 *      Copyright (C) 2021
 *          Mark Broihier
 *
 */
/* ---------------------------------------------------------------------- */
#include "wsprnetAPI.h"
char payload[1024*16*10];
size_t payloadSize;
size_t page = 0;

/* ---------------------------------------------------------------------- */

/* ---------------------------------------------------------------------- */
/*
 *      getInfo.c -- callback that accepts return data from the server
 *
 *      Copyright (C) 2021
 *          Mark Broihier
 *
 */
/* ---------------------------------------------------------------------- */

size_t static getInfo(char *incoming,
                          size_t size, size_t blocks, void *myPointer) {
  if (debug) fprintf(stderr, "Entering getInfo callback\n");
  char * buffer = *(reinterpret_cast<char **>(myPointer));
  char * info = buffer;
  buffer += page * 16384;
  size_t * bufferLengthPtr = *(reinterpret_cast<size_t **>(myPointer+4));
  size_t bufferLength = *bufferLengthPtr;
  if (debug) fprintf(stderr, "Incoming data\n");
  size_t completeReturnBufferSize = size * blocks;
  if (debug) fprintf(stderr, "There is %d bytes of receive buffer space and the servers is sending %d bytes\n",
          bufferLength, completeReturnBufferSize);
  if (completeReturnBufferSize > bufferLength) {
    fprintf(stderr, "Abnormal termination of getInfo - not enough API buffer space\n");
    return 0;
  }
  size_t len = 0;
  size_t totalMessageSoFar = len + page * 16384;
  while (len < completeReturnBufferSize && totalMessageSoFar < bufferLength) {
    *buffer++ = *incoming++;
    len++;
    totalMessageSoFar++;
  }
  if (len == completeReturnBufferSize && totalMessageSoFar < bufferLength) {
    *buffer++ = 0;
    page++;
    if (debug) fprintf(stderr, "Got:\n %s\n", info);
  } else {
    fprintf(stderr, "Error in received data - data received exceeds allocated buffer space\n");
  }
  return len;
}
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
/*
 *      login.cc -- execute the commands to log into the WSPRNET API
 *
 *      Copyright (C) 2021
 *          Mark Broihier
 *
 */
/* ---------------------------------------------------------------------- */
void wsprnetAPI::login() {
  page = 0;
  sendPost = curl_easy_init();
  curl_easy_setopt(sendPost, CURLOPT_URL, urlLogin);
  struct curl_slist *list;
  list = curl_slist_append(NULL, "Content-Type: application/json");
  curl_easy_setopt(sendPost, CURLOPT_USE_SSL, CURLUSESSL_ALL);
  curl_easy_setopt(sendPost, CURLOPT_HTTPHEADER, list);
  curl_easy_setopt(sendPost, CURLOPT_WRITEFUNCTION, getInfo);
  void * parameters[2];
  parameters[0] = payload;
  parameters[1] = &payloadSize;
  payloadSize = sizeof(payload);
  char loginMessage[1024];
  snprintf(loginMessage, sizeof(loginMessage), " { \"name\":\"%s\", \"pass\":\"%s\" }\n", loginID, password);
  if (debug) fprintf(stderr, "size of receive buffer is %d bytes\n", payloadSize);
  curl_easy_setopt(sendPost, CURLOPT_WRITEDATA, parameters);
  curl_easy_setopt(sendPost, CURLOPT_POSTFIELDS, loginMessage);
  curl_easy_setopt(sendPost, CURLOPT_POSTFIELDSIZE, (int64_t) strlen(loginMessage));
  if (debug) {
    fprintf(stderr, "Message body: %s\nMessage Size: %d\n", loginMessage, strlen(loginMessage));
    curl_easy_setopt(sendPost, CURLOPT_VERBOSE, 1L);
  }
  CURLcode result = curl_easy_perform(sendPost);
  if (result) {
    fprintf(stderr, "Curlcode: %d\n", result);
  }
  if (debug) fprintf(stderr, "Message being searched:\n %s\n", payload);
  JSON * login = new JSON(payload);
  sessionName = login->getValue("session_name");
  if (sessionName) {
    if (debug) fprintf(stderr, "sessionName is: %s\n", sessionName);
  } else {
    fprintf(stderr, "session_name was not returned with login request\n");
    exit(-1);
  }
  sessionID = login->getValue("sessid");
  if (sessionID) {
    if (debug) fprintf(stderr, "sessionID is: %s\n", sessionID);
  } else {
    fprintf(stderr, "sessionid was not returned with login request\n");
    exit(-1);
  }
  token = login->getValue("token");
  if (token) {
    if (debug) fprintf(stderr, "token is: %s\n", token);
  } else {
    fprintf(stderr, "token was not returned with login request\n");
    exit(-1);
  }
  delete(login);
  size_t cookieSize = strlen(sessionName) + strlen(sessionID) + 128;
  cookie = reinterpret_cast<char *>(malloc(cookieSize));
  memset(cookie, 0, cookieSize);
  snprintf(cookie, cookieSize, "Cookie: %s=%s", sessionName, sessionID);
  if (debug) fprintf(stderr, "Cookie construction: %s\n", cookie);
  size_t XCSRFTokenSize = strlen(token) + 128;
  XCSRFToken = reinterpret_cast<char *>(malloc(XCSRFTokenSize));
  memset(XCSRFToken, 0, XCSRFTokenSize);
  snprintf(XCSRFToken, XCSRFTokenSize, "X-CSRF-Token: %s", token);
  if (debug) fprintf(stderr, "X-CSRF-Token construction: %s\n", XCSRFToken);
  curl_slist_free_all(list);
  curl_easy_cleanup(sendPost);
}
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
/*
 *      querySpots.cc -- execute the commands to retrieve spots from the 
 *                       WSPRNET API
 *
 *      Copyright (C) 2021
 *          Mark Broihier
 *
 */
/* ---------------------------------------------------------------------- */
void wsprnetAPI::querySpots(const char * filters) {
  char volatileFilters[128];
  memset(volatileFilters, 0, sizeof(volatileFilters));
  strncpy(volatileFilters, filters, sizeof(volatileFilters)-1);
  querySpots(volatileFilters);
}
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
/*
 *      querySpots.cc -- execute the commands to retrieve spots from the 
 *                       WSPRNET API
 *
 *      Copyright (C) 2021
 *          Mark Broihier
 *
 */
/* ---------------------------------------------------------------------- */
void wsprnetAPI::querySpots(char * filters) {
  char command[1024];
  memset(command, 0, sizeof(command));
  snprintf(command, sizeof(command), "%s%s", urlQuerySpots, filters);
  sendQuery(command);
  if (debug) fprintf(stderr, "Spots returned:\n %s\n", payload);
}
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
/*
 *      queryPaths.cc -- execute the commands to retrieve paths from the 
 *                       WSPRNET API
 *
 *      Copyright (C) 2021
 *          Mark Broihier
 *
 */
/* ---------------------------------------------------------------------- */
void wsprnetAPI::queryPaths(const char * filters) {
  char volatileFilters[128];
  memset(volatileFilters, 0, sizeof(volatileFilters));
  strncpy(volatileFilters, filters, sizeof(volatileFilters)-1);
  queryPaths(volatileFilters);
}
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
/*
 *      queryPaths.cc -- execute the commands to retrieve status from the 
 *                       WSPRNET API
 *
 *      Copyright (C) 2021
 *          Mark Broihier
 *
 */
/* ---------------------------------------------------------------------- */
void wsprnetAPI::queryPaths(char * filters) {
  char command[1024];
  memset(command, 0, sizeof(command));
  snprintf(command, sizeof(command), "%s%s", urlQueryPaths, filters);
  sendQuery(command);
  if (debug) fprintf(stderr, "Paths returned:\n %s\n", payload);
}
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
/*
 *      queryStatus.cc -- execute the commands to retrieve status from the 
 *                       WSPRNET API
 *
 *      Copyright (C) 2021
 *          Mark Broihier
 *
 */
/* ---------------------------------------------------------------------- */
void wsprnetAPI::queryStatus(const char * filters) {
  char volatileFilters[128];
  memset(volatileFilters, 0, sizeof(volatileFilters));
  strncpy(volatileFilters, filters, sizeof(volatileFilters)-1);
  queryStatus(volatileFilters);
}
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
/*
 *      queryStatus.cc -- execute the commands to retrieve status from the 
 *                       WSPRNET API
 *
 *      Copyright (C) 2021
 *          Mark Broihier
 *
 */
/* ---------------------------------------------------------------------- */
void wsprnetAPI::queryStatus(char * filters) {
  char command[1024];
  memset(command, 0, sizeof(command));
  snprintf(command, sizeof(command), "%s%s", urlQueryStatus, filters);
  sendQuery(command);
  if (debug) fprintf(stderr, "Status returned:\n %s\n", payload);
}
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
/*
 *      sendQuery.cc -- generically sends a query message to the
 *                       WSPRNET API
 *
 *      Copyright (C) 2021
 *          Mark Broihier
 *
 */
/* ---------------------------------------------------------------------- */
void wsprnetAPI::sendQuery(char * endPointWithFilters) {
  page = 0;
  sendPost = curl_easy_init();
  curl_easy_setopt(sendPost, CURLOPT_URL, endPointWithFilters);
  struct curl_slist *list;
  list = curl_slist_append(NULL, "Content-Type: application/json");
  list = curl_slist_append(list, cookie);
  curl_easy_setopt(sendPost, CURLOPT_USE_SSL, CURLUSESSL_ALL);
  curl_easy_setopt(sendPost, CURLOPT_HTTPHEADER, list);
  curl_easy_setopt(sendPost, CURLOPT_WRITEFUNCTION, getInfo);
  void * parameters[2];
  parameters[0] = payload;
  parameters[1] = &payloadSize;
  payloadSize = sizeof(payload);
  const char body[] = "{}";
  curl_easy_setopt(sendPost, CURLOPT_WRITEDATA, parameters);
  curl_easy_setopt(sendPost, CURLOPT_POSTFIELDS, body);
  curl_easy_setopt(sendPost, CURLOPT_POSTFIELDSIZE, (int64_t) sizeof(body));
  CURLcode result = curl_easy_perform(sendPost);
  if (result) {
    fprintf(stderr, "Curlcode: %d\n", result);
  }
  curl_slist_free_all(list);
  curl_easy_cleanup(sendPost);
}
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
/*
 *      logout.cc -- execute the commands to log out of the WSPRNET API
 *
 *      Copyright (C) 2021
 *          Mark Broihier
 *
 */
/* ---------------------------------------------------------------------- */
void wsprnetAPI::logout() {
  page = 0;
  sendPost = curl_easy_init();
  curl_easy_setopt(sendPost, CURLOPT_URL, urlLogout);
  struct curl_slist *list;
  list = curl_slist_append(NULL, "Content-Type: application/json");
  list = curl_slist_append(list, cookie);
  list = curl_slist_append(list, XCSRFToken);
  curl_easy_setopt(sendPost, CURLOPT_USE_SSL, CURLUSESSL_ALL);
  curl_easy_setopt(sendPost, CURLOPT_HTTPHEADER, list);
  curl_easy_setopt(sendPost, CURLOPT_WRITEFUNCTION, getInfo);
  void * parameters[2];
  parameters[0] = payload;
  parameters[1] = &payloadSize;
  payloadSize = sizeof(payload);
  static const char body[] = " { }\n";
  curl_easy_setopt(sendPost, CURLOPT_WRITEDATA, parameters);
  curl_easy_setopt(sendPost, CURLOPT_POSTFIELDS, body);
  curl_easy_setopt(sendPost, CURLOPT_POSTFIELDSIZE, (int64_t) strlen(body));
  CURLcode result = curl_easy_perform(sendPost);
  if (result) {
    fprintf(stderr, "Curlcode: %d\n", result);
  }
  if (debug) fprintf(stderr, "Logout message returned:\n %s\n", payload);
  curl_slist_free_all(list);
  curl_easy_cleanup(sendPost);
}
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
/*
 *      wsprnetAPI - constructor
 *
 *      Copyright (C) 2021
 *          Mark Broihier
 *
 */
/* ---------------------------------------------------------------------- */

wsprnetAPI::wsprnetAPI() {
  if (debug) fprintf(stderr, "Setup information:\n%s\n%s\n%s\n%s\n%s\n%s\n",  urlLogin, urlQuerySpots,
                     urlQueryPaths, urlQueryStatus, urlLogout, credentials);
  size_t credentialsLength = strlen(credentials);
  size_t index = 0;
  size_t indexSF = 0;
  bool firstField = true;
  while (index < credentialsLength && index < sizeof(loginID)) {
    if (credentials[index] == ':') {
      firstField = false;
      loginID[index] = 0;
      index++;  // skip :
    }
    if (firstField) {
      loginID[index] = credentials[index];
      index++;
    } else {
      password[indexSF] = credentials[index];
      indexSF++;
      index++;
    }
  }
  if (indexSF == 0) {
    fprintf(stderr, "Login credentials are not formatted properly\n");
    exit(-1);
  }
  password[indexSF] = 0;
  if (debug) fprintf(stderr, "login id: %s, password: %s\n", loginID, password);

  curl_version_info_data * versionData;
  versionData = curl_version_info(CURLVERSION_NOW);
  if (versionData->version_num < 0x074000) {  // this code wants 7.64.0 or greater
    fprintf(stderr, "This curl version is not new enough - detected version is: %8.8x\n", versionData->version_num);
    exit(1);
  }
  if (debug) {
    fprintf(stderr, "This curl version is: %8.8x\n", versionData->version_num);
  }
  sessionID = 0;
  sessionName = 0;
  token = 0;
  XCSRFToken = 0;
  cookie = 0;
}
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
/*
 *      wsprnetAPI - destructor
 *
 *      Copyright (C) 2021 
 *          Mark Broihier
 *
 */
/* ---------------------------------------------------------------------- */

wsprnetAPI::~wsprnetAPI() {
  if (debug) fprintf(stderr, "in wsprnetAPI destructor\n");
  if (sessionID) free(sessionID);
  if (sessionName) free(sessionName);
  if (token) free(token);
  if (cookie) free(cookie);
  if (XCSRFToken) free(XCSRFToken);
  if (debug) fprintf(stderr, "done with wsprnetAPI destructor\n");
}
/* ---------------------------------------------------------------------- */
#ifdef INCLUDETESTMAIN
int main(int argc, char *argv[]) {
  wsprnetAPI wsprnet;
  int c;

  memset(payload, 0, sizeof(payload));

  fprintf(stdout, "wspnetAPI VERSION %s\n", STR_VALUE(wsprnetAPI_VERSION_NUMBER));
  if (argc > 1) {
    fprintf(stderr, USAGE_STR, argv[0]);
    return -2;
  }

  int doneProcessing = 0;
  while ((c = getopt_long(argc, argv, "h", longOpts, NULL)) >= 0) {
    switch (c) {
      case 'h': {
        fprintf(stderr, USAGE_STR, argv[0]);
        return -2;
      }
      default:
        break;
    }
  }

  wsprnet.login();
  wsprnet.querySpots("band=14&reporter=kg5yje&minutes=1440&exclude_special=0");
  JSON * queryReplyMessage1 = new JSON(payload);
  fprintf(stdout, "spots received in last day:\n");
  queryReplyMessage1->print(true);
  char * callSign = 0;
  char spotQuery[128];
  char statusQuery[128];
  size_t index = 0;
  bool notDone = true;
  fprintf(stdout, "reported status of each spot:\n");
  do {
    snprintf(spotQuery, sizeof(spotQuery), "[%d]CallSign", index);
    callSign = queryReplyMessage1->getValue(spotQuery);
    if (callSign) {
      fprintf(stdout, "Call sign at element %d is: %s\n", index++, callSign);
      snprintf(statusQuery, sizeof(statusQuery), "callsign=%s", callSign);
      wsprnet.queryStatus(statusQuery);
      JSON * queryReplyMessage2 = new JSON(payload);
      fprintf(stdout, "status of %s:\n", callSign);
      queryReplyMessage2->print(true);
      free(callSign);
      callSign = 0;
      delete(queryReplyMessage2);
    } else {
      notDone = false;
    }
  } while (notDone);
  delete(queryReplyMessage1);
  wsprnet.queryPaths("callsign=kg5yje&minutes=1440&band=14");
  queryReplyMessage1 = new JSON(payload);
  fprintf(stdout, "lastest paths that include call sign kg5yje:\n");
  queryReplyMessage1->print(true);
  delete(queryReplyMessage1);
  wsprnet.queryStatus("band=14");
  queryReplyMessage1 = new JSON(payload);
  fprintf(stdout, "lastest status reports for band 14:\n");
  queryReplyMessage1->print(true);
  delete(queryReplyMessage1);
  wsprnet.logout();
  queryReplyMessage1 = new JSON(payload);
  fprintf(stdout, "logout reply:\n");
  queryReplyMessage1->print(true);
  delete(queryReplyMessage1);
  fprintf(stdout, "wsprnetAPI test main done\n");
  return 0;
}
#endif
/* ---------------------------------------------------------------------- */
