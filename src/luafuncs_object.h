
/* blitwizard game engine - source code file

  Copyright (C) 2012-2013 Jonas Thiem

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

#ifndef BLITWIZARD_OBJECT_H_
#define BLITWIZARD_OBJECT_H_

int luafuncs_object_new(lua_State* l);
void luafuncs_pushbobjidref(lua_State* l, struct blitwizardobject* o);
struct blitwizardobject* toblitwizardobject(lua_State* l, int index, int arg, const char* func);

int luafuncs_object_getPosition(lua_State* l);
int luafuncs_object_setPosition(lua_State* l);
int luafuncs_object_setZIndex(lua_State* l);
int luafuncs_object_destroy(lua_State* l);

// set the function on top of the stack as event function:
// (put nil on top of the stack if you want to clear the event)
void luacfuncs_object_setEvent(lua_State* l,
struct blitwizardobject* o, const char* eventName);

// Call the event function for a given event.
// 'self' will be set to the given object.
//
// If you want to pass arguments to the event functions,
// push them onto the stack (last argument on top)
// and specify their number with the args parameter.
//
// Arguments will be popped from the stack.
int luacfuncs_object_callEvent(lua_State* l,
struct blitwizardobject* o, const char* eventName,
int args);
// Returns 0 if there was a lua error in the
// event function, otherwise 1.

// clear the registry table of the object:
// (used internally for cleanup)
void luacfuncs_object_clearRegistryTable(lua_State* l,
struct blitwizardobject* o);


#endif  // BLITWIZARD_OBJECT_H_

