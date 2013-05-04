
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

#include "os.h"
#include "luaheader.h"
#include "luastate.h"
#include "luafuncs.h"
#include "physics.h"
#include "luafuncs_graphics.h"
#include "luafuncs_graphics_camera.h"
#include "luafuncs_object.h"
#include "luafuncs_objectphysics.h"
#include "luafuncs_physics.h"
#include "luafuncs_net.h"
#include "luafuncs_media_object.h"
#include "luaerror.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "file.h"

// If USE_PHYSICS2D, or USE_PHYSICS3D respectively isn't defined we
// wouldn't want to actually have a function call to
// luastate_register2dphysics_do/luastate_register3dphysics_do
// with the physics function pointer get evaluated by gcc
// because this will result in linker errors.
// With this define, we can avoid linker errors due to the physics
// functions not being present in such a case.
#ifdef USE_PHYSICS2D
#define luastate_register2dphysics luastate_register2dphysics_do
#else
#define luastate_register2dphysics(X, Y, Z) (luastate_register2dphysics_do(X, NULL, Z))
#endif
#ifdef USE_PHYSICS3D
#define luastate_register3dphysics luastate_register2dphysics_do
#else
#define luastate_register3dphysics(X, Y, Z) (luastate_register2dphysics_do(X, NULL, Z))
#endif
#if (defined(USE_PHYSICS2D) || defined(USE_PHYSICS3D))
#define luastate_register2d3dphysics luastate_register2d3dphysics_do
#else
#define luastate_register2d3dphysics(X, Y, Z) (luastate_register2d3dphysics_do(X, NULL, Z))
#endif

#ifdef HAVE_GRAPHICS
#define luastate_registergraphics luastate_registergraphics_do
#else
#define luastate_registergraphics(X, Y, Z) (luastate_registergraphics_do(X, NULL, Z))
#endif

int functionalitymissing_2dphysics(lua_State* l) {
    return haveluaerror(l, "%s", error_nophysics2d);
}

int functionalitymissing_3dphysics(lua_State* l) {
    return haveluaerror(l, "%s", error_nophysics3d);
}

int functionalitymissing_2d3dphysics(lua_State* l) {
    return haveluaerror(l, "%s", error_nophysics2d3d);
}

int functionalitymissing_graphics(lua_State* l) {
    return haveluaerror(l, "%s", error_nographics);
}

void luastate_register2dphysics_do(lua_State* l,
#ifndef USE_PHYSICS2D
__attribute__ ((unused))
#endif
int (*func)(lua_State*), const char* name) {
    lua_pushstring(l, name);
#ifdef USE_PHYSICS2D
    lua_pushcfunction(l, func);
#else
    lua_pushcfunction(l, functionalitymissing_2dphysics);
#endif
    lua_settable(l, -3);
}

void luastate_register3dphysics_do(lua_State* l,
#ifndef USE_PHYSICS3D
__attribute__ ((unused))
#endif
int (*func)(lua_State*), const char* name) {
    lua_pushstring(l, name);
#ifdef USE_PHYSICS3D
    lua_pushcfunction(l, func);
#else
    lua_pushcfunction(l, functionalitymissing_3dphysics);
#endif
    lua_settable(l, -3);
}

void luastate_register2d3dphysics_do(lua_State* l,
#if (!defined(USE_PHYSICS2D)) && (!defined(USE_PHYSICS3D))
__attribute__ ((unused))
#endif
int (*func)(lua_State*), const char* name) {
    lua_pushstring(l, name);
#if (defined(USE_PHYSICS2D) || defined(USE_PHYSICS3D))
    lua_pushcfunction(l, func);
#else
    lua_pushcfunction(l, functionalitymissing_2d3dphysics);
#endif
    lua_settable(l, -3);
}

void luastate_registergraphics_do(lua_State* l, int (*func)(lua_State*), const char* name) {
    lua_pushstring(l, name);
#ifdef HAVE_GRAPHICS
    lua_pushcfunction(l, func);
#else
    lua_pushcfunction(l, functionalitymissing_graphics);
#endif
    lua_settable(l, -3);
}

static void luastate_registerfunc(lua_State* l, void* func, const char* name) {
    // register an object function on the table on stack position -1
    lua_pushstring(l, name);
    lua_pushcfunction(l, func);
    lua_settable(l, -3);
}

void luastate_CreatePhysicsTable(lua_State* l) {
    lua_newtable(l);
    luastate_register2dphysics(l, &luafuncs_set2dGravity, "set2dGravity");
    luastate_register2dphysics(l, &luafuncs_ray2d, "ray2d");
    luastate_register3dphysics(l, &luafuncs_set3dGravity, "set3dGravity");
    luastate_register3dphysics(l, &luafuncs_ray3d, "ray3d");
}

void luastate_CreateNetTable(lua_State* l) {
    lua_newtable(l);
    lua_pushstring(l, "open");
    lua_pushcfunction(l, &luafuncs_netopen);
    lua_settable(l, -3);
    lua_pushstring(l, "server");
    lua_pushcfunction(l, &luafuncs_netserver);
    lua_settable(l, -3);
    lua_pushstring(l, "set");
    lua_pushcfunction(l, &luafuncs_netset);
    lua_settable(l, -3);
    lua_pushstring(l, "send");
    lua_pushcfunction(l, &luafuncs_netsend);
    lua_settable(l, -3);
    lua_pushstring(l, "close");
    lua_pushcfunction(l, &luafuncs_netclose);
    lua_settable(l, -3);
}

