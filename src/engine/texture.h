#pragma once

#include <raylib.h>
#include <stdint.h>

typedef uint32_t TextureHandle;

TextureHandle get_texture_handle(const char * path, bool persistant);
Texture2D get_texture(TextureHandle);
void textures_update(void);

