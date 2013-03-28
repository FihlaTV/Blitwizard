
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

#ifndef BLITWIZARD_LUAERROR_H_
#define BLITWIZARD_LUAERROR_H_

extern char badargument1[]; // "Bad argument #%d to `%s` (%s expected, got %s)"
extern char badargument2[]; // "Bad argument #%d to `%s`: %s"
extern char stackgrowfailure[]; // "Cannot grow stack size"

int haveluaerror(lua_State* l, const char* fmt, ...);
void luatypetoname(int type, char* buf, size_t bufsize);
const char* lua_strtype(lua_State* l, int stack);

void callbackerror(lua_State* l, const char* function, const char* error, ...);

#define error_nophysics2d "functionality not available - blitwizard was compiled without 2d physics support"
#define error_nophysics3d "functionality not available - blitwizard was compiled without 3d physics support"
#define error_nographics "functionality not available - blitwizard was compiled without graphics support"

#endif  // BLITWIZARD_LUAERROR_H_

