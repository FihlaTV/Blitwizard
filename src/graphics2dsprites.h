
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

#include "config.h"
#include "os.h"

#define VALIDATESPRITEPOS

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
// Specify a negative horizontal or vertical size for horizontal/vertical
// mirroring.
struct graphics2dsprite* graphics2dsprites_create(
const char* texturePath, double x, double y, double width, double height);

// Get/set userdata on a sprite:
void graphics2dsprites_setUserdata(struct graphics2dsprite* sprite, void* data);
void* graphics2dsprites_getUserdata(struct graphics2dsprite* sprite);

// Check for sprite pixel geometry dimensions.
// If 0 is returned, the geometry isn't known yet (texture still
// being loaded).
// Otherwise, 1 will be returned and width/height changed.
// Returns -1 in case of a fatal loading error.
int graphics2dsprites_getGeometry(struct graphics2dsprite* sprite,
size_t* width, size_t* height);

// Set a clipping window so only a part of the sprite's basic texture
// will be shown.
void graphics2dsprites_setClippingWindow(struct graphics2dsprite* sprite,
size_t x, size_t y, size_t w, size_t h);

// Unset the clipping window:
void graphics2dsprites_unsetClippingWindow(struct graphics2dsprite* sprite);

// Check if the sprite will be possibly rendered.
// It might not be if the texture isn't loaded yet,
// even if it is set to visible.
int graphics2dsprites_isTextureAvailable(struct graphics2dsprite* sprite);

// Set parallax effect to sprite
void graphics2dsprites_setParallaxEffect(struct graphics2dsprite* sprite,
double value);

// Move a sprite.
void graphics2dsprites_move(struct graphics2dsprite* sprite,
double x, double y, double angle);

// Set sprite to invisible (visible = 0) or back to visible (1).
void graphics2dsprites_setVisible(struct graphics2dsprite* sprite,
int visible);

// Resize a sprite. Set negative sizes for mirroring,
// and 0, 0 for width/height if you want to have the sprite
// size determined purely by texture dimensions.
void graphics2dsprites_resize(struct graphics2dsprite* sprite,
double width, double height);

// Set sprite coloring. (1, 1, 1) is normal full brightness
void graphics2dsprites_setColor(struct graphics2dsprite* sprite,
double r, double g, double b);

// Set sprite alpha from 0 (invisible) to 1 (fully opaque)
void graphics2dsprites_setAlpha(struct graphics2dsprite* sprite,
double alpha);

// Destroy the specified sprite:
void graphics2dsprites_destroy(struct graphics2dsprite* sprite);

// Set the Z index of the sprite (Defaults to 0):
void graphics2dsprites_setZIndex(struct graphics2dsprite* sprite,
int zindex);

// --- internally used to draw sprites: ---

// Set callback for sprite creation/deletion and modification.
// The modified callback will be used for moved, scaled and in other
// ways altered sprites.
//
// If your graphics output creates and uploads meshes/geometry to the
// graphics card, you might want to use those callbacks for that.
//
// You won't get the callback for a sprite unless the graphics texture
// is present and the sprite is visible
// The textures can be invalid after the next call of
// graphicstexture_InvalidateTextures()! (You'll get that information
// with the next call of graphics2dsprite_triggerCallbacks of course)
//
// NOTE: The callbacks are always batched up and you can request them
// to happen with graphics2dsprites_triggerCallbacks.
void graphics2dsprites_setCreatedModifiedDeletedCallbacks(
void (*spriteCreated) (void* handle,
const char* path, struct graphicstexture* tex,
double x, double y,
double width, double height, size_t texWidth, size_t texHeight,
double angle, int horizontalflip,
int verticalflip,
double alpha, double r, double g, double b,
size_t sourceX, size_t sourceY, size_t sourceWidth, size_t sourceHeight,
int zindex, int visible, int cameraId),
void (*spriteModified) (void* handle,
double x, double y,
double width, double height, double angle, int horizontalflip,
int verticalflip,
double alpha, double r, double g, double b,
size_t sourceX, size_t sourceY, size_t sourceWidth, size_t sourceHeight,
int zindex, int visible, int cameraId),
void (*spriteDeleted) (void* handle)
);

// Trigger the sprite callbacks you previously set.
// As soon as the function returns, the callbacks for all
// recent sprite changes will be completed.
void graphics2dsprites_triggerCallbacks(void);

// Get information on all sprites in one go.
//
// If your graphics output simply redraws everything per frame,
// you might want to use this function.
//
// For sprites with no texture loaded/available,
// the tex parameter will be set to NULL.
void graphics2dsprites_doForAllSprites(
void (*spriteInformation) (
const struct graphics2dsprite* sprite,
const char* path, struct graphicstexture* tex,
double r, double g, double b, double alpha,
int visible, int cameraId));
// Sprites will be returned in Z-Index order.
// (lower index first, and for same z-index
// with the older sprites first)

// Get alpha transparency for sprites (defaults to 1):
double graphics2dsprites_getAlpha(
struct graphics2dsprite* sprite);

// Set alpha for sprites from 0 (invisible)
// to 1 (solid).
void graphics2dsprites_setAlpha(
struct graphics2dsprite* sprite, double alpha);

// Set pinned to camera state:
void graphics2dsprites_setPinnedToCamera(struct graphics2dsprite* sprite,
int cameraId);

// Report visibility to texture manager
void graphics2dsprites_reportVisibility(void);

#define SPRITE_EVENT_TYPE_CLICK 1
#define SPRITE_EVENT_TYPE_MOTION 2
#define SPRITE_EVENT_TYPE_COUNT 3
// Get sprite at the given screen position
// (e.g. mouse position) with the given camera
// if event >= 0, then only sprites relevant for the
// given event will be searched (-> faster).
struct graphics2dsprite* 
graphics2dsprites_getSpriteAtScreenPos(
int cameraId, int x, int y, int event);

// enable or disable a sprite for a given event
// search:
void graphics2dsprites_enableForEvent(
struct graphics2dsprite* sprite, int event, int enabled);
// event will be one of SPRITE_EVENT_TYPE_*
// (enable can be 1 for yes or 0 for no)

// set invisible/transparent to event so other sprites below
// won't be obstructed.
// IMPORTANT: this is ignored if enableForEvent is set!
void graphics2dsprites_setInvisibleForEvent(struct graphics2dsprite* sprite,
int event, int invisible);
// set invisible to 1 for invisible, 0 for visible/regular (default)

// Return the total amount of 2d sprites.
size_t graphics2dsprites_Count(void);

// Calculate the size of a given sprite on a given camera:
void graphics2dsprite_calculateSizeOnScreen(
const struct graphics2dsprite* sprite,
int cameraId,
double* screen_x, double* screen_y, double* screen_w,
double* screen_h, double* screen_sourceX, double* screen_sourceY,
double* screen_sourceW, double* screen_sourceH,
double* source_angle, int* phoriflip, int compensaterotation);
// FIXME: This can NOT be safely used outside the doForAllSprites callback!
// (threading issues)

#ifdef __cplusplus
}
#endif

#endif  // USE_GRAPHICS

#endif  // BLITWIZARD_GRAPHICS2DSPRITES_H_

