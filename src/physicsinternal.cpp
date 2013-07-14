
/* blitwizard game engine - source code file

  Copyright (C) 2012-2013 Shahriar Heidrich, Jonas Thiem

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

/* TODO:
    - Currently, specifying offset/rotation before setting up the shape will
     result in undefined behaviour. Change this?
    - Did some work to remedy the previous point - problem: if you change
     offsets/rotation in between the addition of polygons or edges, it's
     undefined behaviour again.
    - Another problem: It's terribly inefficient for 0 offsets/rotation.
    - Actually implement offsets, not just have them sit idly in the shape
     struct - DONE?
    - struct init stuff
    - Look at the edge and poly functions again (linked lists done correctly?)
    - Properly refactor the object creation function
    - Look for memory leaks (lol)
    - Add guard clauses for validating parameters (or don't)
*/

extern "C" {
#include "config.h"
#include "os.h"
}

#if defined(USE_PHYSICS2D) || defined(USE_PHYSICS3D)

#ifdef USE_PHYSICS2D
#include <Box2D.h>
#endif

#include <stdint.h>
#define _USE_MATH_DEFINES
#include <cmath>
#include <stdio.h>

#define EPSILON 0.0001

#include "physics.h"
#include "physicsinternal.h"
#include "mathhelpers.h"
#include "logging.h"

#ifndef NDEBUG
#define BW_E_NO3DYET "Error: 3D is not yet implemented."
#endif


