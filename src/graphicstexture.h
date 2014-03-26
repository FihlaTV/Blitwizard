
/* blitwizard game engine - source code file

  Copyright (C) 2013 Jonas Thiem

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

*/

#ifndef BLITWIZARD_GRAPHICSTEXTURE_H_
#define BLITWIZARD_GRAPHICSTEXTURE_H_

#include <stdint.h>

#include "os.h"

struct graphicstexture;

// Formats (in little endian byte order):
#define PIXELFORMAT_32RGBA 1
#define PIXELFORMAT_32ARGB 2
#define PIXELFORMAT_32ABGR 3
#define PIXELFORMAT_32BGRA 4
#define PIXELFORMAT_UNKNOWN (-1)

// Get the pixel format the renderer wants to receive the texture in:
int graphicstexture_getDesiredFormat(void);

// Create a graphics texture (for 3d accelerated renderers,
// it must be created in GPU memory!) and return a handle:
struct graphicstexture *graphicstexture_create(void *data,
    size_t width, size_t height, int format, uint64_t time);
// time needs to be a milliseconds timestamp.
// It may be required for OpenGL upload waiting.
// Please note in the current implementation, a format not matching
// graphicstexture_getDesiredFormat() is rejected.

#ifdef USE_SDL_GRAPHICS_OPENGL_EFFECTS
// Bind a graphics texture for OpenGL operations.
int graphicstexture_bindGl(struct graphicstexture *gt, uint64_t time);
// Returns 1 on success, 0 on failure. That may happen if the texture
// is still uploading, in which case you simply have to try again later.
#endif

// Destroy graphics texure by handle:
void graphicstexture_destroy(struct graphicstexture *texture);

// Get texture dimensions:
void graphics_getTextureDimensions(struct graphicstexture *texture,
size_t* width, size_t* height);

// Get texture format
int graphics_getTextureFormat(struct graphicstexture *texture);

// Extract pixel data from texture again (not necessarily supported
// by all renderers). The pixels pointer must point to a buffer
// which is large enough to hold the pixel data of the texture.
// Returns 1 on success (pixels will be modified to hold the data),
// otherwise 0 (pixels will have undefined contents).
int graphicstexture_pixelsFromTexture(
struct graphicstexture *gt, void *pixels);

#endif  // BLITWIZARD_GRAPHICSTEXTURE_H_

