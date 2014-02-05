
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

#include "config.h"
#include "os.h"

#if (defined(USE_PHYSICS2D) || defined(USE_PHYSICS3D))

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "physics.h"
#include "physicsinternal.h"

struct cachedimpulse {
    double sourcex, sourcey, sourcez;
    double forcex, forcey, forcez;
};

struct cachedangularimpulse {
    union {
        double angle2d;
        struct {
            double x, y, z, r;
        };
    };
};

struct cachedphysicsobject {
    int is3d;
    
    struct physicsobjectshape* shapes;
    int shapecount;
    
    struct physicsworld* world;
    
    void *userdata;
    
    int movable;
    
    int mass_set;
    double mass;
    
    int masscenter_set;
    double masscenterx, masscentery, masscenterz;
    
    int scale_set;
    double scalex, scaley, scalez;
    
    int gravity_set;
    double gravityx, gravityy, gravityz;
    
    int rotationrestriction2d;
    int rotationrestriction3daxis;
    double rotationrestriction3dx, rotationrestriction3dy,
    rotationrestriction3dz;
    double rotationrestriction3dfull;
    
    int friction_set;
    double friction;
    
    int angulardamping_set;
    double angulardamping;
    
    int lineardamping_set;
    double lineardamping;
    
    int restitution_set;
    double restitution;
    
    int position_set;
    double positionx, positiony, positionz;
    
    int rotation2d_set;
    double rotation2d;
    
    int rotation3d_set;
    double rotation3dx, rotation3dy, rotation3dz, rotation3dr;
    
    int velocity_set;
    double velocityx, velocityy, velocityz;
    
    int angularvelocity2d_set;
    double angularvelocity2d;
    
    // TODO: angular velocity 3D quaternion
    
    int impulsecount;
    struct cachedimpulse* impulses;
    
    int angularimpulsecount;
    struct cachedangularimpulse* angularimpulses;
    
    struct cachedphysicsobject *next;
};

struct deletedphysicsobject {
    struct physicsobject *object;
    struct deletedphysicsobject *next;
};

// Internal function declarations
static struct cachedphysicsobject *physics_createCachedObjects(int count);
static int physics_destroyCachedObject(struct cachedphysicsobject *c_object);
static inline int physics_objectIsCached(void *object);
static struct physicsobject *physics_createObjectFromCache(struct
 cachedphysicsobject *c_object);



// Globals
struct cachedphysicsobject *cachedObjects = NULL;

struct deletedphysicsobject *deletedObjects = NULL;

int isInCallback = 0;

struct userdata_wrapper {
    int (*callback2d)(void *userdata, struct physicsobject *a,
     struct physicsobject *b, double x, double y, double normalx,
     double normaly, double force);
    // TODO: 3D
    void *userdata;
} userdata_wrapper;

// Functions necessary for activating the caching stuff
#ifdef USE_PHYSICS2D
static int physics_callback2dWrapper(void *userdata,
        struct physicsobject *a, struct physicsobject *b,
        double x, double y, double normalx, double normaly,
        double force) {
    isInCallback = 1;
    
    int r = ((struct userdata_wrapper*)userdata)->callback2d(
        ((struct userdata_wrapper*)userdata)->userdata, a, b, x, y,
        normalx, normaly, force);
    
    // Callback has finished, so transform all cached objects into actual ones
    struct cachedphysicsobject *c = cachedObjects;
    while (c != NULL) {
        void (*exchangeObjectCallback)(struct physicsobject *oold,
            struct physicsobject *onew) =
            physics_getWorldExchangeObjectCallback_internal(
                            c->world);
        struct physicsobject *po = physics_createObjectFromCache(c);
        exchangeObjectCallback((struct physicsobject *)c, po);
        c = c->next;
    }
    // ... and delete the former.
    while (c != NULL) {
        c = cachedObjects;
        physics_destroyCachedObject(c);
    }
    
    isInCallback = 0;
    
    // Also delete the objects that were mock-deleted during the callback
    struct deletedphysicsobject *d = deletedObjects;
    while (deletedObjects != NULL) {
        physics_destroyObject(deletedObjects->object); // TODO maybe _internal?
        d = deletedObjects->next;
        free(deletedObjects);
        deletedObjects = d;
    }
    return r;
}
#endif

