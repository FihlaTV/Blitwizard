
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

/// The vector name space provided by blitwizard offers some
// maths functions for vector calculation.
//
// If you want to rotate a vector around another or similar fancy things,
// look no further.
// @author Jonas Thiem (jonas.thiem@gmail.com) et al
// @copyright 2011-2013
// @license zlib
// @module vector

#include "config.h"
#include "os.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "luaheader.h"

#include "mathhelpers.h"
#include "logging.h"
#include "luaerror.h"
#include "luastate.h"
#include "luafuncs_vector.h"


/// Rotate a 2d vector around (0, 0) by a specified angle in degrees.
// @function rotate2d
// @tparam number pos_x the x coordinate of the vector to be rotated
// @tparam number pos_y the y coordinate of the vector to be rotated
// @tparam number angle the angle in degrees by which the vector should be rotated
// @treturn number the X coordinate of the rotated vector
// @treturn number the Y coordinate of the rotated vector
// @usage
//  -- get the vector 1,0 turned around one quarter counter-clockwise
//  -- (by 90 degree):
//  new_x, new_y = blitwizard.vector.rotate2d(1.0, 0.0, 90)
int luafuncs_vector_rotate2d(lua_State* l) {
    const char func[] = "vector.rotate2d";
    if (lua_type(l, 1) != LUA_TNUMBER) {
        return haveluaerror(l, badargument1, 1, func, "number",
        lua_strtype(l, 1));
    }
    if (lua_type(l, 2) != LUA_TNUMBER) {
        return haveluaerror(l, badargument1, 2, func, "number",
        lua_strtype(l, 2));
    }
    if (lua_type(l, 3) != LUA_TNUMBER) {
        return haveluaerror(l, badargument1, 3, func, "number",
        lua_strtype(l, 3));
    }
    double x = lua_tonumber(l, 1);
    double y = lua_tonumber(l, 2);
    if (x == 0 && y == 0) {
        lua_pushnumber(l, 0);
        lua_pushnumber(l, 0);
        return 2;
    }
    double angle = lua_tonumber(l, 3);
    rotatevec(x, y, angle, &x, &y);
    lua_pushnumber(l, x);
    lua_pushnumber(l, y);
    return 2;
}


