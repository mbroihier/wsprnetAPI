#ifndef INCLUDE_WSPRNETAPI_H_
#define INCLUDE_WSPRNETAPI_H_
/*
 *      wsprnetAPI.h - header for wsprnetAPI objects
 *
 *      Copyright (C) 2021
 *          Mark Broihier
 *
 */
/* ---------------------------------------------------------------------- */
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <iostream>
#include <map>
#include <string>
#include <time.h>
#include "../include/json.h"
/* ---------------------------------------------------------------------- */
#define STRINGIZER(arg)  #arg
#define STR_VALUE(arg) STRINGIZER(arg)
const char urlLogin[] = STR_VALUE(PROTOCOL) "//" STR_VALUE(URL_LOGIN);
const char urlQuerySpots[] = STR_VALUE(PROTOCOL) "//" STR_VALUE(URL_QUERY_SPOTS);
const char urlQueryPaths[] = STR_VALUE(PROTOCOL) "//" STR_VALUE(URL_QUERY_PATHS);
const char urlQueryStatus[] = STR_VALUE(PROTOCOL) "//" STR_VALUE(URL_QUERY_STATUS);
const char urlLogout[] = STR_VALUE(PROTOCOL) "//" STR_VALUE(URL_LOGOUT);
const char credentials[] = STR_VALUE(CREDENTIALS);
static bool debug = DEBUG;
class wsprnetAPI {
 private:
  CURL *sendPost;
  char *sessionID;
  char *sessionName;
  char *token;
  char *cookie;
  char *XCSRFToken;
  char * jsonFind(const char * field, char * buffer, size_t bufferLength);
  char loginID[128];
  char password[128];
  void sendQuery(char * endPointWithFilters);

 public:
  void login(void);
  void querySpots(const char * filters);
  void queryPaths(const char * filters);
  void queryStatus(const char * filters);
  void querySpots(char * filters);
  void queryPaths(char * filters);
  void queryStatus(char * filters);
  void logout(void);
  wsprnetAPI();
  ~wsprnetAPI();
};
#endif  // INCLUDE_WSPRNETAPI_H_
