
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

/// The blitwizard graphics namespace allows to set up the graphics device
// for graphical 2d/3d output.
// @author Jonas Thiem  (jonas.thiem@gmail.com)
// @copyright 2011-2013
// @license zlib
// @module blitwizard.graphics

#include "config.h"
#include "os.h"

#if (defined(USE_GRAPHICS))

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include "luaheader.h"

#include "os.h"
#include "logging.h"
#include "luaerror.h"
#include "luastate.h"
#include "blitwizardobject.h"
#include "physics.h"
#include "objectphysicsdata.h"
#include "luafuncs_object.h"
#include "luafuncs_physics.h"
#include "main.h"
#include "graphics.h"

// camera handling:

struct luacameralistentry {
    // This entry is referenced to by garbage collected lua
    // idrefs, and will be deleted once all lua idrefs are gone.
    // It identifies a camera by camera slot id (see graphics.h).

    int cameraslot;  // if -1, the camera at that slot was deleted,
    // and this is a stale entry
    int refcount;  // if 0, we will delete this entry

    struct luacameralistentry* prev, *next;
};
struct luacameralistentry* luacameralist = NULL;

static int garbagecollect_cameraobjref(lua_State* l) {
    // get id reference to object
    struct luaidref* idref = lua_touserdata(l, -1);

    if (!idref || idref->magic != IDREF_MAGIC
    || idref->type != IDREF_CAMERA) {
        // either wrong magic (-> not a luaidref) or not a camera object
        lua_pushstring(l, "internal error: invalid camera object ref");
        lua_error(l);
        return 0;
    }

    // it is a valid camera object, decrease ref count:
    struct luacameralistentry* e = idref->ref.camera;
    e->refcount--;

    if (e->refcount <= 0) {
        // remove entry from list:
        if (e->prev) {
            e->prev->next = e->next;
        } else {
            luacameralist = e->next;
        }
        if (e->next) {
            e->next->prev = e->prev;
        }
        // delete entry:
        free(e);
    }
    return 0;
}

void luacfuncs_pushcameraidref(lua_State* l, struct luacameralistentry* c) {
    // create luaidref userdata struct which points to the blitwizard object
    struct luaidref* ref = lua_newuserdata(l, sizeof(*ref));
    memset(ref, 0, sizeof(*ref));
    ref->magic = IDREF_MAGIC;
    ref->type = IDREF_CAMERA;
    ref->ref.camera = c;

    // set garbage collect callback:
    luastate_SetGCCallback(l, -1,
    (int (*)(void*))&garbagecollect_cameraobjref);

    // set metatable __index to blitwizard.graphics.camera table
    lua_getmetatable(l, -1);
    lua_pushstring(l, "__index");
    lua_getglobal(l, "blitwizard");
    if (lua_type(l, -1) == LUA_TTABLE) {
        // extract blitwizard.graphics:
        lua_pushstring(l, "graphics");
        lua_gettable(l, -2);
        lua_insert(l, -2);
        lua_pop(l, 1);  // pop blitwizard table

        if (lua_type(l, -1) != LUA_TTABLE) {
            // error: blitwizard.graphics isn't a table as it should be
            lua_pop(l, 3); // blitwizard.graphics, "__index", metatable
        } else {
            // extract blitwizard.graphics.camera:
            lua_pushstring(l, "camera");
            lua_gettable(l, -2);
            lua_insert(l, -2);
            lua_pop(l, 1);  // pop blitwizard.graphics table

            if (lua_type(l, -1) != LUA_TTABLE) {
                // error: blitwizard.graphics.camera isn't a table,
                // as it clearly should be
                lua_pop(l, 3); // blitwizard.graphics.camera, "__index",
                                // metatable
            } else {
                lua_rawset(l, -3); // removes blitwizard.graphics.camera,
                                    // "__index"
                lua_setmetatable(l, -2); // setting remaining metatable
                // stack is now back empty, apart from new userdata!
            }
        }
    } else {
        // error: blitwizard namespace is broken. nothing we could do
        lua_pop(l, 3);  // blitwizard, "__index", metatable
    }

    c->refcount++;
}

