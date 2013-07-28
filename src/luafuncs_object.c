
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

/// Blitwizard namespace containing the generic
// @{blitwizard.object|blitwizard game entity object} and various sub
// namespaces for @{blitwizard.physics|physics},
// @{blitwizard.graphics|graphics} and more.
//
// <b>Please note 3d graphics are currently <span style="color:#ff0000">not supported.</span></b>
// @author Jonas Thiem (jonas.thiem@gmail.com) et al
// @copyright 2011-2013
// @license zlib
// @module blitwizard

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdint.h>

#include "os.h"
#ifdef USE_SDL_GRAPHICS
#include "SDL.h"
#endif
#include "file.h"
#include "graphicstexture.h"
#include "graphics.h"
#include "luaheader.h"
#include "timefuncs.h"
#include "luastate.h"
#include "luaerror.h"
#include "luafuncs.h"
#include "blitwizardobject.h"
#include "luafuncs_object.h"
#include "luafuncs_objectgraphics.h"
#include "luafuncs_objectphysics.h"
#include "graphics2dsprites.h"
#include "luafuncs_graphics_camera.h"

// some statistics:
int processedImportantObjects = 0;
int processedNormalObjects = 0;
int processedBoringObjects = 0;


// list of all objects (non-deleted):
struct blitwizardobject* objects = NULL;
// deleted objects:
struct blitwizardobject* deletedobjects = NULL;

// list of all "important" objects:
struct blitwizardobject* importantObjects = NULL;
// (=doAlways or other per-logic-frame stuff)

// list of objects to add or remove from importantObjects list:
struct addRemoveObject {
    int remove;
    struct blitwizardobject* obj;
    struct addRemoveObject* next;
};
struct addRemoveObject* addRemoveImportantObjects = NULL;

// an iterator change will cause all iterators to be come invalid.
static uint64_t iteratorChangeId = 0;

struct objectiteratordata {
    struct blitwizardobject* currentObj;
    uint64_t iteratorChangeId;
};

static int luacfuncs_getObjectsIterator(lua_State* l) {
    // check if iterator info is empty:
    if (lua_type(l, lua_upvalueindex(1)) == LUA_TNIL) {
        // we reached the end of the object list.
        // return nil:
        lua_pushnil(l);
        return 1;
    }
    // get iterator info:
    struct objectiteratordata* oid = lua_touserdata(l,
        lua_upvalueindex(1));
    // if object list changed, stop iterator:
    if (iteratorChangeId != oid->iteratorChangeId) {
        return haveluaerror(l, "object was created or destroyed, "
        "iterator is outdated");
    }
    // iterate further:
    oid->currentObj = oid->currentObj->next;
    // skip all deleted objects:
    while (oid->currentObj && oid->currentObj->deleted) {
        oid->currentObj = oid->currentObj->next;
    }
    struct blitwizardobject* o = oid->currentObj;
    // pop iterator data:
    lua_pop(l, 1);
    if (o) {
        // push & return iterated object:
        luacfuncs_pushbobjidref(l, o);
        return 1;
    } else {
        // end of list reached. clear object data:
        lua_pushnil(l);
        lua_replace(l, lua_upvalueindex(1));
        // return nil:
        lua_pushnil(l);
        return 1;
    }
}

/// Iterate through all existing objects, no matter if 2d or 3d.
// @function getAllObjects
// @treturn function returns an iterator function
// @usage
// for obj in blitwizard.getAllObjects() do
//     local x, y = obj:getPosition()
//     print("object at: " .. x .. ", " .. y)
// end
int luafuncs_getAllObjects(lua_State* l) {
    // prepare iterator data:
    struct objectiteratordata* oid = lua_newuserdata(l, sizeof(*oid));
    memset(oid, 0, sizeof(*oid));
    oid->iteratorChangeId = iteratorChangeId;
    oid->currentObj = objects;

    // create closure:
    lua_pushcclosure(l, luacfuncs_getObjectsIterator, 1);

    // return iterator:
    return 1;
}

// objectAddedDeleted function which gets informed of all created
// and deleted objects.
// This is mainly for invalidating all iterators (see luafuncs_getAllObjects)
// because they would otherwise run through a now inconsistent object list.
void luacfuncs_objectAddedDeleted(__attribute__ ((unused)) struct blitwizardobject* o, int added) {
    if (!added) {
        iteratorChangeId++;
        // wrap over:
        if (iteratorChangeId >= UINT64_MAX-2) {
            iteratorChangeId = 0;
        }
    }
}

static void luacfuncs_object_setImportant(struct blitwizardobject* o,
int important) {
    if ((important != 0) == o->markedImportant) {
        // already marked as intended.
        return;
    }
    o->markedImportant = (important != 0);

    struct addRemoveObject* a = malloc(sizeof(*a));
    if (!a) {
        // nothing we can do
        return;
    }
    memset(a, 0, sizeof(*a));
    a->obj = o;
    a->remove = (important == 0);
    a->next = addRemoveImportantObjects;
    addRemoveImportantObjects = a;
}

/// Blitwizard object which represents an 'entity' in the game world
// with visual representation, behaviour code and collision shape.
// @type object

void cleanupobject(struct blitwizardobject* o, int fullclean) {
    // clear up all the graphics, physics, event function things
    // attached to the object:
    if (fullclean) {
#ifdef USE_GRAPHICS
        luafuncs_objectgraphics_unload(o);
#endif
#if (defined(USE_PHYSICS2D) || defined(USE_PHYSICS3D))
        if (o->physics) {
            luafuncs_freeObjectPhysicsData(o->physics);
            o->physics = NULL;
        }
#endif
        // remove from important object list:
        if (o->importantPrev || o->importantNext || importantObjects == o) {
            if (o->importantPrev) {
                o->importantPrev->importantNext = o->importantNext;
            } else {
                importantObjects = o->importantNext;
            }
            if (o->importantNext) {
                o->importantNext->importantPrev = o->importantPrev;
            }
        }

        // clear table in registry:
        luacfuncs_object_clearRegistryTable(luastate_GetStatePtr(), o);
    } else {
#ifdef USE_GRAPHICS
        luacfuncs_objectgraphics_setVisible(o, 0);
#endif
    }
}

static int luacfuncs_object_deleteIfOk(struct blitwizardobject* o);

static int garbagecollect_blitwizobjref(lua_State* l) {
    // we need to decrease our reference count of the
    // blitwizard object we referenced to.

    // get id reference to object
    struct luaidref* idref = lua_touserdata(l, -1);

    if (!idref || idref->magic != IDREF_MAGIC
    || idref->type != IDREF_BLITWIZARDOBJECT) {
        // either wrong magic (-> not a luaidref) or not a blitwizard object
        lua_pushstring(l, "internal error: invalid blitwizard object ref");
        lua_error(l);
        return 0;
    }

    // it is a valid blitwizard object, decrease ref count:
    struct blitwizardobject* o = idref->ref.bobj;
    o->refcount--;
    return 0;
}

static int luacfuncs_object_deleteIfOk(struct blitwizardobject* o) {
    if (o->deleted && o->refcount <= 0) {
        cleanupobject(o, 1);

        // remove object from the list
        if (o->prev) {
            // adjust prev object to have new next pointer
            o->prev->next = o->next;
        } else {
            // was first object in deleted list -> adjust list start pointer
            deletedobjects = o->next;
        }
        if (o->next) {
            // adjust next object to have new prev pointer
            o->next->prev = o->prev;
        }

        // free object
        free(o);
        return 1;
    }
    return 0;
}