#ifdef USE_PHYSICS3D
// TODO: 3D callback wrapper
#endif

void physics_set2dCollisionCallback(
        struct physicsworld* world,
        int (*callback)(void *userdata, struct physicsobject *a,
            struct physicsobject *b, double x, double y, double normalx,
            double normaly, double force),
        void *userdata) {
    userdata_wrapper.callback2d = callback;
    userdata_wrapper.userdata = userdata;
    physics_set2dCollisionCallback_internal(world, physics_callback2dWrapper,
        &userdata_wrapper);
}

void physics_step(struct physicsworld *world,
        void (*exchangeObjectCallback)(struct physicsobject* oold,
        struct physicsobject* onew)) {
    physics_setWorldExchangeObjectCallback_internal(world,
        exchangeObjectCallback);
    physics_step_internal(world);
}


static struct cachedphysicsobject *physics_createCachedObjects(int count) {
    assert(count > 0);
    struct cachedphysicsobject *c_object;
    for (int i = 0; i < count; ++i) {
        c_object = (struct cachedphysicsobject*)
            malloc(sizeof(*c_object));
        memset (c_object, 0, sizeof(*c_object));
        c_object->next = cachedObjects;
        cachedObjects = c_object;
    }
    return c_object; // Return last element added, i.e. first element in list
}

static int physics_destroyCachedObject(struct cachedphysicsobject *c_object) {
    if (not c_object) {
        return -1;
    }
    struct cachedphysicsobject *c, *c_prev;
    c = cachedObjects;
    c_prev = NULL;
    while (c != NULL) {
        if (c == c_object) {
            if (not c_prev) {
                cachedObjects = c->next;
            } else {
                c_prev->next = c->next;
            }
            free(c_object->impulses);
            free(c_object->angularimpulses);
            free(c);
            return 0;
        }
        c_prev = c;
        c = c->next;
    }
    return 1;
}

static inline int physics_objectIsCached(void *object) {
    struct cachedphysicsobject *c = cachedObjects;
    while (c != NULL) {
        if ((void*)c == object) {
            return 1;
        }
        c = c->next;
    }
    return 0;
}

static struct physicsobject *physics_createObjectFromCache(
        struct cachedphysicsobject *c_object) {
    struct physicsobject *object;
    object = physics_createObject_internal(c_object->world, c_object->userdata,
        c_object->movable, c_object->shapes, c_object->shapecount);
    
    // Apply all params to the newly created object
    struct cachedphysicsobject *c = c_object; // Shorter alias
    if (c->mass_set) {
        physics_setMass_internal(object, c->mass);
    }
    if (c->masscenter_set) {
        if (c->is3d) {
#ifdef USE_PHYSICS3D
            // TBD
#endif
        } else {
#ifdef USE_PHYSICS2D
            physics_set2dMassCenterOffset_internal(object, c->masscenterx,
             c->masscentery);
#endif
        }
    }
    if (c->scale_set) {
        if (c->is3d) {
#ifdef USE_PHYSICS3D
            // TBD
#endif
        } else {
#ifdef USE_PHYSICS2D
            physics_set2dScale_internal(object, c->scalex, c->scaley);
#endif
        }
    }
    if (c->gravity_set) {
        if (c->is3d) {
#ifdef USE_PHYSICS3D
            // TBD
#endif
        } else {
#ifdef USE_PHYSICS2D
            physics_set2dGravity_internal(object, c->gravityx, c->gravityy);
#endif
        }
    }
    if (c->rotationrestriction2d) {
        physics_set2dRotationRestriction_internal(object,
         c->rotationrestriction2d);
    }
    
    // TODO: 3D rotation restriction
    
    if (c->friction_set) {
        physics_setFriction_internal(object, c->friction);
    }
    if (c->angulardamping_set) {
        physics_setAngularDamping_internal(object, c->angulardamping);
    }
    if (c->lineardamping_set) {
        physics_setLinearDamping_internal(object, c->lineardamping);
    }
    if (c->restitution_set) {
        physics_setRestitution_internal(object, c->restitution);
    }
    if (c->position_set || c->rotation2d_set) {
        if (c->is3d) {
#ifdef USE_PHYSICS3D
            // TBD
#endif
        } else {
#ifdef USE_PHYSICS2D
            physics_warp2d_internal(object, c->positionx, c->positiony,
             c->rotation2d);
#endif
        }
    }
    
    // TODO: 3D rotation
    
    if (c->velocity_set) {
        if (c->is3d) {
#ifdef USE_PHYSICS3D
            // TBD
#endif
        } else {
#ifdef USE_PHYSICS2D
            physics_set2dVelocity_internal(object, c->velocityx, c->velocityy);
#endif
        }
    }
    if (c->angularvelocity2d_set) {
        if (c->is3d) {
#ifdef USE_PHYSICS3D
            // TBD
#endif
        } else {
#ifdef USE_PHYSICS2D
            physics_set2dAngularVelocity_internal(object, c->angularvelocity2d);
#endif
        }
    }
    
    for (int i = 0; i < c->impulsecount; ++i) {
        struct cachedimpulse* imp = &(c->impulses[i]);
        if (c->is3d) {
#ifdef USE_PHYSICS3D
            // TBD
#endif
        } else {
#ifdef USE_PHYSICS2D
            physics_apply2dImpulse_internal(object, imp->forcex, imp->forcey,
             imp->sourcex, imp->sourcey);
#endif
        }
    }
    
    for (int i = 0; i < c->angularimpulsecount; ++i) {
        struct cachedangularimpulse* imp = &(c->angularimpulses[i]);
        if (c->is3d) {
#ifdef USE_PHYSICS3D
            // TBD
#endif
        } else {
#ifdef USE_PHYSICS2D
            physics_apply2dAngularImpulse_internal(object, imp->angle2d);
#endif
        }
    }
    
    return object;
}