struct luacameralistentry* toluacameralistentry(lua_State* l,
int index, int arg, const char* func) {
    if (lua_type(l, index) != LUA_TUSERDATA) {
        haveluaerror(l, badargument1, arg, func, "camera",
        lua_strtype(l, index));
    }
    if (lua_rawlen(l, index) != sizeof(struct luaidref)) {
        haveluaerror(l, badargument2, arg, func, "not a valid camera");
    }
    struct luaidref* idref = lua_touserdata(l, index);
    if (!idref || idref->magic != IDREF_MAGIC
    || idref->type != IDREF_CAMERA) {
        haveluaerror(l, badargument2, arg, func, "not a valid camera");
    }
    struct luacameralistentry* c = idref->ref.camera;
    if (c->cameraslot < 0) {
        haveluaerror(l, badargument2, arg, func, "this camera was deleted");
    }
    return c;
}

int toluacameraid(lua_State* l,
int index, int arg, const char* func) {
    struct luacameralistentry* le = toluacameralistentry(l, index, arg,
    func);
    return le->cameraslot;
}

/// Get a table array with all currently active cameras.
// (per default, this is only one)
// @function getCameras
// @treturn table a table list array containing @{blitwizard.graphics.camera|camera} items
int luafuncs_getCameras(lua_State* l) {
    lua_newtable(l);
    int c = graphics_GetCameraCount();
    int i = 0;
    while (i < c) {
        struct luacameralistentry* e = malloc(sizeof(*e));
        if (!e) {
            return 0;
        }
        memset(e, 0, sizeof(*e));
        e->cameraslot = i;
        e->prev = NULL;
        e->next = luacameralist;
        if (e->next) {
            e->next->prev = e;
        }
        luacameralist = e;
        lua_pushnumber(l, i+1);
        luacfuncs_pushcameraidref(l, e);
        lua_settable(l, -3);
        i++;
    }
    return 1;
}

/// Blitwizard camera object which represents a render camera.
//
// A render camera has a rectangle on the screen which is a sort of
// "window" that shows a view into the 2d/3d world of your
// blitwizard game.
//
// Per default, you have one camera that fills up the whole
// screen with one single large view. But if you want to split
// the screen up e.g. for split screen multiplayer games, and show
// different world views side by side, you would want multiple
// cameras.
//
// <b>Important:</b> If you want to show a different part of the game world,
// this is where you want to be! Grab the first camera from
// @{blitwizard.graphics.getCameras} and let's go:
//
// Use @{blitwizard.graphics.camera:set2dCenter|camera:set2dCenter}
// to modify the 2d world
// position shown, and
// @{blitwizard.graphics.camera:set3dCenter|set3dCenter}
// to modify the 3d world position shown if you use any 3d objects.
//
// <i>Please note the SDL 2d renderer doesn't support more than one camera.
// The OGRE 3d renderer supports multiple cameras.</i>
// @type camera

/// Destroy the given camera.
//
// It will be removed from the @{blitwizard.graphics.getCameras} list
// and all references you still have to it will become invalid.
// @function destroy
// @tparam userdata camera the @{blitwizard.graphics.camera|camera} to be destroyed
int luafuncs_camera_destroy(lua_State* l) {
#ifdef USE_SDL_GRAPHICS
    return haveluaerror(l, "the SDL renderer backend doesn't support destroying or adding cameras");
#else
    struct luacameralistentry* e = toluacameralistentry(
    l, 1, 0, "blitwizard.graphics.camera:destroy");
    graphics_DeleteCamera(e->cameraslot);
    int slot = e->cameraslot;
    e->cameraslot = -1;
    // now, disable all existing entries in the list of refs we gave out:
    e = luacameralist;
    while (e) {
        if (e->cameraslot == slot) {
            e->cameraslot = -1;
        }
        if (e->cameraslot > slot) {
            // move this slot down by one:
            e->cameraslot -= 1;
        }
        e = e->next;
    }
    return 0;
#endif
}