void luacfuncs_object_obtainRegistryTable(lua_State* l,
struct blitwizardobject* o);

static void luafuncs_checkObjFunc(lua_State* l, const char* name,
int* result) {
    lua_pushstring(l, name);
    lua_gettable(l, -2);
    if (lua_type(l, -1) == LUA_TFUNCTION) {
        lua_pop(l, 1);
        *result = 1;
        return;
    }
    lua_pop(l, 1);
    *result = 0;
}

static void luacfuncs_updateObjectHandlers(lua_State* l,
struct blitwizardobject* o) {
    luacfuncs_object_obtainRegistryTable(l, o);
    luafuncs_checkObjFunc(l, "onMouseEnter", &o->haveOnMouseEnter);
    luafuncs_checkObjFunc(l, "onMouseLeave", &o->haveOnMouseLeave);
    luafuncs_checkObjFunc(l, "onMouseClick", &o->haveOnMouseClick);
    luafuncs_checkObjFunc(l, "doAlways", &o->haveDoAlways);
    luafuncs_checkObjFunc(l, "doOften", &o->haveDoOften);
    luafuncs_checkObjFunc(l, "onCollision", &o->haveOnCollision);

    // enable mouse event handling:
    if (!o->invisibleToMouse) {
#ifdef USE_GRAPHICS
        if (o->haveOnMouseEnter || o->haveOnMouseLeave) {
            luacfuncs_objectgraphics_enableMouseMoveEvent(o, 1);
        } else {
            luacfuncs_objectgraphics_enableMouseMoveEvent(o, 0);
        }
        if (o->haveOnMouseClick) {
            luacfuncs_objectgraphics_enableMouseClickEvent(o, 1);
        } else {
            luacfuncs_objectgraphics_enableMouseClickEvent(o, 0);
        }
#endif
    }

    // set as important object if doAlways is set:
    if (o->haveDoAlways) {
        // important (= needs to be processed each logic frame)
        luacfuncs_object_setImportant(o, 1);
    } else {
        // unimportant (= processing 4 times a second is sufficient)
        luacfuncs_object_setImportant(o, 0);
    }

    lua_pop(l, 1);  // pop registry table
}

// __newindex handler for blitwizard object refs:
static int luafuncs_bobjnewindex(lua_State* l) {
    int checkForEventHandlers = 0;
    // first, get blitwizard object:
    if (lua_type(l, 1) != LUA_TUSERDATA) {
        return haveluaerror(l, "__newindex handler wasn't passed a "
        "blitwizard object");
    }
    struct blitwizardobject* o = toblitwizardobject(l, 1, 0,
        "blitwizard.object:__newindex"); 
    // see if this handler affects anything that is of interest to us:
    if (lua_type(l, 2) == LUA_TSTRING) {
        const char* p = lua_tostring(l, 2);
        if (strcmp(p, "onMouseClick") == 0 ||
        strcmp(p, "onMouseEnter") == 0 ||
        strcmp(p, "onMouseLeave") == 0 ||
        strcmp(p, "doAlways") == 0 ||
        strcmp(p, "onCollision") == 0 ||
        strcmp(p, "doOften") == 0) {
            checkForEventHandlers = 1;
        }
    }
    // with a stupid key, there is nothing to do:
    if (lua_type(l, 2) != LUA_TSTRING && lua_type(l, 2) != LUA_TNUMBER) {
        return 0;
    }
    // throw everything above arg 3 away:
    if (lua_gettop(l) > 3) {
        lua_pop(l, lua_gettop(l)-3);
    }
    // make sure we have 3 args (= value):
    while (lua_gettop(l) < 3) {
        lua_pushnil(l);
    }
    // now simply do it:
    luacfuncs_object_obtainRegistryTable(l, o);
    lua_insert(l, -3);
    lua_rawset(l, -3);
    lua_pop(l, 1);  // remaining blitwizard object on stack
    // assuming we made it this far, notify of changed event handlers:
    if (checkForEventHandlers) {
        luacfuncs_updateObjectHandlers(l, o);
    }
    return 0;
} 

void luacfuncs_pushbobjidref(lua_State* l, struct blitwizardobject* o) {
    // attempt to obtain ref from registry:
    lua_pushstring(l, o->selfRefName);
    lua_gettable(l, LUA_REGISTRYINDEX);
    if (lua_type(l, -1) == LUA_TUSERDATA) {
        // reference present in registry!
        return;
    }
    lua_pop(l, 1); // pop again whatever that was
    
    // create luaidref userdata struct which points to the blitwizard object
    struct luaidref* ref = lua_newuserdata(l, sizeof(*ref));
    memset(ref, 0, sizeof(*ref));
    ref->magic = IDREF_MAGIC;
    ref->type = IDREF_BLITWIZARDOBJECT;
    ref->ref.bobj = o;

    // set garbage collect callback:
    luastate_SetGCCallback(l, -1,
    (int (*)(void*))&garbagecollect_blitwizobjref);

    // set metatable __index and __newindex to registry table:
    lua_getmetatable(l, -1);
    lua_pushstring(l, "__index");
    luacfuncs_object_obtainRegistryTable(l, o);
    lua_settable(l, -3);
    lua_pushstring(l, "__newindex");
    lua_pushcfunction(l, &luafuncs_bobjnewindex);
    lua_settable(l, -3);
    lua_setmetatable(l, -2);

    o->refcount++;
}

struct blitwizardobject* toblitwizardobject(lua_State* l, int index, int arg, const char* func) {
    if (lua_type(l, index) != LUA_TUSERDATA) {
        haveluaerror(l, badargument1, arg, func,
        "blitwizard object", lua_strtype(l, index));
    }
    if (lua_rawlen(l, index) != sizeof(struct luaidref)) {
        haveluaerror(l, badargument2, arg, func,
        "not a valid blitwizard object");
    }
    struct luaidref* idref = lua_touserdata(l, index);
    if (!idref || idref->magic != IDREF_MAGIC
    || idref->type != IDREF_BLITWIZARDOBJECT) {
        haveluaerror(l, badargument2, arg, func,
        "not a valid blitwizard object");
    }
    struct blitwizardobject* o = idref->ref.bobj;
    if (o->deleted) {
        haveluaerror(l, badargument2, arg, func,
        "this blitwizard object was deleted");
    }
    return o;
}

/// Set an object to be invisible for mouse handling.
// Other objects below this particular object may therefore
// receive the according @{blitwizard.object:onMouseEnter|object:onMouseEnter},
// @{blitwizard.object:onMouseLeave|object:onMouseLeave} and
// @{blitwizard.object:onMouseClick|object:onMouseClick} events instead.
//
// This is useful e.g. for text that isn't supposed to "steal" the events
// from the button it is on, or a fade-in sprite that covers the whole
// @{blitwizard.graphics.camera|camera}
// which shouldn't block the mouse events.
// @function setInvisibleToMouse
// @tparam boolean invisible Set to true to make it invisible to mouse events,
// or false to make it receive or block mouse events (default: false)
int luafuncs_object_setInvisibleToMouse(lua_State* l) {
    struct blitwizardobject* o =
    toblitwizardobject(l, 1, 1, "blitwiz.object:setInvisibleToMouse");

    if (lua_type(l, 2) != LUA_TBOOLEAN) {
        haveluaerror(l, badargument1, 1,
        "blitwizard.object:setInvisibleToMouse", "boolean", lua_strtype(l, 2));
    }
    int b = lua_toboolean(l, 2);
#ifdef USE_GRAPHICS
    luacfuncs_objectgraphics_setInvisibleToMouseEvents(o, b);
#endif
    return 0;
}

