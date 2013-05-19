
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

#ifndef BLITWIZARD_PHYSICS_H_
#define BLITWIZARD_PHYSICS_H_

#include "config.h"
#include "os.h"

#if (defined(USE_PHYSICS2D) || defined(USE_PHYSICS3D))

#ifdef __cplusplus
extern "C" {
#endif

// Create and destroy worlds:
struct physicsworld;
struct physicsobject;
struct physicsworld* physics_createWorld(int use3dphysics);
void physics_destroyWorld(struct physicsworld* world);
void physics_step(struct physicsworld* world);
int physics_getStepSize(struct physicsworld* world);

// Set a collision callback:
#ifdef USE_PHYSICS2D
void physics_set2dCollisionCallback(
struct physicsworld* world,
int (*callback)(void* userdata, struct physicsobject* a,
    struct physicsobject* b, double x, double y,
    double normalx, double normaly, double force),
void* userdata);
#endif
#ifdef USE_PHYSICS3D
void physics_set3dCollisionCallback(
struct physicsworld* world,
int (*callback)(void* userdata, struct physicsobject* a,
    struct physicsobject* b, double x, double y, double z,
    double normalx, double normaly, double normalz, double force),
void* userdata);
#endif
// The callback receives your specified userdata, the collidion objects,
// the position in the center of the collision/overlap,
// the collision penetration normal pointing from object b to object a,
// and the impact force's strength.
// If the callback returns 0, the collision will be ignored.
// If it returns 1, the collision will be processed.
//
// The callback will be called during physics_Step.
// All operations in the callback are supported including deleting the object
// for which the callback was called.

// Obtain shape info struct (for use in object creation):
struct physicsobjectshape;
struct physicsobjectshape* physics_createEmptyShapes(
int count);
void physics_destroyShapes(struct physicsobjectshape* shapes,
int count);
size_t physics_getShapeSize(void);  // get size of one shape struct
#define GET_SHAPE(shapes,x) (struct physicsobjectshape*) \
      (((char*)shapes)+physics_getShapeSize()*x)
#ifdef USE_PHYSICS2D
// Set the given shape to one of those specified shapes:
void physics_set2dShapeRectangle(
struct physicsobjectshape* shape, double width,
double height);
void physics_set2dShapeOval(
struct physicsobjectshape* shape, double width,
double height);
void physics_set2dShapeCircle(
struct physicsobjectshape* shape, double diameter);
// Use those commands multiple times to construct those more complex shapes:
void physics_add2dShapePolygonPoint(
struct physicsobjectshape* shape, double xoffset, double yoffset);
void physics_add2dShapeEdgeList(
struct physicsobjectshape* shape, double x1, double y1,
double x2, double y2);
// Use those commands to move your shapes around from the center:
void physics_set2dShapeOffsetRotation(
struct physicsobjectshape* shape, double xoffset,
double yoffset, double rotation);
void physics_get2dShapeOffsetRotation(
struct physicsobjectshape* shape, double* xoffset,
double* yoffset, double* rotation);
#endif
#ifdef USE_PHYSICS3D
// Set the given shape to one of those specified shapes:
void physics_set3dShapeDecal(struct physicsobjectshape* shape,
double width, double height);
void physics_set3dShapeBox(struct physicsobjectshape* shape,
double x_size, double y_size, double z_size);
void physics_set3dShapeBall(struct physicsobjectshape* shape,
double diameter);
void physics_set3dShapeEllipticBall(struct physicsobjectshape* shape,
double x_size, double y_size, double z_size);
// Use those commands multiple times to construct those more complex shapes:
void physics_add3dShapeMeshTriangle(struct physicsobjectshape* shape,
double x1, double y1, double z1,
double x2, double y2, double z2,
double x3, double y3, double z3);
// Use those commands to move your shapes around from the center:
void physics_set3dShapeOffsetRotation(struct physicsobjectshape* shape,
double xoffset, double yoffset, double zoffset, double rotation);
void physics_get3dShapeOffsetRotation(struct physicsobjectshape* shape,
double* xoffset, double* yoffset, double* zoffset, double* rotation);
#endif

// Create an object with a given pointer to a shapes array:
struct physicsobject* physics_createObject(struct physicsworld* world,
void* userdata, int movable, struct physicsobjectshape* shapelist,
int shapecount);

// Destroy objects or obtain their userdata:
void physics_destroyObject(struct physicsobject* object);
void* physics_getObjectUserdata(struct physicsobject* object);

// Scale object
#ifdef USE_PHYSICS2D
void physics_set2dScale(struct physicsobject* object, double scalex,
 double scaley);
#endif
#ifdef USE_PHYSICS3D
void physics_set3dScale(struct physicsobject* object, double scalex,
 double scaley, double scalez);
#endif

// Get/set various properties
void physics_setMass(struct physicsobject* obj, double mass);
double physics_getMass(struct physicsobject* obj);
#ifdef USE_PHYSICS2D
void physics_set2dMassCenterOffset(struct physicsobject* obj,
double offsetx, double offsety);
void physics_get2dMassCenterOffset(struct physicsobject* obj,
double* offsetx, double* offsety);
#endif
#ifdef USE_PHYSICS3D
void physics_set3dMassCenterOffset(struct physicsobject* obj,
double offsetx, double offsety, double offsetz);
void physics_get3dMassCenterOffset(struct physicsobject* obj,
double* offsetx, double* offsety, double* offsetz);
#endif
#ifdef USE_PHYSICS2D
void physics_set2dGravity(struct physicsobject* obj,
double x, double y);
void physics_set2dWorldGravity(struct physicsworld* world,
double x, double y);
#endif
#ifdef USE_PHYSICS3D
void physics_set3dGravity(struct physicsobject* obj,
double x, double y, double z);
void physics_set3dWorldGravity(struct physicsworld* world,
double x, double y, double z);
#endif
void physics_unsetGravity(struct physicsobject* obj);
#ifdef USE_PHYSICS2D
void physics_set2dRotationRestriction(struct physicsobject* obj,
int restricted);
#endif
#ifdef USE_PHYSICS3D
void physics_set3dRotationRestrictionAroundAxis(
struct physicsobject* obj, int restrictedaxisx,
int restrictedaxisy, int restrictedaxisz);
void physics_set3dRotationRestrictionAllAxis(
struct physicsobject* obj);
void physics_set3dNoRotationRestriction(void);
#endif
void physics_setFriction(struct physicsobject* obj,double friction);
void physics_setAngularDamping(struct physicsobject* obj, double damping);
void physics_setLinearDamping(struct physicsobject* obj, double damping);
void physics_setRestitution(struct physicsobject* obj, double restitution);

// Change and get position/rotation and apply impulses
#ifdef USE_PHYSICS2D
void physics_get2dPosition(struct physicsobject* obj, double* x, double* y);
void physics_get2dRotation(struct physicsobject* obj, double* angle);
void physics_warp2d(struct physicsobject* obj, double x, double y, double angle);
void physics_apply2dImpulse(struct physicsobject* obj, double forcex, double forcey, double sourcex, double sourcey);
#endif
#ifdef USE_PHYSICS3D
void physics_get3dPosition(struct physicsobject* obj, double* x, double* y, double* z);
void physics_get3dRotationQuaternion(struct physicsobject* obj, double* qx, double* qy, double* qz, double* qrot);
void physics_warp3d(struct physicsobject* obj, double x, double y, double z, double qx, double qy, double qz, double qrot);
void physics_apply3dImpulse(struct physicsobject* obj, double forcex, double forcey, double forcez, double sourcex, double sourcey, double sourcez);
#endif

// Change and get velocity
#ifdef USE_PHYSICS2D
void physics_get2dVelocity(struct physicsobject* obj, double *vx, double* vy);
double physics_get2dAngularVelocity(struct physicsobject* obj, double* omega);
void physics_set2dVelocity(struct physicsobject* obj, double vx, double vy);
void physics_set2dAngularVelocity(struct physicsobject* obj, double omega);
void physics_apply2dAngularImpulse(struct physicsobject* obj, double impulse);
#endif
#ifdef USE_PHYSICS3D
void physics_get3dVelocity(struct physicsobject* obj, double *vx, double* vy,
 double* vz);
void physics_set3dVelocity(struct physicsobject* obj, double vx, double vy,
 double vz);
void physics_get3dAngularVelocityQuaternion(struct physicsobject* obj,
 double* qx, double* qy, double* qz, double* qrot);
void physics_set3dAngularVelocityQuaternion(struct physicsobject* obj,
 double qx, double qy, double qz, double qrot);
void physics_apply3dAngularImpulse(struct physicsobject* obj,
 double qx, double qy, double qz, double qrot); // ? no idea if this is correct
#endif

// Collision test ray
#ifdef USE_PHYSICS2D
int physics_ray2d(
    struct physicsworld* world,
    double startx, double starty, double targetx, double targety,
    double* hitpointx, double* hitpointy,
    struct physicsobject** objecthit,
    double* hitnormalx, double* hitnormaly
); // returns 1 when something is hit, otherwise 0  -- XXX: not thread-safe!
#endif
#ifdef USE_PHYSICS3D
int physics_ray3d
    struct physicsworld* world, double startx, double starty,
    double startz, double targetx, double targety, double targetz,
    double* hitpointx, double* hitpointy, double* hitpointz,
    struct physicsobject** objecthit,
    double* hitnormalx, double* hitnormaly, double* hitnormalz
); // returns 1 when something is hit, otherwise 0  -- XXX: not thread-safe!
#endif

#ifdef __cplusplus
}
#endif

#endif  // USE_PHYSICS2D || USE_PHYSICS3D

#endif  // BLITWIZARD_PHYSICS_H_