/*
 Implementation of physics.h starts here
 */

struct physicsobject *physics_createObject(struct physicsworld* world,
        void *userdata, int movable, struct physicsobjectshape* shapelist,
        int shapecount) {
    if (isInCallback) {
        struct cachedphysicsobject *c_object = physics_createCachedObjects(1);
        c_object->world = world;
        c_object->is3d = physics_worldIs3d_internal(world);
        c_object->userdata = userdata;
        c_object->movable = movable;
        c_object->shapes = shapelist;
        c_object->shapecount = shapecount;
        return (struct physicsobject*)c_object;
    } else {
        return physics_createObject_internal(world, userdata, movable,
         shapelist, shapecount);
    }
}

void physics_destroyObject(struct physicsobject *object) {
    struct cachedphysicsobject *c_object = (struct cachedphysicsobject*)object;
    // Relies on physics_destroyCachedObject() returning 1 for non-cached objs
    /* TODO: We can make this faster by checking for isInCallback instead if
     [cached objects exist] <=> [isInCallBack] holds.
     */
    if (isInCallback) {
        // Just add it to the list of objects that are to be deleted later
        struct deletedphysicsobject *d = (struct deletedphysicsobject*)malloc(
         sizeof(*d));
        d->object = object;
        d->next = deletedObjects;
        deletedObjects = d;
    } else {
        if (physics_destroyCachedObject(c_object)) {
            physics_destroyObject_internal(object);
        }
    }
}

void *physics_getObjectUserdata(struct physicsobject *object) {
    if (physics_objectIsCached(object)) {
        return ((struct cachedphysicsobject*)object)->userdata;
    } else {
        return physics_getObjectUserdata_internal(object);
    }
}

#ifdef USE_PHYSICS2D
void physics_set2dScale(struct physicsobject *object, double scalex,
 double scaley) {
    struct cachedphysicsobject *c_object = (struct cachedphysicsobject*)object;
    if (physics_objectIsCached(object)) {
        c_object->scalex = scalex;
        c_object->scaley = scaley;
        c_object->scale_set = 1;
    } else {
        physics_set2dScale_internal(object, scalex, scaley);
    }
}
#endif

void physics_setMass(struct physicsobject *obj, double mass) {
    struct cachedphysicsobject *c_object = (struct cachedphysicsobject*)obj;
    if (physics_objectIsCached(obj)) {
        c_object->mass = mass;
        c_object->mass_set = 1;
    } else {
        physics_setMass_internal(obj, mass);
    }
}

double physics_getMass(struct physicsobject *obj) {
    if (physics_objectIsCached(obj)) {
        return ((struct cachedphysicsobject*)obj)->mass;
    } else {
        return physics_getMass_internal(obj);
    }
}

