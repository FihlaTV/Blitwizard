
/* blitwizard 2d engine - source code file

  Copyright (C) 2011-2012 Jonas Thiem

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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "lua.h"

#include "luastate.h"
#include "luafuncs_physics.h"
#include "physics.h"
#include "main.h"

struct luaphysicsobj {
    int movable;
    struct physicsobject* object;
    double friction;
    double restitution;
    double lineardamping;
    double angulardamping;
    int rotationrestriction;
};

static struct luaphysicsobj* toluaphysicsobj(lua_State* l, int index) {
    char invalid[] = "Not a valid physics object reference";
    if (lua_type(l, index) != LUA_TUSERDATA) {
        lua_pushstring(l, invalid);
        lua_error(l);
        return NULL;
    }
    if (lua_rawlen(l, index) != sizeof(struct luaidref)) {
        lua_pushstring(l, invalid);
        lua_error(l);
        return NULL;
    }
    struct luaidref* idref = (struct luaidref*)lua_touserdata(l, index);
    if (!idref || idref->magic != IDREF_MAGIC || idref->type != IDREF_PHYSICS) {
        lua_pushstring(l, invalid);
        lua_error(l);
        return NULL;
    }
    struct luaphysicsobj* obj = idref->ref.ptr;
    return obj;
}

static int garbagecollect_physobj(lua_State* l) {
    struct luaphysicsobj* pobj = toluaphysicsobj(l, -1);
    if (!pobj) {
        //not a physics object!
        return 0;
    }
    if (pobj->object) {
        //Close the associated physics object
        physics_DestroyObject(pobj->object);
    }
    //free the physics object itself
    free(pobj);
    return 0;
}

static struct luaidref* createphysicsobj(lua_State* l) {
    //Create a luaidref userdata struct which points to a luaphysicsobj:
    struct luaidref* ref = lua_newuserdata(l, sizeof(*ref));
    struct luaphysicsobj* obj = malloc(sizeof(*obj));
    if (!obj) {
        lua_pop(l, 1);
        lua_pushstring(l, "Failed to allocate physics object wrap struct");
        lua_error(l);
        return NULL;
    }
    //initialise structs:
    memset(obj, 0, sizeof(*obj));
    memset(ref, 0, sizeof(*ref));
    ref->magic = IDREF_MAGIC;
    ref->type = IDREF_PHYSICS;
    ref->ref.ptr = obj;
    //make sure it gets garbage collected lateron:
    luastate_SetGCCallback(l, -1, (int (*)(void*))&garbagecollect_physobj);
    return ref;
}

int luafuncs_createMovableObject(lua_State* l) {
    struct luaidref* ref = createphysicsobj(l);
    ((struct luaphysicsobj*)ref->ref.ptr)->movable = 1;
    return 1;
}

int luafuncs_createStaticObject(lua_State* l) {
    createphysicsobj(l);
    return 1;
}

static void applyobjectsettings(struct luaphysicsobj* obj) {
    if (!obj->object) {return;}
    physics_SetRotationRestriction(obj->object, obj->rotationrestriction);
    physics_SetRestitution(obj->object, obj->restitution);
    physics_SetFriction(obj->object, obj->friction);
    physics_SetAngularDamping(obj->object, obj->angulardamping);
    physics_SetLinearDamping(obj->object, obj->lineardamping);
}

int luafuncs_impulse(lua_State* l) {
    struct luaphysicsobj* obj = toluaphysicsobj(l, 1);
    if (!obj->movable) {
        lua_pushstring(l, "Impulse can be only applied to movable objects");
        return lua_error(l);
    }
    if (!obj->object) {
        lua_pushstring(l, "Physics object has no shape");
        return lua_error(l);
    }
    if (lua_type(l, 2) != LUA_TNUMBER) {
        lua_pushstring(l, "Second parameter is not a valid source x number");
        return lua_error(l);
    }
    if (lua_type(l, 3) != LUA_TNUMBER) {
        lua_pushstring(l, "Third parameter is not a valid source y number");
        return lua_error(l);
    }
    if (lua_type(l, 4) != LUA_TNUMBER) {
        lua_pushstring(l, "Fourth parameter is not a valid force x number");
        return lua_error(l);
    }
    if (lua_type(l, 5) != LUA_TNUMBER) {
        lua_pushstring(l, "Fifth parameter is not a valid force y number");
        return lua_error(l);
    }
    double sourcex = lua_tonumber(l, 2);
    double sourcey = lua_tonumber(l, 3);
    double forcex = lua_tonumber(l, 4);
    double forcey = lua_tonumber(l, 5);
    physics_ApplyImpulse(obj->object, forcex, forcey, sourcex, sourcey);
    return 0;
}

int luafuncs_ray(lua_State* l) {
    if (lua_type(l, 1) != LUA_TNUMBER) {
        lua_pushstring(l, "First parameter is not a valid start x position");
        return lua_error(l);
    }
    if (lua_type(l, 2) != LUA_TNUMBER) {
        lua_pushstring(l, "Second parameter is not a valid start y position");
        return lua_error(l);
    }
    if (lua_type(l, 3) != LUA_TNUMBER) {
        lua_pushstring(l, "Third parameter is not a valid target x position");
        return lua_error(l);
    }
    if (lua_type(l, 4) != LUA_TNUMBER) {
        lua_pushstring(l, "Fourth parameter is not a valid target y position");
        return lua_error(l);
    }
    double startx = lua_tonumber(l, 1);
    double starty = lua_tonumber(l, 2);
    double targetx = lua_tonumber(l, 3);
    double targety = lua_tonumber(l, 4);

    struct physicsobject* obj;
    double hitpointx,hitpointy;
    double normalx,normaly;
    if (physics_Ray(main_DefaultPhysicsPtr(), startx, starty, targetx, targety, &hitpointx, &hitpointy, &obj, &normalx, &normaly)) {
        struct luaidref* ref = lua_newuserdata(l, sizeof(*ref));
        memset(ref, 0, sizeof(*ref));
        ref->magic = IDREF_MAGIC;
        ref->type = IDREF_PHYSICS;
        ref->ref.ptr = obj;
        lua_pushnumber(l, hitpointx);
        lua_pushnumber(l, hitpointy);
        lua_pushnumber(l, normalx);
        lua_pushnumber(l, normaly);
        return 5;
    }
    lua_pushnil(l);
    return 1;
}

int luafuncs_restrictRotation(lua_State* l) {
    struct luaphysicsobj* obj = toluaphysicsobj(l, 1);
    if (lua_type(l, 2) != LUA_TBOOLEAN) {
        lua_pushstring(l, "Second parameter is not a valid rotation restriction boolean");
        return lua_error(l);
    }
    if (!obj->movable) {
        lua_pushstring(l, "Mass can be only set on movable objects");
        return lua_error(l);
    }
    obj->rotationrestriction = lua_toboolean(l, 2);
    applyobjectsettings(obj);
    return 0;
}

int luafuncs_setGravity(lua_State* l) {
    struct luaphysicsobj* obj = toluaphysicsobj(l, 1);
    if (!obj->object) {
        lua_pushstring(l, "Physics object has no shape");
        return lua_error(l);
    }

    int set = 0;
    double gx,gy;
    if (lua_gettop(l) >= 3 && lua_type(l, 3) != LUA_TNIL) {
        if (lua_type(l, 2) != LUA_TNUMBER) {
            lua_pushstring(l, "Second parameter is not a valid gravity x number");
            return lua_error(l);
        }
        if (lua_type(l, 3) != LUA_TNUMBER) {
            lua_pushstring(l, "Third parameter is not a valid gravity y number");
            return lua_error(l);
        }
        gx = lua_tonumber(l, 2);
        gy = lua_tonumber(l, 3);
        set = 1;
    }
    if (set) {
        physics_SetGravity(obj->object, gx, gy);
    }else{
        physics_UnsetGravity(obj->object);
    }
    return 0;
}

int luafuncs_setMass(lua_State* l) {
    struct luaphysicsobj* obj = toluaphysicsobj(l, 1);
    if (!obj->movable) {
        lua_pushstring(l, "Mass can be only set on movable objects");
        return lua_error(l);
    }
    if (lua_gettop(l) < 2 || lua_type(l, 2) != LUA_TNUMBER || lua_tonumber(l, 2) <= 0) {
        lua_pushstring(l, "Second parameter is not a valid mass number");
        return lua_error(l);
    }
    double centerx = 0;
    double centery = 0;
    double mass = lua_tonumber(l, 2);
    if (lua_gettop(l) >= 3 && lua_type(l, 3) != LUA_TNIL) {
        if (lua_type(l, 3) != LUA_TNUMBER) {
            lua_pushstring(l, "Third parameter is not a valid x center offset number");
            return lua_error(l);
        }
        centerx = lua_tonumber(l, 3);
    }
    if (lua_gettop(l) >= 4 && lua_type(l, 4) != LUA_TNIL) {
        if (lua_type(l, 4) != LUA_TNUMBER) {
            lua_pushstring(l, "Fourth parameter is not a valid y center offset number");
            return lua_error(l);
        }
        centery = lua_tonumber(l, 4);
    }
    if (!obj->object) {
        lua_pushstring(l, "Physics object has no shape");
        return lua_error(l);
    }
    physics_SetMass(obj->object, mass);
    physics_SetMassCenterOffset(obj->object, centerx, centery);
    return 0;
}

void transferbodysettings(struct physicsobject* oldbody, struct physicsobject* newbody) {
    double mass = physics_GetMass(oldbody);
    double massx,massy;
    physics_GetMassCenterOffset(oldbody, &massx, &massy);
    physics_SetMass(newbody, mass);
    physics_SetMassCenterOffset(newbody, massx, massy);
}

int luafuncs_warp(lua_State* l) {
    struct luaphysicsobj* obj = toluaphysicsobj(l, 1);
    if (!obj->object) {
        lua_pushstring(l, "Physics object doesn't have a shape");
        return lua_error(l);
    }
    double warpx,warpy,warpangle;
    physics_GetPosition(obj->object, &warpx, &warpy);
    physics_GetRotation(obj->object, &warpangle);
    if (lua_gettop(l) >= 2 && lua_type(l, 2) != LUA_TNIL) {
        if (lua_type(l, 2) != LUA_TNUMBER) {
            lua_pushstring(l, "Second parameter not a valid warp x position number");
            return lua_error(l);
        }
        warpx = lua_tonumber(l, 2);
    }
    if (lua_gettop(l) >= 3 && lua_type(l, 3) != LUA_TNIL) {
        if (lua_type(l, 3) != LUA_TNUMBER) {
            lua_pushstring(l, "Third parameter not a valid warp y position number");
            return lua_error(l);
        }
        warpy = lua_tonumber(l, 3);
    }
    if (lua_gettop(l) >= 4 && lua_type(l, 4) != LUA_TNIL) {
        if (lua_type(l, 4) != LUA_TNUMBER) { 
            lua_pushstring(l, "Fourth parameter not a valid warp angle number");
            return lua_error(l);
        }
        warpangle = lua_tonumber(l, 4);
    }
    physics_Warp(obj->object, warpx, warpy, warpangle);
    return 0;
}

int luafuncs_getPosition(lua_State* l) {
    struct luaphysicsobj* obj = toluaphysicsobj(l, 1);
    if (!obj->object) {
        lua_pushstring(l, "Physics object doesn't have a shape");
        return lua_error(l);
    }
    double x,y;
    physics_GetPosition(obj->object, &x, &y);
    lua_pushnumber(l, x);
    lua_pushnumber(l, y);
    return 2;
}

int luafuncs_setRestitution(lua_State* l) {
    struct luaphysicsobj* obj = toluaphysicsobj(l, 1);
    if (lua_type(l, 2) != LUA_TNUMBER) {
        lua_pushstring(l, "Second parameter not a valid restitution number");
        return lua_error(l);
    }
    obj->restitution = lua_tonumber(l, 2);
    applyobjectsettings(obj);
    return 0;
}

int luafuncs_setFriction(lua_State* l) {
    struct luaphysicsobj* obj = toluaphysicsobj(l, 1);
    if (lua_type(l, 2) != LUA_TNUMBER) {
        lua_pushstring(l, "Second parameter not a valid friction number");
        return lua_error(l);
    }
    obj->friction = lua_tonumber(l, 2);
    applyobjectsettings(obj);    
    return 0;
}

int luafuncs_setLinearDamping(lua_State* l) {
    struct luaphysicsobj* obj = toluaphysicsobj(l, 1);
    if (lua_type(l, 2) != LUA_TNUMBER) {
        lua_pushstring(l, "Second parameter not a valid angular damping number");
        return lua_error(l);
    }
    obj->lineardamping = lua_tonumber(l, 2);
    applyobjectsettings(obj);
    return 0;
}

int luafuncs_setAngularDamping(lua_State* l) {
    struct luaphysicsobj* obj = toluaphysicsobj(l, 1);
    if (lua_type(l, 2) != LUA_TNUMBER) {
        lua_pushstring(l, "Second parameter not a valid angular damping number");
        return lua_error(l);
    }
    obj->angulardamping = lua_tonumber(l, 2);
    applyobjectsettings(obj);   
    return 0;
}

int luafuncs_getRotation(lua_State* l) {
    struct luaphysicsobj* obj = toluaphysicsobj(l, 1);
    if (!obj->object) {
        lua_pushstring(l, "Physics object doesn't have a shape");
        return lua_error(l);
    }
    double angle;
    physics_GetRotation(obj->object, &angle);
    lua_pushnumber(l, angle);
    return 1;
}

int luafuncs_setShapeEdges(lua_State* l) {
    struct luaphysicsobj* obj = toluaphysicsobj(l, 1);
    if (lua_gettop(l) < 2 || lua_type(l, 2) != LUA_TTABLE) {
        lua_pushstring(l, "Second parameter is not a valid edge list table");
        return lua_error(l);
    }
    if (obj->movable) {
        lua_pushstring(l, "This shape is not allowed for movable objects");
        return lua_error(l);
    }

    struct physicsobjectedgecontext* context = physics_CreateObjectEdges_Begin(main_DefaultPhysicsPtr(), obj, 0, obj->friction);

    int haveedge = 0;
    double d = 1;
    while (1) {
        lua_pushnumber(l, d);
        lua_gettable(l, 2);
        if (lua_type(l, -1) != LUA_TTABLE) {
            if (lua_type(l, -1) == LUA_TNIL && haveedge) {
                break;
            }
            printf("type: %d\n", lua_type(l, -1));
            lua_pushstring(l, "Edge list contains non-table value or is empty");
            physics_DestroyObject(physics_CreateObjectEdges_End(context));
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

        physics_CreateObjectEdges_Do(context, x1, y1, x2, y2);
        lua_pop(l, 1);
        d++;
    }
    
    struct physicsobject* oldobject = obj->object;

    obj->object = physics_CreateObjectEdges_End(context);
    if (!obj->object) {
        lua_pushstring(l, "Creation of the edges shape failed");
        return lua_error(l);
    }
    
    if (oldobject) {
        transferbodysettings(oldobject, obj->object);
        physics_DestroyObject(oldobject);
    }
    applyobjectsettings(obj);
    return 0;
}

int luafuncs_setShapeCircle(lua_State* l) {
    struct luaphysicsobj* obj = toluaphysicsobj(l, 1);
    if (lua_gettop(l) < 2 || lua_type(l, 2) != LUA_TNUMBER || lua_tonumber(l, 2) <= 0) {
        lua_pushstring(l, "Not a valid circle radius number");
        return lua_error(l);
    }
    double radius = lua_tonumber(l, 2);
    struct physicsobject* oldobject = obj->object;
    obj->object = physics_CreateObjectOval(main_DefaultPhysicsPtr(), obj, obj->movable, obj->friction, radius, radius);
    if (!obj->object) {
        lua_pushstring(l, "Failed to allocate shape");
        return lua_error(l);
    }

    if (oldobject) {
        transferbodysettings(oldobject, obj->object);
        physics_DestroyObject(oldobject);
    }
    applyobjectsettings(obj);
    return 0;
}


int luafuncs_setShapeOval(lua_State* l) {
    struct luaphysicsobj* obj = toluaphysicsobj(l, 1);
    if (lua_gettop(l) < 2 || lua_type(l, 2) != LUA_TNUMBER || lua_tonumber(l, 2) <= 0) {
        lua_pushstring(l, "Not a valid oval width number");
        return lua_error(l);
    }
    if (lua_gettop(l) < 3 || lua_type(l, 3) != LUA_TNUMBER || lua_tonumber(l, 3) <= 0) {
        lua_pushstring(l, "Not a valid oval height number");
        return lua_error(l);
    }
    double width = lua_tonumber(l, 2);
    double height = lua_tonumber(l, 3);
    struct physicsobject* oldobject = obj->object;
    obj->object = physics_CreateObjectOval(main_DefaultPhysicsPtr(), obj, obj->movable, obj->friction, width, height);
    if (!obj->object) {
        lua_pushstring(l, "Failed to allocate shape");
        return lua_error(l);
    }

    if (oldobject) {
        transferbodysettings(oldobject, obj->object);
        physics_DestroyObject(oldobject);
    }
    applyobjectsettings(obj);
    return 0;
}


int luafuncs_setShapeRectangle(lua_State* l) {
    struct luaphysicsobj* obj = toluaphysicsobj(l, 1);
    if (lua_gettop(l) < 2 || lua_type(l, 2) != LUA_TNUMBER || lua_tonumber(l, 2) <= 0) {
        lua_pushstring(l, "Not a valid rectangle width number");
        return lua_error(l);
    }
    if (lua_gettop(l) < 3 || lua_type(l, 3) != LUA_TNUMBER || lua_tonumber(l, 3) <= 0) {
        lua_pushstring(l, "Not a valid rectangle height number");
        return lua_error(l);
    }
    double width = lua_tonumber(l, 2);
    double height = lua_tonumber(l, 3);
    struct physicsobject* oldobject = obj->object;
    obj->object = physics_CreateObjectRectangle(main_DefaultPhysicsPtr(), obj, obj->movable, obj->friction, width, height);
    if (!obj->object) {
        lua_pushstring(l, "Failed to allocate shape");
        return lua_error(l);
    }

    if (oldobject) {
        transferbodysettings(oldobject, obj->object);
        physics_DestroyObject(oldobject);
    }
    applyobjectsettings(obj);
    return 0;
}

