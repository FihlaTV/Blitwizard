
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

#ifndef BLITWIZARD_LUAFUNCS_H_
#define BLITWIZARD_LUAFUNCS_H_

#include "luaheader.h"

// blitwizard.*:
int luafuncs_setstep(lua_State* l);
int luafuncs_getTemplateDirectory(lua_State* l);
int luafuncs_loadResourceArchive(lua_State* l);

// Base:
int luafuncs_loadfile(lua_State* l);
int luafuncs_dofile(lua_State* l);
int luafuncs_print(lua_State* l);
int luafuncs_dostring(lua_State* l);

// Time:
int luafuncs_getTime(lua_State* l);
int luafuncs_sleep(lua_State* l);

// Graphics:
int luafuncs_getRendererName(lua_State* l);
int luafuncs_setWindow(lua_State* l);
int luafuncs_getWindowSize(lua_State* l);
int luafuncs_getDisplayModes(lua_State* l);
int luafuncs_getDesktopDisplayMode(lua_State* l);

// Sound:
int luafuncs_getBackendName(lua_State* l);
int luafuncs_play(lua_State* l);
int luafuncs_playing(lua_State* l);
int luafuncs_stop(lua_State* l);
int luafuncs_adjust(lua_State* l);

// Math:
int luafuncs_trandom(lua_State* l);

// Internal error handling:
void luacfuncs_onError(const char* funcname, const char* error);

// Misc:
size_t lua_tosize_t(lua_State* l, int index);

#endif  // BLITWIZARD_LUAFUNCS_H_

