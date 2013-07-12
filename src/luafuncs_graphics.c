
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

/// The blitwizard graphics namespace allows to set up the graphics device
// for graphical 2d/3d output.
// @author Jonas Thiem (jonas.thiem@gmail.com) et al
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


/// This function sets the graphics mode.
// You can specify a resolution, whether your game should run in a window
// or fullscreen, and if windowed whether the window is resizable.
//
// If you run in full screen, you should only use screen resolutions
// (width/height) present in @{blitwizard.graphics.getDisplayModes}.
//
// Feel free to use this function multiple times to change the mode again
// if you feel like doing so.
//
// If you specify no parameters, the graphics output will be closed again.
//
// Throws an error if graphics initialisation fails.
// @function setMode
// @tparam number width Set the width of the output in pixels
// @tparam height height Set the height of the output in pixels
// @tparam string title (optional) Set the window title shown in the title bar. You can also specify nil if you want the default
// @tparam boolean fullscreen (optional) Specify if you want to run fullscreen or windowed
// @tparam string renderer (optional) Specify a specific renderer you want. Available renderers are "direct3d" (Windows only), "opengl" and "software". Software rendering is slow and should only be used as a fallback. It's also only available if you compiled with SDL graphics. If you don't specify a renderer, the best one will be used
// @usage
// -- Open a window with 800 x 600 pixel size and title "Test"
// blitwizard.graphics.setMode(800, 600, "Test")
int luafuncs_setMode(lua_State* l) {
#ifdef USE_GRAPHICS
    if (lua_gettop(l) <= 0) {
        // close window if no parameters
        graphics_Quit();
        return 0;
    }

    // get new resolution:
    int x = lua_tonumber(l, 1);
    if (x <= 0) {
        return haveluaerror(l, badargument2, 1,
        "argument #1 is not a valid resolution width");
    }
    int y = lua_tonumber(l, 2);
    if (y <= 0) {
        return haveluaerror(l, badargument2, 1,
        "argument #2 is not a valid resolution height");
    }

    // get window title:
    char defaulttitle[] = "blitwizard";
    const char* title = lua_tostring(l, 3);
    if (!title) {
        title = defaulttitle;
    }

    // obtain fullscreen option:
    int fullscreen = 0;
    if (lua_type(l, 4) == LUA_TBOOLEAN) {
        fullscreen = lua_toboolean(l, 4);
    } else {
        if (lua_gettop(l) >= 4) {
            lua_pushstring(l, "Fourth argument is not a valid fullscreen boolean");
            return lua_error(l);
        }
    }

    // get renderer name if any:
    const char* renderer = lua_tostring(l, 5);

    // apply mode:
    char* error;
    if (!graphics_SetMode(x, y, fullscreen, 0, title, renderer, &error)) {
        if (error) {
            lua_pushstring(l, error);
            free(error);
            return lua_error(l);
        }
        lua_pushstring(l, "Unknown error on setting mode");
        return lua_error(l);
    }
    return 0;
#else // ifdef USE_GRAPHICS
    lua_pushstring(l, compiled_without_graphics);
    return lua_error(l);
#endif
}

/// Get the extent (in pixels) a @{blitwizard.object:setPosition|game unit}
// in the 2d world has on the screen,
// at a default camera zoom level of 1.
//
// (for cameras with other zoom levels, multiply with their
// @{blitwizard.graphics.camera:get2dZoomFactor|zoom factor}
// to know how large a game unit is on that camera - unless
// for objects which you @{blitwizard.object:pinToCamera|pinned
// to the screen} which are always drawn with zoom factor 1)
//
// This function is <b>useless for 3d objects</b>. For them,
// a game unit should be roughly treated as one meter and there
// is no generic way to tell how this ends up on the screen due
// to the very dynamic way objects look depending on the
// camera position, camera angle etc.
//
// @function gameUnitToPixels
// @treturn number pixels The amount of pixels that equals one game unit at default zoom of 1
// @usage
// -- Get the amount of pixels for one game unit:
// -- (this will be inaccurate if your camera is zoomed)
// local pixels = blitwizard.graphics.gameUnitToPixels()
//
// -- Get the amount of pixels for one game unit, taking the
// -- actual zoom level of the default camera into account:
// local pixels = blitwizard.graphics.gameUnitToPixels() * blitwizard.graphics.getCameras()[1]:get2dZoomFactor
int luafuncs_gameUnitToPixels(lua_State* l) {
    if (!unittopixelsset) {
        return haveluaerror(l, "this function is unavailable before the first "
        "blitwizard.graphics.setMode call");
    }
    lua_pushnumber(l, UNIT_TO_PIXELS);
    return 1;
}