/// Add a new camera.
// Returns the new @{blitwizard.graphics.camera|camera}, and
// it will be automatically part of the list of all followup
// @{blitwizard.graphics.getCameras} calls aswell.
//
// If you want to get rid of it again, check out
// @{blitwizard.graphics.camera:destroy}.
// @function new
int luafuncs_camera_new(lua_State* l) {
#ifdef USE_SDL_GRAPHICS
    return haveluaerror(l, "the SDL renderer doesn't support multiple cameras");
#else
    int i = graphics_AddCamera();
    if (i < 0) {
        return haveluaerror(l, "error when adding the new camera");
    }
    // see if we have an existing entry:
    struct luacameralistentry* e = luacameralist;
    while (e) {
        if (e->cameraslot == i) {
            e->refcount++;

            // return idref this existing entry:
            luacfuncs_pushcameraidref(l, e);
            return 1;
        }
        e = e->next;
    }
    // we need to add a new entry:
    e = malloc(sizeof(*e));
    if (!e) {
        return haveluaerror(l, "allocating camera entry failed");
    }
    memset(e, 0, sizeof(*e));
    e->cameraslot = i;
    if (luacameralist) {
        e->next = luacameralist;
        luacameralist->prev = e;
    }
    luacameralist = e;
    luacfuncs_pushcameraidref(l, e);
    return 1;
#endif
}

/// Specify a 2d position in the range from 0,0 to
// w,h with w,h being the camera visible area size
// as from @{blitwizard.graphics.camera:getDimensions|
// getDimensions} with consider_zoom set to <b>false</b>.
//
// (So the position you specify is the coordinates a
// @{blitwizard.object:pinToCamera|pinned object} would have on screen
// or the coordinates you get from @{blitwizard.onMouseMove})
//
// The position can also be smaller than 0,0 or larger than the
// actual camera dimensions, in which case you'll get an out-of-screen
// world position as a result.
// @function screenPosTo2dWorldPos
// @tparam number pos_x the X coordinate of the screen position
// @tparam number pos_y the Y coordinate of the screen position
// @tparam number parallax (optional) if you want, specify the @{blitwizard.object:setParallax|parallax} effect strength to get the world position displaced accordingly for an object with that parallax effect strength
// @treturn number X coordinate of the resulting world position
// @treturn number Y coordinate of the resulting world position
int luafuncs_camera_screenPosTo2dWorldPos(lua_State* l) {
    struct luacameralistentry* e = toluacameralistentry(
    l, 1, 0, "blitwizard.graphics.camera:screenPosTo2dWorldPos");

    if (lua_type(l, 2) != LUA_TNUMBER) {
        return haveluaerror(l, badargument1, 1,
        "blitwizard.graphics.camera:screenPosTo2dWorldPos", "number", lua_strtype(l, 2));
    }
    if (lua_type(l, 3) != LUA_TNUMBER) {
        return haveluaerror(l, badargument1, 2,
        "blitwizard.graphics.camera:screenPosTo2dWorldPos", "number", lua_strtype(l, 3));
    }
    double parallax = 1;
    if (lua_gettop(l) >= 4 && lua_type(l, 4) != LUA_TNIL) {
        if (lua_type(l, 4) != LUA_TNUMBER) {
            return haveluaerror(l, badargument1, 3,
            "blitwizard.graphics.camera:screenPosTo2dWorldPos", "number", lua_strtype(l, 4));
        }
        if (lua_tonumber(l, 4) <= 0) {
            return haveluaerror(l, badargument2, 3,
            "blitwizard.graphics.camera:screenPosTo2dWorldPos",
            "parallax effect strength needs to be greater than zero");
        }
        parallax = lua_tonumber(l, 4);
    }
    double x = lua_tonumber(l, 2);
    double y = lua_tonumber(l, 3);
    
    // calculate camera top left world position:
    double tx = graphics_GetCamera2DCenterX(e->cameraslot);
    double ty = graphics_GetCamera2DCenterY(e->cameraslot);
    double zoomscale =
    (UNIT_TO_PIXELS * graphics_GetCamera2DZoom(e->cameraslot))
    / (double)UNIT_TO_PIXELS_DEFAULT;
    double cameraWidth = graphics_GetCameraWidth(e->cameraslot)
        / (double)UNIT_TO_PIXELS;
    double cameraHeight = graphics_GetCameraHeight(e->cameraslot)
        / (double)UNIT_TO_PIXELS;
    tx -= (cameraWidth / zoomscale) * 0.5f;
    ty -= (cameraHeight / zoomscale) * 0.5f;

    // apply parallax effect as desired with screen center as parallax center:
    x -= cameraWidth / 2;
    y -= cameraHeight / 2;
    x /= parallax;
    y /= parallax;
    x += cameraWidth / 2;
    y += cameraHeight / 2;

    // the onscreen coordinates need to be translated into "zoomed" space:
    x /= graphics_GetCamera2DZoom(e->cameraslot);
    y /= graphics_GetCamera2DZoom(e->cameraslot);

    lua_pushnumber(l, tx + x);
    lua_pushnumber(l, ty + y);
    return 2;
}

