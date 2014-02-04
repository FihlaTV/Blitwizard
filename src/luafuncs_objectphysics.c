
/* blitwizard game engine - source code file

  Copyright (C) 2011-2014 Jonas Thiem

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
// @author Jonas Thiem  (jonas.thiem@gmail.com)
// @copyright 2011-2013
// @license zlib
// @module blitwizard

#include "config.h"
#include "os.h"

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include "luaheader.h"

#include "threading.h"
#include "logging.h"
#include "luaerror.h"
#include "luastate.h"
#include "blitwizardobject.h"
#include "physics.h"
#include "objectphysicsdata.h"
#include "luafuncs_object.h"
#include "luafuncs_objectphysics.h"
#include "main.h"
#include "mathhelpers.h"

/// Blitwizard object which represents an 'entity' in the game world
/// with visual representation, behaviour code and collision shape.
// @type object

#if (defined(USE_PHYSICS2D) || defined(USE_PHYSICS3D))

void transferbodysettings(struct physicsobject* oldbody,
struct physicsobject* newbody);

// Attempt to trigger onCollision callback for a given object.
// When no callback is set by the user or if the callback succeeds,
// 1 will be returned.
// In case of a lua error in the callback, 0 will be returned and
// luacfuncs_OnError will be triggered.
static int luafuncs_trycollisioncallback(struct blitwizardobject* obj, struct blitwizardobject* otherobj, double x, double y, double z, double normalx, double normaly, double normalz, double force, int* enabled, int use3d) {
    // get global lua state we use for blitwizard (no support for multiple
    // states as of now):
    lua_State* l = luastate_GetStatePtr();

    // push all args:
    lua_checkstack(l, 10);
    luacfuncs_pushbobjidref(l, otherobj);
    lua_pushnumber(l, x);
    lua_pushnumber(l, y);
    if (use3d) {
        lua_pushnumber(l, z);
    }
    lua_pushnumber(l, normalx);
    lua_pushnumber(l, normaly);
    if (use3d) {
        lua_pushnumber(l, normalz);
    }
    lua_pushnumber(l, force);

    // attempt callback:
    if (obj->haveOnCollision) {
        int boolreturn;
        int r = luacfuncs_object_callEvent(l,
        obj, "onCollision", 6 + 2 * use3d, &boolreturn);
        if (!r) {
            // an error happened.
            // enable the collision per default:
            *enabled = 1;
            return 0;
        }
        *enabled = boolreturn;
    } else {
        *enabled = 1;
        lua_pop(l, 6 + 2 * use3d);
    }
    return 1;
}

static int luafuncs_trycollisioncallback3d(struct blitwizardobject* obj, struct blitwizardobject* otherobj, double x, double y, double z, double normalx, double normaly, double normalz, double force, int* enabled) {
    return luafuncs_trycollisioncallback(obj, otherobj, x, y, z, normalx, normaly, normalz, force, enabled, 1);
}

static int luafuncs_trycollisioncallback2d(struct blitwizardobject* obj, struct blitwizardobject* otherobj, double x, double y, double normalx, double normaly, double force, int* enabled) {
    return luafuncs_trycollisioncallback(obj, otherobj, x, y, 0, normalx, normaly, 0, force, enabled, 0);
}

// This function can throw lua out of memory errors (but no others) and should
// therefore be pcall'ed. Since we don't handle out of memory sanely anyway,
// it isn't pcalled for now: FIXME
// This function gets the information about two objects colliding, and will
// subsequently attempt to call both object's collision callbacks.
//
// The user callbacks can decide that the collision shouldn't be handled,
// in which case this function will return 0. Otherwise, it will return 1.
// If a lua error happens in the user callbacks (apart from out of memory),
// it will instant-quit blitwizard with backtrace (it will never return).
int luafuncs_globalcollision2dcallback_unprotected(void* userdata, struct physicsobject* a, struct physicsobject* b, double x, double y, double normalx, double normaly, double force) {
    // we want to track if any of the callbacks wants to ignore the collision:
    int enabled = 1;

    // get the associated blitwizard objects to the collision objects:
    struct blitwizardobject* aobj = (struct blitwizardobject*)physics_getObjectUserdata(a);
    struct blitwizardobject* bobj = (struct blitwizardobject*)physics_getObjectUserdata(b);
#ifdef VALIDATEBOBJ
    assert(strcmp(aobj->validatemagic, VALIDATEMAGIC) == 0);
    assert(strcmp(bobj->validatemagic, VALIDATEMAGIC) == 0);
#endif

    // call first object's callback:
    if (!luafuncs_trycollisioncallback2d(aobj, bobj, x, y, normalx, normaly, force, &enabled)) {
        // a lua error happened and backtrace was spilled out -> ignore and continue
    }

    // call second object's callback:
    if (!luafuncs_trycollisioncallback2d(bobj, aobj, x, y, -normalx, -normaly, force, &enabled)) {
        // a lua error happened in the callback was spilled out -> ignore and continue
    }

    // if any of the callbacks wants to ignore the collision, return 0:
    if (!enabled) {
        return 0;
    }
    return 1;
}

int luafuncs_globalcollision3dcallback_unprotected(void* userdata, struct physicsobject* a, struct physicsobject* b, double x, double y, double z, double normalx, double normaly, double normalz, double force) {
    // we want to track if any of the callbacks wants to ignore the collision:
    int enabled = 1;

    // get the associated blitwizard objects to the collision objects:
    struct blitwizardobject* aobj = (struct blitwizardobject*)physics_getObjectUserdata(a);
    struct blitwizardobject* bobj = (struct blitwizardobject*)physics_getObjectUserdata(b);

    // call first object's callback:
    if (!luafuncs_trycollisioncallback3d(aobj, bobj, x, y, z, normalx, normaly, normalz, force, &enabled)) {
        // a lua error happened and backtrace was spilled out -> ignore and continue
    }

    // call second object's callback:
    if (!luafuncs_trycollisioncallback3d(bobj, aobj, x, y, z, -normalx, -normaly, -normalz, force, &enabled)) {
        // a lua error happened in the callback was spilled out -> ignore and continue
    }

    // if any of the callbacks wants to ignore the collision, return 0:
    if (!enabled) {
        return 0;
    }
    return 1;
}

void luacfuncs_object_initialisePhysicsCallbacks(void) {
#ifdef USE_PHYSICS2D
    physics_set2dCollisionCallback(main_DefaultPhysics2dPtr(),
    &luafuncs_globalcollision2dcallback_unprotected, NULL);
#endif
}

/// Disable the physics simulation on an object. It will no longer collide
// with anything.
// @function disableCollision
int luafuncs_object_disableCollision(lua_State* l) {
    struct blitwizardobject* obj = toblitwizardobject(l, 1, 0,
    "blitwizard.object:disableCollision");
    assert(obj->refcount > 0);

    if (!obj->physics) {
        // no physics info was set, ignore
        return 0;
    }

    if (obj->physics->object) {
        // transfer position/rotation first:
        if (obj->is3d) {
#ifdef USE_PHYSICS3D
            physics_get3dRotationQuaternion(obj->physics->object,
            &(obj->rotation.quaternion.x),
            &(obj->rotation.quaternion.y),
            &(obj->rotation.quaternion.z),
            &(obj->rotation.quaternion.r));
            physics_get3dPosition(obj->physics->object,
            &(obj->x), &(obj->y), &(obj->z));
#endif
        } else {
#ifdef USE_PHYSICS2D
            physics_get2dRotation(obj->physics->object, &(
            obj->rotation.angle));
            physics_get2dPosition(obj->physics->object, &(obj->x),
            &(obj->y));
#endif
        }
            
        // delete object:
        physics_destroyObject(obj->physics->object);
        obj->physics->object = NULL;
    }
    return 0;
}

int luafuncs_enableCollision(lua_State* l, int movable) {
    struct blitwizardobject* obj = toblitwizardobject(l, 1, 1,
    "blitwizard.object:enableCollision");

    if (!obj->is3d) {
        if (obj->parallax != 1) {
            return haveluaerror(l, "cannot use collision on object with "
            "parallax effect");
        }
    }

    // validate: parameters need to be a list of shape info tables
    int argcount = lua_gettop(l)-1;
    if (argcount <= 0) {
        return haveluaerror(l, badargument1, 2,
        "blitwizard.object:enableCollision", "table", "nil");
    } else {
        // check for args to be a table
        int i = 0;
        while (i < argcount) {
            if (lua_type(l, 2+i) != LUA_TTABLE &&
            lua_type(l, 2+i) != LUA_TNIL) {
                if (i == 0) {
                    return haveluaerror(l, badargument1, 2+i,
                    "blitwizard.object:enableCollision", "table",
                    lua_strtype(l, 2+i));
                } else {
                    return haveluaerror(l, badargument1, 2+i,
                    "blitwizard.object:enableCollision", "table or nil",
                    lua_strtype(l, 2+i));
                }
            }
            i++;
        }
    }

    // construct a shape list from the given shape tables:
    struct physicsobjectshape* shapes =
    physics_createEmptyShapes(argcount);
    int i = 0;
    while (i < argcount) {
        if (lua_type(l, 2+i) != LUA_TTABLE) {
            physics_destroyShapes(shapes, argcount);
            return haveluaerror(l, badargument2, 2+i,
            "blitwizard.object:enableCollision",
            "shape parameter invalid: expected table");
        }
        lua_pushstring(l, "type");
        lua_gettable(l, 2+i);
        if (lua_type(l, -1) != LUA_TSTRING) {
            physics_destroyShapes(shapes, argcount);
            return haveluaerror(l, badargument2, 2+i,
            "blitwizard.object:enableCollision",
            "shape has invalid type: expected string");
        } else {
            // check the shape type being valid:
            const char* shapetype = lua_tostring(l, -1);
            if (obj->is3d) {
#ifndef USE_PHYSICS3D
                return haveluaerror(l, error_nophysics3d);
#else
                // see if this is a usable 3d shape:
                int isok = 0;
                if (strcmp(shapetype, "decal") == 0) {
                    isok = 1;
                    // flat 3d decal with width and height
                    int width,height;
                    lua_pushstring(l, "width");
                    lua_gettable(l, 2+i);
                    if (lua_type(l, -1) != LUA_TNUMBER) {
                        physics_destroyShapes(shapes, argcount);
                        return haveluaerror(l, badargument2, 2+i,
                        "blitwizard.object:enableCollision",
                        "shape \"decal\" needs \"width\" specified"
                        " as a number");
                    }
                    width = lua_tonumber(l, -1);
                    lua_pop(l, 1);
                    lua_pushstring(l, "height");
                    lua_gettable(l, 2+i);
                    if (lua_type(l, -1) != LUA_TNUMBER) {
                        physics_destroyShapes(shapes, argcount);
                        return haveluaerror(l, badargument2, 2+i,
                        "blitwizard.object:enableCollision",
                        "shape \"decal\" needs \"height\" specified"
                        " as a number");
                    }
                    height = lua_tonumber(l, -1);
                    lua_pop(l, 1);
                    physics_set3dShapeDecal(GET_SHAPE(shapes, i),
                    width, height);
                }
                if (strcmp(shapetype, "ball") == 0) {
                    isok = 1;
                    // 3d ball with diameter
                    int diameter;
                    lua_pushstring(l, "diameter");
                    lua_gettable(l, 2+i);
                    if (lua_type(l, -1) != LUA_TNUMBER) {
                        physics_destroyShapes(shapes, argcount);
                        return haveluaerror(l, badargument2, 2+i,
                        "blitwizard.object:enableCollision",
                        "shape \"ball\" needs \"diameter\" specified"
                        " as a number");
                    }
                    diameter = lua_tonumber(l, -1);
                    lua_pop(l, 1);
                    physics_set3dShapeBall(GET_SHAPE(shapes, i),
                    diameter);
                }
                if (strcmp(shapetype, "box") == 0 ||
                strcmp(shapetype, "elliptic ball") == 0) {
                    isok = 1;
                    // box or elliptic ball with x/y/z_size
                    int x_size,y_size,z_size;
                    lua_pushstring(l, "x_size");
                    lua_gettable(l, 2+i);
                    if (lua_type(l, -1) != LUA_TNUMBER) {
                        physics_destroyShapes(shapes, argcount);
                        return haveluaerror(l, badargument2, 2+i,
                        "blitwizard.object:enableCollision",
                        "shape \"box\" or \"elliptic ball\" needs"
                        " \"x_size\" specified as a number");
                    }
                    x_size = lua_tonumber(l, -1);
                    lua_pop(l, 1);

                    lua_pushstring(l, "y_size");
                    lua_gettable(l, 2+i);
                    if (lua_type(l, -1) != LUA_TNUMBER) {
                        physics_destroyShapes(shapes, argcount);
                        return haveluaerror(l, badargument2, 2+i,
                        "blitwizard.object:enableCollision",
                        "shape \"box\" or \"elliptic ball\" needs"
                        " \"y_size\" specified as a number");
                    }
                    y_size = lua_tonumber(l, -1);
                    lua_pop(l, 1);

                    lua_pushstring(l, "y_size");
                    lua_gettable(l, 2+i);
                    if (lua_type(l, -1) != LUA_TNUMBER) {
                        physics_destroyShapes(shapes, argcount);
                        return haveluaerror(l, badargument2, 2+i,
                        "blitwizard.object:enableCollision",
                        "shape \"box\" or \"elliptic ball\" needs"
                        " \"y_size\" specified as a number");
                    }
                    z_size = lua_tonumber(l, -1);
                    lua_pop(l, 1);

                    if (strcmp(shapetype, "box") == 0) {
                        physics_set3dShapeBox(GET_SHAPE(shapes, i),
                        x_size, y_size, z_size);
                    } else {
                        physics_set3dShapeBox(GET_SHAPE(shapes, i),
                        x_size, y_size, z_size);
                    }
                }
                if (!isok || 1) {  // || 1 : disable 3d shapes for now
                    // not a valid shape for a 3d object
                    char invalidshape[50];
                    snprintf(invalidshape, sizeof(invalidshape),
                    "not a valid shape for a 3d object: \"%s\"", shapetype);
                    invalidshape[sizeof(invalidshape)-1] = 0;
                    physics_destroyShapes(shapes, argcount);
                    return haveluaerror(l, badargument2, 2+i,
                    "blitwizard.object:enableCollision",
                    invalidshape);
                }
#endif
            } else {
#ifndef USE_PHYSICS2D
                return haveluaerror(l, error_nophysics2d);
#else
                // see if this is a usable 2d shape:
                int isok = 0;
                if (strcmp(shapetype, "rectangle") == 0 ||
                strcmp(shapetype, "oval") == 0) {
                    isok = 1;
                    // rectangle or oval with width and height
                    double width,height;
                    lua_pushstring(l, "width");
                    lua_gettable(l, 2+i);
                    if (lua_type(l, -1) != LUA_TNUMBER ||
                    lua_tonumber(l, -1) <= 0) {
                        physics_destroyShapes(shapes, argcount);
                        return haveluaerror(l, badargument2, 2+i,
                        "blitwizard.object:enableCollision",
                        "shape \"rectangle\" or \"oval\" needs"
                        " \"width\" specified as a positive number");
                    }
                    width = lua_tonumber(l, -1);
                    lua_pop(l, 1);

                    lua_pushstring(l, "height");
                    lua_gettable(l, 2+i);
                    if (lua_type(l, -1) != LUA_TNUMBER ||
                    lua_tonumber(l, -1) <= 0) {
                        physics_destroyShapes(shapes, argcount);
                        return haveluaerror(l, badargument2, 2+i,
                        "blitwizard.object:enableCollision",
                        "shape \"rectangle\" or \"oval\" needs"
                        " \"height\" specified as a number");
                    }
                    height = lua_tonumber(l, -1);
                    lua_pop(l, 1);

                    if (strcmp(shapetype, "oval") == 0) {
                        physics_set2dShapeOval(GET_SHAPE(shapes, i),
                        width, height);
                    } else {
                        physics_set2dShapeRectangle(GET_SHAPE(shapes, i),
                        width, height);
                    }
                }
                if (strcmp(shapetype, "polygon") == 0) {
                    // polygon with a multiple points list
                    isok = 1;

                    lua_pushstring(l, "points");
                    lua_gettable(l, 2+i);
                    if (lua_type(l, -1) != LUA_TTABLE) {
                        physics_destroyShapes(shapes, argcount);
                        return haveluaerror(l, badargument2, 2+i,
                        "blitwizard.object:enableCollision",
                        "shape \"polygon\" needs \"points\" specified "
                        "as a list table containing a list of "
                        "coordinate points");
                    }
                    if (lua_rawlen(l, -1) < 3) {
                        physics_destroyShapes(shapes, argcount);
                        return haveluaerror(l, badargument2, 2+i,
                        "blitwizard.object:enableCollision",
                        "shape \"polygon\" needs a \"points\" list with "
                        "at least 3 points or more");
                    }
                    if (lua_rawlen(l, -1) > 8) {
                        physics_destroyShapes(shapes, argcount);
                        return haveluaerror(l, badargument2, 2+i,
                        "blitwizard.object:enableCollision",
                        "shape \"polygon\" doesn't support more than 8 "
                        "polygon points");
                    }

                    // this large loop will extract all points and
                    // verify the points list is a valid list of
                    // purely clockwise or counter-clockwise points
                    int pointCount = 0;
                    int ccwPolygon = 0;
                    lua_pushnil(l);
                    double px[8];
                    double py[8];
                    while (lua_next(l, -2)) {
                        // each list item needs to be a table with two numbers in it:
                        if (lua_type(l, -1) != LUA_TTABLE
                        || lua_rawlen(l, -1) != 2) {
                            physics_destroyShapes(shapes, argcount);
                            char msg[512];
                            snprintf(msg, sizeof(msg),
                            "the \"points\" list specified "
                            "has an invalid point #%d - "
                            "not a table with two numbers",
                            pointCount+1);
                            return haveluaerror(l, badargument2, 2+i,
                            "blitwizard.object:enableCollision",
                            msg);
                        }
                        // verify the two table items to be numbers:
                        lua_pushnumber(l, 1);
                        lua_gettable(l, -2);
                        if (lua_type(l, -1) != LUA_TNUMBER) {
                            physics_destroyShapes(shapes, argcount);
                            char msg[512];
                            snprintf(msg, sizeof(msg),
                            "the \"points\" list specified "
                            "has an invalid point #%d - "
                            "first coordinate not a number",
                            pointCount+1);
                            return haveluaerror(l, badargument2, 2+i,
                            "blitwizard.object:enableCollision",
                            msg);
                        }
                        px[pointCount] = lua_tonumber(l, -1);
                        lua_pop(l, 1);
                        lua_pushnumber(l, 2);
                        lua_gettable(l, -2);
                        if (lua_type(l, -1) != LUA_TNUMBER) {
                            physics_destroyShapes(shapes, argcount);
                            char msg[512];
                            snprintf(msg, sizeof(msg),
                            "the \"points\" list specified "
                            "has an invalid point #%d - "
                            "second coordinate not a number",
                            pointCount+1);
                            return haveluaerror(l, badargument2, 2+i,
                            "blitwizard.object:enableCollision",
                            msg);
                        }
                        py[pointCount] = lua_tonumber(l, -1);
                        lua_pop(l, 1);

                        // verify this not to be the same point as a previous one:
                        int j = 0;
                        while (j < pointCount) {
                            if (px[j] == px[pointCount] && py[j] == py[pointCount]) {
                                physics_destroyShapes(shapes, argcount);
                                char msg[512];
                                snprintf(msg, sizeof(msg),
                                "the \"points\" list specified "
                                "has an invalid point #%d - "
                                "point is a duplicate of a previous one",
                                pointCount+1);
                                return haveluaerror(l, badargument2, 2+i,
                                "blitwizard.object:enableCollision",
                                msg);
                            }
                            j++;
                        }

                        // verify this to be a valid convex hull point
                        if (pointCount > 1) {
                            if (pointCount == 2) {
                                // this point determines whether the whole polygon is ccw or not:
                                ccwPolygon = pointisccw(px[0], py[0], px[1], py[1], px[2], py[2]);
                            } else {
                                // verify this point is ccw or non-ccw as the polygon demands:
                                if (ccwPolygon != pointisccw(
                                px[pointCount-2], py[pointCount-2],
                                px[pointCount-1], py[pointCount-1],
                                px[pointCount], py[pointCount])) {
                                    physics_destroyShapes(shapes, argcount);
                                    char msg[512];
                                    snprintf(msg, sizeof(msg),
                                    "the \"points\" list specified "
                                    "has an invalid point #%d - "
                                    "should be %s but it is not",
                                    pointCount+1, ccwPolygon ? "counter-clockwise" : "clockwise");
                                    return haveluaerror(l, badargument2, 2+i,
                                    "blitwizard.object:enableCollision",
                                    msg);
                                }
                            }
                        }

                        // done. advance to next point:
                        pointCount++;
                        lua_pop(l, 1);  // pop point coordinates table
                    }
                    lua_pop(l, 1);  // pop "points" table

                    // at this point, we need to check that no line
                    // between any points intersects with another one:
                    // FIXME: do that

                    // add points to the shape:
                    if (ccwPolygon) {
                        // add in the order we got them:
                        int k = 0;
                        while (k < pointCount) {
                            physics_add2dShapePolygonPoint(
                            GET_SHAPE(shapes, i), px[k], py[k]);
                            k++;
                        }
                    } else {
                        // add in reverse order so physics get a nice ccw shape:
                        int k = pointCount - 1;
                        while (k > 0) {
                            physics_add2dShapePolygonPoint(
                            GET_SHAPE(shapes, i), px[k], py[k]);
                            k--;
                        }
                    }
                }
                if (strcmp(shapetype, "edge list") == 0) {
                    // polygon with a multiple points list
                    isok = 1;

                    lua_pushstring(l, "edges");
                    lua_gettable(l, 2+i);
                    if (lua_type(l, -1) != LUA_TTABLE) {
                        physics_destroyShapes(shapes, argcount);
                        return haveluaerror(l, badargument2, 2+i,
                        "blitwizard.object:enableCollision",
                        "shape \"edge list\" needs \"edges\" specified "
                        "as a list table containing a list of "
                        "coordinate point pairs (see documentation)");
                    }
                    if (lua_rawlen(l, -1) < 1) {
                        physics_destroyShapes(shapes, argcount);
                        return haveluaerror(l, badargument2, 2+i,
                        "blitwizard.object:enableCollision",
                        "shape \"edge list\" needs an \"edges\" list with "
                        "at least 1 pair of points or more");
                    }

                    // this large loop will extract all point pairs
                    int pointPairCount = 0;
                    lua_pushnil(l);
                    double px[8];
                    double py[8];
                    while (lua_next(l, -2)) {
                        // each list item needs to be a table
                        // with two numbers in it:
                        if (lua_type(l, -1) != LUA_TTABLE
                        || lua_rawlen(l, -1) != 2) {
                            physics_destroyShapes(shapes, argcount);
                            char msg[512];
                            snprintf(msg, sizeof(msg),
                            "the \"edges\" list specified "
                            "has an invalid point pair #%d - "
                            "not a table with two point tables",
                            pointPairCount+1);
                            return haveluaerror(l, badargument2, 2+i,
                            "blitwizard.object:enableCollision",
                            msg);
                        }
                        // look at the two point tables of the pair:
                        double coords[2][2];
                        int c = 0;
                        while (c < 2) {
                            lua_pushnumber(l, c + 1);
                            lua_gettable(l, -2);
                            static char first[] = "first";
                            static char second[] = "second";
                            const char* firstsecond = first;
                            if (c == 1) {
                                firstsecond = second;
                            }
                            if (lua_type(l, -1) != LUA_TTABLE
                            || lua_rawlen(l, -1) != 2) {
                                physics_destroyShapes(shapes, argcount);
                                char msg[512];
                                snprintf(msg, sizeof(msg),
                                "the \"edges\" list specified "
                                "has an invalid point pair #%d - "
                                "%s point not a table of length 2",
                                pointPairCount+1, firstsecond);
                                return haveluaerror(l, badargument2, 2+i,
                                "blitwizard.object:enableCollision",
                                msg);
                            }
                            // extract coordinates of this point:
                            int c2 = 0;
                            while (c2 < 2) {
                                const char* firstsecond2 = first;
                                if (c2 == 1) {
                                    firstsecond2 = second;
                                }
                                lua_pushnumber(l, c2 + 1);
                                lua_gettable(l, -2);
                                if (lua_type(l, -1) != LUA_TNUMBER) {
                                    physics_destroyShapes(shapes, argcount);
                                    char msg[512];
                                    snprintf(msg, sizeof(msg),
                                    "the \"edges\" list specified "
                                    "has an invalid point pair #%d - "
                                    "%s point's %s coordinate not a number",
                                    pointPairCount+1, firstsecond, firstsecond2);
                                    return haveluaerror(l, badargument2, 2+i,
                                    "blitwizard.object:enableCollision",
                                    msg);
                                }
                                // extract point:
                                coords[c][c2] = lua_tonumber(l, -1);
                                // pop coordinate from stack:
                                lua_pop(l, 1);
                                c2++;
                            }
                            // pop point table from stack:
                            lua_pop(l, 1);
                            c++;
                        }
                        pointPairCount++;

                        // add two point pair as edge to the shape:
                        physics_add2dShapeEdgeList(GET_SHAPE(shapes, i),
                        coords[0][0], coords[0][1], coords[1][0], coords[1][1]);

                        lua_pop(l, 1);  // pop two point pair table
                    }
                    lua_pop(l, 1);  // pop "edge" table
                }
                if (strcmp(shapetype, "circle") == 0) {
                    isok = 1;
                    // rectangle or oval with width and height
                    double diameter;
                    lua_pushstring(l, "diameter");
                    lua_gettable(l, 2+i);
                    if (lua_type(l, -1) != LUA_TNUMBER) {
                        physics_destroyShapes(shapes, argcount);
                        return haveluaerror(l, badargument2, 2+i,
                        "blitwizard.object:enableCollision",
                        "shape \"circle\" needs \"diameter\" specified"
                        " as a number");
                    }
                    diameter = lua_tonumber(l, -1);
                    lua_pop(l, 1);

                    physics_set2dShapeCircle(GET_SHAPE(shapes, i),
                    diameter);
                }
                if (!isok) {
                    // not a valid shape for a 2d object
                    char invalidshape[50];
                    snprintf(invalidshape, sizeof(invalidshape),
                    "not a valid shape for a 2d object: \"%s\"", shapetype);
                    invalidshape[sizeof(invalidshape)-1] = 0;
                    physics_destroyShapes(shapes, argcount);
                    return haveluaerror(l, badargument2, 2+i,
                    "blitwizard.object:enableCollision",
                    invalidshape);
                }
#endif
            }
            lua_pop(l, 1);  // pop shapetype string
        }
        i++;
    }

    // prepare physics data:
    if (!obj->physics) {
        obj->physics = malloc(sizeof(struct objectphysicsdata));
        memset(obj->physics, 0, sizeof(*(obj->physics)));
    }

    // remember the old representation if any:
    struct physicsobject* old = obj->physics->object;

    // create a physics object from the shapes:
    obj->physics->object = physics_createObject(main_DefaultPhysics2dPtr(),
    obj, movable, shapes, argcount);
    physics_destroyShapes(shapes, argcount);

    // destroy old representation after transferring settings:
    if (old) {
        transferbodysettings(old, obj->physics->object);
        physics_destroyObject(old);
    } else {
        // if no old representation, transfer over the current position:
        if (obj->is3d) {
#ifdef USE_PHYSICS3D
            physics_warp3d(obj->physics->object,
            obj->x, obj->y, obj->z,
            obj->rotation.quaternion.x, obj->rotation.quaternion.y,
            obj->rotation.quaternion.z, obj->rotation.quaternion.r);
#endif
        } else {
#ifdef USE_PHYSICS2D
            physics_warp2d(obj->physics->object,
            obj->x, obj->y, obj->rotation.angle);
#endif
        }
    }
    obj->physics->movable = movable;

    // remember the size at which we initialised the shape:
    if (obj->is3d) {
        obj->physics->pinitx = obj->scale3d.x;
        obj->physics->pinity = obj->scale3d.y;
        obj->physics->pinitz = obj->scale3d.z;
    } else {
        obj->physics->pinitx = obj->scale2d.x;
        obj->physics->pinity = obj->scale2d.y;
    }
    obj->physics->phullx = obj->physics->pinitx;
    obj->physics->phully = obj->physics->pinity;
    obj->physics->phullz = obj->physics->pinitz;
    return 1;
}

void luacfuncs_object_handleScalingForPhysics(
struct blitwizardobject* obj) {
    if (!obj->physics || !obj->physics->object) {
        // object has no shape, nothing to scale here.
        return;
    }

    // first, check if we want to rescale things at all:
    int dorescale = 0;
    double dif = 0.05;
    if (obj->is3d) {
        if (fabs(obj->scale3d.x - obj->physics->phullx) > dif) {
            dorescale = 1;
        }
        if (fabs(obj->scale3d.y - obj->physics->phully) > dif) {
            dorescale = 1;
        }
        if (fabs(obj->scale3d.z - obj->physics->phullz) > dif) {
            dorescale = 1;
        }
    } else {
        if (fabs(obj->scale2d.x - obj->physics->phullx) > dif) {
            dorescale = 1;
        }
        if (fabs(obj->scale2d.y - obj->physics->phully) > dif) {
            dorescale = 1;
        }
    }
    if (!dorescale) {
        // nothing to do.
        return;
    }
    // calculate the new scale factors we need:
    double newscfx, newscfy, newscfz;
    if (obj->is3d) {
        newscfx = obj->scale3d.x / obj->physics->pinitx;
        newscfy = obj->scale3d.y / obj->physics->pinity;
        newscfz = obj->scale3d.z / obj->physics->pinitz;
    } else {
        newscfx = obj->scale2d.x / obj->physics->pinitx;
        newscfy = obj->scale2d.y / obj->physics->pinity;
    }
    // aply new scale:
    if (!obj->is3d) {
#ifdef USE_PHYSICS2D
        physics_set2dScale(obj->physics->object, newscfx, newscfy);
#endif
    } else {
#ifdef USE_PHYSICS3D
        physics_set3dScale(obj->physics->object,
        newscfx, newscfy, newscfz);
#endif
    }
    // remember we did this scaling:
    obj->physics->phullx = newscfx;
    obj->physics->phully = newscfy;
    if (obj->is3d) {
        obj->physics->phullz = newscfz;
    }
}

/// This is how you should submit shape info to
// @{object:enableStaticCollision} and
// @{object:enableMovableCollision}
// (THIS TABLE DOESN'T EXIST, it is just a
// guide on how to construct it yourself)
//
// All shape sizes, dimensions etc are specified in game units.
//
// <b>Note on object scaling:</b>
//
// When you @{blitwizard.object:enableMovableCollision|enable collision},
// the collision shape will be exactly as large
// as specified on creation, no matter how large the current
// @{blitwizard.object:setScale|scaling} is
// (that is, you can directly set any sizes from
// @{blitwizard.object:getDimensions|object:getDimensions} if you want).
//
// As soon as your object is scaled after collision was enabled, the
// physics collision shape will be scaled accordingly.
// (e.g. if your object scale goes from 2 to 4, the physics hull will
// double in size aswell)
//
// <b>Special notes on various shapes:</b>
//
// Please note the shapes "edge list" and "triangle mesh" may only be used
// for static collision. They don't work with movable collision objects.
//
// <b>Combining shapes:</b>
//
// You can combine shapes when enabling collision,
// by specifying multiple shapes (the final collision shape will be
// all those shapes merged into one).
// @tfield string type The shape type, for 2d shapes: "rectangle", "circle", "oval", "polygon" (needs to be convex!), "edge list" (simply a list of lines that don't need to be necessarily connected as it is for the polygon), for 3d shapes: "decal" (= 3d rectangle), "box", "ball", "elliptic ball" (deformed ball with possibly non-uniform radius, e.g. rather a capsule), "triangle mesh" (a list of 3d triangles)
// @tfield number width required for "rectangle", "oval" and "decal"
// @tfield number height required for "rectangle", "oval" and "decal"
// @tfield number diameter required for "circle" and "ball"
// @tfield number x_size required for "box" and "elliptic ball"
// @tfield number y_size required for "box" and "elliptic ball"
// @tfield number z_size required for "box" and "elliptic ball"
// @tfield table points required for "polygon": a list of two pair coordinates which specify the corner points of the polygon, e.g. { { 0, 0 }, { 1, 0 }, { 0, 1 } }  (keep in mind the polygon needs to be <a href="http://en.wikipedia.org/wiki/Convex_polygon">convex!</a>)
// @tfield table edges required for "edge list": a list of edges, whereas an edge is itself a 2-item list of two 2d points, each 2d point being a list of two coordinates. Both convex and concave shapes are supported. Example: { { { 0, 0 }, { 1, 0 } }, { { 0, 1 }, { 1, 1 } } }
// @tfield table triangles required for "triangle mesh": a list of triangles, whereas a triangle is itself a 3-item list of three 3d points, each 3d point being a list of three coordinates.
// @tfield number x_offset (optional) x coordinate offset for any 2d or 3d shape, defaults to 0
// @tfield number y_offset (optional) y coordinate offset for any 2d or 3d shape, defaults to 0
// @tfield number z_offset (optional) z coordinate offset for any 3d shape, defaults to 0
// @tfield number rotation (optional) rotation of any 2d shape 0..360 degree (defaults to 0)
// @tfield number rotation_pan (optional) rotation of any 3d shape 0..360 degree left and right (horizontally)
// @tfield number rotation_tilt (optional) rotation of any 3d shape 0..360 degree up and down, applied after horizontal rotation
// @tfield number rotation_roll (optional) rotation of any 3d shape 0..360 degree around itself while remaining faced forward (basically overturning/leaning on the side), applied after the horizontal and vertical rotations
// @table shape_info
// @usage
// -- Create a new 2d object from an image:
// local myobject = blitwizard.object:new(blitwizard.object.o2d, "someimage.png")
//
// -- Enable collision for our object as soon as its size is known:
// function myobject:onGeometryLoaded()
//     -- set collision size exactly to the dimensions of the image:
//     local w,h = myobject:getDimensions()
//     myobject:enableMovableCollision({type='rectangle', width=w, height=h})
// end

/// Enable the physics simulation on the given object and make it
// movable and collide with other movable and static objects.
// You will be required to
// provide shape information that specifies the desired collision shape
// of the object (not necessarily similar to its visual appearance).
//
// Note: some complex shape types are unavailable for
// movable objects, and some shapes (very thin/long, very complex or
// very tiny or huge) can be unstable for movables. Create all your movable objects
// roughly of sizes between 0.1 and 10 to avoid instability.
// @function enableMovableCollision
// @tparam table shape_info a @{object.shape_info|shape_info} table with info for a given physics shape. Note: you can add more shape info tables as additional parameters following this one - the final collision shape will consist of all overlapping shapes
int luafuncs_object_enableMovableCollision(lua_State* l) {
    return luafuncs_enableCollision(l, 1);
}

/// Enable the physics simulation on the given object and allow other
// objects to collide with it. The object itself will remain static
// - this is useful for immobile level geometry. You will be required to
// provide shape information that specifies the desired collision shape
// of the object (not necessarily similar to its visual appearance).
// @function enableStaticCollision
// @tparam table shape_info a @{object.shape_info|shape_info} table with info for a given physics shape. Note: you can add more shape info tables as additional parameters following this one - the final collision shape will consist of all overlapping shapes
int luafuncs_object_enableStaticCollision(lua_State* l) {
    return luafuncs_enableCollision(l, 0);
}

int luafuncs_freeObjectPhysicsData(struct objectphysicsdata* d) {
    // free the given physics data
    if (d->object) {
        // void collision callback
        /*char funcname[200];
        snprintf(funcname, sizeof(funcname), "collisioncallback%p",
        d->object);
        funcname[sizeof(funcname)-1] = 0;
        lua_pushstring(l, funcname);
        lua_pushnil(l);
        lua_settable(l, LUA_REGISTRYINDEX);*/

        // delete physics body
        physics_destroyObject(d->object);
        d->object = NULL;
    }
    free(d);
    return 0;
}

