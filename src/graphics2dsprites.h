
/* blitwizard game engine - source code file

  Copyright (C) 2011-2013 Jonas Thiem

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

#ifndef BLITWIZARD_GRAPHICS2DSPRITES_H_
#define BLITWIZARD_GRAPHICS2DSPRITES_H_

#ifdef USE_GRAPHICS

#ifdef __cplusplus
extern "C" {
#endif

struct graphics2dsprite;

// Create a sprite at a given position (x/y specify the sprite center).
//
// The geometry callback will be called once as soon as the actual
// width and height obtained by the texture loaded from texturePath
// is known. userdata is set to the specified userdata.
//
// The visibility callback will be called once the texture is
// fully loaded and the sprite will be truly visible on screen.
//
// userdata is a pointer that will be passed to your callbacks.
//
// Please note you can freely move/resize/destroy the sprite even
// when the geometryCallback or visibilityCallback haven't been
// called yet.
struct graphics2dsprite* graphics2dsprites_Create(
const char* texturePath, double x, double y, double width, double height,
void (*geometryCallback)(double width, double height, void* userdata),
void (*visibilityCallback)(void* userdata),
void* userdata);

// Move a sprite.
void graphics2dsprites_Move(struct graphics2dsprite* sprite,
double x, double y);

// Resize a sprite.
void graphics2dsprites_Resize(struct graphics2dsprite* sprite,
double width, double height);

// Destroy the specified sprite:
void graphics2dsprite_Destroy(struct graphics2dsprite* sprite);

#ifdef __cplusplus
}
#endif

#endif  // USE_GRAPHICS

#endif  // BLITWIZARD_GRAPHICS2DSPRITES_H_