/// Set the camera's 2d center which is the position in the 2d world
// centered by the camera. This allows you to move the camera's shown
// part of the 2d world around, focussing on other places.
//
// The position is specified in <b>game units</b> (the same units used
// for specifying the @{blitwizard.object:setPosition|position of objects}).
// The camera initially looks at position 0, 0.
//
// If you want the camera to follow a 2d object, simply use this function
// to set the 2d center to the coordinates of the object (you can do this
// in the object's @{blitwizard.object:doAlways|doAlways} function).
// @function set2dCenter
// @tparam number x_position X position of the 2d point to look at
// @tparam number y_position Y position of the 2d point to look at
// @usage
// -- make the default camera look at position 3, 3:
// local camera = blitwizard.graphics.getCameras()[1]
// camera:set2dCenter(3, 3)
int luafuncs_camera_set2dCenter(lua_State* l) {
    struct luacameralistentry* e = toluacameralistentry(
    l, 1, 0, "blitwizard.graphics.camera:set2dCenter");
    if (lua_type(l, 2) != LUA_TNUMBER) {
        return haveluaerror(l, badargument1, 1,
        "blitwizard.graphics.camera:set2dCenter", "number", lua_strtype(l, 2));
    }
    if (lua_type(l, 3) != LUA_TNUMBER) {
        return haveluaerror(l, badargument1, 2,
        "blitwizard.graphics.camera:set3dCenter", "number", lua_strtype(l, 3));
    }
    graphics_SetCamera2DCenterXY(e->cameraslot, lua_tonumber(l, 2),
    lua_tonumber(l, 3)); 
    return 0;
}

/// Get the size of the camera in game units (so the size of the
// visible world area it shows).
//
// consider_zoom = false gives you the area of
// @{blitwizard.object:pinToCamera|pinned objects} which the camera
// shows, which is not influenced by zoom. (this is the default if
// consider_zoom is no specified at all)
//
// consider_zoom = true is a special case useful for 2d,
// which gives you the camera's dimensions according to the current
// zoom factor.
//
// The result of this function depends on the camera's
// @{blitwizard.graphics.camera:getPixelDimensionsOnScreen|
// actual size in pixels on the screen}, the camera's
// @{blitwizard.graphics.camera:get2dZoomFactor|
// 2d zoom factor} (for consider_zoom set to true) and the camera's
// @{blitwizard.graphics.camera:get2dAspectRatio|
// 2d aspect ratio}.
//
// <i>Note on @{blitwizard.object:pinToCamera|pinned} 2d objects:</i>
// If you want to know the area visible in game units for
// @{blitwizard.object:pinToCamera|pinned objects} which are unaffected
// by camera zoom when drawn, specify <i>false</i> as first parameter
// for not taking the zoom into account.
// @function getDimensions
// @tparam boolean consider_zoom (optional) defaults to false.
// Specifies whether the 2d zoom factor should be taken into account.
// Set to <i>true</i> if yes, <i>false</i> if not.
// @treturn number width the width of the camera in game units
// @treturn number height the height of the camera in game units
int luafuncs_camera_getDimensions(lua_State* l) {
    struct luacameralistentry* e = toluacameralistentry(
    l, 1, 0, "blitwizard.graphics.camera:getDimensions");

    // if the camera default unit -> pixel conversion size isn't known yet:
    if (!unittopixelsset) {
        return haveluaerror(l, "this function is unavailable before the "
        "first blitwizard.graphics.setMode call");
    }

    int considerzoom = 0;
    // get consider_zoom parameter:
    if (lua_gettop(l) >= 2 && lua_type(l, 2) != LUA_TNIL) {
        if (lua_type(l, 2) != LUA_TBOOLEAN) {
            return haveluaerror(l, badargument1, 1,
            "blitwizard.graphics.camera:"
            "getDimensions", "boolean", lua_strtype(l, 2));
        }
        considerzoom = lua_toboolean(l, 2);
    }

    // return visible area dimensions:
    // FIXME: take aspect ratio into account once it can be changed
    double w = graphics_GetCameraWidth(e->cameraslot);
    double h = graphics_GetCameraHeight(e->cameraslot);
    double z = graphics_GetCamera2DZoom(e->cameraslot);
    if (!considerzoom) {
        z = 1;
    }
    lua_pushnumber(l, (w/UNIT_TO_PIXELS) * z);
    lua_pushnumber(l, (h/UNIT_TO_PIXELS) * z);
    return 2;
}

