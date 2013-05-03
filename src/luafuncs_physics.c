
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

/// The blitwizard physics namespace contains various physics functions
// for collision tests and the like. You would handle most physics
// functionality by using functions of the
// @{blitwizard.object|blitwizard object} though.
// @author Jonas Thiem (jonas.thiem@gmail.com) et al
// @copyright 2011-2013
// @license zlib
// @module blitwizard.physics

#if (defined(USE_PHYSICS2D) || defined(USE_PHYSICS3D))

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include "luaheader.h"

#include "logging.h"
#include "luaerror.h"
#include "luastate.h"
#include "blitwizardobject.h"
#include "physics.h"
#include "objectphysicsdata.h"
#include "luafuncs_object.h"
#include "luafuncs_physics.h"
#include "main.h"


int luafuncs_ray(lua_State* l, int use3d) {
    char func[64];
    if (use3d) {
        strcpy(func, "blitwizard.physics.ray3d");
    } else {
        strcpy(func, "blitwizard.physics.ray2d");
    }
    if (lua_type(l, 1) != LUA_TNUMBER) {
        return haveluaerror(l, badargument1, 1, func, "number",
        lua_strtype(l, 1));
    }
    if (lua_type(l, 2) != LUA_TNUMBER) {
        lua_pushstring(l, "Second parameter is not a valid start y position");
        return lua_error(l);
    }
    if (use3d) {
        if (lua_type(l, 3) != LUA_TNUMBER) {
            lua_pushstring(l, "Fourth parameter is not a valid start z position");
            return lua_error(l);
        }
    }
    if (lua_type(l, 3 + use3d) != LUA_TNUMBER) {
        lua_pushstring(l, "Third parameter is not a valid target x position");
        return lua_error(l);
    }
    if (lua_type(l, 4 + use3d) != LUA_TNUMBER) {
        lua_pushstring(l, "Fourth parameter is not a valid target y position");
        return lua_error(l);
    }
    if (use3d) {
        if (lua_type(l, 6) != LUA_TNUMBER) {
            lua_pushstring(l, "Fourth parameter is not a valid target z position");
            return lua_error(l);
        }
    }

    double startx = lua_tonumber(l, 1);
    double starty = lua_tonumber(l, 2);
    double startz;
    if (use3d) {
        startz = lua_tonumber(l, 3);
    }
    double targetx = lua_tonumber(l, 3+use3d);
    double targety = lua_tonumber(l, 4+use3d);
    double targetz;
    if (use3d) {
        targetz = lua_tonumber(l, 6);
    }

    struct physicsobject* obj;
    double hitpointx,hitpointy,hitpointz;
    double normalx,normaly,normalz;

    int returnvalue;
    if (use3d) {
#ifdef USE_PHYSICS3D
        returnvalue = physics_ray3d(main_DefaultPhysics2dPtr(),
        startx, starty, startz,
        targetx, targety, targetz,
        &hitpointx, &hitpointy, &hitpointz,
        &obj,
        &normalx, &normaly, &normalz);
#endif
    } else {
        returnvalue = physics_ray2d(main_DefaultPhysics2dPtr(),
        startx, starty,
        targetx, targety,
        &hitpointx, &hitpointy,
        &obj,
        &normalx, &normaly);
    }

    if (returnvalue) {
        // create a new reference to the (existing) object the ray has hit:
        luacfuncs_pushbobjidref(l, (struct blitwizardobject*)
        physics_getObjectUserdata(obj));

        // push the other information we also want to return:
        lua_pushnumber(l, hitpointx);
        lua_pushnumber(l, hitpointy);
        if (use3d) {
            lua_pushnumber(l, hitpointz);
        }
        lua_pushnumber(l, normalx);
        lua_pushnumber(l, normaly);
        if (use3d) {
            lua_pushnumber(l, normalz);
        }
        return 5+2*use3d;  // return it all
    }
    lua_pushnil(l);
    return 1;
}


/// Do a ray collision test by shooting out a ray and checking where it hits
// in 2d realm.
// @function ray2d
// @tparam number startx Ray starting point, x coordinate (please note the starting point shouldn't be inside the collision shape of an object)
// @tparam number starty Ray starting point, y coordinate
// @tparam number targetx Ray target point, x coordinate
// @tparam number targety Ray target point, y coordinate
// @treturn userdata Returns a @{blitwizard.object|blitwizard object} if an object was hit by the ray (the closest object that was hit), or otherwise it returns nil
int luafuncs_ray2d(lua_State* l) {
    return luafuncs_ray(l, 0);
}

/// Do a ray collision test by shooting out a ray and checking where it hits
// in 3d realm.
// @function ray3d
// @tparam number startx Ray starting point, x coordinate (please note the starting point shouldn't be inside the collision shape of an object)
// @tparam number starty Ray starting point, y coordinate
// @tparam number startz Ray starting point, z coordinate
// @tparam number targetx Ray target point, x coordinate
// @tparam number targety Ray target point, y coordinate
// @tparam number targetz Ray target point, z coordinate
// @treturn userdata Returns a @{blitwizard.object|blitwizard object} if an object was hit by the ray (the closest object that was hit), or otherwise it returns nil
int luafuncs_ray3d(lua_State* l) {
    return luafuncs_ray(l, 1);
}

/// Set the world gravity for all 2d objects.
// @function set2dGravity
// @tparam number x gravity force on x axis
// @tparam number y gravity force on y axis
int luafuncs_set2dGravity(lua_State* l) {
#ifdef USE_PHYSICS2D
    if (lua_type(l, 1) != LUA_TNUMBER) {
        return haveluaerror(l, badargument1, 1,
        "blitwizard.physics.set2dGravity", "number", lua_strtype(l, 1));
    }
    if (lua_type(l, 2) != LUA_TNUMBER) {
        return haveluaerror(l, badargument1, 2,
        "blitwizard.physics.set2dGravity", "number", lua_strtype(l, 2));
    }
    physics_set2dWorldGravity(main_DefaultPhysics2dPtr(),
    lua_tonumber(l, 1), lua_tonumber(l, 2));
    return 0;
#endif
}

/// Set the world gravity for all 3d objects.
// @function set3dGravity
int luafuncs_set3dGravity(lua_State* l) {
    return 0;
}

#endif  // USE_PHYSICS2D || USE_PHYSICS3D

