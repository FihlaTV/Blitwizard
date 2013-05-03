
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

#endif  // USE_GRAPHICS