/// Get the dimensions of a camera on the actual
// screen. It usually fills the whole window (= the full size of the
// resolution you specified in @{blitwizard.graphics.setMode}),
// but that may be changed with
// @{blitwizard.graphics.camera:setPixelDimensionsOnScreen|
// camera:setPixelDimensionsOnScreen}.
// @function getPixelDimensionsOnScreen
// @treturn number width Width of the camera on the screen in pixels
// @treturn number height Height of the camera on the screen in pixels
int luafuncs_camera_getPixelDimensionsOnScreen(lua_State* l) {
    struct luacameralistentry* e = toluacameralistentry(
    l, 1, 0, "blitwizard.graphics.camera:getPixelDimensionsOnScreen");
    lua_pushnumber(l, graphics_GetCameraWidth(e->cameraslot));
    lua_pushnumber(l, graphics_GetCameraHeight(e->cameraslot));
    return 2;
}

/// Set the dimensions of a camera on the screen.
// @function setPixelDimensionsOnScreen
// @tparam number width Width of the camera on the screen in pixels
// @tparam number height Height of the camera on the screen in pixels
int luafuncs_camera_setPixelDimensionsOnScreen(lua_State* l) {
#ifdef USE_SDL_GRAPHICS
    return haveluaerror(l, "the SDL renderer doesn't support resizing cameras");
#else
    struct luacameralistentry* e = toluacameralistentry(
    l, 1, 0, "blitwizad.graphics.camera:setPixelDimensionsOnScreen");
    if (lua_type(l, 2) != LUA_TNUMBER) {
        return haveluaerror(l, badargument1, 1, "blitwizard.graphics.camera:"
        "setPixelDimensionsOnScreen", "number", lua_strtype(l, 2));
    }
    if (lua_type(l, 3) != LUA_TNUMBER) {
        return haveluaerror(l, badargument1, 2, "blitwizard.graphics.camera:"
        "setPixelDimensionsOnScreen", "number", lua_strtype(l, 3));
    }
    graphics_SetCameraSize(e->cameraslot, lua_tonumber(l, 2),
    lua_tonumber(l, 3));
    return 0;
#endif
}

/// Get the 2d zoom factor (defaults to 1). See also
// @{blitwizard.graphics.camera:set2dZoomFactor}.
// @function get2dZoomFactor
// @treturn number zoom_factor the zoom factor of the camera
int luafuncs_camera_get2dZoomFactor(lua_State* l) {
    struct luacameralistentry* e = toluacameralistentry(
    l, 1, 0, "blitwizard.graphics.camera:get2dZoomFactor");
    lua_pushnumber(l, graphics_GetCamera2DZoom(0));
    return 1;
}

/// Set the 2d zoom factor. Only the 2d layer will be affected.
//
// A large zoom factor will zoom into the details of the scene,
// a small zoom factor will show more surroundings from a larger distance.
// (e.g. 2 means twice as large, 0.5 means half as large)
// @function set2dZoomFactor
// @tparam number zoom_factor the new zoom factor
int luafuncs_camera_set2dZoomFactor(lua_State* l) {
    struct luacameralistentry* e = toluacameralistentry(
    l, 1, 0, "blitwizard.graphics.camera:set2dZoomFactor");
    if (lua_type(l, 2) != LUA_TNUMBER) {
        return haveluaerror(l, badargument1, 1,
        "blitwizard.graphics.camera:set2dZoomFactor",
        "number", lua_strtype(l, 2));
    }
    if (lua_tonumber(l, 2) <= 0) {
        return haveluaerror(l, badargument2, 1,
        "blitwizard.graphics.camera:set2dZoomFactor",
        "zoom factor is zero or negative");
    }
    graphics_SetCamera2DZoom(e->cameraslot, lua_tonumber(l, 2));
    return 0;
}

#endif  // USE_GRAPHICS

