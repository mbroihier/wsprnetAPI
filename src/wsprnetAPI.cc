/*
 *      wsprnetAPI
 *
 *      Copyright (C) 2021
 *          Mark Broihier
 *
 */
/* ---------------------------------------------------------------------- */
#include <dirent.h>
#include <getopt.h>
#include <unistd.h>
#include "wsprnetAPIConfig.h"
#include "wsprnetAPI.h"
/* ---------------------------------------------------------------------- */


static const char USAGE_STR[] = "\n"
        "Usage: %s \n"
        "  -h                       : help\n"
        "  --help                   : help\n";

static struct option longOpts[] = {
  {"help", no_argument, NULL, 1},
  { NULL, 0, NULL, 0 }
};

static const char * testMessage = "{\"sessid\":\"H0Urg_saXBwHDcgj8tepvnndDL6YNCgSa_uNcBn5IuA\",\"session_name\":\"SSESSe706824a3745c30a9d4279773b743146\",\"token\":\"ZBZOaILjprl8bI38G48GQuindKmPQmJewPAP_HWYNPo\",\"user\":{\"uid\":\"158595\",\"name\":\"KG5YJE\",\"mail\":\"mbroihier@yahoo.com\",\"theme\":\"\",\"signature\":\"\",\"signature_format\":\"1\",\"created\":\"1629580233\",\"access\":\"1635799992\",\"login\":1635803922,\"status\":\"1\",\"timezone\":\"UTC\",\"language\":\"\",\"picture\":null,\"data\":false,\"roles\":{\"2\":\"authenticated user\"},\"profile_firstname\":\"Mark\",\"profile_qth\":\"Plano, TX/USA\",\"profile_grid\":\"EM13\",\"profile_station\":\"rtlsdr dipole\",\"profile_callsign\":\"KG5YJE\"}}";

static char payload[1024*16*10];
static size_t payloadSize;
static size_t page = 0;


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
  fprintf(stderr, "There is %d bytes of receive buffer space and the servers is sending %d bytes\n",
          bufferLength, completeReturnBufferSize);
  if (completeReturnBufferSize > bufferLength) {
    fprintf(stderr, "Abnormal termination of getInfo\n");
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
    if (debug) fprintf(stderr, "Error in received data - data received exceeds allocated buffer space\n");
  }
  return len;
}
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
/*
 *      jsonFind.cc -- execute the commands to log into the WSPRNET API
 *
 *      Copyright (C) 2021
 *          Mark Broihier
 *
 */

/* ---------------------------------------------------------------------- */
char * wsprnetAPI::jsonFind(const char * fieldName, char * buffer, size_t bufferLength) {
  const int FIELD_SIZE = 1024;
  char * lastValidLocation = buffer + bufferLength - 1;
  char * location = strstr(buffer, fieldName);
  char * value = 0;
  size_t destination = 0;
  if (location) {
    value = reinterpret_cast<char *>(malloc(FIELD_SIZE));
    location += strlen(fieldName);
    while (*location != '"' && location < lastValidLocation) location++;
    if (*location ==  '"') location++;
    while (*location != '"' && location < lastValidLocation) {
      value[destination++] = *location++;
      if (destination >= (FIELD_SIZE - 1)) {
        fprintf(stderr, "Field too large to fit in buffer - terminating and deleting allocated space\n");
        free(value);
        value = 0;
        break;
      }
    }
    if (value) value[destination] = 0;
  }
  return value;
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
  fprintf(stderr, "size of receive buffer is %d bytes\n", payloadSize);
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
  fprintf(stderr, "Message being searched:\n %s\n", payload);
  sessionName = jsonFind("\"session_name\":", payload, strlen(payload));
  if (sessionName) {
    fprintf(stderr, "sessionName is: %s\n", sessionName);
  }
  sessionID = jsonFind("\"sessid\":", payload, strlen(payload));
  if (sessionID) {
    fprintf(stderr, "sessionID is: %s\n", sessionID);
  }
  token = jsonFind("\"token\":", payload, strlen(payload));
  if (token) {
    fprintf(stderr, "token is: %s\n", token);
  }
  size_t cookieSize = strlen(sessionName) + strlen(sessionID) + 128;
  cookie = reinterpret_cast<char *>(malloc(cookieSize));
  memset(cookie, 0, cookieSize);
  snprintf(cookie, cookieSize, "Cookie: %s=%s", sessionName, sessionID);
  fprintf(stderr, "Cookie construction: %s\n", cookie);
  size_t XCSRFTokenSize = strlen(token) + 128;
  XCSRFToken = reinterpret_cast<char *>(malloc(XCSRFTokenSize));
  memset(XCSRFToken, 0, XCSRFTokenSize);
  snprintf(XCSRFToken, XCSRFTokenSize, "X-CSRF-Token: %s", token);
  fprintf(stderr, "X-CSRF-Token construction: %s\n", XCSRFToken);
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
  fprintf(stderr, "Spots returned:\n %s\n", payload);
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
  fprintf(stderr, "Paths returned:\n %s\n", payload);
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
  fprintf(stderr, "Status returned:\n %s\n", payload);
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
  fprintf(stderr, "Logout message returned:\n %s\n", payload);
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
  if (sessionID) free(sessionID);
  if (sessionName) free(sessionName);
  if (token) free(token);
  if (cookie) free(cookie);
  if (XCSRFToken) free(XCSRFToken);
}
/* ---------------------------------------------------------------------- */

int main(int argc, char *argv[]) {
  wsprnetAPI wsprnet;
  int c;

  memset(payload, 0, sizeof(payload));
  memcpy(payload, testMessage, strlen(testMessage));

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
  wsprnet.queryPaths("band=14&callsign=kg5yje&minutes=60&exclude_special=0");
  wsprnet.queryStatus("band=14&callsign=kg5yje&minutes=60&exclude_special=0");
  wsprnet.querySpots("band=14&reporter=kg5yje&minutes=60&exclude_special=0");
  wsprnet.logout();
  JSON * logoutReplyMessage = new(JSON);
  logoutReplyMessage->parse(payload);
  logoutReplyMessage->printStack();

  return 0;
}
/* ---------------------------------------------------------------------- */