/// Create a new blitwizard object which is represented as a 2d or
// 3d object in the game world.
// 2d objects are on a separate 2d plane, and 3d objects are inside
// the 3d world.
// Objects can have behaviour and collision info attached and move
// around. They are what eventually makes the action in your game!
// @function new
// @tparam number type specify object type: blitwizard.object.o2d or blitwizard.object.o3d
// @tparam string resource (optional) if you specify the file path to a resource here (optional), this resource will be loaded and used as a visual representation for the object. The resource must be a supported graphical object, e.g. an image (.png) or a 3d model (.mesh). You can also specify nil here if you don't want any resource to be used.
// @treturn userdata Returns a @{blitwizard.object|blitwizard object}
// @usage
// -- Create a new 2d sprite object from the image file myimage.png
// local obj = blitwizard.object:new(blitwizard.object.o2d, "myimage.png")
int luafuncs_object_new(lua_State* l) {
    // technical first argument is the object table,
    // which we don't care about in the :new function.
    // actual specified first argument is the second one
    // on the lua stack.

    //printf("[0] entering object:new %llu\n", time_GetMicroseconds());

    // first argument needs to be 2d/3d type:
    if (lua_type(l, 2) != LUA_TNUMBER) {
        return haveluaerror(l, badargument1, 1, "blitwizard.object:new",
        "number", lua_strtype(l, 2));
    }
    int is3d = (lua_tointeger(l, 2) == 1);

    // second argument, if present, needs to be the resource:
    const char* resourcerelative = NULL;
    if (lua_gettop(l) >= 3 && lua_type(l, 3) != LUA_TNIL) {
        if (lua_type(l, 3) != LUA_TSTRING) {
            return haveluaerror(l, badargument1, 2, "blitwizard.object:new",
            "string", lua_strtype(l, 3));
        }
        resourcerelative = lua_tostring(l, 3);
    }

    // immediately turn this into an absolute path so os.chdir() is safe:
    char* resource = NULL;
    if (resourcerelative) {
        resource = file_GetAbsolutePathFromRelativePath(
        resourcerelative);
        if (!resource) {
            return haveluaerror(l, "failed to allocate or determine "
            "absolute path");
        }
    }

    // create new object
    struct blitwizardobject* o = malloc(sizeof(*o));
    if (!o) {
        luacfuncs_object_clearRegistryTable(l, o);
        free(resource);
        return haveluaerror(l, "failed to allocate new object");
    }
    memset(o, 0, sizeof(*o));
    o->is3d = is3d;
    if (!is3d) {
        o->parallax = 1;
    }

    // compose object registry entry string
    snprintf(o->regTableName, sizeof(o->regTableName),
    "bobj_table_%p", o);

    // remember resource path:
    if (resource) {
        o->respath = resource;
    }

    //printf("[3] Calling luacfuncs_objectAddedDeleted %llu\n", time_GetMicroseconds());

    luacfuncs_objectAddedDeleted(o, 1);

    // add us to the object list:
    o->next = objects;
    if (objects) {
        objects->prev = o;
    }
    objects = o;

    // prepare registry entry name for self reference:
    snprintf(o->selfRefName, sizeof(o->selfRefName), "bobj_self_%p", o);

    // if resource is present, start loading it:
#ifdef USE_GRAPHICS
    luafuncs_objectgraphics_load(o, o->respath);
#endif

    // create idref to object, which we will store up to object deletion:
    lua_pushstring(l, o->selfRefName);
    luacfuncs_pushbobjidref(l, o);
    lua_settable(l, LUA_REGISTRYINDEX);

    // obtain ref again:
    luacfuncs_pushbobjidref(l, o); 

    //printf("[9] done! %llu\n", time_GetMicroseconds());
    //fflush(stdout);
    return 1;
}

void luacfuncs_object_clearRegistryTable(lua_State* l,
struct blitwizardobject* o) {
    // clear entry:
    lua_pushstring(l, o->regTableName);
    lua_pushnil(l);
    lua_settable(l, LUA_REGISTRYINDEX);
}

void luacfuncs_object_obtainRegistryTable(lua_State* l,
struct blitwizardobject* o) {
    // obtain the hidden table that makes the blitwizard
    // object userdata behave like a table.

    // obtain registry entry:
    lua_checkstack(l, 4);
    lua_pushstring(l, o->regTableName);
    lua_gettable(l, LUA_REGISTRYINDEX);

    // if table is not present yet, create it:
    if (lua_type(l, -1) != LUA_TTABLE) {
        lua_pop(l, 1);  // pop whatever value this is

        // create a proper table:
        lua_pushstring(l, o->regTableName);
        lua_newtable(l);

        // resize stack:
        luaL_checkstack(l, 5,
        "insufficient stack to obtain object registry table");

        // the registry table's __index should go to
        // blitwizard.object:
        if (!lua_getmetatable(l, -1)) {
            // we need to create the meta table first:
            lua_newtable(l);
            lua_setmetatable(l, -2);

            // obtain it again:
            lua_getmetatable(l, -1);
        }
        lua_pushstring(l, "__index");
        lua_getglobal(l, "blitwizard");
        if (lua_type(l, -1) == LUA_TTABLE) {
            // extract blitwizard.object:
            lua_pushstring(l, "object");
            lua_gettable(l, -2);
            lua_insert(l, -2);
            lua_pop(l, 1);  // pop blitwizard table

            if (lua_type(l, -1) != LUA_TTABLE) {
                // error: blitwizard.object isn't a table as it should be
                lua_pop(l, 3); // blitwizard.object, "__index", metatable
            } else {
                lua_rawset(l, -3); // removes blitwizard.object, "__index"
                lua_setmetatable(l, -2); // setting remaining metatable
                // stack is now back empty, apart from new userdata!
            }
        } else {
            // error: blitwizard namespace is broken. nothing we could do
            lua_pop(l, 3);  // blitwizard, "__index", metatable
        }
        lua_settable(l, LUA_REGISTRYINDEX);

        // obtain it again:
        lua_pushstring(l, o->regTableName);
        lua_gettable(l, LUA_REGISTRYINDEX);
    }
}

