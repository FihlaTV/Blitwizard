
/* blitwizard game engine - source code file

  Copyright (C) 2013-2014 Jonas Thiem

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

#ifndef BLITWIZARD_LUAFUNCS_SETTIMEOUT_H_
#define BLITWIZARD_LUAFUNCS_SETTIMEOUT_H_

#include "luaheader.h"

// tick function to be called each frame which operates delaeyd runs:
void luacfuncs_runDelayed_Do(void);

// lua function to schedule a delayed run:
int luafuncs_runDelayed(lua_State* l);

// get a count of currently scheduled functions to be run later:
size_t luacfuncs_runDelayed_getScheduledCount(void);

#endif  // BLITWIZARD_LUAFUNCS_SETTIMEOUT_H_