static void applyobjectsettings(struct blitwizardobject* obj) {
    if (!obj->physics || !obj->physics->object) {
        return;
    }
    if (obj->is3d) {
        if (obj->physics->rotationrestriction3dfull) {
#ifdef USE_PHYSICS3D
            physics_set3dRotationRestrictionAllAxis(obj->physics->object);
#endif
        } else {
            if (obj->physics->rotationrestriction3daxis) {
#ifdef USE_PHYSICS3D
                physics_set3dRotationRestrictionAroundAxis(
                obj->physics->object,
                obj->physics->rotationrestriction3daxisx,
                obj->physics->rotationrestriction3daxisy,
                obj->physics->rotationrestriction3daxisz);
#endif
            } else {
#ifdef USE_PHYSICS3D
                physics_set3dNoRotationRestriction();
#endif
            }
        }
    } else {
        physics_set2dRotationRestriction(obj->physics->object,
        obj->physics->rotationrestriction2d);
    }
    physics_setRestitution(obj->physics->object, obj->physics->restitution);
    physics_setFriction(obj->physics->object, obj->physics->friction);
    physics_setAngularDamping(obj->physics->object,
    obj->physics->angulardamping);
    physics_setLinearDamping(obj->physics->object,
    obj->physics->lineardamping);
}