// implicitely calls luacfuncs_onError() when an error happened
int luacfuncs_object_callEvent(lua_State* l,
struct blitwizardobject* o, const char* eventName,
int args, int* boolreturn) {
    // some events might be temporarily disabled, due to
    // previous errors in the event function:
    if (o->disabledDoAlways >= time(NULL) &&
    strcmp(eventName, "doAlways") == 0) {
        lua_pop(l, args);
        return 1;
    }
    if (o->disabledOnCollision >= time(NULL)
    && strcmp(eventName, "onCollision") == 0) {
        lua_pop(l, args);
        return 1;
    }

    // for speed reasons, disable GC:
    //luastate_suspendGC();

    // ensure sufficient stack size:
    lua_checkstack(l, 4+args);

    // get object table:
    luacfuncs_object_obtainRegistryTable(l, o);

    // get event function
    lua_pushstring(l, eventName);
    lua_gettable(l, -2);

    // if function is not a function, don't call:
    if (lua_type(l, -1) != LUA_TFUNCTION) {
        lua_pop(l, 2);  // pop function, registry table
        lua_pop(l, args);  // remove args
        //luastate_resumeGC();
        return 1;
    }

    // get rid of the object table again:
    lua_insert(l, -2);  // push function below table
    lua_pop(l, 1);  // remove table

    // move function in front of all args:
    if (args > 0) {
        lua_insert(l, -(args+1));
    }

    // push self as first argument:
    luacfuncs_pushbobjidref(l, o);
    if (args > 0) {
        // move in front of all args:
        lua_insert(l, -(args+1));
    }

    // push error handling function onto stack:
    lua_pushcfunction(l, internaltracebackfunc());

    // move error handling function in front of function + args
    lua_insert(l, -(args+3));

    // call function:
    int returnvalues = 0;
    if (boolreturn) {
        returnvalues = 1;
    }
    int ret = lua_pcall(l, args+1, returnvalues, -(args+3));

    int errorHappened = 0;
    // process errors:
    if (ret != 0) {
        const char* e = lua_tostring(l, -1);

        char funcName[128];
        snprintf(funcName, sizeof(funcName),
        "blitwizard.object event function \"%s\"", eventName);
        luacfuncs_onError(funcName, e);

        if (strcmp(eventName, "doAlways") == 0) {
            // temporarily disable for 1-2 seconds:
            o->disabledDoAlways = time(NULL)+1;
        } else if (strcmp(eventName, "onCollision") == 0) {
            o->disabledOnCollision = time(NULL)+1;
        }
    }

    // obtain return value:
    if (boolreturn) {
        *boolreturn = lua_toboolean(l, -1);
        lua_pop(l, 1);
    }

    // pop error handling function from stack again:
    lua_pop(l, 1);

    //luastate_resumeGC();
    return (errorHappened == 0);
}

/// Destroy the given object explicitely, to make it instantly disappear
// from the game world.
//
// If you still have references to this object, they will no longer
// work.
// @function destroy
int luafuncs_object_destroy(lua_State* l) {
    // delete the given object
    struct blitwizardobject* o =
    toblitwizardobject(l, 1, 1, "blitwiz.object:destroy");

    // tell getAllObjects() iterator of change:
    luacfuncs_objectAddedDeleted(o, 0);

    // remove self ref in registry:
    lua_pushstring(l, o->selfRefName);
    lua_pushnil(l);
    lua_settable(l, LUA_REGISTRYINDEX);

    // mark it deleted, and move it over to deletedobjects:
    o->deleted = 1;

    // do a first temp cleanup:
    cleanupobject(o, 0);
    return 0;
}

/// Get the current position of the object.
// Returns two coordinates for a 2d object, and three coordinates
// for a 3d object.
// @function getPosition
// @treturn number x coordinate
// @treturn number y coordinate
// @treturn number (if 3d object) z coordinate
int luafuncs_object_getPosition(lua_State* l) {
    struct blitwizardobject* obj = toblitwizardobject(l, 1, 0,
    "blitwizard.object:getPosition");
    double x,y,z;
    objectphysics_getPosition(obj, &x, &y, &z);
    lua_pushnumber(l, x);
    lua_pushnumber(l, y);
    if (obj->is3d) {
        lua_pushnumber(l, z);
        return 3;
    }
    return 2;
}

/// Set the object to a new position. Specify two coordinates for 2d
// objects (x, y) and three for 3d objects (x, y, z).
//
// Please note this game position is in <b>game units</b>,
// not pixels.<br>
//  
// <b>What is a game unit:</b>
//
// Game units are a specific measure which are abstracted from actual
// pixels. This allows blitwizard to remain largely independent of the
// @{blitwizard.graphics.setMode|screen resolution} of a game.
//
// <b>How much is one game unit:</b>
//
// For 2d, one game unit is usually around 50 pixels on the screen unless
// you @{blitwizard.graphics.camera:set2dZoomFactor|zoom around}.
// For large resolutions it will appear larger though,
// so a game unit's final size on the screen scales with the screen resolution.
//
// For 3d objects, the rule of thumb is 1 game unit should be handled as roughly 1 meter.
// (this works best for the physics)
//
// If you need to run your game at very large zoom factors or very
// small ones, consider changing your @{blitwizard.object:setScale|object scale}
// instead of making up for all of it with zooming around largely.
//
// To find out how much e.g. a 100x100 pixel texture is in game units
// at scale 1, check @{blitwizard.graphics.gameUnitToPixels}.
// (please note! a 100x100 pixel object is NOT necessarily 100x100
// pixel large on the screen at @{blitwizard.object:setScale|scale 1} - this depends on the
// @{blitwizard.graphics.setMode|screen resolution}. With larger resolution it will be
// larger, so it always takes up the same relative amount on screen)
//
// @function setPosition
// @tparam number pos_x x coordinate
// @tparam number pos_y y coordinate
// @tparam number pos_z (only for 3d objects) z coordinate
int luafuncs_object_setPosition(lua_State* l) {
    struct blitwizardobject* obj = toblitwizardobject(l, 1, 0,
    "blitwizard.object:setPosition");
    if (lua_type(l, 2) != LUA_TNUMBER) {
        return haveluaerror(l, badargument1, 1,
        "blitwizard.object:setPosition", "number", lua_strtype(l, 2));
    }
    if (lua_type(l, 3) != LUA_TNUMBER) {
        return haveluaerror(l, badargument1, 2,
        "blitwizard.object:setPosition", "number", lua_strtype(l, 3));
    }
    if (obj->is3d && lua_type(l, 4) != LUA_TNUMBER) {
        return haveluaerror(l, badargument1, 3,
        "blitwizard.object:setPosition", "number", lua_strtype(l, 4));
    }
    double x,y,z;
    x = lua_tonumber(l, 2);
    y = lua_tonumber(l, 3);
    if (obj->is3d) {
        z = lua_tonumber(l, 4);
    } else {
        z = 0;
    }
    objectphysics_setPosition(obj, x, y, z);
    return 0;
}

/// Set the transparency of the visual representation of this object.
// Defaults to 0 (solid).
// @function setTransparency
// @tparam number transparency transparency from 0 (solid) to 1 (invisible)
int luafuncs_object_setTransparency(lua_State* l) {
    struct blitwizardobject* obj = toblitwizardobject(l, 1, 0,
    "blitwizard.object:setTransparency");
    if (lua_type(l, 2) != LUA_TNUMBER) {
        return haveluaerror(l, badargument1, 1,
        "blitwizard.object:setTransparency", "number", lua_strtype(l, 2));
    }
    double a = 1 - lua_tonumber(l, 2);
    if (a < 0) {
        a = 0;
    }
    if (a > 1) {
        a = 1;
    }
#ifdef USE_GRAPHICS
    luacfuncs_objectgraphics_setAlpha(obj, a);
#endif
    return 0;
}

/// Get the transparency of the visual representation of this object.
// @function getTransparency
// @treturn number transparency from 0 (solid) to 1 (invisible)
int luafuncs_object_getTransparency(lua_State* l) {
    struct blitwizardobject* obj = toblitwizardobject(l, 1, 0,
    "blitwizard.object:setTransparency");
#ifdef USE_GRAPHICS
    return 1-luacfuncs_objectgraphics_getAlpha(obj);
#else
    return 1;
#endif
}

