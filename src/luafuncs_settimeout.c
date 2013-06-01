
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

///
// @author Jonas Thiem  (jonas.thiem@gmail.com)
// @copyright 2011-2013
// @license zlib
// @module blitwizard

#include "os.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <float.h>
#include <math.h>

#include "luaheader.h"

#include "luastate.h"
#include "luaerror.h"
#include "luafuncs_settimeout.h"
#include "timefuncs.h"

int nexttimeoutfuncid = 0;
struct timeoutfunc {
    int cancelled;
    int id;
    uint64_t triggerTime;
    struct timeoutfunc* next;
};
struct timeoutfunc* timeoutfuncs = NULL;
uint64_t timeoutTS = 0;
uint64_t currentRelativeTS = 0;

void luacfuncs_settimeout_Do() {
    lua_State* l = luastate_GetStatePtr();
    if (timeoutTS == 0) {
        timeoutTS = time_GetMilliseconds();
        return;
    }
    uint64_t oldTime = timeoutTS;
    timeoutTS = time_GetMilliseconds();
    if (oldTime < timeoutTS) {
        // time has passed, check timeouts
        struct timeoutfunc* tf = timeoutfuncs;
        struct timeoutfunc* tfprev = NULL;
        while (tf) {
            struct timeoutfunc* tfnext = tf->next;
            if (tf->triggerTime <= timeoutTS && !tf->cancelled) {
                currentRelativeTS = tf->triggerTime;
                // obtain callback:
                char funcname[64];
                snprintf(funcname, sizeof(funcname),
                "timeoutfunc%d", tf->id);
                lua_pushstring(l, funcname);
                lua_gettable(l, LUA_REGISTRYINDEX);

                // run callback:


                // remove callback function:
                lua_pushstring(l, funcname);
                lua_pushnil(l);
                lua_settable(l, LUA_REGISTRYINDEX);

                // remove trigger:
                if (tfprev) {
                    tfprev->next = tf->next;
                } else {
                    timeoutfuncs->next = tf->next;
                }
                free(tf);

                // unset current relative ts:
                currentRelativeTS = 0;
            } else {
                tfprev = tf;
            }
            tf = tfnext;
        }
    }
}

/// Cancel a function which was scheduled to be run with
// @{blitwizard.setTimeout} using the handle you received
// for it.
//
// If the function has already been executed, this function
// will return false, otherwise it will return true and
// the function won't be executed.
int luafuncs_cancelTimeout(lua_State* l) {
    if (lua_type(l, 1) != LUA_TUSERDATA) {
        return haveluaerror(l, badargument1, 1, "blitiwzard.cancelTimeout",
        "setTimeout handle", lua_strtype(l, 1));
    }
    if (lua_rawlen(l, 2) != sizeof(struct luaidref)) {
        return haveluaerror(l, badargument2, 2, "blitwizard.cancelTimeout",
        "not a valid setTimeout handle");
    }
    struct luaidref* idref = lua_touserdata(l, index);
    if (!idref || idref->magic != IDREF_MAGIC
    || idref->type != IDREF_TIMEOUTHANDLE) {
        return haveluaerror(l, badargument2, 2, "blitwizard.canccelTimeout",
        "not a valid setTimeout handle");
    }
    int id = idref->ref.id;
    struct timeoutfunc* tf = timeoutfuncs;
    while (tf) {
        if (tf->id == id) {
            tf->cancelled = 1;
            lua_pushboolean(l, 1);
            return 1;
        }
        tf = tf->next;
    }
    lua_pushboolean(l, 0);
    return 1;
}

/// This function allows scheduling a function to be executed
// later. blitwizard.setTimeout will return instantly, and lateron
// when the specified amount of time has passed, the function will
// be run once.
//
// If you know JavaScript, this function should be familiar
// to you.
//
// The return value is a handle which you can use to cancel the
// scheduled function run with @{blitwizard.cancelTimeout},
// as long as it hasn't been run yet.
// @function setTimeout
// @tparam function function the function which shall be executed later
// @tparam number delay the delay in milliseconds before executing the function
int luafuncs_setTimeout(lua_State* l) {
    // check function parameter:
    if (lua_type(l, 1) != LUA_TFUNCTION) {
        return haveluaerror(l, badargument1, 1, "luafuncs.setTimeout",
        "function", lua_strtype(l, 1));
    }

    // check delay parameter:
    if (lua_type(l, 2) != LUA_TNUMBER) {
        return haveluaerror(l, badargument1, 2, "luafuncs.setTimeout",
        "number", lua_strtype(l, 2));
    }
    int d = lua_tointeger(l, 2);
    if (d < 0) {
        return haveluaerror(l, badargument2, 2, "luafuncs.setTimeout",
        "timeout needs to be positive");
    }

    // see which id we would want to use:
    int useid = nexttimeoutfuncid;
    if (nexttimeoutfuncid >= INT_MAX) {
        nexttimeoutfuncid = 0;
    } else {
        nexttimeoutfuncid++;
    }

    // set function provided to registry:
    char funcname[256];
    snprintf(funcname, sizeof(funcname),
    "timeoutfunc%d", useid);
    if (lua_gettop(l) > 2) {  // remove all unrequired stuff
        lua_pop(l, lua_gettop(l)-2);
    }
    lua_insert(l, -2);  // move delay back to front
    lua_pushstring(l, funcname);
    lua_insert(l, -2);  // move func name in front of value
    lua_settable(l, LUA_REGISTRYINDEX);

    // allocate timeout info struct:
    struct timeoutfunc* tf = malloc(sizeof(*tf));
    if (!tf) {
        // remove function in lua registry:
        lua_pushstring(l, funcname);
        lua_pushnil(l);
        lua_insert(l, LUA_REGISTRYINDEX);
        return haveluaerror(l, "setTimeout struct allocation failed");
    }
    memset(tf, 0, sizeof(*tf));

    // set trigger info:
    tf->id = nexttimeoutfuncid;
    tf->triggerTime = currentRelativeTS;
    if (tf->triggerTime == 0) {
        tf->triggerTime = timeoutTS;
    }
    tf->triggerTime += d;

    tf->next = timeoutfuncs;
    timeoutfuncs = tf;
    return 0;
}

