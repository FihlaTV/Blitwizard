
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

#include <stdlib.h>

struct graphicstexture;

// Formats (in little endian byte order):
#define PIXELFORMAT_32RGBA 1

// Create a graphics texture (for 3d accelerated renderers,
// it must be created in GPU memory!) and return a handle:
struct graphicstexture* graphicstexture_Create(void* data,
size_t width, size_t height, int format);

// Destroy graphics texure by handle:
void graphicstexture_Destroy(struct graphicstexture* texture);

// Get texture dimensions:
void graphics_GetTextureDimensions(struct graphicstexture* texture,
size_t* width, size_t* height);

// Get texture format
int graphics_GetTextureFormat(struct graphicstexture* texture);

// Extract pixel data from texture again (not necessarily supported
// by all renderers). The pixels pointer must point to a buffer
// which is large enough to hold the pixel data of the texture.
// Returns 1 on success (pixels will be modified to hold the data),
// otherwise 0 (pixels will have undefined contents).
int graphicstexture_PixelsFromTexture(
struct graphicstexture* gt, void* pixels);

#endif  // BLITWIZARD_GRAPHICSTEXTURE_H_

