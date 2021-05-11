#pragma once

#include <stdint.h>

void stat_frame(void);
uint32_t stat_register(const char * name);
void stat_submit(uint32_t index, uint32_t count);
void stat_print(uint32_t index);
void stat_print_per_frame(uint32_t index);

#define STATV(NAME, COUNT) \
  do { \
    static uint32_t _stat_handle = 0; \
    _stat_handle = _stat_handle ? _stat_handle : stat_register(#NAME) + 1; \
    stat_submit(_stat_handle - 1, COUNT); \
  } while(0)

#define STAT(NAME) STATV(NAME, 1)

#define PRINT_STAT(NAME) \
  do { \
    static uint32_t _stat_handle = 0; \
    _stat_handle = _stat_handle ? _stat_handle : stat_register(#NAME) + 1; \
    stat_print(_stat_handle); \
  } while(0)

#define PRINT_STAT_PER_FRAME(NAME) \
  do { \
    static uint32_t _stat_handle = 0; \
    _stat_handle = _stat_handle ? _stat_handle : stat_register(#NAME) + 1; \
    stat_print_per_frame(_stat_handle); \
  } while(0)

