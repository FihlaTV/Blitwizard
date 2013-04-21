
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

/// This is the standard "string" module of Lua. (<a href="http://www.lua.org/manual/5.2/manual.html#6.4">Check here for its documentation</a>)
//
// However, it contains a few additional functions
// provided by blitwizard, as documented here.
// May they be useful to your game building!
// @author Jonas Thiem  (jonas.thiem@gmail.com)
// @copyright 2011-2013
// @license zlib
// @module string

#include "os.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <float.h>
#include <math.h>

#include "luaheader.h"

#include "os.h"
#include "luaerror.h"
#include "luafuncs.h"
#ifdef USE_SDL_GRAPHICS
#include "SDL.h"
#endif
#include "graphicstexture.h"
#ifdef WINDOWS
#include <windows.h> // needed for HWND in graphics.h
#endif
#include "graphics.h"
#include "timefuncs.h"
#include "luastate.h"
#include "audio.h"
#include "audiomixer.h"
#include "file.h"
#include "filelist.h"
#include "main.h"
#include "win32console.h"
#include "osinfo.h"
#include "logging.h"
#include "resources.h"
#include "zipfile.h"

/// Split up a given string with a given delimiter,
// and return it as an array.
//
// You can specify a maximum
// amount of splits to happen, otherwise the string
// will be split up as often as the delimiter was found
// (and you'll get delimiter+1 string parts).
// @function split
// @tparam string string string to be split
// @tparam string delimiter delimiter which is searched in the string, and where the string is split
// @tparam number max_splits (number) Maximum number of splits to be done
int luafuncs_split(lua_State* l) {
    size_t len1,len2;
    const char* src1 = luaL_checklstring(l, 1, &len1);
    const char* src2 = luaL_checklstring(l, 2, &len2);
    int maxsplits = -1;
    if (lua_type(l, 3) != LUA_TNIL && lua_gettop(l) >= 3) {
        maxsplits = luaL_checkint(l, 3);
        if (maxsplits < 0) {maxsplits = 0;}
        if (maxsplits == 0) {
            lua_pushlstring(l, src1, len1);
            return 1;
        }
    }
    if (len2 == 0) {
        lua_pushlstring(l, src1, len1);
        return 1;
    }
    unsigned int returncount = 0;
    // split off as often as we can:
    while ((maxsplits > 0 || maxsplits < 0) && len1 >= len2) {
        unsigned int i = 0;
        int match = 0;
        while (i <= len1 - len2) {
            if (memcmp(src1 + i, src2, len2) == 0) {
                // found a delimeter match!
                lua_pushlstring(l, src1, i);
                src1 += i + len2;
                len1 -= i + len2;
                returncount++;
                match = 1;
                if (!lua_checkstack(l, 10)) {
                    lua_pop(l, returncount);
                    return haveluaerror(l,
                    "Exceeded Lua stack space - cannot grow stack further");
                }
                break;
            }
            i++;
        }
        if (match) {
            if (maxsplits > 0) {
                maxsplits--;
            }
        }else{
            break;
        }
    }
    // return remaining string:
    lua_pushlstring(l, src1, len1);
    return returncount + 1;
}

/// Check if a given string starts with another.
// (The other string being shorter or the same length)
//
// Returns true if the first specified string contains
// the second at the beginning, false if not.
// @function startswith
// @tparam string examined_string the string of which you want to examine the beginning
// @tparam string search_string the starting string you search inside the examined string
// @treturn boolean true if the search string matches the examined string's beginning, false if not
int luafuncs_startswith(lua_State* l) {
    size_t len1,len2;
    const char* src1 = lua_tolstring(l, 1, &len1);
    const char* src2 = lua_tolstring(l, 2, &len2);
    if (!src1 || !src2 || len2 > len1) {
        lua_pushboolean(l, 0);
        return 1;
    }
    if (memcmp(src1, src2, len2) == 0) {
        lua_pushboolean(l, 1);
        return 1;
    }
    lua_pushboolean(l, 0);
    return 1;
}

/// Check if a given string ends with another.
// (The other string being shorter or the same length)
// Also compare to @{startswith}
//
// Returns true if the first specified string contains
// the second at the very end, false if not.
// @function endswith
// @tparam string examined_string the string of which you want to examine the end
// @tparam string search_string the starting string you search inside the examined string
// @treturn boolean true if the search string is the examined string's end, false if not
int luafuncs_endswith(lua_State* l) {
    size_t len1,len2;
    const char* src1 = lua_tolstring(l, 1, &len1);
    const char* src2 = lua_tolstring(l, 2, &len2);
    if (!src1 || !src2 || len2 > len1) {
        lua_pushboolean(l, 0);
        return 1;
    }
    if (memcmp(src1+(len1-len2), src2, len2) == 0) {
        lua_pushboolean(l, 1);
        return 1;
    }
    lua_pushboolean(l, 0);
    return 1;
}


