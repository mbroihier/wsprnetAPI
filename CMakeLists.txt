#
#  As a minimum, CREDENTIALS to the values necessary to access your wsprnet account
#  Do NOT commit this file after alteration because you'll be storing that information into your repository.
#
cmake_minimum_required(VERSION 3.7)
project(wsprnetAPI)
set(PROJECT_VERSION "0.1")
set(PROTOCOL "https:")
set(URL_LOGIN "www.wsprnet.org/drupal/rest/user/login")
set(URL_QUERY_SPOTS "www.wsprnet.org/drupal/wsprnet/spots/json?")
set(URL_QUERY_PATHS "www.wsprnet.org/drupal/wsprnet/paths/json?")
set(URL_QUERY_STATUS "www.wsprnet.org/drupal/wsprnet/status/json?")
set(URL_LOGOUT "www.wsprnet.org/drupal/rest/user/logout.json")
set(CREDENTIALS "username:password")
set(DEBUG 0)
configure_file("src/wsprnetAPIConfig.h.in" "include/wsprnetAPIConfig.h")
include_directories("${CMAKE_SOURCE_DIR}/include")
include_directories("${CMAKE_BINARY_DIR}/include")
set(WSPRNETAPI_SRC src/wsprnetAPI.cc src/json.cc)
add_executable(wsprnetAPI ${WSPRNETAPI_SRC})
target_link_libraries(wsprnetAPI curl )