/// Enable a 2d parallax effect at a given strength.
// The <a href="http://en.wikipedia.org/wiki/Parallax">parallax effect</a>
// simulates depth by displacing objects. Effectively, it just
// makes them less (for strength > 1.0) or more (< 1.0) displaced
// depending on the @{blitwizard.graphics.camera:set2dCenter|camera position}
// than they would normally be.
//
// Therefore, use values smaller than 1 (and greater than zero) for
// close objects, or greater than 1 for far away objects.
//
// This function does only work with 2d objects.
// It doesn't have any effect for
// @{blitwizard.object:pinToCamera|pinned objects}.
// @function setParallax
// @tparam number strength of the effect (greater than zero) or 1.0 to disable
int luafuncs_object_setParallax(lua_State* l) {
    struct blitwizardobject* obj = toblitwizardobject(l, 1, 0,
    "blitwizard.object:setParallax");
    if (obj->is3d) {
        return haveluaerror(l, "setParallax may only be used on 2d objects");
    }
#if (defined(USE_PHYSICS2D) || defined(USE_PHYSICS3D))
    if (obj->physics && obj->physics->object) {
        return haveluaerror(l, "cannot enable parallax effect on objects "
        "with collision");
    }
#endif
    if (lua_type(l, 2) != LUA_TNUMBER) {
        return haveluaerror(l, badargument1, 1,
        "blitwizard.object:setParallax", "number", lua_strtype(l, 2));
    }
    if (lua_tonumber(l, 2) <= 0) {
        return haveluaerror(l, badargument2, 1,
        "blitwizard.object:setParallax", "parallax effect strength must be >0."
        " Use 1.0 for no parallax");
    }
    obj->parallax = lua_tonumber(l, 2);
#ifdef USE_GRAPHICS
    luacfuncs_objectgraphics_updateParallax(obj);
#endif
    return 0;
}

/// Get the dimensions of an object in game units (with
// @{blitwizard.object:setScale|scaling} taken into account).
//
// For a 2d sprite, this returns two components (x, y) which
// match the sprite's width and height, for a 3d object,
// this returns a 3d box which would encapsulate the 3d mesh
// in its first initial animation frame.
//
// If you call this function before getting the
// @{blitwizard.object:onGeometryLoaded|geometry callback},
// an error will be thrown that the information is not available
// yet.
//
// Note with use of @{blitwizard.object:set2dTextureClipping} the dimensions
// change and they might no longer reflect the initial size of the
// texture.
// @function getDimensions
// @treturn number x_size X dimension value
// @treturn number y_size Y dimension value
// @treturn number z_size (only for 3d objects) Z dimension value
int luafuncs_object_getDimensions(lua_State* l) {
    struct blitwizardobject* obj = toblitwizardobject(l, 1, 0,
    "blitwizard.object:setPosition");
    double x, y, z;
#ifdef USE_GRAPHICS
    if (!luacfuncs_objectgraphics_getOriginalDimensions(obj, &x, &y, &z)) {
        return haveluaerror(l, "Object dimensions not known");
    }
#else
    x = 0; y = 0; z = 0;
#endif
    double sx, sy, sz;
    if (obj->is3d) {
        sx = obj->scale3d.x;
        sy = obj->scale3d.y;
        sz = obj->scale3d.z;
    } else {
        sx = obj->scale2d.x;
        sy = obj->scale2d.y;
        sz = 0;
    }
    lua_pushnumber(l, x * sx);
    lua_pushnumber(l, y * sy);
    if (obj->is3d) {
        lua_pushnumber(l, z * sz);
    }
    return 2+(obj->is3d);
}

/// Get the original, unscaled dimensions of an object.
// Returns the same values as @{blitwizard.object:getScale},
// but with assuming a @{blitwizard.object:setScale|scale factor of 1},
// even if it's different.
// @function getOriginalDimensions
// @treturn number x_size X dimension value
// @treturn number y_size Y dimension value
// @treturn number z_size (only for 3d objects) Z dimension value 
int luafuncs_object_getOriginalDimensions(lua_State* l) {
    struct blitwizardobject* obj = toblitwizardobject(l, 1, 0,
    "blitwizard.object:setPosition");
    double x, y, z;
#ifdef USE_GRAPHICS
    if (!luacfuncs_objectgraphics_getOriginalDimensions(obj, &x, &y, &z)) {
        return haveluaerror(l, "Object dimensions not known");
    }
#else
    x = 0; y = 0; z = 0;
#endif
    lua_pushnumber(l, x);
    lua_pushnumber(l, y);
    if (obj->is3d) {
        lua_pushnumber(l, z);
    }
    return 2+(obj->is3d);
}

/// Get the object's scale, which per default is 1,1 for 2d objects
// and 1,1,1 for 3d objects. The scale is a factor applied to the
// dimensions of an object to stretch or shrink it.
//
// Please note if you want to scale up a unit to precisely
// match a specific size in game units (instead of just making
// it twice the size, three times the size etc with setScale),
// you might want to use @{blitwizard.object:scaleToDimensions|
// object:scaleToDimensions}.
//
// Returns 2 values for 2d objects, 3 values for 3d objects.
// @function getScale
// @treturn number x_scale X scale factor
// @treturn number y_scale Y scale factor
// @treturn number z_scale (optional) Z scale factor
int luafuncs_object_getScale(lua_State* l) {
    struct blitwizardobject* obj = toblitwizardobject(l, 1, 0,
    "blitwizard.object:getScale");
    if (obj->is3d) {
        lua_pushnumber(l, obj->scale3d.x);
        lua_pushnumber(l, obj->scale3d.y);
        lua_pushnumber(l, obj->scale3d.z);
        return 3;
    } else {
        lua_pushnumber(l, obj->scale2d.x);
        lua_pushnumber(l, obj->scale2d.y);
        return 2;
    }
}