#ifdef USE_GRAPHICS
void luastate_CreateCameraTable(lua_State* l) {
    lua_newtable(l);
    luastate_registerfunc(l, &luafuncs_camera_new, "new");
    luastate_registerfunc(l, &luafuncs_camera_destroy, "destroy");
    luastate_registerfunc(l, &luafuncs_camera_set2dCenter,
    "set2dCenter");
    luastate_registerfunc(l, &luafuncs_camera_getScreenDimensions,
    "getScreenDimensions");
    luastate_registerfunc(l, &luafuncs_camera_setScreenDimensions,
    "setScreenDimensions");
    luastate_registerfunc(l, &luafuncs_camera_get2dZoomFactor,
    "get2dZoomFactor");
    luastate_registerfunc(l, &luafuncs_camera_set2dZoomFactor,
    "set2dZoomFactor");
    luastate_registerfunc(l, &luafuncs_camera_gameUnitToPixels,
    "gameUnitToPixels");
}
#endif

void luastate_CreateGraphicsTable(lua_State* l) {
    lua_newtable(l);
    luastate_registergraphics(l, &luafuncs_getRendererName, "getRendererName");
    luastate_registergraphics(l, &luafuncs_setMode, "setMode");
    luastate_registergraphics(l, &luafuncs_getWindowSize, "getWindowSize");
    luastate_registergraphics(l, &luafuncs_getDisplayModes, "getDisplayModes");
    luastate_registergraphics(l, &luafuncs_getDesktopDisplayMode,
    "getDesktopDisplayMode");
    luastate_registergraphics(l, &luafuncs_getCameras, "getCameras");
#ifdef USE_GRAPHICS
    lua_pushstring(l, "camera");
    luastate_CreateCameraTable(l);
    lua_settable(l, -3);
#endif
}

void luastate_CreateObjectTable(lua_State* l) {
    // create a lua table populated with all object functions (blitwizard.object)
    lua_newtable(l);

    // "regular" functions:
    luastate_registerfunc(l, &luafuncs_object_new, "new");
    luastate_registerfunc(l, &luafuncs_object_destroy, "destroy");
    luastate_registerfunc(l, &luafuncs_object_getPosition, "getPosition");
    luastate_registerfunc(l, &luafuncs_object_setPosition, "setPosition");
    luastate_registerfunc(l, &luafuncs_object_setZIndex, "setZIndex");
    luastate_registerfunc(l, &luafuncs_object_getDimensions, "getDimensions");
    luastate_registerfunc(l, &luafuncs_object_getScale, "getScale");
    luastate_registerfunc(l, &luafuncs_object_setScale, "setScale");

    // graphics/visual stuff:
    luastate_registergraphics(l, &luafuncs_object_setTransparency,
    "setTransparency");
    luastate_registergraphics(l, &luafuncs_object_getTransparency,
    "getTransparency");
    luastate_registergraphics(l, &luafuncs_object_pinToCamera, "pinToCamera");
    luastate_registergraphics(l, &luafuncs_object_set2dTextureClipping,
    "set2dTextureClipping");
    luastate_registergraphics(l, &luafuncs_object_setVisible, "setVisible");

    // physics functions:
    luastate_register2d3dphysics(l, &luafuncs_object_enableStaticCollision,
    "enableStaticCollision");
    luastate_register2d3dphysics(l, &luafuncs_object_enableMovableCollision,
    "enableMovableCollision");
    luastate_register2d3dphysics(l, &luafuncs_object_disableCollision,
    "disableCollision");
    luastate_register2d3dphysics(l, &luafuncs_object_restrictRotation,
    "restrictRotation");
    luastate_register3dphysics(l, &luafuncs_object_restrictRotationAroundAxis,
    "restrictRotationAroundAxis");
    luastate_register2d3dphysics(l, &luafuncs_object_impulse,
    "impulse");
    luastate_register2d3dphysics(l, &luafuncs_object_setMass,
    "setMass");
    luastate_register2d3dphysics(l, &luafuncs_object_setRestitution,
    "setRestitution");
    luastate_register2d3dphysics(l, &luafuncs_object_setFriction,
    "setFriction");
    luastate_register2d3dphysics(l, &luafuncs_object_setAngularDamping,
    "setAngularDamping");
    luastate_register2d3dphysics(l, &luafuncs_object_setLinearDamping,
    "setLinearDamping");
    luastate_register2d3dphysics(l, &luafuncs_object_setGravity,
    "setGravity");
}

void luastate_CreateSimpleSoundTable(lua_State* l) {
    lua_newtable(l);
    luastate_registerfunc(l, &luafuncs_media_simpleSound_new, "new");
    luastate_registerfunc(l, &luafuncs_media_simpleSound_play, "play");
    luastate_registerfunc(l, &luafuncs_media_simpleSound_adjust, "adjust");
    luastate_registerfunc(l, &luafuncs_media_simpleSound_setPriority, "setPriority");
    luastate_registerfunc(l, &luafuncs_media_simpleSound_stop, "stop");
}

void luastate_CreateAudioTable(lua_State* l) {
    lua_newtable(l);

    lua_pushstring(l, "simpleSound");
    luastate_CreateSimpleSoundTable(l);
    lua_settable(l, -3);
}

void luastate_CreateTimeTable(lua_State* l) {
    lua_newtable(l);
    lua_pushstring(l, "getTime");
    lua_pushcfunction(l, &luafuncs_getTime);
    lua_settable(l, -3);
    lua_pushstring(l, "sleep");
    lua_pushcfunction(l, &luafuncs_sleep);
    lua_settable(l, -3);
}


