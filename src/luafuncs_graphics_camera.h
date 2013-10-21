
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

#ifndef BLItWIZARD_GRAPHICS_CAMERA_H_
#define BLITWIZARD_GRAPHICS_CAMERA_H_

#if defined(USE_GRAPHICS)

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include "luaheader.h"

int luafuncs_getCameras(lua_State* l);
int luafuncs_camera_new(lua_State* l);
int luafuncs_camera_destroy(lua_State* l);
int luafuncs_camera_screenPosTo2dWorldPos(lua_State* l);
int luafuncs_camera_getPixelDimensionsOnScreen(lua_State* l);
int luafuncs_camera_setPixelDimensionsOnScreen(lua_State* l);
int luafuncs_camera_getDimensions(lua_State* l);
int luafuncs_camera_get2dCenter(lua_State* l);
int luafuncs_camera_set2dCenter(lua_State* l);
int luafuncs_camera_set2dZoomFactor(lua_State* l);
int luafuncs_camera_get2dZoomFactor(lua_State* l);

int toluacameraid(lua_State* l,
int index, int arg, const char* func);

#endif  // USE_GRAPHICS

#endif  // BLITWIZARD_GRAPHICS_CAMERA_H_