#ifdef USE_PHYSICS2D
void physics_set2dMassCenterOffset(struct physicsobject *obj,
 double offsetx, double offsety) {
    struct cachedphysicsobject *c_object = (struct cachedphysicsobject*)obj;
    if (physics_objectIsCached(obj)) {
        c_object->masscenterx = offsetx;
        c_object->masscentery = offsety;
        c_object->masscenter_set = 1;
    } else {
        physics_set2dMassCenterOffset_internal(obj, offsetx, offsety);
    }
}
#endif

#ifdef USE_PHYSICS2D
void physics_get2dMassCenterOffset(struct physicsobject *obj,
 double* offsetx, double* offsety) {
    struct cachedphysicsobject *c_object = (struct cachedphysicsobject*)obj;
    if (physics_objectIsCached(obj)) {
        *offsetx = c_object->masscenterx;
        *offsety = c_object->masscentery;
    } else {
        physics_get2dMassCenterOffset_internal(obj, offsetx, offsety);
    }
}
#endif

#ifdef USE_PHYSICS2D
void physics_set2dGravity(struct physicsobject *obj, double x, double y) {
    struct cachedphysicsobject *c_object = (struct cachedphysicsobject*)obj;
    if (physics_objectIsCached(obj)) {
        c_object->gravityx = x;
        c_object->gravityy = y;
        c_object->gravity_set = 1;
    } else {
        physics_set2dGravity_internal(obj, x, y);
    }
}
#endif

void physics_unsetGravity(struct physicsobject *obj) {
    struct cachedphysicsobject *c_object = (struct cachedphysicsobject*)obj;
    if (physics_objectIsCached(obj)) {
        c_object->gravityx = 0;
        c_object->gravityy = 0;
        c_object->gravity_set = 0;
    } else {
        physics_unsetGravity_internal(obj);
    }
}

#ifdef USE_PHYSICS2D
void physics_set2dRotationRestriction(struct physicsobject *obj,
 int restricted) {
    struct cachedphysicsobject *c_object = (struct cachedphysicsobject*)obj;
    if (physics_objectIsCached(obj)) {
        c_object->rotationrestriction2d = restricted;
    } else {
        physics_set2dRotationRestriction_internal(obj, restricted);
    }
}
#endif

void physics_setFriction(struct physicsobject *obj, double friction) {
    struct cachedphysicsobject *c_object = (struct cachedphysicsobject*)obj;
    if (physics_objectIsCached(obj)) {
        c_object->friction = friction;
        c_object->friction_set = 1;
    } else {
        physics_setFriction_internal(obj, friction);
    }
}

void physics_setAngularDamping(struct physicsobject *obj, double damping) {
    struct cachedphysicsobject *c_object = (struct cachedphysicsobject*)obj;
    if (physics_objectIsCached(obj)) {
        c_object->angulardamping = damping;
        c_object->angulardamping_set = 1;
    } else {
        physics_setAngularDamping_internal(obj, damping);
    }
}

void physics_setLinearDamping(struct physicsobject *obj, double damping) {
    struct cachedphysicsobject *c_object = (struct cachedphysicsobject*)obj;
    if (physics_objectIsCached(obj)) {
        c_object->lineardamping = damping;
        c_object->lineardamping_set = 1;
    } else {
        physics_setLinearDamping_internal(obj, damping);
    }
}

void physics_setRestitution(struct physicsobject *obj, double restitution) {
    struct cachedphysicsobject *c_object = (struct cachedphysicsobject*)obj;
    if (physics_objectIsCached(obj)) {
        c_object->restitution = restitution;
        c_object->restitution_set = 1;
    } else {
        physics_setRestitution_internal(obj, restitution);
    }
}

#ifdef USE_PHYSICS2D
void physics_get2dPosition(struct physicsobject *obj, double* x, double* y) {
    struct cachedphysicsobject *c_object = (struct cachedphysicsobject*)obj;
    if (physics_objectIsCached(obj)) {
        *x = c_object->positionx;
        *y = c_object->positiony;
    } else {
        physics_get2dPosition_internal(obj, x, y);
    }
}
#endif

