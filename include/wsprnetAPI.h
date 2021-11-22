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
#include <curl/curl.h>
#include <dirent.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <iostream>
#include <map>
#include <string>
#include "wsprnetAPIConfig.h"
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

static const char USAGE_STR[] = "\n"
        "Usage: %s \n"
        "  -h                       : help\n"
        "  --help                   : help\n";

static struct option longOpts[] = {
  {"help", no_argument, NULL, 1},
  { NULL, 0, NULL, 0 }
};

static char payload[1024*16*10];

static size_t payloadSize;
static size_t page = 0;

class wsprnetAPI {
 private:
  CURL *sendPost;
  char *sessionID;
  char *sessionName;
  char *token;
  char *cookie;
  char *XCSRFToken;
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
