
/* blitwizard game engine - source code file

  Copyright (C) 2011-2014 Jonas Thiem

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

#ifndef BLITWIZARD_GRAPHICS_H_
#define BLITWIZARD_GRAPHICS_H_

#include "config.h"
#include "os.h"

#ifdef USE_GRAPHICS

#ifdef __cplusplus
extern "C" {
#endif

#ifdef WINDOWS
// for HWND usage:
#include <windows.h>
#endif

#define UNIT_TO_PIXELS_DEFAULT (40.0)
extern double UNIT_TO_PIXELS;
extern int unittopixelsset;

int graphics_areGraphicsRunning(void);
// Returns 1 if the graphics are open/active, otherwise 0.

int graphics_setMode(int width, int height, int fullscreen,
    int resizable, const char *title, const char *renderer, char **error);
// Set or change graphics mode.
// This can possibly cause all textures to become unloaded and reloaded,
// so is a possibly very slow operation.
// Renderer can be NULL (for any), "software", "opengl" or "direct3d".
// If you want the best for your system, go for NULL.

// Return the name of the current renderer
const char *graphics_getCurrentRendererName(void);
// Get the renderer currently used for the active graphics mode.
// Returns NULL when no mode has been set.

#ifdef WINDOWS
HWND graphics_getWindowHWND(void); // get win32 HWND handle for the window
#endif

int graphics_getNumberOfVideoModes(void);
// Get the number of supported video modes (= the modes usable in fullscreen)

void graphics_getVideoMode(int index, int* width, int* height);
// Get the video mode at the given index (0..graphics_GetNumberOfVideoMode()-1)

void graphics_getDesktopVideoMode(int* x, int* y);
// Get the current video mode of the desktop

void graphics_close(int preservetextures);
// Close the graphics. preservetextures 1: keep them available for use,
// 0: dispose of them

void graphics_quit(void);
// Quit the graphics completely

int graphics_isFullscreen(void);
// Return if the graphics are currently running at full screen.
// 1: yes, 0: no. Undefined result when no graphics mode set

void graphics_minimizeWindow(void);
// Minimize the window

int graphics_getWindowDimensions(unsigned int* width, unsigned int* height);
// 1 on success, 0 on error (window not opened most likely)

const char* graphics_getWindowTitle(void);
// Return the current title of the window

void graphics_checkEvents(void (*quitevent)(void), void (*mousebuttonevent)(int button, int release, int x, int y), void (*mousemoveevent)(int x, int y), void (*keyboardevent)(const char* button, int release), void (*textevent)(const char* text), void (*putinbackground)(int background));
// Check for events and return info about them through the provided callbacks

void graphics_transferTexturesFromHW(void);
// Transfer textures from SDL, e.g. if app is in background

int graphics_transferTexturesToHW(void);
// Transfer textures back to SDL

#ifdef ANDROID
void graphics_reopenForAndroid(void);
// Reopen graphics and reupload textures. Required when coming back into foreground
#endif

int graphics_haveValidWindow(void);
// Returns 1 if a window is open, otherwise 0


// CAMERA HANDLING:

int graphics_getCameraCount(void);
// Get count of cameras

int graphics_getCameraX(int index);
// Get the screen X position of the specified camera
// 'index' specifies the index from 0..count-1
// for the specific camera.

int graphics_getCameraY(int index);
// Get the screen Y position of the specified camera

int graphics_getCameraWidth(int index);
// Get the screen width of the specified camera

int graphics_getCameraHeight(int index);
// Get the screen height of the specified camera

void graphics_setCameraXY(int index, int x, int y);
// Update the screen X/Y position of the camera

void graphics_setCameraSize(int index, int width, int height);
// Update width/height of the given camera

double graphics_getCamera2DZoom(int index);
// Get 2d zoom factor of camera
// A 2d unit on screen equals (UNIT_TO_PIXELS * factor)
// pixels.

void graphics_setCamera2DZoom(int index, double zoom);
// Set 2d camera zoom factor

double graphics_getCamera3DFov(int index);
// Get the camera's 3d field of view.
// Returns the angle that is visible through
// the camera horizontally, e.g. 90 for 90 degree.

double graphics_setCamera3DFov(int index, double fov);
// Set the camera's 3d field of view.

double graphics_getCamera2DAspectRatio(int index);
// Get camera 2d aspect ratio.
// Specified in vertical/horizontal,
// 1: square, 0.5: twice as wide as vertically high

void graphics_setCamera2DAspectRatio(int index, double ratio);
// Set camera 2d aspect ratio

double graphics_getCamera3DAspectRatio(int index);
// Get the camera 3d aspect ratio, see 2d aspect ratio.

void graphics_setCamera3DAspectRatio(int index, double ratio);
// Set the camera 3d aspect ratio

double graphics_getCamera2DCenterX(int index);
// Get camera 2d center x position

double graphics_getCamera2DCenterY(int index);
// Get camera 2d center y position

void graphics_setCamera2DCenterXY(int index, double x, double y);
// Set a new camera 2d center to focus at

double graphics_getCamera3DCenterX(int index);
double graphics_getCamera3DCenterY(int index);
double graphics_getCamera3DCenterZ(int index);
// Get camera 3d center x/y/z position

void graphics_setCamera3DCenterXYZ(int index, double x, double y,
double z);
// Set a new camera 3d center to focus at 

double graphics_getCamera2DRotation(int index);
// Get the 2d rotation angle (degree, counter-clockwise around the
// center)

void graphics_setCamera2DRotation(int index, double degree);
// Set the 2d rotation angle

void graphics_getCamera3DRotation(int index,
double* x, double* y, double* z, double* r);
// Get camera 3d rotation as quaternion

void graphics_setCamera3DRotation(int index,
double x, double y, double z, double r);
// Set camera 3d rotation as quaternion

double graphics_getCamera3DZNear(int index);
// Get 3d camera z near clipping value

double graphics_getCamera3DZFar(int index);
// Get 3d camera z far clipping value

void graphics_setCamera3DZNearFar(int index,
double near, double far);
// Set 3d camera z near/far clipping values

int graphics_addCamera(void);
// Add a camera. Returns new camera index on success (>=0),
// -1 on error.

void graphics_deleteCamera(int index);
// Delete a specified camera.
// IMPORTANT: This will cause all cameras with a higher
// index to move down by one!

void graphics_calculateUnitToPixels(int screenWidth, int screenHeight);
// Called internally on first setMode to adapt the unit-to-pixels ratio to a nice default

int graphics_getCameraAt(int x, int y);
// Get camera which covers the given position (or -1 if none).

#ifdef __cplusplus
}
#endif

#else  // ifdef USE_GRAPHICS

#define compiled_without_graphics "No graphics available - this binary was compiled with graphics (including null device) disabled"

#endif  // ifdef USE_GRAPHICS

#endif  // BLITWIZARD_GRAPHICS_H_

