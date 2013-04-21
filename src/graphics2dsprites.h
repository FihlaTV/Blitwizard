
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

#ifndef BLITWIZARD_GRAPHICS2DSPRITES_H_
#define BLITWIZARD_GRAPHICS2DSPRITES_H_

#ifdef USE_GRAPHICS

#ifdef __cplusplus
extern "C" {
#endif

#include "graphicstexture.h"

struct graphics2dsprite;

// All functions of this sprite api are thread-safe.

// Create a sprite at a given position (x/y specify the sprite center).
// If you specify 0,0 for the size, it will be set automatically
// as soon as the geometry information is available.
struct graphics2dsprite* graphics2dsprites_Create(
const char* texturePath, double x, double y, double width, double height);

// Check for sprite pixel geometry dimensions.
// If 0 is returned, the geometry isn't known yet.
// Otherwise, 1 will be returned and width/height changed.
int graphics2dsprites_GetGeometry(struct graphics2dsprite* sprite,
size_t* width, size_t* height);

// Check if the sprite will be possibly rendered.
// It might not be if the texture isn't loaded yet,
// even if it is set to visible.
int graphics2dsprites_IsTextureAvailable(struct graphics2dsprite* sprite);

// Move a sprite.
void graphics2dsprites_Move(struct graphics2dsprite* sprite,
double x, double y, double angle);

// Set sprite to invisible (visible = 0) or back to visible (1).
void graphics2dsprites_SetVisible(struct graphics2dsprite* sprite,
int visible);

// Resize a sprite.
void graphics2dsprites_Resize(struct graphics2dsprite* sprite,
double width, double height);

// Flip/mirror a sprite
void graphics2dsprites_Flip(struct graphics2dsprite* sprite,
int horizontalflip, int verticalflip);

// Set sprite coloring. (1, 1, 1) is normal full brightness
void graphics2dsprites_SetColor(struct graphics2dsprite* sprite,
double r, double g, double b);

// Set sprite alpha from 0 (invisible) to 1 (fully opaque)
void graphics2dsprites_SetAlpha(struct graphics2dsprite* sprite,
double alpha);

// Destroy the specified sprite:
void graphics2dsprites_Destroy(struct graphics2dsprite* sprite);


// --- internally used to draw sprites: ---

// Set callback for sprite creation/deletion. modified/moved sprites
// will end up deleted and recreated.
//
// If your graphics output creates and uploads meshes/geometry to the
// graphics card, you might want to use those callbacks for that.
//
// You won't get the callback for a sprite unless the graphics texture
// is present. The textures can be invalid after the next call of
// graphicstexture_InvalidateTextures()! (You'll get that information
// with the next call of graphics2dsprite_TriggerCallbacks of course)
//
// NOTE: The callbacks are always batched up and you can request them
// to happen with graphics2dsprite_TriggerCallbacks.
void graphics2dsprites_SetCreateDeleteCallbacks(
void (*spriteCreated) (void* handle,
const char* path, struct graphicstexture* tex,
double x, double y,
double width, double height, double angle, int horizontalflip,
int verticalflip,
double alpha, double r, double g, double b,
int zindex),
void (*spriteDeleted) (void* handle)
);

// Trigger the sprite callbacks you previously set.
// As soon as the function returns, the callbacks for all
// recent sprite changes will be completed.
void graphics2dsprites_TriggerCallbacks(void);

// Get information on all sprites in one go.
//
// If your graphics output simply redraws everything per frame,
// you might want to use this function.
//
// For sprites with no texture loaded/available,
// the tex parameter will be set to NULL.
void graphics2dsprites_DoForAllSprites(
void (*spriteInformation) (const char* path, struct graphicstexture* tex,
double x, double y, double width, double height,
double angle, int horizontalflip,
int verticalflip, double alpha, double r, double g, double b));
// Sprites will be returned in Z-Index order.
// (lower index first, and for same z-index
// with the older sprites first)

#ifdef __cplusplus
}
#endif

#endif  // USE_GRAPHICS

#endif  // BLITWIZARD_GRAPHICS2DSPRITES_H_

