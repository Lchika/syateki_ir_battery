#ifndef _IR_BATTERY_CONTROL_DEBUG_H_
#define _IR_BATTERY_CONTROL_DEBUG_H_

//#define DEBUG_BUILD

#include <stdio.h>

#ifdef DEBUG_BUILD
  #define DEBUG_PRINTF(fmt, ...) \
    printf("[%s] %s:%u # " fmt "\n", \
      __DATE__, __FILE__, \
      __LINE__, ##__VA_ARGS__)
  #define ERROR_PRINTF(fmt, ...) \
    printf("<err>[%s] %s:%u # " fmt "\n", \
      __DATE__, __FILE__, \
      __LINE__, ##__VA_ARGS__)
#else
  #define DEBUG_PRINTF(fmt, ...)
  #define ERROR_PRINTF(fmt, ...) \
    printf("<err>[%s] %s:%u # " fmt "\n", \
      __DATE__, __FILE__, \
      __LINE__, ##__VA_ARGS__)
#endif

#endif // _IR_BATTERY_CONTROL_DEBUG_H_