/// Apply a physics impulse onto an object (which will make it move,
// for example as if someone had pushed it).
//
// Use this instead of @{blitwizard.object:setPosition|object:setPosition}
// for movement that takes collisions into account!
//
// This will only work if the object has movable collision enabled through
// @{object:enableMovableCollision|object:enableMovableCollision}.
//
// <b>Important</b>: Some parameters are not present for 2d objects, see list below.
// @function impulse
// @tparam number force_x the x coordinate of the force vector applied through the impulse
// @tparam number force_y the y coordinate of the force vector
// @tparam number force_z <i>(parameter only present for 3d objects)</i> the z coordinate of the force vector
// @tparam number source_x (optional, defaults to object's x position) the x source coordinate from where the push will be given
// @tparam number source_y (optional, defaults to object's y position) the y source coordinate
// @tparam number source_z (optional, defaults to object's z position) <i>(parameter only present for 3d objects)</i> the z source coordinate
// @usage
// -- apply an upward impulse to a 2d object with collsion enabled:
// obj:impulse(0, -1)
int luafuncs_object_impulse(lua_State* l) {
    struct blitwizardobject* obj = toblitwizardobject(l, 1, 0,
    "blitwizard.object:impulse");
    char funcname[] = "blitwizard.object.impulse";
    if (!obj->physics || !obj->physics->object) {
        lua_pushstring(l, "object has no shape");
        return lua_error(l);
    }
    if (!obj->physics->movable) {
        lua_pushstring(l, "impulse can be only applied to movable objects");
        return lua_error(l);
    }
    // validate force parameters:
    if (lua_type(l, 2) != LUA_TNUMBER) {  // force x
        return haveluaerror(l, badargument1, 1, funcname, "number",
        lua_strtype(l, 2));
    }
    if (lua_type(l, 3) != LUA_TNUMBER) {  // force y
        return haveluaerror(l, badargument1, 2, funcname, "number",
        lua_strtype(l, 3));
    }
    if (obj->is3d) {
        if (lua_type(l, 4) != LUA_TNUMBER) {  // force z
            return haveluaerror(l, badargument1, 3, funcname,
            "number", lua_strtype(l, 4));
        }
    }
    // see if we have a source parameter:
    int havesource = 0;
    if ((obj->is3d && lua_gettop(l) > 4) || (!obj->is3d && lua_gettop(l) > 3)) {
        // validate source parameters:
        havesource = 1;
        if (lua_type(l, 4+obj->is3d) != LUA_TNUMBER) { // source x
            return haveluaerror(l, badargument1, 3+obj->is3d, funcname, "number",
            lua_strtype(l, 4+obj->is3d));
        }
        if (lua_type(l, 5+obj->is3d) != LUA_TNUMBER) { // source y
            return haveluaerror(l, badargument1, 4+obj->is3d, funcname, "number",
            lua_strtype(l, 5+obj->is3d));
        }
        if (obj->is3d) {
            if (lua_type(l, 7) != LUA_TNUMBER) { // source z
                return haveluaerror(l, badargument1, 6, funcname, "number",
                lua_strtype(l, 7));
            }
        }
    }
    double sourcex,sourcey,sourcez;
    double forcex, forcey, forcez;
    // get force:
    forcex = lua_tonumber(l, 2);
    forcey = lua_tonumber(l, 3);
    forcez = 0;
    if (obj->is3d) {
        forcez = lua_tonumber(l, 4);
    }
    // get source:
    if (havesource) {
        sourcex = lua_tonumber(l, 4+obj->is3d);
        sourcey = lua_tonumber(l, 5+obj->is3d);
        sourcez = 0;
        if (obj->is3d) {
            forcez = lua_tonumber(l, 7);
        }
    } else {
        // initialise source to object position:
        sourcez = 0;
        objectphysics_getPosition(obj, &sourcex, &sourcey, &sourcez);
    }
    // apply impulse:
    if (obj->is3d) {
#ifdef USE_PHYSICS3D
        physics_apply3dImpulse(obj->physics->object,
        forcex, forcey, forcez, sourcex, sourcey, sourcez);
#endif
    } else {
        physics_apply2dImpulse(obj->physics->object,
        forcex, forcey, sourcex, sourcey);
    }
    return 0;
}


