
/* blitwizard 2d engine - source code file

  Copyright (C) 2011 Jonas Thiem

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

#ifndef BLITWIZARD_LUAHEADER_H_
#define BLITWIZARD_LUAHEADER_H_

#include "os.h"

#ifdef LUA_5_2_HEADER
#include "lua5.2/lua.h"
#include "lua5.2/lauxlib.h"
#include "lua5.2/lualib.h"
#else
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#endif

// We don't really care about the distinction of TNIL and TNONE,
// since unspecified parameters (=TNONE) should have the same
// effect as if specified as nil (=TNIL).
// Therefore, we override lua_type to map TNONE to TNIL.

__attribute__ ((unused))
static int lua_wrappedtype(lua_State* l, int index) {
    int i = lua_type(l, index);
    if (i == LUA_TNONE) {
        return LUA_TNIL;
    }
    return i;
}

#define lua_type lua_wrappedtype

#endif  // BLITWIZARD_LUAHEADER_H_