/// Change an object's scale, also see @{blitwizard.object:getScale}.
// This allows to stretch/shrink an object. If physics are enabled
// with @{blitwizard.object:enableMovableCollision|
// object:enableMovableCollision}
// or @{blitwizard.object:enableStaticCollision|
// object:enableStaticCollision},
// the physics hull will be scaled accordingly aswell.
//
// Please note if you want to scale up a unit to precisely
// match a specific size in game units (instead of just making
// it twice the size, three times the size etc with setScale),
// you might want to use @{blitwizard.object:scaleToDimensions|
// object:scaleToDimensions}.
//
// Specify x, y scaling for 2d objects and x, y, z scaling for 3d objects.
// @function setScale
// @tparam number x_scale X scale factor
// @tparam number y_scale Y scale factor
// @tparam number z_scale (optional) Z scale factor
// @usage
// -- scale a 2d object to twice the original size
// obj3d:setScale(2, 2)
//
// -- scale 3d object to half its original size
// obj3d:setScale(0.5, 0.5, 0.5)
int luafuncs_object_setScale(lua_State* l) {
    struct blitwizardobject* obj = toblitwizardobject(l, 1, 0,
    "blitwizard.object:setScale");
    if (lua_type(l, 2) != LUA_TNUMBER) {
        return haveluaerror(l, badargument1, 1,
        "blitwizard.object:setScale", "number", lua_strtype(l, 2));
    }
    if (lua_type(l, 3) != LUA_TNUMBER) {
        return haveluaerror(l, badargument1, 2,
        "blitwizard.object:setScale", "number", lua_strtype(l, 3));
    }
    if (obj->is3d && lua_type(l, 4) != LUA_TNUMBER) {
        return haveluaerror(l, badargument1, 3,
        "blitwizard.object:setScale", "number", lua_strtype(l, 4));
    }
    double x,y,z;
    x = lua_tonumber(l, 2);
    if (x <= 0) {
        return haveluaerror(l, badargument2, 1,
        "blitwizard.object:setScale", "scale must be positive number");
    }
    y = lua_tonumber(l, 3);
    if (y <= 0) {
        return haveluaerror(l, badargument2, 2,
        "blitwizard.object:setScale", "scale must be positive number");
    }
    if (obj->is3d) {
        z = lua_tonumber(l, 4);
        if (z <= 0) {
            return haveluaerror(l, badargument2, 3,
            "blitwizard.object:setScale", "scale must be positive number");
        }
        obj->scale3d.x = x;
        obj->scale3d.y = y;
        obj->scale3d.z = z;
    } else {
        obj->scale2d.x = x;
        obj->scale2d.y = y;
    }
#if (defined(USE_PHYSICS2D) || defined(USE_PHYSICS3D))
    luacfuncs_object_handleScalingForPhysics(obj);
#endif
    return 0;
}

/// Scale up an object to precise boundaries in game units.
// An according @{blitwizard.object:setScale|scale factor}
// will be picked to achieve this.
//
// For 2d objects, a width and a height can be specified.
// For 3d objects, the size along all three axis (x, y, z)
// can be specified.
//
// If you want the object to be scaled up but maintain
// its aspect ratio, specify just one parameter and pass
// <i>nil</i> for the others which will then be calculated
// accordingly.
// @function scaleToDimensions
// @tparam number width width or x size in game units
// @tparam number height height or y size
// @tparam number z_size (only present for 3d objects) z size
// @usage
// -- Scale up a 3d object so its height matches 2 game units:
// -- (it's proportions will be kept)
// obj3d:scaleToDimensions(nil, nil, 2)
int luafuncs_object_scaleToDimensions(lua_State* l) {
    struct blitwizardobject* obj = toblitwizardobject(l, 1, 0,
    "blitwizard.object:scaleToDimensions");

    // check parameters:
    if (lua_type(l, 2) != LUA_TNUMBER && lua_type(l, 2) != LUA_TNIL) {
        return haveluaerror(l, badargument1, 1,
        "blitwizard.object:scaleToDimensions", "number or nil",
        lua_strtype(l, 2));
    }
    if (lua_type(l, 3) != LUA_TNUMBER && lua_type(l, 3) != LUA_TNIL) {
        return haveluaerror(l, badargument1, 1,
        "blitwizard.object:scaleToDimensions", "number or nil",
        lua_strtype(l, 3));
    }
    if (obj->is3d &&
        lua_type(l, 4) != LUA_TNUMBER && lua_type(l, 4) != LUA_TNIL) {
        return haveluaerror(l, badargument1, 3,
        "blitwizard.object:scaleToDimensions", "number or nil",
        lua_strtype(l, 4));
    }

    double x,y,z;
    z = 1234;
#ifdef USE_GRAPHICS
    if (!luacfuncs_objectgraphics_getOriginalDimensions(obj,
    &x, &y, &z)) {
        return haveluaerror(l, "object dimensions not known yet, "
        "wait for onGeometryLoaded event before using this function");
    }
    if (x <= 0 || y <= 0 || z <= 0) {
#endif
        return haveluaerror(l, "for objects with a size of zero, "
        "there is no way to scale them up to a given size");
#ifdef USE_GRAPHICS
    }

    // get parameters:
    double xdim = 0;
    double ydim = 0;
    double zdim = 0;
    int gotdimension = 0;
    if (lua_type(l, 2) == LUA_TNUMBER) {
        xdim = lua_tonumber(l, 2);
        gotdimension = 1;
        if (xdim <= 0) {
            return haveluaerror(l, "cannot scale to dimension which is "
            "zero or negative");
        }
    }
    if (lua_type(l, 3) == LUA_TNUMBER) {
        ydim = lua_tonumber(l, 3);
        gotdimension = 1;
        if (ydim <= 0) {
            return haveluaerror(l, "cannot scale to dimension which is "
            "zero or negative");
        }
    }
    if (obj->is3d && lua_type(l, 4) == LUA_TNUMBER) {
        zdim = lua_tonumber(l, 4);
        gotdimension = 1;
        if (zdim <= 0) {
            return haveluaerror(l, "cannot scale to dimension which is "
            "zero or negative");
        }
    }

    // we require one dimension at least:
    if (!gotdimension) {
        return haveluaerror(l, "you need to specify at least one dimension "
        "for scaling");
    }

    // calculate factors:
    double xfactor = 0;
    double yfactor = 0;
    double zfactor = 0;
    if (xdim > 0) {
        xfactor = xdim/x;
    }
    if (ydim > 0) {
        yfactor = ydim/y;
    }
    if (obj->is3d && zdim > 0) {
        zfactor = zdim/z;
    }

    // calculate missing factors:
    if (xfactor == 0) {
        if (yfactor > 0 || !obj->is3d) {
            xfactor = yfactor;
        } else {
            xfactor = zfactor;
        }
    }
    if (yfactor == 0) {
        if (zfactor > 0 && obj->is3d) {
            yfactor = zfactor;
        } else {
            yfactor = xfactor;
        }
    }
    if (obj->is3d && zfactor == 0) {
        if (xfactor > 0) {
            zfactor = xfactor;
        } else {
            zfactor = yfactor;
        }
    }

    // do scaling:
    if (obj->is3d) {
        obj->scale3d.x = xfactor;
        obj->scale3d.y = yfactor;
        obj->scale3d.z = zfactor;
    } else {
        obj->scale2d.x = xfactor;
        obj->scale2d.y = yfactor;
    }
    return 0;
#endif
}

/// Set the z-index of the object (only for 2d objects).
// An object with a higher z index will be drawn above
// others with a lower z index. If two objects have the same
// z index, the newer object will be drawn on top.
//
// The z index will be internally set to an integer,
// so use numbers like -25, 0, 1, 2, 3, 99, ...
//
// The default z index is 0.
// @function setZIndex
// @tparam number z_index New z index
int luafuncs_object_setZIndex(lua_State* l) {
    struct blitwizardobject* obj = toblitwizardobject(l, 1, 0,
    "blitwizard.object:setZIndex");
    if (lua_type(l, 2) != LUA_TNUMBER) {
        return haveluaerror(l, badargument1, 1,
        "blitwizard.object:setZIndex", "number", lua_strtype(l, 2));
    }
    
    if (obj->is3d) {
        return haveluaerror(l, "z index can only be set for 2d objects");
    } else {
#ifdef USE_GRAPHICS
        if (obj->graphics && obj->graphics->sprite) {
            graphics2dsprites_setZIndex(obj->graphics->sprite, lua_tointeger(l, 2));
        }
#else
        return haveluaerror(l, error_nographics);
#endif
    }
    return 0;
}