/// Apply a rotational impulse onto a 2d object (which will make it rotate as if pushed).
// While @{blitwizard.object:setRotationAngle|object:setRotationAngle} might seem easier,
// this will actually take obstacles and collision into account.
//
// Use @{blitwizard.object:angularImpulse3d|object:angularImpulse3d} for 3d objects.
//
// This function will only work if the object has movable collision enabled through
// @{object:enableMovableCollision|object:enableMovableCollision}.
// @function angularImpulse2d
// @tparam number force the rotational force (positive for counter-clockwise, negative for clockwise)
// @usage
// -- apply an angular impulse to a 2d object with collsion enabled,
// -- and make it turn left:
// obj:angularImpulse2d(0.2)
int luafuncs_object_angularImpulse2d(lua_State* l) {
    struct blitwizardobject* obj = toblitwizardobject(l, 1, 0,
    "blitwizard.object:impulse");
    char funcname[] = "blitwizard.object.impulse";
    if (!obj->physics || !obj->physics->object) {
        lua_pushstring(l, "object has no shape");
        return lua_error(l);
    }
    if (!obj->physics->movable) {
        lua_pushstring(l, "impulse can be only applied to movable objects");
        return lua_error(l);
    }
    if (obj->is3d) {
        return haveluaerror(l, "you may only use this on 2d objects");
    }
    // validate force parameter:
    if (lua_type(l, 2) != LUA_TNUMBER) {  // force x
        return haveluaerror(l, badargument1, 1, funcname, "number",
        lua_strtype(l, 2));
    }
    // get force:
    double force = lua_tonumber(l, 2);

    // apply impulse:
    physics_apply2dAngularImpulse(obj->physics->object, -force);
    return 0;
}


