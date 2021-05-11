#include "stats.h"

#include "pp.h"

#include <raylib.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

SOA(Stats
  , ( const char *, name )
  , ( uint32_t, sort )
  , ( uint32_t, total )
  , ( uint32_t, accum ))

static struct Stats _stats;
static uint32_t _top;

void stat_frame(void) {
  for(uint32_t i = 0; i < _stats.length; i++) {
    _stats.total[i] += _stats.accum[i];
    _stats.accum[i] = 0;
  }
  _top = 0;
}

static int _stats_search(const char * name, const uint32_t * index) {
  return strcmp(name, _stats.name[*index]);
}

static int _stats_sort(const uint32_t * a, const uint32_t * b) {
  return strcmp(_stats.name[*a], _stats.name[*b]);
}

uint32_t stat_register(const char * name) {
  uint32_t * find = bsearch(name, _stats.sort, _stats.length, sizeof(*_stats.sort), (int (*)(const void *, const void *))_stats_search);
  if(find != NULL) {
    return *find;
  }

  uint32_t index = _stats.length;
  Stats_set_length(&_stats, index + 1);

  _stats.name[index] = strdup(name);
  _stats.sort[index] = index;

  qsort(_stats.sort, _stats.length, sizeof(*_stats.sort), (int (*)(const void *, const void *))_stats_sort);
}

void stat_submit(uint32_t index, uint32_t count) {
  _stats.accum[index] += count;
}

void stat_print(uint32_t index) {
  DrawText(TMP_STR(64, "%s : %u", _stats.name[index], _stats.total[index]), _top, 0, 8, RAYWHITE);
}

void stat_print_per_frame(uint32_t index) {
}

