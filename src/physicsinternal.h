
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

#ifndef BLITWIZARD_PHYSICSINTERNAL_H_
#define BLITWIZARD_PHYSICSINTERNAL_H_

#include "config.h"
#include "os.h"

#if (defined(USE_PHYSICS2D) || defined(USE_PHYSICS3D))

#ifdef __cplusplus
extern "C" {
#endif

#ifdef USE_PHYSICS2D
void physics_set2dCollisionCallback_internal(
struct physicsworld* world,
int (*callback)(void* userdata, struct physicsobject* a,
    struct physicsobject* b, double x, double y, double normalx,
    double normaly, double force),
void* userdata);
#endif
#ifdef USE_PHYSICS3D
void physics_set3dCollisionCallback
struct physicsworld* world,
int (*callback)(void* userdata, struct physicsobject* a,
    struct physicsobject* b, double x, double y, double z,
    double normalx, double normaly, double normalz, double force),
void* userdata);
#endif

struct physicsobject* physics_createObject_internal(
struct physicsworld* world, void* userdata, int movable,
struct physicsobjectshape* shapelist, int shapecount);

void physics_destroyObject_internal(struct physicsobject* object);

void physics_setMass_internal(struct physicsobject* obj, double mass);
double physics_getMass_internal(struct physicsobject* obj);
#ifdef USE_PHYSICS2D
void physics_set2dMassCenterOffset_internal(
struct physicsobject* obj, double offsetx, double offsety);
void physics_get2dMassCenterOffset_internal(
struct physicsobject* obj, double* offsetx, double* offsety);
#endif
#ifdef USE_PHYSICS3D
void physics_set3dMassCenterOffset_internal(
struct physicsobject* obj, double offsetx,
double offsety, double offsetz);
void physics_get3dMassCenterOffset_internal(
struct physicsobject* obj, double* offsetx,
double* offsety, double* offsetz);
#endif
#ifdef USE_PHYSICS2D
void physics_set2dGravity_internal(
struct physicsobject* obj,
double x, double y);
void physics_set2dWorldGravity_internal(
struct physicsworld* world, double x,
double y);
#endif
#ifdef USE_PHYSICS3D
void physics_set3dGravity_internal(
struct physicsobject* obj, double x,
double y, double z);
void physics_set3dWorldGravity_internal(
struct physicsworld* world,
double x, double y,
 double z);
#endif
void physics_unsetGravity_internal(
struct physicsobject* obj);
#ifdef USE_PHYSICS2D
void physics_set2dRotationRestriction_internal(
struct physicsobject* obj, int restricted);
#endif
#ifdef USE_PHYSICS3D
void physics_set3dRotationRestrictionAroundAxis_internal(struct physicsobject* obj, int restrictedaxisx, int restrictedaxisy, int restrictedaxisz);
void physics_set3dRotationRestrictionAllAxis_internal(struct physicsobject* obj);
void physics_set3dNoRotationRestriction_internal(void);
#endif
void physics_setFriction_internal(struct physicsobject* obj, double friction);
void physics_setAngularDamping_internal(struct physicsobject* obj, double damping);
void physics_setLinearDamping_internal(struct physicsobject* obj, double damping);
void physics_setRestitution_internal(struct physicsobject* obj, double restitution);

// Change and get position/rotation and apply impulses
#ifdef USE_PHYSICS2D
void physics_get2dPosition_internal(struct physicsobject* obj, double* x, double* y);
void physics_get2dRotation_internal(struct physicsobject* obj, double* angle);
void physics_warp2d(struct physicsobject* obj, double x, double y, double angle);
void physics_apply2dImpulse_internal(struct physicsobject* obj, double forcex, double forcey, double sourcex, double sourcey);
#endif
#ifdef USE_PHYSICS3D
void physics_get3dPosition_internal(struct physicsobject* obj, double* x, double* y, double* z);
void physics_get3dRotationQuaternion_internal(struct physicsobject* obj, double* qx, double* qy, double* qz, double* qrot);
void physics_warp3d_internal(struct physicsobject* obj, double x, double y, double z, double qx, double qy, double qz, double qrot);
void physics_apply3dImpulse_internal(struct physicsobject* obj, double forcex, double forcey, double forcez, double sourcex, double sourcey, double sourcez);
#endif

// Change and get velocity
#ifdef USE_PHYSICS2D
void physics_get2dVelocity_internal(struct physicsobject* obj, double *vx, double* vy);
double physics_get2dAngularVelocity_internal(struct physicsobject* obj, double* omega);
void physics_set2dVelocity_internal(struct physicsobject* obj, double vx, double vy);
void physics_set2dAngularVelocity_internal(struct physicsobject* obj, double omega);
void physics_apply2dAngularImpulse_internal(struct physicsobject* obj, double impulse);
#endif
#ifdef USE_PHYSICS3D
void physics_get3dVelocity(struct physicsobject* obj, double *vx, double* vy,
 double* vz);
void physics_set3dVelocity(struct physicsobject* obj, double vx, double vy,
 double vz);
void physics_get3dAngularVelocityQuaternion_internal(struct physicsobject* obj,
 double* qx, double* qy, double* qz, double* qrot);
void physics_set3dAngularVelocityQuaternion_internal(struct physicsobject* obj,
 double qx, double qy, double qz, double qrot);
void physics_apply3dAngularImpulse_internal(struct physicsobject* obj,
 double qx, double qy, double qz, double qrot); // ? no idea if this is correct
#endif

// Collision test ray
#ifdef USE_PHYSICS2D
int physics_ray2d_internal(struct physicsworld* world, double startx, double starty, double targetx, double targety, double* hitpointx, double* hitpointy, struct physicsobject** objecthit, double* hitnormalx, double* hitnormaly); // returns 1 when something is hit, otherwise 0  -- XXX: not thread-safe!
#endif
#ifdef USE_PHYSICS3D
int physics_ray3d_internal(struct physicsworld* world, double startx, double starty, double startz, double targetx, double targety, double targetz, double* hitpointx, double* hitpointy, double* hitpointz, struct physicsobject** objecthit, double* hitnormalx, double* hitnormaly, double* hitnormalz); // returns 1 when something is hit, otherwise 0  -- XXX: not thread-safe!
#endif

#ifdef __cplusplus
}
#endif

#endif  // USE_PHYSICS2D || USE_PHYSICS3D

#endif  // BLITWIZARD_PHYSICSINTERNAL_H_