/// Restrict the ability to rotate for a given object. For 2d, the rotation
// can be totally restricted or not, for 3d it can be restricted around a specific
// axis (e.g. like a door), completely (object can not rotate at all), or not at all.
//
// For 2d and complete 3d restriction, specify 'true'. For no restriction, 'false'.
//
// For 3d restriction around a specific axis (only available for 3d objects obviously),
// specify 3 coordinates for an axis direction vector, then optionally an additional 3 coordinates
// for the point where the axis should go through (if not specified the object's center).
// @function restrictRotation
// @tparam number/boolean total_or_axis_x Specify true or false for either full restriction or none, or the first coordinate of an axis
// @tparam number axis_y If the first parameter was the first coordinate of an axis, specify the second here
// @tparam number axis_z If the first parameter was the first coordinate of an axis, specify the third here
// @tparam number axis_point_x (optional) If you want to specify a point the axis goes through, specify its x coordinate here
// @tparam number axis_point_y (optional) y coordinate
// @tparam number axis_point_z (optional) z coordinate
int luafuncs_object_restrictRotation(lua_State* l) {
    char func[] = "blitwizard.object:restrictRotation";
    struct blitwizardobject* obj = toblitwizardobject(l, 1, 0,
    "blitwizard.object:restrictRotation");
    if (!obj->physics || !obj->physics->object) {
        lua_pushstring(l, "object has no shape");
        return lua_error(l);
    }
    if (!obj->is3d) {
        if (lua_type(l, 2) != LUA_TBOOLEAN) {
            lua_pushstring(l, "Second parameter is not a valid rotation restriction boolean");
            return lua_error(l);
        }
        if (!obj->physics->movable) {
            lua_pushstring(l, "Rotation restriction can be only set on movable objects");
            return lua_error(l);
        }
        obj->physics->rotationrestriction2d = lua_toboolean(l, 2);
        applyobjectsettings(obj);
    } else {
        if (lua_type(l, 2) != LUA_TBOOLEAN && lua_type(l, 2) != LUA_TNUMBER) {
            return haveluaerror(l, badargument2, 1, func, "string or number", lua_strtype(l, 2));
        }
        if (!obj->physics->movable) {
            return haveluaerror(l, "Rotation restriction can only be set on movable objects");
        }
        if (lua_type(l, 2) == LUA_TBOOLEAN) {
            obj->physics->rotationrestriction3dfull = lua_toboolean(l, 2);
            obj->physics->rotationrestriction3daxis = 0;
        } else {
            if (lua_type(l, 3) != LUA_TNUMBER) {
                return haveluaerror(l, badargument2, 2, func, "number", lua_strtype(l, 3));
            }
            if (lua_type(l, 4) != LUA_TNUMBER) {
                return haveluaerror(l, badargument2, 3, func, "number", lua_strtype(l, 3));
            }
            obj->physics->rotationrestriction3dfull = 0;
            obj->physics->rotationrestriction3daxis = 1;
            obj->physics->rotationrestriction3daxisx = lua_tonumber(l, 2);
            obj->physics->rotationrestriction3daxisy = lua_tonumber(l, 3);
            obj->physics->rotationrestriction3daxisz = lua_tonumber(l, 4);
        }
        applyobjectsettings(obj);
    }
    return 0;
}

