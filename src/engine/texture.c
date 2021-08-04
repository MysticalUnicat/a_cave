#include "texture.h"

#include <string.h>
#include <stdlib.h>

#include "pp.h"

#define FRAMES_TO_KEEP 4
static uint32_t frame = 2;

SOA(TextureCache
  , ( const char *, path )
  , ( Texture2D, texture )
  , ( uint32_t, sort )
  , ( uint32_t, frame ))

static struct TextureCache _texcache;

static int _texcache_bsearch(const char * path, const uint32_t * index) {
  return strcmp(path, _texcache.path[*index]);
}

static int _texcache_sort(const uint32_t * a, const uint32_t * b) {
  return strcmp(_texcache.path[*a], _texcache.path[*b]);
}

TextureHandle get_texture_handle(const char * path, bool persistant) {
  uint32_t * find = bsearch(path, _texcache.sort, _texcache.length, sizeof(*_texcache.sort), (int (*)(const void *, const void *))_texcache_bsearch);
  if(find != NULL) {
    _texcache.frame[*find] = frame;
    return *find;
  }

  uint32_t index = _texcache.length;
  TextureCache_set_length(&_texcache, _texcache.length + 1);

  _texcache.path[index] = strdup(path);
  _texcache.sort[index] = index;
  _texcache.frame[index] = persistant ? 1 : frame;

  qsort(_texcache.sort, _texcache.length, sizeof(*_texcache.sort), (int (*)(const void *, const void *))_texcache_sort);

  return index;
}

Texture2D get_texture(TextureHandle handle) {
  if(_texcache.frame[handle] == 0) {
    _texcache.texture[handle] = LoadTexture(_texcache.path[handle]);
  }
  _texcache.frame[handle] = frame;
  return _texcache.texture[handle];
}

void textures_update(void) {
  for(uint32_t i = 0; i < _texcache.length; i++) {
    if(_texcache.frame[i] == 0) {
      // skip unloaded textures
      continue;
    }

    if(_texcache.frame[i] == 1) {
      // skip persistant textures
      continue;
    }

    if(frame - _texcache.frame[i] > FRAMES_TO_KEEP) {
      _texcache.frame[i] = 0;
      UnloadTexture(_texcache.texture[i]);
    }
  }

  frame++;
}