/// Get the current size of the graphics output window
// (which was set using @{blitwizard.graphics.setMode}) in pixels.
// @function getWindowSize
// @treturn number width
// @treturn number height
int luafuncs_getWindowSize(lua_State* l) {
#ifdef USE_GRAPHICS
    unsigned int w,h;
    if (!graphics_GetWindowDimensions(&w,&h)) {
        lua_pushstring(l, "Failed to get window size");
        return lua_error(l);
    }
    lua_pushnumber(l, w);
    lua_pushnumber(l, h);
    return 2;
#else // ifdef USE_GRAPHICS
    lua_pushstring(l, compiled_without_graphics);
    return lua_error(l);
#endif
}

/// Get all supported display modes as a list.
//
// Those are the width/height values you can possibly
// use for @{blitwizard.graphics.setMode} for fullscreen.
//
// Returns a list of tables, each one containing
// two entries for width and height.
//
// @function getDisplayModes
// @treturn table list of resolution tables
// @usage
//   -- print out all display modes:
//   for i,v in ipairs(blitwizard.graphics.getDisplayModes())
//       print("Display mode: width: " .. v[1] .. ", height: " .. v[2])
//   end
int luafuncs_getDisplayModes(lua_State* l) {
#ifdef USE_GRAPHICS
    int c = graphics_GetNumberOfVideoModes();
    lua_createtable(l, 1, 0);

    // first, add desktop mode
    int desktopw,desktoph;
    graphics_GetDesktopVideoMode(&desktopw, &desktoph);

    // resolution table with desktop width, height
    lua_createtable(l, 2, 0);
    lua_pushnumber(l, 1);
    lua_pushnumber(l, desktopw);
    lua_settable(l, -3);
    lua_pushnumber(l, 2);
    lua_pushnumber(l, desktoph);
    lua_settable(l, -3);

    // add table into our list
    lua_pushnumber(l, 1);
    lua_insert(l, -2);
    lua_settable(l, -3);

    int i = 1;
    int index = 2;
    while (i <= c) {
        // add all supported video modes...
        int w,h;
        graphics_GetVideoMode(i, &w, &h);

        // ...but not the desktop mode twice
        if (w == desktopw && h == desktoph) {
            i++;
            continue;
        }

        // table containing the resolution width, height
        lua_createtable(l, 2, 0);
        lua_pushnumber(l, 1);
        lua_pushnumber(l, w);
        lua_settable(l, -3);
        lua_pushnumber(l, 2);
        lua_pushnumber(l, h);
        lua_settable(l, -3);

        // add the table into our list
        lua_pushnumber(l, index);
        lua_insert(l, -2);
        lua_settable(l, -3);
        index++;
        i++;
    }
    return 1;
#else // ifdef USE_GRAPHICS
    lua_pushstring(l, compiled_without_graphics);
    return lua_error(l);
#endif
}

/// Get the current display mode of the desktop.
// You might want to use this with @{blitwizard.graphics.setMode}
// if you don't want to change the resolution in fullscreen
// at all (but instead use whatever the current resolution is).
// @function getDesktopDisplayMode
// @treturn number width in pixels
// @treturn number height in pixels
int luafuncs_getDesktopDisplayMode(lua_State* l) {
#ifdef USE_GRAPHICS
    int w,h;
    graphics_GetDesktopVideoMode(&w, &h);
    lua_pushnumber(l, w);
    lua_pushnumber(l, h);
    return 2;
#else // ifdef USE_GRAPHICS
    lua_pushstring(l, compiled_without_graphics);
    return lua_error(l);
#endif
}


#endif  // USE_GRAPHICS