/// Set the rotation angle of a 2d object. (You need to use
// @{blitwizard.object:setRotationAngles|object:setRotationAngles},
// @{blitwizard.object:setRotationFromQuaternion|
// object:setRotationFromQuaternion}
// or @{blitwizard.object:turnToPosition|object:turnToPosition}
// for 3d objects)
//
// An angle of 0 is the default rotation, 90 will turn it
// on its side counter-clockwise by 90° degree, etc.
// @function setRotationAngle
// @tparam number rotation the rotation angle
int luafuncs_object_setRotationAngle(lua_State* l) {
    struct blitwizardobject* obj = toblitwizardobject(l, 1, 0,
    "blitwizard.object:setRotationAngle");
    if (obj->is3d) {
        return haveluaerror(l, "not a 2d object");
    }
    if (lua_type(l, 2) != LUA_TNUMBER) {
        return haveluaerror(l, badargument1, 1,
        "blitwizard.object:setRotationAngle", "number", lua_strtype(l, 2));
    }
    objectphysics_set2dRotation(obj, fmod(lua_tonumber(l, 2), 360));
    return 0;
}

/// Get the rotation angle of a 2d object.
// (Use @{blitwizard.object.getRotationAngles|object:getRotationAngles} or
// @{blitwizard.object.getRotationQuaternion|object:getRotationQuaternion}
// for 3d objects)
//
// Also see @{blitwizard.object:setRotationAngle|object:setRotationAngle}.
// @function getRotationAngle
// @treturn number the current 2d rotation of the object
int luafuncs_object_getRotationAngle(lua_State* l) {
    struct blitwizardobject* obj = toblitwizardobject(l, 1, 0,
    "blitwizard.object:getRotationAngle");
    if (obj->is3d) {
        return haveluaerror(l, "not a 2d object");
    }
    double angle = 0;
    objectphysics_get2dRotation(obj, &angle);
    angle = fmod(angle, 360);
    if (angle < 0) {
        angle += 360;
    }
    lua_pushnumber(l, angle);
    return 1;
}

/// For 2d sprite objects, set a clipping window inside the
// original texture used for the sprite which allows you to
// specify a sub-rectangle of the texture to be used as
// image source.
//
// This means you can make your sprite show just a part of the
// texture, not all of it.
//
// The clipping window defaults to full texture size. Changing
// it will also affect the size reported by
// @{blitwizard.object:getDimensions|object:getDimensions}.
//
// You can omit all parameters which will reset the clipping window
// to the full texture dimensions.
// @function set2dTextureClipping
// @tparam number x the x offset in the texture, in pixels (default: 0)
// @tparam number y the y offset in the texture, in pixels (default: 0)
// @tparam number width the clipping window width in the texture in pixels (defaults to full texture width)
// @tparam number height the clipping window height in the texture in pixels 8defaults to full texture height)
#ifdef USE_GRAPHICS
int luafuncs_object_set2dTextureClipping(lua_State* l) {
    struct blitwizardobject* obj = toblitwizardobject(l, 1, 0,
    "blitwizard.object:set2dTextureClipping");
    if (obj->is3d) {
        // FIXME: support 3d decals here
        return haveluaerror(l, "Not a 2d object");
    }
    if (lua_gettop(l) == 1) {
        // no further args.
        // the user wants to unset the texture clipping.
        luacfuncs_objectgraphics_unsetTextureClipping(obj);
        return 0;
    }

    // check all for args for being a number:
    int i = 2;
    while (i <= 5) {
        if (lua_type(l, i) != LUA_TNUMBER) {
            return haveluaerror(l, badargument1, i-1,
            "blitwizard.object:set2dTextureClipping", "number",
            lua_strtype(l, i));
        }
        i++;
    }
     
    // set texture clipping:
    luacfuncs_objectgraphics_setTextureClipping(obj,
    lua_tosize_t(l, 2), lua_tosize_t(l, 3), lua_tosize_t(l, 4),
    lua_tosize_t(l, 5));
    return 0;
}
#endif

/// Pin a 2d object to a given @{blitwizard.graphics.camera|game camera}.
// 
// It will only be visible on that given camera, it will ignore the
// camera's 2d position and zoom and will simply display with default zoom
// with 0,0 being the upper left corner.
//
// It will also be above all unpinned 2d object.
//
// You might want to use this for on-top interface graphics that shouldn't
// move with the level but stick to the screen.
//
// Unpin a pinned 
// @function pinToCamera
// @tparam userdata (optional) camera the @{blitwizard.graphics.camera|camera} to pin to. If you don't specify any, the default camera will be used
#ifdef USE_GRAPHICS
int luafuncs_object_pinToCamera(lua_State* l) {
    struct blitwizardobject* obj = toblitwizardobject(l, 1, 0,
    "blitwizard.object:pinToCamera");

    if (obj->is3d) {
        return haveluaerror(l, "cannot pin 3d objects");
    }

    int cameraId = -1;
    if (lua_gettop(l) >= 2 && lua_type(l, 2) != LUA_TNIL) {
        cameraId = toluacameraid(l, 2, 1, "blitwizard.object:pinToCamera");
    } else {
        if (graphics_GetCameraCount() <= 0) {
            return haveluaerror(l, "there are no cameras");
        }
        cameraId = 0;
    }

    // pin it to the given camera:
    luacfuncs_objectgraphics_pinToCamera(obj, cameraId);
    return 0;
}
#endif

/// Unpin a sprite pinned with @{blitwizard.object.pinToCamera|pinToCamera.
// @function unpinFromCamera
#ifdef USE_GRAPHICS
int luafuncs_object_unpinFromCamera(lua_State* l) {
    struct blitwizardobject* obj = toblitwizardobject(l, 1, 0,
    "blitwizard.object:pinToCamera");

    if (obj->is3d) {
        return haveluaerror(l, "cannot pin or unpin 3d objects");
    }

    // unpin from camera:
    luacfuncs_objectgraphics_pinToCamera(obj, -1);
    return 0;
}
#endif

static void luacfuncs_object_doStep(lua_State* l,
struct blitwizardobject* o, int doOften) {
    if (o->deleted) {
        return;
    }

    if (doOften && o->haveDoOften) {
        luacfuncs_object_callEvent(l, o, "doOften", 0, NULL);
    }
    if (o->haveDoAlways) {
        luacfuncs_object_callEvent(l, o, "doAlways", 0, NULL);
    }
}