extern "C" {

// TODO (cleanup, rename, ...):
/*
    Unsorted 2D-specific stuff
*/
class mycontactlistener;
class mycallback;

/*
    Enums, ...
*/

enum shape_types_2d { BW_S2D_RECT=0, BW_S2D_POLY, BW_S2D_CIRCLE, BW_S2D_EDGE,
 BW_S2D_UNINITIALISED };



/*
    Purely internal structs
*/

struct physicsobject2d;
struct physicsworld2d;
struct physicsobjectshape2d;

struct physicsobject3d;
struct physicsworld3d;
struct physicsobjectshape3d;

/*
    Structs
*/


/*
    2D-specific structs
*/

struct physicsworld2d {
    mycontactlistener* listener;
    b2World* w;
    double gravityx,gravityy;
    void* callbackuserdata; // FIXME: remove once appropriate
    int (*callback)(void* userdata, struct physicsobject2d* a, struct physicsobject2d* b, double x, double y, double normalx, double normaly, double force); // FIXME: remove once appropriate
};

#define DISABLEDCONTACTBLOCKSIZE 16
#define MAXDISABLEDBLOCKS 16

struct physicsobject2d {
#ifdef USE_PHYSICS2D
    int movable;
    b2World* world;
    b2Body* body;
    
    // needed for scaling, NULL on creation, init on scale
    // Array of pointers
    b2Shape** orig_shapes;
    int orig_shape_count;
    // XXX Is currently only allocated for shapes that need it (ugly as fuck).
    // I.e. length(orig_shape_info) <= length(orig_shapes)
    struct b2_shape_info* orig_shape_info;
    
    int gravityset;
    double gravityx,gravityy;
    void* userdata;
    struct physicsworld2d* pworld; // FIXME: remove once appropriate

    // we store disabled contacts here because
    // box2d won't let us keep it in b2Contact
    // (which sucks!)
    b2Contact** disabledContacts
        [MAXDISABLEDBLOCKS];  // list of blocks of disabled contacts
    int disabledContactCount;
    int disabledContactBlockCount;
#endif
};

/* This is not actually used for finalised objects, but only while setting up
 the shapes that will later be used to construct the final object. Maybe rename
 to reflect this fact.
 */
struct physicsobjectshape2d {
#ifdef USE_PHYSICS2D
    union specific_type_of_shape {
        struct rectangle2d* rectangle;
        struct polygonpoint* polygonpoints;
        b2CircleShape* circle;
        struct edge* edges;
    } b2;
    enum shape_types_2d type;
    double xoffset;
    double yoffset;
    double rotation;
#endif
};

struct bodyuserdata {
    void* userdata;
    struct physicsobject* pobj;
};

struct rectangle2d {
    double width, height;
};

struct edge {
    int inaloop;
    int processed;
    int adjacentcount;
    double x1,y1,x2,y2;
    struct edge* next;
    struct edge* adjacent1, *adjacent2;
};

struct polygonpoint {
  double x,y;
  struct polygonpoint* next;
};

struct physicsobject2dedgecontext {
    struct physicsobject2d* obj;
    double friction;
    struct edge* edgelist;
};

struct physicsjoint2d {
    // Not much to it
    b2Joint* b2joint;
};


/*
    3d-specific structs
*/

struct physicsobject3d {
    // TODO
};

struct physicsobjectshape3d {
    // TODO
};

struct physicsworld3d {
    // TODO
};

struct physicsjoint3d {
    // TODO
};

/*
    General (2D/3D) structs
*/

struct physicsobject {
    union {
        struct physicsobject2d object2d;
        struct physicsobject3d object3d;
    };
    int deleted; // 1: deleted inside collision callback, 0: everything normal
    int is3d;
    struct physicsworld* pworld;
};

struct physicsworld {
    union {
        struct physicsworld2d world2d;
        struct physicsworld3d world3d;
    };
    float stepsize;
    int is3d;
    void* callbackuserdata;
    int (*callback)(void* userdata, struct physicsobject* a, struct physicsobject* b, double x, double y, double normalx, double normaly, double force);
};

struct physicsjoint {
    union {
        struct physicsjoint2d joint2d;
        struct physicsjoint3d joint3d;
    };
    struct physicsworld* world;
};

struct physicsobjectshape {
    union {
        struct physicsobjectshape2d shape2d;
        struct physicsobjectshape3d shape3d;
    };
    int is3d;
};

struct b2_shape_info {
    // b2ChainShape only
    int is_loop; // or chain if not
    //no: struct b2_shape_info* next;
};


/*
    Purely internal functions
*/

void _physics_destroy2dShape(struct physicsobjectshape2d* shape);
int _physics_check2dEdgeLoop(struct edge* edge, struct edge* target);
void _physics_add2dShapeEdgeList_Do(struct physicsobjectshape* shape, double x1, double y1, double x2, double y2);
inline void _physics_rotateThenOffset2dPoint(double xoffset, double yoffset,
 double rotation, double* x, double* y);
void _physics_apply2dOffsetRotation(struct physicsobjectshape2d* shape2d,
 b2Vec2* targets, int count);
static int _physics_create2dObj(struct physicsworld2d* world,
struct physicsobject* object, void* userdata, int movable);
void _physics_create2dObjectEdges_End(struct edge* edges, struct physicsobject2d* object, struct physicsobjectshape2d* shape2d);
void _physics_create2dObjectPoly_End(struct polygonpoint* polygonpoints, struct physicsobject2d* object, struct physicsobjectshape2d* shape2d);
// Scaling aux
void _physics_setb2ShapeOval(b2Shape* shape, double width, double height);
b2ChainShape* _physics_copyb2ChainShape(b2ChainShape* src, b2_shape_info*
 shape_info);
b2CircleShape* _physics_copyb2CircleShape(b2CircleShape* src, b2_shape_info*
 shape_info);
b2PolygonShape* _physics_copyb2PolygonShape(b2PolygonShape* src, b2_shape_info*
 shape_info);
void _physics_fill2dOrigShapeCache(struct physicsobject2d* object);
void _physics_delete2dOrigShapeCache(struct physicsobject2d* object);


class mycontactlistener : public b2ContactListener {
public:
    mycontactlistener();
    ~mycontactlistener();
private:
    void PreSolve(b2Contact *contact, const b2Manifold *oldManifold);
    void BeginContact(b2Contact* contact);
    void EndContact(b2Contact* contact);
};

mycontactlistener::mycontactlistener() {return;}
mycontactlistener::~mycontactlistener() {return;}

static void physics_handleContact(b2Contact* contact);

void mycontactlistener::PreSolve(b2Contact *contact, const b2Manifold *oldManifold) {
    physics_handleContact(contact);
}

void mycontactlistener::BeginContact(b2Contact* contact) {
    physics_handleContact(contact);
}

void mycontactlistener::EndContact(b2Contact* contact) {
    // since we assume all EndContact events roughly occur
    // at the same point (when stepping of all contacts is
    // finished), we no longer want to keep the disabled
    // contact information - it was kept to avoid refiring
    // the events during presolve, which is now done with
    // endcontact.
    // Hence, clear all disabled contacts up:
    struct physicsobject* obj1 = ((struct bodyuserdata*)contact->GetFixtureA()
        ->GetBody()->GetUserData())->pobj;
    struct physicsobject* obj2 = ((struct bodyuserdata*)contact->GetFixtureB()
        ->GetBody()->GetUserData())->pobj;
    obj1->object2d.disabledContactCount = 0;
    obj2->object2d.disabledContactCount = 0;
}

#include "timefuncs.h"

// store a contact as disabled up to the 
static void physics_storeDisabledContact(struct physicsobject* obj,
b2Contact* contact) {
    // check if we need more disabled contact blocks:
    if (obj->object2d.disabledContactCount+1 >
    obj->object2d.disabledContactBlockCount *
    DISABLEDCONTACTBLOCKSIZE) {
        if (obj->object2d.disabledContactBlockCount+1 >=
        MAXDISABLEDBLOCKS) {
            // we ran out of disabled contact space.
            // we won't store it then (this is simply a performance
            // problem, not a huge issue).
            return;
        }

        // allocate new block:
        obj->object2d.disabledContacts[obj->object2d.
        disabledContactBlockCount] = 
        (struct b2Contact**)malloc(sizeof(void*) *
        DISABLEDCONTACTBLOCKSIZE);

        if (!obj->object2d.disabledContacts[obj->object2d.
        disabledContactBlockCount]) {
            // allocation failed! nothing we can do
            return;
        }

        // we need a larger disabled contact list:
        obj->object2d.disabledContactBlockCount++;
    }
    // figure out the index where we want to store the
    // disabled contact:
    int i = obj->object2d.disabledContactCount;
    int block = 0;
    while (i >= DISABLEDCONTACTBLOCKSIZE) {
        i -= DISABLEDCONTACTBLOCKSIZE;
        block++;
    }

    // store it:
    obj->object2d.disabledContacts[block][i]
     = contact;

    obj->object2d.disabledContactCount++;
}

static int physics_contactIsDisabled(struct physicsobject* obj,
b2Contact* contact) {
    int i = 0;
    int c = 0;
    while (i < obj->object2d.disabledContactBlockCount) {
        int k = 0;
        while (k < DISABLEDCONTACTBLOCKSIZE &&
        c < obj->object2d.disabledContactCount) {
            if (obj->object2d.disabledContacts[i][k]
             == contact) {
                return 1;
            }
            k++;
            c++;
        }
        i++;
    }
    return 0;
}

// handle the contact event and pass it on to the physics callback:
static void physics_handleContact(b2Contact* contact) {
    //printf("handlecontact0: %llu\n", time_GetMicroseconds());
    struct physicsobject* obj1 = ((struct bodyuserdata*)contact->GetFixtureA()
        ->GetBody()->GetUserData())->pobj;
    struct physicsobject* obj2 = ((struct bodyuserdata*)contact->GetFixtureB()
        ->GetBody()->GetUserData())->pobj;
    if (obj1->deleted || obj2->deleted) {
        // one of the objects should be deleted already, ignore collision
        contact->SetEnabled(false);
        return;
    }

    // if the contact is remembered as disabled,
    // bail out here:
    if (physics_contactIsDisabled(obj1, contact) ||
    physics_contactIsDisabled(obj2, contact)) {
        contact->SetEnabled(false);
        return;
    }

    // get collision point
    // (this is never really accurate, but mostly sufficient)
    int n = contact->GetManifold()->pointCount;
    b2WorldManifold wmanifold;
    contact->GetWorldManifold(&wmanifold);
    float collidex = wmanifold.points[0].x;
    float collidey = wmanifold.points[0].y;
    float divisor = 1;
    int i = 1;
    while (i < n) {
        collidex += wmanifold.points[i].x;
        collidey += wmanifold.points[i].y;
        divisor += 1;
        i++;
    }
    collidex /= divisor;
    collidey /= divisor;

    // get collision normal ("push out" direction)
    float normalx = wmanifold.normal.x;
    float normaly = wmanifold.normal.y;

    // impact force:
    float impact = contact->GetManifold()->points[0].normalImpulse; //oldManifold->points[0].normalImpulse; //impulse->normalImpulses[0];

    // find our current world
    struct physicsworld* w = obj1->pworld;

    // return the information through the callback
    contact->SetEnabled(false);
    if (w->callback) {
        if (!w->callback(w->callbackuserdata, obj1, obj2,
        collidex, collidey, normalx, normaly, impact)) {
            // contact should be disabled:
            contact->SetEnabled(false);

            // remember it being disabled:
            physics_storeDisabledContact(obj1, contact);
            physics_storeDisabledContact(obj2, contact);
        } else {
            // contact should remain enabled:
            contact->SetEnabled(true);
        }
    }
    //printf("handlecontact1: %llu\n", time_GetMicroseconds());
}

/*
    Struct helper functions
*/
inline int _physics_objIs3D(struct physicsobject* object) {
    return object->is3d;
}

inline void _physics_setObjIs3D(struct physicsobject* object, int is3d) {
    object->is3d = is3d;
}

inline int _physics_objIsInit(struct physicsobject* object) {
    if (object->is3d == -1)
        return 0;
    else
        return 1;
}

inline void _physics_resetObj(struct physicsobject* object) {
    object->is3d = -1;
}

inline int _physics_worldIs3D(struct physicsworld* world) {
    return world->is3d;
}

inline void _physics_setWorldIs3D(struct physicsworld* world, int is3d) {
    world->is3d = is3d;
}

// Isn't really needed since worlds are always initialised from the very beginning, anyway...
inline int _physics_worldIsInit(struct physicsworld* world) {
    if (world->is3d == -1)
        return 0;
    else
        return 1;
}

inline void _physics_resetWorld(struct physicsworld* world) {
    world->is3d = -1;
}

inline int _physics_shapeIs3D(struct physicsobjectshape* shape) {
    return shape->is3d;
}

inline void _physics_setShapeIs3D(struct physicsobjectshape* shape, int is3d) {
    shape->is3d = is3d;
}

inline int _physics_shapeIsInit(struct physicsobjectshape* shape) {
    if (shape->is3d == -1)
        return 0;
    else
        return 1;
}

// -1: None; 0: 2D; 1: 3D
inline int _physics_shapeType(struct physicsobjectshape* shape) {
    return shape->is3d;
}

inline void _physics_resetShape(struct physicsobjectshape* shape) {
    shape->is3d = -1;
}


/*
    Functions
*/
struct physicsworld* physics_createWorld(int use3dphysics) {
    struct physicsworld* world = (struct physicsworld*)malloc(sizeof(*world));
    if (!world) {
        return NULL;
    }
    memset(world, 0, sizeof(*world));
    if (not use3dphysics) {
#ifdef USE_PHYSICS2D
        struct physicsworld2d* world2d = &(world->world2d);
        if (!world2d) {
            return NULL;
        }
        memset(world2d, 0, sizeof(*world2d));
        b2Vec2 gravity(0.0f, 0.0f);
        world2d->w = new b2World(gravity);
        world2d->w->SetAllowSleeping(true);
        world2d->gravityx = 0;
        world2d->gravityy = 9.81;
        world2d->listener = new mycontactlistener();
        world2d->w->SetContactListener(world2d->listener);
        _physics_setWorldIs3D(world, 0);
        
        // set step size now that everything else is initialised:
        world->stepsize = 1.0f/(1000.0f/(float)physics_getStepSize(world));
        return world;
#else
        printerror("Error: Trying to create 2D physics world, but USE_PHYSICS2D is disabled.");
        return NULL;
#endif
    } else {
#ifdef USE_PHYSICS3D
        printerror(BW_E_NO3DYET);
        return NULL;
#else
        printerror("Error: Trying to create 3D physics world, but USE_PHYSICS3D is disabled.");
        return NULL;
#endif
    }
}

void physics_destroyWorld(struct physicsworld* world) {
    if (not world->is3d) {
        delete world->world2d.listener;
        delete world->world2d.w;
        free(world);
    } else {
        printerror(BW_E_NO3DYET);
    }
}

int physics_worldIs3d_internal(struct physicsworld* world) {
    return _physics_worldIs3D(world);
}

void physics_step(struct physicsworld* world) {
    if (!world->is3d) {
#ifdef USE_PHYSICS2D
        struct physicsworld2d* world2d = &(world->world2d);
        // Do a collision step
        int i = 0;
        while (i < 2) {
            double forcefactor = (1.0/(1000.0f/physics_getStepSize(world)))*2;
            b2Body* b = world2d->w->GetBodyList();
            while (b) {
                // obtain physics object struct from body
                struct physicsobject* obj = ((struct bodyuserdata*)b->GetUserData())->pobj;
                struct physicsobject2d* obj2d = &obj->object2d;
                if (obj) {
                    if (obj2d->gravityset) {
                        // custom gravity which we want to apply
                        b->ApplyLinearImpulse(b2Vec2(obj2d->gravityx * forcefactor, obj2d->gravityy * forcefactor), b2Vec2(b->GetPosition().x, b->GetPosition().y));
                    } else {
                        // no custom gravity -> apply world gravity
                        b->ApplyLinearImpulse(b2Vec2(world2d->gravityx * forcefactor, world2d->gravityy * forcefactor), b2Vec2(b->GetPosition().x, b->GetPosition().y));
                    }
                }
                b = b->GetNext();
            }
#if defined(ANDROID) || defined(__ANDROID__)
            // less accurate on Android
            int it1 = 4;
            int it2 = 2;
#else
            // more accurate on desktop
            int it1 = 8;
            int it2 = 3;
#endif
            world2d->w->Step(1.0 /(1000.0f/physics_getStepSize(world)), it1, it2);
            i++;
        }
#endif
    } else {
        printerror(BW_E_NO3DYET);
    }
}

int physics_getStepSize(struct physicsworld* world) {
    // TODO: 3D?
#if defined(ANDROID) || defined(__ANDROID__)
    // less accurate physics on Android (40 FPS)
    return (1000/40);
#else
    // more accurate physics on desktop (60 FPS)
    return (1000/60);
#endif
}


#ifdef USE_PHYSICS2D
void physics_set2dCollisionCallback_internal(struct physicsworld* world, int (*callback)(void* userdata, struct physicsobject* a, struct physicsobject* b, double x, double y, double normalx, double normaly, double force), void* userdata) {
    world->callback = callback;
    world->callbackuserdata = userdata;
}
#endif

#ifdef USE_PHYSICS3D
void physics_set3dCollisionCallback(struct physicsworld* world, int (*callback)(void* userdata, struct physicsobject* a, struct physicsobject* b, double x, double y, double z, double normalx, double normaly, double normalz, double force), void* userdata) {
    printerror(BW_E_NO3DYET);
}
#endif


/* Shapes start here I guess */
struct physicsobjectshape* physics_createEmptyShapes(int count) {
    struct physicsobjectshape* shapes = (struct physicsobjectshape*)malloc((sizeof(*shapes))*count);
    if (!shapes) {
        return NULL;
    }
    int i = 0;
    while (i < count) {
        _physics_resetShape(&(shapes[i]));
        ++i;
    }
    return shapes;
}

#ifdef USE_PHYSICS2D
void _physics_destroy2dShape(struct physicsobjectshape2d* shape) {
    switch ((int)(shape->type)) {
        case BW_S2D_RECT:
            free(shape->b2.rectangle);
        break;
        case BW_S2D_POLY:
            struct polygonpoint* p,* p2;
            p = shape->b2.polygonpoints;
            p2 = NULL;
            while (p != NULL) {
                p2 = p->next;
                free(p);
            }
            free(p2);
        break;
        case BW_S2D_CIRCLE:
            delete shape->b2.circle;
        break;
        case BW_S2D_EDGE:
            struct edge* e,* e2;
            e = shape->b2.edges;
            e2 = NULL;
            while (e != NULL) {
                e2 = e->next;
                free(e);
            }
            free(e2);
        break;
        default:
            printerror("Error: Unknown 2D shape type.");
    }
}
#endif

void physics_destroyShapes(struct physicsobjectshape* shapes, int count) {
    int i = 0;
    while (i < count) {
        switch (_physics_shapeType(&(shapes[i]))) {
            case -1:
                // not initialised -> do nothing special
            break;
            case 0:
#ifdef USE_PHYSICS2D
                // 2D
                _physics_destroy2dShape(&(shapes[i].shape2d));
#endif
            break;
            case 1:
#ifdef USE_PHYSICS3D
                // 3D
                printerror(BW_E_NO3DYET);
#endif
            break;
        }
        ++i;
    }
    // do this no matter what
    free(shapes);
}

size_t physics_getShapeSize(void) {
    return sizeof(struct physicsobjectshape*);
}


#ifdef USE_PHYSICS2D
void _physics_createEmpty2dShape(struct physicsobjectshape2d* shape2d) {
    shape2d->type = BW_S2D_UNINITIALISED;
    shape2d->xoffset = 0;
    shape2d->yoffset = 0;
    shape2d->rotation = 0;
}
#endif

#ifdef USE_PHYSICS2D
void physics_set2dShapeRectangle(struct physicsobjectshape* shape, double width, double height) {
/*
 it is of critical importance for all of this shit (i.e. functions below as well) that the object is absolutely uninitialised
   or had its former shape data deleted when this function (or functions below) is called. otherwise:
   XXX MEMORY LEAKS XXX
  checks needed?
*/
    
#ifndef NDEBUG
    if (shape == NULL) {
        printerror("Trying to apply physics_Set2dShapeRectangle() to NULL "\
         "shape.");
        return;
    }
    switch (_physics_shapeType(shape)) {
        case 1:
            if (shape->shape2d.type != BW_S2D_UNINITIALISED) {
                printerror("Trying to apply physics_Set2dShapeRectangle() to "
                 "already initialised shape.");
                return;
            }
        break;
        case 2:
            printerror("Trying to apply physics_Set2dShapeRectangle() to 3D "\
             "shape.");
            return;
        break;
    }
#endif
    
    struct rectangle2d* rectangle = (struct rectangle2d*)malloc(
     sizeof(*rectangle));
    if (!rectangle)
        return;
    
    if (_physics_shapeType(shape) != 1) {
        _physics_createEmpty2dShape(&(shape->shape2d));
    }
    
    rectangle->width = width;
    rectangle->height = height;
    shape->shape2d.b2.rectangle = rectangle;
    shape->shape2d.type = BW_S2D_RECT;
    
    shape->is3d = 0;
}
#endif

#ifdef USE_PHYSICS2D
#define OVALVERTICES 8
void physics_set2dShapeOval(struct physicsobjectshape* shape, double width, double height) {
    if (fabs(width - height) < EPSILON) {
        physics_set2dShapeCircle(shape, width);
        return;
    }

    //construct oval shape - by manually calculating the vertices
    struct polygonpoint* vertices = (struct polygonpoint*)malloc(sizeof(*vertices)*OVALVERTICES);
    
    if (_physics_shapeType(shape) != 1) {
        _physics_createEmpty2dShape(&(shape->shape2d));
    }
    
    
    //go around with the angle in one full circle:
    int i = 0;
    double angle = 0;
    while (angle < 2*M_PI && i < OVALVERTICES) {
        //calculate and set vertex point
        double x,y;
        ovalpoint(angle, width, height, &x, &y);
        
        vertices[i].x = x;
        vertices[i].y = -y;
        vertices[i].next = &vertices[i+i];
        
        //advance to next position
        angle -= (2*M_PI)/((double)OVALVERTICES);
        i++;
    }
    vertices[i-1].next = NULL;
    
    shape->shape2d.b2.polygonpoints = vertices;
    shape->shape2d.type = BW_S2D_POLY;

    shape->is3d = 0;
}
#endif

#ifdef USE_PHYSICS2D
void physics_set2dShapeCircle(struct physicsobjectshape* shape, double diameter) {
    b2CircleShape* circle = new b2CircleShape;
    double radius = diameter/2;
    circle->m_radius = radius - 0.01;
    
    if (_physics_shapeType(shape) != 1) {
        _physics_createEmpty2dShape(&(shape->shape2d));
    }
    
    shape->shape2d.b2.circle = circle;
    shape->shape2d.type = BW_S2D_CIRCLE;

    shape->is3d = 0;
}
#endif

#ifdef USE_PHYSICS2D
void physics_add2dShapePolygonPoint(struct physicsobjectshape* shape, double xoffset, double yoffset) {
    if (_physics_shapeType(shape) != 1) {
        _physics_createEmpty2dShape(&(shape->shape2d));
    }
    
    if (not shape->shape2d.type == BW_S2D_POLY) {
        shape->shape2d.b2.polygonpoints = NULL;
    }
    struct polygonpoint* p = shape->shape2d.b2.polygonpoints;
    
    struct polygonpoint* new_point = (struct polygonpoint*)malloc(sizeof(*new_point));
    new_point->x = xoffset;
    new_point->y = yoffset;
    
    if (p != NULL) {
      new_point->next = p;
    }
    p = new_point;
    
    shape->shape2d.type = BW_S2D_POLY;
    
    shape->is3d = 0;
}
#endif

#ifdef USE_PHYSICS2D
int _physics_check2dEdgeLoop(struct edge* edge, struct edge* target) {
    struct edge* e = edge;
    struct edge* eprev = NULL;
    while (e) {
        if (e == target) {return 1;}
        struct edge* nextprev = e;
        if (e->adjacent1 && e->adjacent1 != eprev) {
            e = e->adjacent1;
        } else {
            if (e->adjacent2 && e->adjacent2 != eprev) {
                e = e->adjacent2;
            } else {
                e = NULL;
            }
        }
        eprev = nextprev;
    }
    return 0;
}
#endif

#ifdef USE_PHYSICS2D
void _physics_add2dShapeEdgeList_Do(struct physicsobjectshape* shape, double x1, double y1, double x2, double y2) {
    struct edge* newedge = (struct edge*)malloc(sizeof(*newedge));
    if (!newedge) {return;}
    memset(newedge, 0, sizeof(*newedge));
    newedge->x1 = x1;
    newedge->y1 = y1;
    newedge->x2 = x2;
    newedge->y2 = y2;
    newedge->adjacentcount = 1;
    
    //search for adjacent edges
    struct edge* e = shape->shape2d.b2.edges;
    while (e) {
        if (!newedge->adjacent1) {
            if (fabs(e->x1 - newedge->x1) < EPSILON && fabs(e->y1 - newedge->y1) < EPSILON && e->adjacent1 == NULL) {
                if (_physics_check2dEdgeLoop(e, newedge)) {
                    newedge->inaloop = 1;
                } else {
                    e->adjacentcount += newedge->adjacentcount;
                    newedge->adjacentcount = e->adjacentcount;
                }
                newedge->adjacent1 = e;
                e->adjacent1 = newedge;
                e = e->next;
                continue;
            }
            if (fabs(e->x2 - newedge->x1) < EPSILON && fabs(e->y2 - newedge->y1) < EPSILON && e->adjacent2 == NULL) {
                if (_physics_check2dEdgeLoop(e, newedge)) {
                    newedge->inaloop = 1;
                } else {
                    e->adjacentcount += newedge->adjacentcount;
                    newedge->adjacentcount = e->adjacentcount;
                }
                newedge->adjacent1 = e;
                e->adjacent2 = newedge;
                e = e->next;
                continue;
            }
        }
        if (!newedge->adjacent2) {
            if (fabs(e->x1 - newedge->x2) < EPSILON && fabs(e->y1 - newedge->y2) < EPSILON && e->adjacent1 == NULL) {
                if (_physics_check2dEdgeLoop(e, newedge)) {
                    newedge->inaloop = 1;
                } else {
                    e->adjacentcount += newedge->adjacentcount;
                    newedge->adjacentcount = e->adjacentcount;
                }
                newedge->adjacent2 = e;
                e->adjacent1 = newedge;
                e = e->next;
                continue;
            }
            if (fabs(e->x2 - newedge->x2) < EPSILON && fabs(e->y2 - newedge->y2) < EPSILON && e->adjacent2 == NULL) {
                if (_physics_check2dEdgeLoop(e, newedge)) {
                    newedge->inaloop = 1;
                } else {
                    e->adjacentcount += newedge->adjacentcount;
                    newedge->adjacentcount = e->adjacentcount;
                }
                newedge->adjacent2 = e;
                e->adjacent2 = newedge;
                e = e->next;
                continue;
            }
        }
        e = e->next;
    }

    //add us to the unsorted linear list
    newedge->next = shape->shape2d.b2.edges;
    shape->shape2d.b2.edges = newedge;
}
#endif

#ifdef USE_PHYSICS2D
void physics_add2dShapeEdgeList(struct physicsobjectshape* shape, double x1, double y1, double x2, double y2) {
    if (_physics_shapeType(shape) != 1) {
        _physics_createEmpty2dShape(&(shape->shape2d));
    }
    
    // FIXME FIXME FIXME this function might be complete bollocks, reconsider pls
    if (not shape->shape2d.type == BW_S2D_EDGE) {
        shape->shape2d.b2.edges = NULL;
    }
    
    _physics_add2dShapeEdgeList_Do(shape, x1, y1, x2, y2);
    
    struct edge* e = shape->shape2d.b2.edges;
    while (e != NULL and e->next != NULL) {
        e = e->next;
    }
    struct edge* new_edge = (struct edge*)malloc(sizeof(*new_edge));
    new_edge->x1 = x1;
    new_edge->y1 = y1;
    new_edge->x2 = x2;
    new_edge->y2 = y2;
    e->next = new_edge;
    
    shape->shape2d.type = BW_S2D_EDGE;

    shape->is3d = 0;
}
#endif

#ifdef USE_PHYSICS2D
inline void _physics_rotateThenOffset2dPoint(double xoffset, double yoffset,
 double rotation, double* x, double* y) {
    rotatevec(*x, *y, rotation, x, y);
    *x += xoffset;
    *y += yoffset;
}
#endif

#ifdef USE_PHYSICS2D
void _physics_apply2dOffsetRotation(struct physicsobjectshape2d* shape2d,
 b2Vec2* targets, int count) {
    int i = 0;
    double x,y;
    while (i < count) {
        x = targets[i].x;
        y = targets[i].y;
        _physics_rotateThenOffset2dPoint(shape2d->xoffset, shape2d->yoffset,
         shape2d->rotation, &x, &y);
        targets[i].x = x;
        targets[i].y = y;
        
        ++i;
    }
}
#endif

#ifdef USE_PHYSICS2D
void physics_set2dShapeOffsetRotation(struct physicsobjectshape* shape, double xoffset, double yoffset, double rotation) {
    if (_physics_shapeType(shape) != 1) {
        _physics_createEmpty2dShape(&(shape->shape2d));
        printerror("Shouldn't be doing this: Set2dShapeOffsetRotation before "\
         "having assigned any shape data.");
    }
    
    struct physicsobjectshape2d* s = &(shape->shape2d);
    s->xoffset = xoffset;
    s->yoffset = yoffset;
    s->rotation = rotation;
}
#endif

#ifdef USE_PHYSICS2D
void physics_get2dShapeOffsetRotation(struct physicsobjectshape* shape, double* xoffset, double* yoffset, double* rotation) {
    struct physicsobjectshape2d* s = &(shape->shape2d);
    *xoffset = s->xoffset;
    *yoffset = s->yoffset;
    *rotation = s->rotation;
}
#endif

// Everything about object creation starts here
#ifdef USE_PHYSICS2D
static int _physics_create2dObj(struct physicsworld2d* world,
struct physicsobject* object, void* userdata, int movable) {
    struct physicsobject2d* obj2d = &object->object2d;
    memset(obj2d, 0, sizeof(*obj2d));

    struct bodyuserdata* pdata = (struct bodyuserdata*)malloc(sizeof(*pdata));
    if (!pdata) {
        return 0;
    }
    memset(pdata, 0, sizeof(*pdata));
    pdata->userdata = userdata;
    pdata->pobj = object;

    b2BodyDef bodyDef;
    if (movable) {
        bodyDef.type = b2_dynamicBody;
    }
    obj2d->movable = movable;
    bodyDef.userData = (void*)pdata;
    obj2d->userdata = pdata;
    obj2d->body = world->w->CreateBody(&bodyDef);
    obj2d->body->SetFixedRotation(false);
    obj2d->world = world->w;
    obj2d->pworld = world;
    if (!obj2d->body) {
        free(pdata);
        return 0;
    }
    return 1;
}
#endif

#ifdef USE_PHYSICS2D
// TODO: Weaken coupling, i.e. no direct reference to physicsobject2d,
// instead take edges and return b2 chains
// (goal: common code for fixture etc. in outer function)
// problem: memory mgmt., variable number of returned edge shapes, scaling sh-t
void _physics_create2dObjectEdges_End(struct edge* edges,
 struct physicsobject2d* object, struct physicsobjectshape2d* shape2d) {
    /* not sure if this is needed anymore, probably not
    if (!context->edgelist) {
        physics2d_DestroyObject(context->obj);
        free(context);
        return NULL;
    }*/
    
    int num_shapes = 0;
    struct edge* e = edges;
    while (e) {
        //skip edges we already processed
        if (e->processed) {
            e = e->next;
            continue;
        }

        //only process edges which are start of an adjacent chain, in a loop or lonely
        if (e->adjacent1 && e->adjacent2 && !e->inaloop) {
            e = e->next;
            continue;
        }

        int varraysize = e->adjacentcount+1;
        b2Vec2* varray = new b2Vec2[varraysize];
        b2ChainShape chain;
        e->processed = 1;

        //see into which direction we want to go
        struct edge* eprev = e;
        struct edge* e2;
        if (e->adjacent1) {
            varray[0] = b2Vec2(e->x2, e->y2);
            varray[1] = b2Vec2(e->x1, e->y1);
            e2 = e->adjacent1;
        } else {
            varray[0] = b2Vec2(e->x1, e->y1);
            varray[1] = b2Vec2(e->x2, e->y2);
            e2 = e->adjacent2;
        }

        //ok let's take a walk:
        int i = 2;
        while (e2) {
            if (e2->processed) {break;}
            e2->processed = 1;
            struct edge* enextprev = e2;

            //Check which vertex we want to add
            if (e2->adjacent1 == eprev) {
                varray[i] = b2Vec2(e2->x2, e2->y2);
            } else {
                varray[i] = b2Vec2(e2->x1, e2->y1);
            }

            //advance to next edge
            if (e2->adjacent1 && e2->adjacent1 != eprev) {
                e2 = e2->adjacent1;
            } else {
                if (e2->adjacent2 && e2->adjacent2 != eprev) {
                    e2 = e2->adjacent2;
                } else {
                    e2 = NULL;
                }
            }
            eprev = enextprev;
            i++;
        }

        //debug print values
        /*int u = 0;
        while (u < e->adjacentcount + 1 - (1 * e->inaloop)) {
            printf("Chain vertex: %f, %f\n", varray[u].x, varray[u].y);
            u++;
        }*/
        
        // Rotate and offset every vector in varray
        _physics_apply2dOffsetRotation(shape2d, varray, varraysize);
        
        // Needed for scaling lolmao good coding 10/10
        // TODO: put into a separate fn, make the whole thing more oop
        num_shapes++;
        object->orig_shape_count = num_shapes;
        object->orig_shape_info = (b2_shape_info*) realloc(
         object->orig_shape_info, sizeof(*(object->orig_shape_info)) *
         num_shapes);
        
        //construct an edge shape from this
        if (e->inaloop) {
            chain.CreateLoop(varray, e->adjacentcount);
            // More scaling workaround stuff
            object->orig_shape_info[num_shapes].is_loop = 1;
        } else {
            chain.CreateChain(varray, e->adjacentcount+1);
            // And again
            object->orig_shape_info[num_shapes].is_loop = 0;
        }

        //add it to our body
        b2FixtureDef fixtureDef;
        fixtureDef.shape = &chain;
        fixtureDef.friction = 0.5; // TODO: ???
        fixtureDef.density = 1; // TODO: ???
        object->body->CreateFixture(&fixtureDef);

        delete[] varray;
    }
}
#endif

#ifdef USE_PHYSICS2D
// TODO: cf. function above
void _physics_create2dObjectPoly_End(struct polygonpoint* polygonpoints,
 struct physicsobject2d* object, struct physicsobjectshape2d* shape2d) {
    struct polygonpoint* p = polygonpoints;
    // TODO: cache this instead?
    int i = 0;
    while (p != NULL) {
        ++i;
        p = p->next;
    }
    b2Vec2* varray = new b2Vec2[i];
    p = polygonpoints;
    i = 0;
    while (p != NULL) {
        varray[i].Set(p->x, p->y);
        ++i;
        p = p->next;
    }
    _physics_apply2dOffsetRotation(shape2d, varray, i);
    
    b2PolygonShape shape;
    shape.Set(varray, i);
    
    b2FixtureDef fixtureDef;
    fixtureDef.shape = &shape;
    fixtureDef.friction = 1; // TODO: ???
    fixtureDef.density = 1; // TODO: ???
    object->body->CreateFixture(&fixtureDef);
    
    delete[] varray;
}
#endif

struct physicsobject* physics_createObject_internal(struct physicsworld* world,
 void* userdata, int movable, struct physicsobjectshape* shapelist,
 int shapecount) {
    struct physicsobject* obj = (struct physicsobject*)malloc(sizeof(*obj));
    memset(obj, 0, sizeof(*obj));
    if (!(world->is3d)) {
#ifdef USE_PHYSICS2D
        if (!_physics_create2dObj(&(world->world2d), obj, userdata,
        movable)) {
            free(obj);
            return NULL;
        }
        
        struct physicsobjectshape* s = shapelist;
        int i = 0;
        while (i < shapecount) {
            if (_physics_shapeType(s) != 0) {
                // TODO: error msg?
                break;
            }
            b2FixtureDef fixtureDef;
            switch ((int)(s->shape2d.type)) {
                case BW_S2D_RECT:
                    // TODO: bit messy (no memory avail. check)
                    fixtureDef.shape = new b2PolygonShape;
                    ((b2PolygonShape*)(fixtureDef.shape))->SetAsBox(
                     s->shape2d.b2.rectangle->width,
                     s->shape2d.b2.rectangle->height,
                     b2Vec2(s->shape2d.xoffset, s->shape2d.yoffset),
                     s->shape2d.rotation);
                    fixtureDef.friction = 1; // TODO: ???
                    fixtureDef.density = 1;
                    obj->object2d.body->SetFixedRotation(false);
                    obj->object2d.body->CreateFixture(&fixtureDef);
                    delete fixtureDef.shape;
                break;
                case BW_S2D_POLY:
                    _physics_create2dObjectPoly_End(s->shape2d.b2.polygonpoints,
                     &obj->object2d, &(s->shape2d));
                break;
                case BW_S2D_CIRCLE:
                    fixtureDef.shape = (new b2CircleShape);
                    ((b2CircleShape*)(fixtureDef.shape))->m_p = b2Vec2(
                     s->shape2d.xoffset, s->shape2d.yoffset);
                    ((b2CircleShape*)(fixtureDef.shape))->m_radius = s->shape2d.\
                     b2.circle->m_radius;
                    fixtureDef.friction = 1; // TODO: ???
                    fixtureDef.density = 1;
                    obj->object2d.body->SetFixedRotation(false);
                    obj->object2d.body->CreateFixture(&fixtureDef);
                    delete fixtureDef.shape;
                break;
                case BW_S2D_EDGE:
                    _physics_create2dObjectEdges_End(s->shape2d.b2.edges,
                    &obj->object2d, &(s->shape2d));
                    break;
            }
            s += 1;
            ++i;
        }
        
        obj->is3d = 0;
#endif
    } else {
#ifdef USE_PHYSICS3D
        printerror(BW_E_NO3DYET);
#endif
    }
    if (!obj) return NULL;
    // Dimension-independent
    if (movable)
        physics_setMass_internal(obj, 1);
    else
        physics_setMass_internal(obj, 0);
    obj->pworld = world;
    return obj;
}


// Everything about object deletion starts here
static void _physics_destroyObjectDo(struct physicsobject* obj) {
    if (!obj->is3d) {
#ifdef USE_PHYSICS2D
        _physics_delete2dOrigShapeCache(&(obj->object2d));
        if (obj->object2d.body) {
            obj->object2d.world->DestroyBody(obj->object2d.body);
        }
        if (obj->object2d.disabledContactBlockCount > 0) {
            int i = 0;
            while (i < obj->object2d.disabledContactBlockCount) {
                free(obj->object2d.disabledContacts[i]);
                i++;
            }
        }
#endif
    }
    free(obj);
}

void physics_destroyObject_internal(struct physicsobject* obj) {
    if (obj->deleted == 1) {
        return;
    }
    _physics_destroyObjectDo(obj);
}


void* physics_getObjectUserdata_internal(struct physicsobject* object) {
    if (not object->is3d) {
#ifdef USE_PHYSICS2D
        return ((struct bodyuserdata*)object->object2d.body->
        GetUserData())->userdata;
#endif
    } else {
#ifdef USE_PHYSICS3D
        printerror(BW_E_NO3DYET);
        return NULL;
#endif
    }
    return NULL;
}

// Scaling aux functions begin here

#ifdef USE_PHYSICS2D
// Alternate version of physics_set2dShapeOval which outputs a new b2 shape
void _physics_setb2ShapeOval(b2Shape* shape, double width, double height) {
    if (fabs(width - height) < EPSILON) {
        shape = new b2CircleShape();
        ((b2CircleShape*)shape)->m_radius = width;
        return;
    }

    //construct oval shape - by manually calculating the vertices
    b2Vec2* vertices = new b2Vec2[OVALVERTICES];
    
    //go around with the angle in one full circle:
    int i = 0;
    double angle = 0;
    while (angle < 2*M_PI && i < OVALVERTICES) {
        //calculate and set vertex point
        double x,y;
        ovalpoint(angle, width, height, &x, &y);
        
        vertices[i].x = x;
        vertices[i].y = -y;
        
        //advance to next position
        angle -= (2*M_PI)/((double)OVALVERTICES);
        i++;
    }
    
    shape = new b2PolygonShape();
    ((b2PolygonShape*)shape)->Set(vertices, OVALVERTICES);
    
    delete[] vertices;
}
#endif

#ifdef USE_PHYSICS2D
b2ChainShape* _physics_copyb2ChainShape(b2ChainShape* src, b2_shape_info*
 shape_info) {
    b2ChainShape* dest;
    dest = new b2ChainShape();
    if (shape_info->is_loop)
        dest->CreateLoop(src->m_vertices, src->m_count);
    else
        dest->CreateChain(src->m_vertices, src->m_count);
    return dest;
}
#endif

#ifdef USE_PHYSICS2D
b2CircleShape* _physics_copyb2CircleShape(b2CircleShape* src, b2_shape_info*
 shape_info) {
    b2CircleShape* dest;
    dest = new b2CircleShape();
    dest->m_p = src->m_p;
    dest->m_radius = src->m_radius;
    return dest;
}
#endif

#ifdef USE_PHYSICS2D
b2PolygonShape* _physics_copyb2PolygonShape(b2PolygonShape* src, b2_shape_info*
 shape_info) {
    b2PolygonShape* dest;
    dest = new b2PolygonShape();
    dest->Set(src->m_vertices, src->m_vertexCount);
    return dest;
}
#endif

#ifdef USE_PHYSICS2D
void _physics_fill2dOrigShapeCache(struct physicsobject2d* object) {
    b2Body* body = object->body;
    b2Fixture* f = body->GetFixtureList();
    union { b2ChainShape* chain; b2CircleShape* circle;
     b2EdgeShape* edge; b2PolygonShape* poly; };
    /* XXX IMPORTANT: This relies on b2Body::GetFixtureList() returning
     fixtures in the reverse order they were added in.
     */
#warning "Code relies on b2Body::GetFixtureList() returning fixtures in the \
reverse order they were added in."
    int fixture_count = 0;
    int orig_shape_info_count = 0;
    while (f != NULL) {
        if (f->GetType() == b2Shape::e_chain)
            ++orig_shape_info_count;
        ++fixture_count;
        f = f->GetNext();
    }
    object->orig_shapes = (b2Shape**)malloc(sizeof(b2Shape*)*fixture_count);
    object->orig_shape_count = fixture_count;
    
    int orig_shape_info_index = orig_shape_info_count-1;
    int fixture_index = fixture_count-1;
    f = body->GetFixtureList();
    while (f != NULL) {
        switch (f->GetType()) {
            case b2Shape::e_chain:
                chain = _physics_copyb2ChainShape(
                 (b2ChainShape*)(f->GetShape()),
                 &(object->orig_shape_info[orig_shape_info_index]));
                object->orig_shapes[fixture_index] = (b2Shape*)malloc(
                 sizeof(b2ChainShape*));
                object->orig_shapes[fixture_index] = (b2Shape*)chain;
                --orig_shape_info_index;
            break;
            case b2Shape::e_circle:
                circle = _physics_copyb2CircleShape(
                 (b2CircleShape*)(f->GetShape()), NULL);
                object->orig_shapes[fixture_index] = (b2Shape*)malloc(
                 sizeof(b2CircleShape*));
                object->orig_shapes[fixture_index] = (b2Shape*)circle;
            break;
            case b2Shape::e_edge:
                //never happens
                //edge = f->GetShape();
            break;
            case b2Shape::e_polygon:
                poly = _physics_copyb2PolygonShape(
                 (b2PolygonShape*)(f->GetShape()), NULL);
                object->orig_shapes[fixture_index] = (b2Shape*)malloc(
                 sizeof(b2PolygonShape*));
                object->orig_shapes[fixture_index] = (b2Shape*)poly;
            break;
            default:
                printerror("fatal.");
            break;
        }
        --fixture_index;
        f = f->GetNext();
    }
}
#endif

#ifdef USE_PHYSICS2D
void _physics_delete2dOrigShapeCache(struct physicsobject2d* object) {
    for (int i = 0; i < object->orig_shape_count; ++i) {
        delete object->orig_shapes[i]; // delete shape
    }
    free(object->orig_shapes); // delete array of pointers to shapes
    free(object->orig_shape_info); // delete array of shape info structs
    // Just in case:
    object->orig_shape_count = 0;
}
#endif

#ifdef USE_PHYSICS2D
void physics_set2dScale_internal(struct physicsobject* object, double scalex,
 double scaley) {
    // TODO maybe (though it's stupid): do nothing when old_scale == new_scale
    struct physicsobject2d* object2d = &(object->object2d);
    b2Body* body = object2d->body;
    b2Fixture* f = body->GetFixtureList();
    double mass = physics_getMass_internal(object);
    union { b2ChainShape* chain; b2CircleShape* circle;
     b2EdgeShape* edge; b2PolygonShape* poly; };
    
    // Firstly, make sure the original shape cache thingy is initialised
    if (object2d->orig_shape_count == 0) {
        _physics_fill2dOrigShapeCache(object2d);
    }
    // Secondly, delete all the old fixtures+
    float oldfriction = 0.5f;
    while (f != NULL) {
        oldfriction = f->GetFriction();
        body->DestroyFixture(f);
        f = body->GetFixtureList();
    }
    /* TODO: Write the rest of this function.
     PROTIP: In order to do just that, we should first clean up the shape
     creation functions we already have so we can re-use the one for ovals
     when scaling circles. Also, the way orig_shape_info is filled in the one
     on edges->chains might not be correct, so look into it.
    */
    
    // Should be able to re-use this
    b2FixtureDef fixtureDef;
    fixtureDef.friction = oldfriction;
    fixtureDef.density = 1; // TODO: ???
    
    int i = 0; // orig_shapes[i]
    int j = 0; // orig_shape_info[j]
    b2Shape* s = NULL;
    b2Shape* new_shape = NULL;
    while (i < object2d->orig_shape_count) {
        s = object2d->orig_shapes[i];
        if (scalex == 1 and scaley == 1) {
            fixtureDef.shape = s;
            object2d->body->CreateFixture(&fixtureDef);
        } else {
            switch (s->GetType()) {
                case b2Shape::e_chain:
                    chain = _physics_copyb2ChainShape((b2ChainShape*)s,
                     &(object2d->orig_shape_info[j]));
                    // Actual scaling:
                    for (int k = 0; k < chain->m_count; ++k) {
                        chain->m_vertices[k].x *= scalex;
                        chain->m_vertices[k].y *= scaley;
                    }
                    fixtureDef.shape = chain;
                    object2d->body->CreateFixture(&fixtureDef);
                    delete chain;
                    ++j;
                break;
                case b2Shape::e_circle: // Re-use functions for ovals
                    _physics_setb2ShapeOval(new_shape, (s->m_radius) * scalex,
                     (s->m_radius) * scaley);
                    if (new_shape->GetType() == b2Shape::e_circle) {
                        circle = (b2CircleShape*)new_shape;
                        circle->m_p = ((b2CircleShape*)s)->m_p;
                        circle->m_p.x *= scalex;
                        circle->m_p.y *= scaley;
                    } else { // polygon-approximated oval
                        poly = (b2PolygonShape*)new_shape;
                        // Adjust vertices for m_p; normals are unaffected.
                        for (int k = 0; k < poly->m_vertexCount; ++k) {
                            poly->m_vertices[k].x +=
                             (((b2CircleShape*)s)->m_p.x) * scalex;
                            poly->m_vertices[k].y +=
                             (((b2CircleShape*)s)->m_p.y) * scaley;
                            poly->m_centroid = ((b2CircleShape*)s)->m_p;
                            poly->m_centroid.x *= scalex;
                            poly->m_centroid.y *= scaley;
                            fixtureDef.shape = poly;
                        }
                    }
                    fixtureDef.shape = new_shape;
                    object2d->body->CreateFixture(&fixtureDef);
                    delete new_shape;
                break;
                case b2Shape::e_edge:
                    //never happens
                break;
                case b2Shape::e_polygon:
                    /* Here, normals do change. */
                    poly = _physics_copyb2PolygonShape((b2PolygonShape*)s,
                     NULL);
                    for (int k = 0; k < poly->m_vertexCount; ++k) {
                        poly->m_vertices[k].x *= scalex;
                        poly->m_vertices[k].y *= scaley;
                    }
                    // Hilarious way to have Box2D recalculate normals:
#warning "Code relies on b2PolygonShape::Set() just copying vertices without \
re-allocation of memory for m_vertices."
                    poly->Set(poly->m_vertices, poly->m_vertexCount);
                    fixtureDef.shape = poly;
                    object2d->body->CreateFixture(&fixtureDef);
                    delete poly;
                break;
                default:
                    printerror("fatal error: unknown physics shape");
                break;
            } // switch
        } // else
        ++i;
    } // while
    physics_setMass_internal(object, mass);
}
#endif

// Everything about "various properties" starts here
void physics_setMass_internal(struct physicsobject* obj, double mass) {
    if (not obj->is3d) {
#ifdef USE_PHYSICS2D
        struct physicsobject2d* obj2d = &obj->object2d;
        if (!obj2d->movable) {return;}
        if (!obj2d->body) {return;}
        if (mass > 0) {
            if (obj2d->body->GetType() == b2_staticBody) {
                obj2d->body->SetType(b2_dynamicBody);
            }
        } else {
            mass = 0;
            if (obj2d->body->GetType() == b2_dynamicBody) {
                obj2d->body->SetType(b2_staticBody);
            }
        }
        b2MassData mdata;
        obj2d->body->GetMassData(&mdata);
        mdata.mass = mass;
        obj2d->body->SetMassData(&mdata);
#endif
    } else {
#ifdef USE_PHYSICS3D
        printerror(BW_E_NO3DYET);
#endif
    }
}

double physics_getMass_internal(struct physicsobject* obj) {
    if (not obj->is3d) {
#ifdef USE_PHYSICS2D
        b2MassData mdata;
        obj->object2d.body->GetMassData(&mdata);
        return mdata.mass;
#endif
    } else {
#ifdef USE_PHYSICS3D
        printerror(BW_E_NO3DYET);
#else
        // nothing useful we could return here:
        return 0;
#endif
    }
}

#ifdef USE_PHYSICS2D
void physics_set2dMassCenterOffset_internal(struct physicsobject* obj,
double offsetx, double offsety) {
    b2MassData mdata;
    obj->object2d.body->GetMassData(&mdata);
    mdata.center = b2Vec2(offsetx, offsety);
    obj->object2d.body->SetMassData(&mdata);
}
#endif

#ifdef USE_PHYSICS2D
void physics_get2dMassCenterOffset_internal(struct physicsobject* obj,
double* offsetx, double* offsety) {
    b2MassData mdata;
    obj->object2d.body->GetMassData(&mdata);
    *offsetx = mdata.center.x;
    *offsety = mdata.center.y;
}
#endif

#ifdef USE_PHYSICS2D
void physics_set2dGravity_internal(struct physicsobject* obj, double x, double y) {
    obj->object2d.gravityset = 1;
    obj->object2d.gravityx = x;
    obj->object2d.gravityy = y;
}
#endif

#ifdef USE_PHYSICS2D
void physics_set2dWorldGravity(struct physicsworld* world, double x, double y) {
    struct physicsworld2d* world2d = &(world->world2d);
    world2d->gravityx = x;
    world2d->gravityy = y;
}
#endif

void physics_unsetGravity_internal(struct physicsobject* obj) {
    if (not obj->is3d) {
#ifdef USE_PHYSICS2D
        obj->object2d.gravityset = 0;
#endif
    } else {
#ifdef USE_PHYSICS3D
        printerror(BW_E_NO3DYET);
#endif
    }
}

#ifdef USE_PHYSICS2D
void physics_set2dRotationRestriction_internal(struct physicsobject* obj, int restricted) {
    if (restricted) {
        obj->object2d.body->SetFixedRotation(true);
    } else {
        obj->object2d.body->SetFixedRotation(false);
    }
}
#endif

void physics_setFriction_internal(struct physicsobject* obj, double friction) {
    if (not obj->is3d) {
#ifdef USE_PHYSICS2D
        b2Fixture* f = obj->object2d.body->GetFixtureList();
        while (f) {
            f->SetFriction(friction);
            f = f->GetNext();
        }
        b2ContactEdge* e = obj->object2d.body->GetContactList();
        while (e) {
            e->contact->ResetFriction();
            e = e->next;
        }
#endif
    } else {
#ifdef USE_PHYSICS3D
        printerror(BW_E_NO3DYET);
#endif
    }
}

void physics_setAngularDamping_internal(struct physicsobject* obj, double damping) {
    if (not obj->is3d) {
#ifdef USE_PHYSICS2D
        obj->object2d.body->SetAngularDamping(damping);
#endif
    } else {
#ifdef USE_PHYSICS3D
        printerror(BW_E_NO3DYET);
#endif
    }
}

void physics_setLinearDamping_internal(struct physicsobject* obj, double damping) {
    if (not obj->is3d) {
#ifdef USE_PHYSICS2D
        if (!obj->object2d.body) {
            return;
        }
        obj->object2d.body->SetLinearDamping(damping);
#endif
    } else {
#ifdef USE_PHYSICS3D
        printerror(BW_E_NO3DYET);
#endif
    }
}

void physics_setRestitution_internal(struct physicsobject* obj, double restitution) {
    if (not obj->is3d) {
#ifdef USE_PHYSICS2D
        if (restitution > 1) {restitution = 1;}
        if (restitution < 0) {restitution = 0;}
        if (!obj->object2d.body) {
            return;
        }
        b2Fixture* f = obj->object2d.body->GetFixtureList();
        while (f) {
            f->SetRestitution(restitution);
            f = f->GetNext();
        }
        b2ContactEdge* e = obj->object2d.body->GetContactList();
        while (e) {
            e->contact->SetRestitution(restitution);
            e = e->next;
        }
#endif
    } else {
#ifdef USE_PHYSICS3D
        printerror(BW_E_NO3DYET);
#endif
    }
}

#ifdef USE_PHYSICS2D
void physics_get2dPosition_internal(struct physicsobject* obj, double* x, double* y) {
    b2Vec2 pos = obj->object2d.body->GetPosition();
    *x = pos.x;
    *y = pos.y;
}
#endif

#ifdef USE_PHYSICS2D
void physics_get2dRotation_internal(struct physicsobject* obj, double* angle) {
    *angle = (obj->object2d.body->GetAngle() * 180)/M_PI;
}
#endif

#ifdef USE_PHYSICS2D
void physics_warp2d_internal(struct physicsobject* obj, double x, double y, double angle) {
    obj->object2d.body->SetTransform(b2Vec2(x, y), angle * M_PI / 180);
}
#endif

#ifdef USE_PHYSICS2D
void physics_apply2dImpulse_internal(struct physicsobject* obj, double forcex, double forcey, double sourcex, double sourcey) {
    obj->object2d.body->ApplyLinearImpulse(b2Vec2(forcex, forcey),
    b2Vec2(sourcex, sourcey));
}
#endif

#ifdef USE_PHYSICS2D
void physics_get2dVelocity_internal(struct physicsobject* obj, double *vx, double* vy) {
    b2Vec2 vel = obj->object2d.body->GetLinearVelocity();
    *vx = vel.x;
    *vy = vel.y;
}
#endif

#ifdef USE_PHYSICS2D
double physics_get2dAngularVelocity_internal(struct physicsobject* obj) {
    return obj->object2d.body->GetAngularVelocity();
}
#endif

#ifdef USE_PHYSICS2D
void physics_set2dVelocity_internal(struct physicsobject* obj, double vx, double vy) {
    obj->object2d.body->SetLinearVelocity(b2Vec2(vx, vy));
}
#endif

#ifdef USE_PHYSICS2D
void physics_set2dAngularVelocity_internal(struct physicsobject* obj, double omega) {
    obj->object2d.body->SetAngularVelocity(omega);
}
#endif

#ifdef USE_PHYSICS2D
void physics_apply2dAngularImpulse_internal(struct physicsobject* obj, double impulse) {
    obj->object2d.body->ApplyAngularImpulse(impulse);
}
#endif

#ifdef USE_PHYSICS2D
physicsjoint* physics_add2dObjectDistanceJoint_internal(
 struct physicsobject* obj1, struct physicsobject* obj2,
 double distance,
 double a1x, double a1y, double a2x, double a2y,
 double frequency, double damping) {
    b2DistanceJointDef def;
    def.localAnchorA = b2Vec2(a1x, a1y);
    def.localAnchorB = b2Vec2(a2x, a2y);
    def.length = distance;
    def.frequencyHz = frequency;
    def.dampingRatio = damping;
    def.bodyA = obj1->object2d.body;
    def.bodyB = obj2->object2d.body;
    obj1->pworld->world2d.w->CreateJoint(&def);
    // TODO? def.userData? def.collideConnected?
    // TODO: Create physicsjoint instance etc. etc. and return it
    return NULL; // lol
}
#endif

#ifdef USE_PHYSICS2D
class mycallback : public b2RayCastCallback {
public:
    b2Body* closestcollidedbody;
    b2Vec2 closestcollidedposition;
    b2Vec2 closestcollidednormal;

    mycallback() {
        closestcollidedbody = NULL;
    }

    virtual float ReportFixture(b2Fixture* fixture, const b2Vec2& point, const b2Vec2& normal, float32 fraction) {
        closestcollidedbody = fixture->GetBody();
        closestcollidedposition = point;
        closestcollidednormal = normal;
        return fraction;
    }
};
#endif

#ifdef USE_PHYSICS2D
int physics_ray2d(struct physicsworld* world, double startx, double starty, double targetx, double targety, double* hitpointx, double* hitpointy, struct physicsobject** objecthit, double* hitnormalx, double* hitnormaly) {
    struct physicsworld2d* world2d = &(world->world2d);    
    
    // create callback object which finds the closest impact
    mycallback callbackobj;
    
    // cast a ray and have our callback object check for closest impact
    world2d->w->RayCast(&callbackobj, b2Vec2(startx, starty), b2Vec2(targetx, targety));
    if (callbackobj.closestcollidedbody) {
        // we have a closest collided body, provide hitpoint information:
        *hitpointx = callbackobj.closestcollidedposition.x;
        *hitpointy = callbackobj.closestcollidedposition.y;
        *objecthit = ((struct bodyuserdata*)callbackobj.closestcollidedbody->GetUserData())->pobj;
        *hitnormalx = callbackobj.closestcollidednormal.x;
        *hitnormaly = callbackobj.closestcollidednormal.y;
        return 1;
    }
    // no collision found
    return 0;
}
#endif


} //extern "C"

#endif