/// Set the angular damping factor. Angular damping
// is a 'slow down' which is constantly applied to rotation speed,
// so rotating objects with high angular damping will stop rotating
// after a while, even if not colliding with other objects.
// Angular damping is meaningless for objects without movable collision
// enabled.
// @function setAngularDamping
// @tparam number damping Angular damping factor from 0 (none) to 1 (full)
int luafuncs_object_setAngularDamping(lua_State* l) {
    struct blitwizardobject* obj = toblitwizardobject(l, 1, 0,
    "blitwizard.object:setAngularDamping");
    if (!obj->physics) {
        lua_pushstring(l, "object has no shape");
        return lua_error(l);
    }
    if (lua_type(l, 2) != LUA_TNUMBER) {
        return haveluaerror(l, badargument1, 1,
        "blitwizard.object:setAngularDamping",
        "number", lua_strtype(l, 2));
    }
    obj->physics->angulardamping = lua_tonumber(l, 2);
    applyobjectsettings(obj);
    return 0;
}

/// Set the surface friction on the given object.
// Only useful on objects with collision enabled.
// @function setFriction
// @tparam number friction Friction value from 0 to 1 (0 is no friction, 1 is full friction)
int luafuncs_object_setFriction(lua_State* l) {
    struct blitwizardobject* obj = toblitwizardobject(l, 1, 0,
    "blitwizard.object:setFriction");
    if (lua_type(l, 2) != LUA_TNUMBER) {
        return haveluaerror(l, badargument1, 1,
        "blitwizard.object:setFriction",
        "number", lua_strtype(l, 2));
    }
    double friction = lua_tonumber(l, 2);
    obj->physics->friction = friction;
    applyobjectsettings(obj);
    return 0;
}

