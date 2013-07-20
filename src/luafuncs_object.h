
/* blitwizard game engine - source code file

  Copyright (C) 2012-2013 Jonas Thiem

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

#ifndef BLITWIZARD_OBJECT_H_
#define BLITWIZARD_OBJECT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "blitwizardobject.h"

int luafuncs_object_new(lua_State* l);
void luacfuncs_pushbobjidref(lua_State* l, struct blitwizardobject* o);
struct blitwizardobject* toblitwizardobject(lua_State* l, int index, int arg, const char* func);

int luafuncs_object_getPosition(lua_State* l);
int luafuncs_object_setPosition(lua_State* l);
int luafuncs_object_setZIndex(lua_State* l);
int luafuncs_object_destroy(lua_State* l);
int luafuncs_object_setParallax(lua_State* l);
int luafuncs_object_getDimensions(lua_State* l);
int luafuncs_object_getOriginalDimensions(lua_State* l);
int luafuncs_object_getScale(lua_State* l);
int luafuncs_object_setScale(lua_State* l);
int luafuncs_object_scaleToDimensions(lua_State* l);
int luafuncs_object_setInvisibleToMouse(lua_State* l);

// iterate through all objects:
int luafuncs_getAllObjects(lua_State* l);

// set the function on top of the stack as event function:
// (put nil on top of the stack if you want to clear the event)
void luacfuncs_object_setEvent(lua_State* l,
struct blitwizardobject* o, const char* eventName);

// Call the event function for a given event.
// 'self' will be set to the given object.
//
// If you want to pass arguments to the event functions,
// push them onto the stack (last argument on top)
// and specify their number with the args parameter.
//
// Arguments will be popped from the stack.
int luacfuncs_object_callEvent(lua_State* l,
struct blitwizardobject* o, const char* eventName,
int args, int* boolreturnvalue);
// Returns 0 if there was a lua error in the
// event function, otherwise 1.

// clear the registry table of the object:
// (used internally for cleanup)
void luacfuncs_object_clearRegistryTable(lua_State* l,
struct blitwizardobject* o);

// do all doAlways events and other event things.
// Specify a higher count than 1 if you want to do
// more logic events than 1 at once.
// The function will return the amount of events
// it has done (not necessarily as much as you advised!).
int luacfuncs_object_doAllSteps(int count);

// update the object's graphics:
void luacfuncs_object_updateGraphics(void);

// get/set object transparency (0 solid, 1 invisible):
int luafuncs_object_getTransparency(lua_State* l);
int luafuncs_object_setTransparency(lua_State* l);

// set 2d texture clipping window:
int luafuncs_object_set2dTextureClipping(lua_State* l);

// pin a 2d object to a camera:
int luafuncs_object_pinToCamera(lua_State* l);

// set object visibility:
int luafuncs_object_setVisible(lua_State* l);

// get and adjust 2d rotation:
int luafuncs_object_setRotationAngle(lua_State* l);
int luafuncs_object_getRotationAngle(lua_State* l);

// statistics for debugging:
extern int processedImportantObjects;
extern int processedNormalObjects;
extern int processedBoringObjects;

#ifdef __cplusplus
}
#endif

#endif  // BLITWIZARD_OBJECT_H_

