#pragma once

#include "pebble.h"

#define DEBUG_MODE

#ifdef DEBUG_MODE
#define IS_DEBUGGING true
#else
#define IS_DEBUGGING false
#endif

#ifdef DEBUG_MODE
#define VERBOSE_LOGGING_ENABLED
#endif

#ifdef VERBOSE_LOGGING_ENABLED
#define VERBOSE_LOG(msg, ...) APP_LOG(APP_LOG_LEVEL_DEBUG, msg, ##__VA_ARGS__)
#else
#define VERBOSE_LOG(msg, ...)
#endif