#ifdef USE_PHYSICS2D
void physics_get2dRotation(struct physicsobject *obj, double* angle) {
    struct cachedphysicsobject *c_object = (struct cachedphysicsobject*)obj;
    if (physics_objectIsCached(obj)) {
        *angle = c_object->rotation2d;
    } else {
        physics_get2dRotation_internal(obj, angle);
    }
}
#endif

#ifdef USE_PHYSICS2D
void physics_warp2d(struct physicsobject *obj, double x, double y,
 double angle) {
    struct cachedphysicsobject *c_object = (struct cachedphysicsobject*)obj;
    if (physics_objectIsCached(obj)) {
        c_object->positionx = x;
        c_object->positiony = y;
        c_object->rotation2d = angle;
        c_object->position_set = 1;
        c_object->rotation2d_set = 1;
    } else {
        physics_warp2d_internal(obj, x, y, angle);
    }
}
#endif

#ifdef USE_PHYSICS2D
void physics_apply2dImpulse(struct physicsobject *obj, double forcex,
 double forcey, double sourcex, double sourcey) {
    struct cachedphysicsobject *c_object = (struct cachedphysicsobject*)obj;
    if (physics_objectIsCached(obj)) {
        c_object->impulsecount++;
        c_object->impulses = (struct cachedimpulse*)realloc(c_object->impulses,
         sizeof(struct cachedimpulse) * (c_object->impulsecount));
        struct cachedimpulse* imp = &(c_object->impulses[
         c_object->impulsecount-1]);
        imp->forcex = forcex;
        imp->forcey = forcey;
        imp->sourcex = sourcex;
        imp->sourcey = sourcey;
    } else {
        physics_apply2dImpulse_internal(obj, forcex, forcey, sourcex, sourcey);
    }
}
#endif

#ifdef USE_PHYSICS2D
void physics_get2dVelocity(struct physicsobject *obj, double *vx, double* vy) {
    struct cachedphysicsobject *c_object = (struct cachedphysicsobject*)obj;
    if (physics_objectIsCached(obj)) {
        *vx = c_object->velocityx;
        *vy = c_object->velocityy;
    } else {
        physics_get2dVelocity_internal(obj, vx, vy);
    }
}
#endif

#ifdef USE_PHYSICS2D
double physics_get2dAngularVelocity(struct physicsobject *obj) {
    struct cachedphysicsobject *c_object = (struct cachedphysicsobject*)obj;
    if (physics_objectIsCached(obj)) {
        return c_object->angularvelocity2d;
    } else {
        return physics_get2dAngularVelocity_internal(obj);
    }
}
#endif

#ifdef USE_PHYSICS2D
void physics_set2dVelocity(struct physicsobject *obj, double vx, double vy) {
    struct cachedphysicsobject *c_object = (struct cachedphysicsobject*)obj;
    if (physics_objectIsCached(obj)) {
        c_object->velocityx = vx;
        c_object->velocityy = vy;
        c_object->velocity_set = 1;
    } else {
        physics_set2dVelocity_internal(obj, vx, vy);
    }
}
#endif

#ifdef USE_PHYSICS2D
void physics_set2dAngularVelocity(struct physicsobject *obj, double omega) {
    struct cachedphysicsobject *c_object = (struct cachedphysicsobject*)obj;
    if (physics_objectIsCached(obj)) {
        c_object->angularvelocity2d = omega;
        c_object->angularvelocity2d_set = 1;
    } else {
        physics_set2dAngularVelocity_internal(obj, omega);
    }
}
#endif

#ifdef USE_PHYSICS2D
void physics_apply2dAngularImpulse(struct physicsobject *obj, double impulse) {
    struct cachedphysicsobject *c_object = (struct cachedphysicsobject*)obj;
    if (physics_objectIsCached(obj)) {
        c_object->angularimpulsecount++;
        c_object->angularimpulses = (struct cachedangularimpulse*)realloc(
         c_object->angularimpulses, sizeof(struct cachedangularimpulse) * \
         (c_object->angularimpulsecount));
        struct cachedangularimpulse* imp = &(c_object->angularimpulses[
         c_object->angularimpulsecount-1]);
        imp->angle2d = impulse;
    } else {
        physics_apply2dAngularImpulse_internal(obj, impulse);
    }
}
#endif

#endif  // defined(USE_PHYSICS2D) || defined(USE_PHYSICS3D)