/// Set a gravity vector onto an object with movable collision enabled
// (see @{object:enableMovableCollision|object:enableMovableCollision}).
// If no parameters are provided,
// the object gravity will be removed again.
// Please note you would normally want to use
// @{blitwizard.physics.set2dGravity|blitwizard.physics.set2dGravity} or
// @{blitwizard.physics.set3dGravity|blitwizard.physics.set3dGravity}
// instead.
// @function setGravity
// @tparam number gravity_x x coordinate of gravity vector
// @tparam number gravity_y y coordinate of gravity vector
// @tparam number gravity_z (only for 3d objects) z coordinate of gravity vector
int luafuncs_object_setGravity(lua_State* l) {
    struct blitwizardobject* obj = toblitwizardobject(l, 1, 0,
    "blitwizard.object:restrictRotation");
    if (!obj->physics || !obj->physics->object) {
        lua_pushstring(l, "object has no shape");
        return lua_error(l);
    }

    int set = 0;
    double gx,gy,gz;

    // if the necessary amount of vector coordinates is given,
    // set new gravity vector:
    if ((!obj->is3d && lua_gettop(l) >= 3 && lua_type(l, 3) != LUA_TNIL) ||
    (obj->is3d && lua_gettop(l) >= 4 && lua_type(l, 4) != LUA_TNIL)) {
        if (lua_type(l, 2) != LUA_TNUMBER) {
            return haveluaerror(l, badargument1, 1,
            "blitwizard.object:setGravity", "number", lua_strtype(l, 2));
        }
        if (lua_type(l, 3) != LUA_TNUMBER) {
            return haveluaerror(l, badargument1, 2,
            "blitwizard.object:setGravity", "number", lua_strtype(l, 3));
        }
        if (obj->is3d && lua_type(l, 4) != LUA_TNUMBER) {
            return haveluaerror(l, badargument1, 3,
            "blitwizard.object:setGravity", "number", lua_strtype(l, 4));
        }
        gx = lua_tonumber(l, 2);
        gy = lua_tonumber(l, 3);
        if (obj->is3d) {
            gz = lua_tonumber(l, 4);
        }
        set = 1;
    }
    if (set) {
        if (obj->is3d) {
#ifdef USE_PHYSICS3D
            physics_set3dGravity(obj->physics->object, gx, gy, gz);
#endif
        } else {
            physics_set2dGravity(obj->physics->object, gx, gy);
        }
    } else {
        physics_unsetGravity(obj->physics->object);
    }
    return 0;
}

/// Set the linear damping factor. Linear damping
// is a 'slow down' which is constantly applied to the movement speed
// so moving objects with high linear damping will eventually stop moving
// even if never colliding with any other objects (assuming there is no
// notable gravity).
// Linear damping is meaningless for objects without movable collision
// enabled.
// @function setLinearDamping
// @tparam number damping Linear damping factor from 0 (none) to 1 (full). Default: 0
int luafuncs_object_setLinearDamping(lua_State* l) {
    struct blitwizardobject* obj = toblitwizardobject(l, 1, 0,
    "blitwizard.object:setLinearDamping");
    if (!obj->physics) {
        lua_pushstring(l, "object has no shape");
        return lua_error(l);
    }
    if (lua_type(l, 2) != LUA_TNUMBER) {
        return haveluaerror(l, badargument1, 1,
        "blitwizard.object:setLinearDamping",
        "number", lua_strtype(l, 2));
    }
    obj->physics->lineardamping = lua_tonumber(l, 2);
    applyobjectsettings(obj);
    return 0;
}

/// Get the mass of an object. This is only applicable
// for objects with movable collision enabled.
//
// The mass of an object may be altered with
// @{blitwizard.object:setMass|setMass}.
// @function getMass
// @treturn number mass The mass of the object in kilograms.
// @treturn number mass_center_x The x coordinate of the mass center (relative to object enter).
// @treturn number mass_center_y The y coordinate of the mass center.
// @treturn number mass_center_z (only returned for 3d objects) The z coordinate of the mass center.
int luafuncs_object_getMass(lua_State* l) {
    struct blitwizardobject* obj = toblitwizardobject(l, 1, 0,
    "blitwizard.object:setMass");
    if (!obj->physics || !obj->physics->object) {
        lua_pushstring(l, "object has no shape");
        return lua_error(l);
    }
    if (!obj->physics->movable) {
        lua_pushstring(l, "Mass can be only be obtained for movable objects");
        return lua_error(l);
    }
    lua_pushnumber(l, physics_getMass(obj->physics->object));
    double centerx, centery, centerz;
    if (obj->is3d) {
#ifdef USE_PHYSICS3D
        physics_get3dMassCenterOffset(obj->physics->object,
        &centerx, &centery, &centerz);
#else
        centerx = 0;
        centery = 0;
        centerz = 0;
#endif
    } else {
#ifdef USE_PHYSICS2D
        physics_get2dMassCenterOffset(obj->physics->object,
        &centerx, &centery);
#else
        centerx = 0;
        centery = 0;
#endif
        centerz = 0;
    }
    lua_pushnumber(l, centerx);
    lua_pushnumber(l, centery);
    if (obj->is3d) {
        lua_pushnumber(l, centerz);
    }
    return 3+(obj->is3d ? 1 : 0);
}

/// Set the mass and the center of an object. Only applicable for
// objects with movable collision enabled. (objects with static
// collision have infinite mass since they're not movable)
//
// Obtain the mass that is currently set to an object with
// @{blitwizard.object:getMass|object:getMass}.
// @function setMass
// @tparam number mass Set the mass of the object in kilograms. You should experiment and pick mass here which works well for you (it shouldn't be very small or very large), rather than using the most truthful numbers.
// @tparam number mass_center_x (optional) Set the x coordinate of the mass center (default: 0)
// @tparam number mass_center_y (optional) Set the y coordinate of the mass center (default: 0)
// @tparam number mass_center_z (optional) Set the z coordinate of the mass center (default: 0)
int luafuncs_object_setMass(lua_State* l) {
    struct blitwizardobject* obj = toblitwizardobject(l, 1, 0,
    "blitwizard.object:setMass");
    if (!obj->physics || !obj->physics->object) {
        lua_pushstring(l, "object has no shape");
        return lua_error(l);
    }
    if (!obj->physics->movable) {
        lua_pushstring(l, "Mass can be only set on movable objects");
        return lua_error(l);
    }
    if (lua_gettop(l) < 2 || lua_type(l, 2) != LUA_TNUMBER || lua_tonumber(l, 2) <= 0) {
        return haveluaerror(l, badargument1, 1, "blitwizard.object:setMass",
        "number", lua_strtype(l, 2));
    }
    double centerx = 0;
    double centery = 0;
    double centerz = 0;
    double mass = lua_tonumber(l, 2);
    if (lua_gettop(l) >= 3 && lua_type(l, 3) != LUA_TNIL) {
        if (lua_type(l, 3) != LUA_TNUMBER) {
            return haveluaerror(l, badargument1, 2, "blitwizard.object:setMass",
            "number", lua_strtype(l, 3));
        }
        centerx = lua_tonumber(l, 3);

        if (lua_type(l, 4) != LUA_TNUMBER) {
            return haveluaerror(l, badargument1, 3, "blitwizard.object:setMass",
            "number", lua_strtype(l, 4));
        }
        centery = lua_tonumber(l, 4);

        if (lua_type(l, 5) != LUA_TNUMBER) {
            return haveluaerror(l, badargument1, 4, "blitwiz.object:setMass", "number", lua_strtype(l, 5));
        }
        centerz = lua_tonumber(l, 5);
    }
    physics_setMass(obj->physics->object, mass);
    if (obj->is3d) {
#ifdef USE_PHYSICS3D
        physics_set3dMassCenterOffset(obj->physics->object,
        centerx, centery, centerz);
#endif
    } else {
        physics_set2dMassCenterOffset(obj->physics->object,
        centerx, centery);
    }
    return 0;
}

int luafuncs_object_setRestitution(lua_State* l) {
    struct blitwizardobject* obj = toblitwizardobject(l, 1, 0,
    "blitwizard.object:setRestitution");
    if (!obj->physics || !obj->physics->object) {
        lua_pushstring(l, "object has no shape");
        return lua_error(l);
    }
    if (lua_type(l, 2) != LUA_TNUMBER) {
        return haveluaerror(l, badargument1, 1,
        "blitwizard.object:setRestitution",
        "number", lua_strtype(l, 2));
    }
    obj->physics->restitution = lua_tonumber(l, 2);
    applyobjectsettings(obj);
    return 0;
}