uint64_t lastFullStep = 0;
int luacfuncs_object_doAllSteps(int count) {
    if (count < 1) {
        count = 1;
    }
    struct blitwizardobject* o;
    lua_State* l = luastate_GetStatePtr();

    // see if we want to do a full step (consider all objects)
    // or a non-full one (consider just the important objects).
    // we want to do a full one 4 times a second.
    uint64_t now = time_GetMilliseconds();
    int full = 0;
    if (lastFullStep + 2000 < now) {
        // we missed so many full steps, it makes no sense to catch up:
        lastFullStep = now;
    } else {
        if (lastFullStep < now) {
            lastFullStep += 250;
            full = 1;
        }
    }

    // cycle through all objects:
    if (full) {
        o = objects;
        processedNormalObjects = 0;
    } else {
        o = importantObjects;
        processedImportantObjects = 0;
    }
    while (o) {
        // call doStep on object:
        struct blitwizardobject* onext = o->next;
        if (!full) {
            onext = o->importantNext;
        }
        int i = 0;
        while (i < count && !o->deleted) {
            luacfuncs_object_doStep(l, o, full);
            i++;
        }
        // if object is deleted, move to deletedobjects:
        if (o->deleted) {
            // remove from objects list:
            if (o->prev) {
              o->prev->next = o->next;
            } else {
              objects = o->next;
            }
            if (o->next) {
              o->next->prev = o->prev;
            }

            // remove from important list:
            if (o->importantPrev || o->importantNext ||
            importantObjects == o) {
                // object is in important list -> remove it
                if (o->importantPrev) {
                    o->importantPrev->importantNext = o->
                    importantNext;
                } else {
                    importantObjects = o->importantNext;
                }
                if (o->importantNext) {
                    o->importantNext->importantPrev = o->
                    importantPrev;
                }
            }

            // add to deleted list:
            o->next = deletedobjects;
            if (o->next) {
                o->next->prev = o;
            }
            deletedobjects = o;
            o->prev = NULL;
        }
        // advance counters
        if (full) {
            processedNormalObjects++;
        } else {
            processedImportantObjects++;
        }
        o = onext;
    }

    // remove/add objects to importantObjects list:
    struct addRemoveObject* a = addRemoveImportantObjects;
    while (a) {
        struct addRemoveObject* anext = a->next;
        if (a->remove || a->obj->deleted) {
            if (a->obj->importantPrev || a->obj->importantNext ||
            importantObjects == a->obj) {
                // object is in important list -> remove it
                if (a->obj->importantPrev) {
                    a->obj->importantPrev->importantNext = a->obj->
                    importantNext;
                } else {
                    importantObjects = a->obj->importantNext;
                }
                if (a->obj->importantNext) {
                    a->obj->importantNext->importantPrev = a->obj->
                    importantPrev;
                }
            }
        } else {
            if (!a->obj->importantPrev && !a->obj->importantNext &&
            importantObjects != a->obj) {
                // object isn't in important list -> add it
                a->obj->importantNext = importantObjects;
                if (a->obj->importantNext) {
                    a->obj->importantNext->importantPrev = a->obj;
                }
                importantObjects = a->obj;
            }
        }
        // free data struct:
        free(a);
        a = anext;
    }
    addRemoveImportantObjects = NULL;

    // actually delete any objects marked as deleted:
    o = deletedobjects;
    while (o) {
        struct blitwizardobject* onext = o->next;
        if (luacfuncs_object_deleteIfOk(o)) {
            if (deletedobjects == o) {
                deletedobjects = onext;
            }
        }
        o = onext;
    }
    return count;
}

void luacfuncs_object_updateGraphics() {
#ifdef USE_GRAPHICS
    lua_State* l = luastate_GetStatePtr();
    // update visual representations of objects:
    struct blitwizardobject* o = objects;
    while (o) {
        // attempt to load graphics if not done yet:
        luafuncs_objectgraphics_load(o, o->respath);

        if (luafuncs_objectgraphics_NeedGeometryCallback(o)) {
            luacfuncs_object_callEvent(l, o, "onGeometryLoaded", 0, NULL);
        }

        luacfuncs_objectgraphics_updatePosition(o);
        o = o->next;
    }
#endif
}


/// Change whether an object is shown at all, or whether it is hidden.
// If you know objects aren't going to be needed at some point or
// if you want to hide them until something specific happens,
// use this function to hide them temporarily.
//
// @function setVisible
// @tparam boolean visible specify true if you want the object to be shown (default), or false if you want it to be hidden. Please note this has no effect on collision
#ifdef USE_GRAPHICS
int luafuncs_object_setVisible(lua_State* l) {
    struct blitwizardobject* obj = toblitwizardobject(l, 1, 0,
    "blitwizard.object:pinToCamera");
    if (lua_type(l, 2) != LUA_TBOOLEAN) {
        return haveluaerror(l, badargument1, 1,
        "blitwizard.object:pinToCamera", "boolean",
        lua_strtype(l, 2));
    }        
    luacfuncs_objectgraphics_setVisible(obj,
    lua_toboolean(l, 2));
    return 0;
}
#endif

/// Set this event function to a custom function
// of yours to get notified when the geometry
// (size/dimension and animation data etc) of your
// object has been fully loaded.
//
// <b>This function does not exist</b> before you
// set it on a particular object.
//
// See usage on how to define this function for an
// object of your choice.
// @function onGeometryLoaded
// @usage
//   -- create a 2d sprite and output its size:
//   local obj = obj:new(blitwizard.object.o2d, "my_image.png")
//   function obj:onGeometryLoaded()
//     -- call @{blitwizard.object:getDimensions|self:getDimensions} to get
//     -- its dimensions
//     print("My dimensions are: " .. self:getDimensions()[1],
//     self:getDimensions()[2])
//   end

/// Set this event function to a custom function of yours
// to have the object do something
// over and over again moderately fast (4 times a second).
//
// You might want to use this function for things where a small
// reaction time is acceptable but which need to happen constantly,
// e.g. an enemy checking if it can see the player (to start an attack).
//
// <b>This function does not exist</b> before you
// set it on a particular object.
//
// This function is called with all objects with a
// frequency of 4 times a second.
// @function doOften

/// Set this event function to a custom
// function of yours to have the object do something
// over and over again (very rapidly).
//
// This function is mainly useful for fluid movements, e.g.
// for an elevator, you might want to move the elevator
// one bit each time this function is called.
//
// <b>This function does not exist</b> before you
// set it on a particular object.
//
// This function is called with all objects with a
// frequency of 60 times a second (it can be changed
// globally with @{blitwizard.setStep} if you know
// what you're doing).
//
// For decisions and changes that don't need to be done so fast,
// use @{blitwizard.object:doOften|object:doOften} which runs
// 4 times a second (which is still very often).
// @function doAlways
// @usage
// -- have a sprite move up the screen
// local obj = obj:new(blitwizard.object.o2d, "my_image.png")
// function obj:doAlways()
//   -- get old position:
//   local pos_x, pos_y = self:getPosition()
//   -- move position up a bit:
//   pos_y = pos_y - 0.01
//   -- set position again:
//   self:setPosition(pos_x, pos_y)
// end

/// Set this event function to get notified when
// a mouse click is done on your object.
//
// A double-click will be reported as two onMouseClick events
// in short succession.
//
// Any object above this object (higher z-index) can cover this one
// and steal its mouse events, even if it doesn't have any handlers
// for mouse events itself. To prevent that, set such covering objects
// to @{blitwizard.object:setInvisibleToMouse|
// ignore mouse events (object:setInvisibleToMouse)}.
// @function onMouseClick

/// Set this event function to get notified when
// the mouse is moved onto your object. (this will also be triggered
// if the object itself moves in a way so it ends up under the mouse
// cursor)
//
// Please note with lots of @{blitwizard.object|objects}, enabling this
// on objects with a low Z index (with many others above them) can have
// an impact on performance.
// @function onMouseEnter

/// Set this event function to get notified when
// the mouse was moved onto your object and has now moved
// away again. (this will also be triggered if the object itself
// moves away from the mouse position)
//
// Please note with lots of @{blitwizard.object|objects}, enabling this
// on objects with a low Z index (with many others above them) can have
// an impact on performance.
// @function onMouseLeave