void transferbodysettings(struct physicsobject* oldbody,
struct physicsobject* newbody) {
    // transfer position, mass etc from an old physics body
    // to a new one
    if (((struct blitwizardobject*)physics_getObjectUserdata(oldbody))->is3d
    != ((struct blitwizardobject*)physics_getObjectUserdata(newbody))->is3d) {
        return;
    }
    int is3d = ((struct blitwizardobject*)physics_getObjectUserdata(oldbody))
    ->is3d;
    double mass = physics_getMass(oldbody);
    double massx,massy,massz;
    double x,y,z;
    double angle;
    double qx,qy,qz,qrot;
    if (is3d) {
#ifdef USE_PHYSICS3D
        physics_get3dMassCenterOffset(oldbody, &massx, &massy, &massz);
        physics_get3dPosition(oldbody, &x, &y, &z);
        physics_get3dRotationQuaternion(oldbody, &qx, &qy, &qz, &qrot);
#endif
    } else {
        physics_get2dMassCenterOffset(oldbody, &massx, &massy);
        physics_get2dPosition(oldbody, &x, &y);
        physics_get2dRotation(oldbody, &angle);
    }
    physics_setMass(newbody, mass);
    if (is3d) {
#ifdef USE_PHYSICS3D
        physics_set3dMassCenterOffset(newbody, massx, massy, massz);
        physics_warp3d(newbody, x, y, z, qx, qy, qz, qrot);
#endif
    } else {
        physics_set2dMassCenterOffset(newbody, massx, massy);
        physics_warp2d(newbody, x, y, angle);
    }
}

#endif  // USE_PHYSICS2D || USE_PHYSICS3D

void objectphysics_warp3d(struct blitwizardobject* obj, double x, double y,
double z, double qx, double qy, double qz, double qrot, int anglespecified) {
#ifdef USE_PHYSICS3D
    if (!anglespecified) {
        physics_get3dRotationQuaternion(obj->physics->object,
        &qx, &qy, &qz, &qrot);
    }
    physics_warp3d(obj->physics->object, x, y, z, qx, qy, qz, qrot);
#endif
}

void objectphysics_warp2d(struct blitwizardobject* obj, double x, double y,
double angle, int anglespecified) {
#ifdef USE_PHYSICS2D
    if (!anglespecified) {
        physics_get2dRotation(obj->physics->object, &angle);
    }
    physics_warp2d(obj->physics->object, x, y, angle);
#endif
}

void objectphysics_getPosition(struct blitwizardobject* obj,
double* x, double* y, double* z) {
#if (defined(USE_PHYSICS2D) || defined(USE_PHYSICS3D))
    if (!obj->physics || !obj->physics->object) {
#else
    if (1) {
#endif
        *x = obj->x;
        *y = obj->y;
        if (obj->is3d) {
            *z = obj->vpos.z;
        }
        return;
    }
#if defined(USE_PHYSICS2D) || defined(USE_PHYSICS3D)
    if (obj->is3d) {
#ifdef USE_PHYSICS3D
        physics_get3dPosition(obj->physics->object, x, y, z);
#endif
    } else {
#ifdef USE_PHYSICS2D
        physics_get2dPosition(obj->physics->object, x, y);
#endif
    }
#endif
}

void objectphysics_setPosition(struct blitwizardobject* obj,
double x, double y, double z) {
#if (defined(USE_PHYSICS2D) || defined(USE_PHYSICS3D))
    if (!obj->physics || !obj->physics->object) {
#else
    if (1) {
#endif
        obj->x = x;
        obj->y = y;
        if (obj->is3d) {
            obj->vpos.z = z;
        }
        return;
    }
#if defined(USE_PHYSICS2D) || defined(USE_PHYSICS3D)
    if (obj->is3d) {
#ifdef USE_PHYSICS3D
        double qx,qy,qz,qrot;
        physics_get3dRotationQuaternion(obj->physics->object,
        &qx, &qy, &qz, &qrot);
        physics_warp3d(obj->physics->object, x, y, z,
        qx, qy, qz, qrot);
#endif
    } else {
#ifdef USE_PHYSICS2D
        double angle;
        physics_get2dRotation(obj->physics->object, &angle);
        physics_warp2d(obj->physics->object, x, y, angle);
#endif
    }
#endif
}

void objectphysics_set2dRotation(struct blitwizardobject* obj,
double angle) {
    if (obj->is3d) {
        return;
    }
#ifdef USE_PHYSICS2D
    if (obj->physics && obj->physics->object) {
        double x, y;
        physics_get2dPosition(obj->physics->object, &x, &y);
        physics_warp2d(obj->physics->object, x, y, angle);
    } else {
#endif
        obj->rotation.angle = angle;
#ifdef USE_PHYSICS2D
    }
#endif
}

void objectphysics_get2dRotation(struct blitwizardobject* obj,
double* angle) {
    if (obj->is3d) {
        *angle = 0;
        return;
    }
#ifdef USE_PHYSICS2D
    if (obj->physics && obj->physics->object) {
        physics_get2dRotation(obj->physics->object, angle);
    } else {
#endif
        *angle = obj->rotation.angle;
#ifdef USE_PHYSICS2D
    }
#endif
}

void objectphysics_get3dRotation(struct blitwizardobject* obj,
double* qx, double* qy, double* qz, double* qrot) {
#ifdef USE_PHYSICS3D
    if (obj->physics && obj->physics->object) {
        physics_get3dRotationQuaternion(obj->physics->object,
        qx, qy, qz, qrot);
    } else {
#endif
        *qx = obj->rotation.quaternion.x;
        *qy = obj->rotation.quaternion.y;
        *qz = obj->rotation.quaternion.z;
        *qrot = obj->rotation.quaternion.r;
#ifdef USE_PHYSICS3D
    }
#endif
}

/*int luafuncs_setShapeEdges(lua_State* l) {
    struct blitwizardobject* obj = toblitwizardobject(l, 1, 0,
    "blitwizard.object:setShapeEdges");
    if (lua_gettop(l) < 2 || lua_type(l, 2) != LUA_TTABLE) {
        lua_pushstring(l, "Second parameter is not a valid edge list table");
        return lua_error(l);
    }
    if (obj->physics->movable) {
        lua_pushstring(l, "This shape is not allowed for movable objects");
        return lua_error(l);
    }

    struct physicsobject2dedgecontext* context = physics2d_CreateObjectEdges_Begin(main_DefaultPhysics2dPtr(), obj, 0, obj->friction);

    int haveedge = 0;
    double d = 1;
    while (1) {
        lua_pushnumber(l, d);
        lua_gettable(l, 2);
        if (lua_type(l, -1) != LUA_TTABLE) {
            if (lua_type(l, -1) == LUA_TNIL && haveedge) {
                break;
            }
            lua_pushstring(l, "Edge list contains non-table value or is empty");
            physics2d_DestroyObject(physics2d_CreateObjectEdges_End(context));
            return lua_error(l);
        }
        haveedge = 1;

        double x1,y1,x2,y2;
        lua_pushnumber(l, 1);
        lua_gettable(l, -2);
        x1 = lua_tonumber(l, -1);
        lua_pop(l, 1);

        lua_pushnumber(l, 2);
        lua_gettable(l, -2);
        y1 = lua_tonumber(l, -1);
        lua_pop(l, 1);

        lua_pushnumber(l, 3);
        lua_gettable(l, -2);
        x2 = lua_tonumber(l, -1);
        lua_pop(l, 1);

        lua_pushnumber(l, 4);
        lua_gettable(l, -2);
        y2 = lua_tonumber(l, -1);
        lua_pop(l, 1);

        physics2d_CreateObjectEdges_Do(context, x1, y1, x2, y2);
        lua_pop(l, 1);
        d++;
    }

    struct physicsobject2d* oldobject = obj->object;

    obj->object = physics2d_CreateObjectEdges_End(context);
    if (!obj->object) {
        lua_pushstring(l, "Creation of the edges shape failed");
        return lua_error(l);
    }

    if (oldobject) {
        transferbodysettings(oldobject, obj->object);
        physics2d_DestroyObject(oldobject);
    }
    applyobjectsettings(obj);
    return 0;
}*/



