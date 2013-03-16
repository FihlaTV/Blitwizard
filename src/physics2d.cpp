
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

#if defined(USE_PHYSICS2D) || defined(USE_PHYSICS3D)

#ifndef NDEBUG
#include <logging.h>
#endif

#ifdef USE_PHYSICS2D
#include <Box2D.h>
#endif

#include <stdint.h>
#define _USE_MATH_DEFINES
#include <cmath>
#include <stdio.h>

#define EPSILON 0.0001

#include "physics.h"
#include "mathhelpers.h"


#ifndef NDEBUG
#define BW_E_NO3DYET "Error: 3D is not yet implemented."
#endif


extern "C" {

// Purely internal structs
#ifdef USE_PHYSICS2D
struct physicsobject2d;
struct physicsworld2d;
struct physicsobjectshape2d;
#endif
#ifdef USE_PHYSICS3D
struct physicsobject3d;
struct physicsworld3d;
struct physicsobjectshape3d;
#endif


inline int _physics_ObjIs3D(struct physicsobject* object) {
#if defined(USE_PHYSICS2D) && defined(USE_PHYSICS3D)
    return object->is3d;
#elif defined(USE_PHYSICS2D)
    return 0;
#elif defined(USE_PHYSICS3D)
    return 1;
#endif
}

inline void _physics_SetObjIs3D(struct physicsobject* object, int is3d) {
#if defined(USE_PHYSICS2D) && defined(USE_PHYSICS3D)
    object->is3d = is3d;
#endif
}

inline int _physics_WorldIs3D(struct physicsworld* world) {
#if defined(USE_PHYSICS2D) && defined(USE_PHYSICS3D)
    return world->is3d;
#elif defined(USE_PHYSICS2D)
    return 0;
#elif defined(USE_PHYSICS3D)
    return 1;
#endif
}

inline void _physics_SetWorldIs3D(struct physicsworld* world, int is3d) {
#if defined(USE_PHYSICS2D) && defined(USE_PHYSICS3D)
    world->is3d = is3d;
#endif
}

inline int _physics_ShapeIs3D(struct physicsobjectshape* shape) {
#if defined(USE_PHYSICS2D) && defined(USE_PHYSICS3D)
    return shape->is3d;
#elif defined(USE_PHYSICS2D)
    return 0;
#elif defined(USE_PHYSICS3D)
    return 1;
#endif
}

inline void _physics_SetShapeIs3D(struct physicsobject* shape, int is3d) {
#if defined(USE_PHYSICS2D) && defined(USE_PHYSICS3D)
    shape->is3d = is3d;
#endif
}


struct physicsobject {
    union dimension_specific_object {
#ifdef USE_PHYSICS2D
        struct physicsobject2d* ect2d;
#endif
#ifdef USE_PHYSICS3D
        struct physicsobject3d* ect3d;
#endif
    } obj;
#if defined(USE_PHYSICS2D) && defined(USE_PHYSICS3D)
    int is3d;
#endif
    struct physicsworld* pworld;
};

struct physicsworld {
    union dimension_specific_world {
#ifdef USE_PHYSICS2D
        struct physicsworld2d* ld2d;
#endif
#ifdef USE_PHYSICS3D
        struct physicsworld3d* ld3d;
#endif
    } wor;
#if defined(USE_PHYSICS2D) && defined(USE_PHYSICS3D)
    int is3d;
#endif
    void* callbackuserdata;
    int (*callback)(void* userdata, struct physicsobject* a, struct physicsobject* b, double x, double y, double normalx, double normaly, double force);
};

struct physicsobjectshape {
    union dimension_specific_shape {
#ifdef USE_PHYSICS2D
        struct physicsobjectshape2d* pe2d;
#endif
#ifdef USE_PHYSICS3D
        struct physicsobjectshape3d* pe3d;
#endif
    } sha;
#if defined(USE_PHYSICS2D) && defined(USE_PHYSICS3D)
    int is3d;
#endif
};


class mycontactlistener;
static int insidecollisioncallback = 0;

struct physicsworld2d {
    mycontactlistener* listener;
    b2World* w;
    double gravityx,gravityy;
    void* callbackuserdata; // FIXME: remove once appropriate
    int (*callback)(void* userdata, struct physicsobject2d* a, struct physicsobject2d* b, double x, double y, double normalx, double normaly, double force); // FIXME: remove once appropriate
};

struct physicsobject2d {
    int movable;
    b2World* world;
    b2Body* body;
    int gravityset;
    double gravityx,gravityy;
    void* userdata;
    struct physicsworld2d* pworld; // FIXME: remove once appropriate
    int deleted; // 1: deleted inside collision callback, 0: everything normal
};

struct physicsobjectshape2d {
    union specific_type_of_shape {
        struct polygonpoint* polygonpoints;
        b2CircleShape* circle;
        struct edge* edges;
    } b2;
    int type;
};

struct deletedphysicsobject2d {
    struct physicsobject2d* obj;
    struct deletedphysicsobject2d* next;
};

struct deletedphysicsobject2d* deletedlist = NULL;

struct bodyuserdata {
    void* userdata;
    struct physicsobject* pobj;
};


#ifdef USE_PHYSICS2D
void physics_Set2dCollisionCallback(struct physicsworld* world, int (*callback)(void* userdata, struct physicsobject* a, struct physicsobject* b, double x, double y, double normalx, double normaly, double force), void* userdata) {
    world->callback = callback;
    world->callbackuserdata = userdata;
}
#ifdef USE_PHYSICS2D

#ifdef USE_PHYSICS3D
void physics_Set3dCollisionCallback(struct physicsworld* world, int (*callback)(void* userdata, struct physicsobject* a, struct physicsobject* b, double x, double y, double z, double normalx, double normaly, double normalz, double force), void* userdata) {
    printerror(BW_E_NO3DYET);
}
#endif

void* physics_GetObjectUserdata(struct physicsobject* object) {
#if defined(USE_PHYSICS2D) && defined(USE_PHYSICS3D)
    if (not object->is3d) {
#endif
#ifdef USE_PHYSIC2D
    return ((struct bodyuserdata*)object->obj.ect2d->body->GetUserData())->userdata;
#endif
#if defined(USE_PHYSICS2D) && defined(USE_PHYSICS3D)
    }else{
#endif
#ifdef USE_PHYSICS3D
    printerror(BW_E_NO3DYET);
    return NULL;
#endif
#if defined(USE_PHYSICS2D) && defined(USE_PHYSICS3D)
    }
#endif
}


#ifdef USE_PHYSICS2D
// 2D only due to being derived from b2 class
class mycontactlistener : public b2ContactListener {
public:
    mycontactlistener();
    ~mycontactlistener();
private:
    void PreSolve(b2Contact *contact, const b2Manifold *oldManifold);
};

mycontactlistener::mycontactlistener() {return;}
mycontactlistener::~mycontactlistener() {return;}


void mycontactlistener::PreSolve(b2Contact *contact, const b2Manifold *oldManifold) {
    struct physicsobject* obj1 = ((struct bodyuserdata*)contact->GetFixtureA()->GetBody()->GetUserData())->pobj;
    struct physicsobject* obj2 = ((struct bodyuserdata*)contact->GetFixtureB()->GetBody()->GetUserData())->pobj;
    if (obj1->obj.ect2d->deleted || obj2->obj.ect2d->deleted) {
        // one of the objects should be deleted already, ignore collision
        contact->SetEnabled(false);
        return;
    }

    // get collision point (this is never really accurate, but mostly sufficient)
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
    if (w->callback) {
        if (!w->callback(w->callbackuserdata, obj1, obj2, collidex, collidey, normalx, normaly, impact)) {
            // contact should be disabled:
            contact->SetEnabled(false);
        }else{
            // contact should remain enabled:
            contact->SetEnabled(true);
        }
    }
}
#endif


struct physicsworld* physics_CreateWorld(int use3dphysics) {
    struct physicsworld* world = (struct physicsworld*)malloc(sizeof(*world));
    if (!world) {
        return NULL;
    }
#if defined(USE_PHYSICS2D) && defined(USE_PHYSICS3D)
    if (not use3dphysics) {
#endif
#ifdef USE_PHYSIC2D
    struct physicsworld2d* world2d = (struct physicsworld2d*)malloc(sizeof(*world2d));
    if (!world2d) {
        return NULL;
    }
    memset(world2d, 0, sizeof(*world2d));
    b2Vec2 gravity(0.0f, 0.0f);
    world2d->w = new b2World(gravity);
    world2d->w->SetAllowSleeping(true);
    world2d->gravityx = 0;
    world2d->gravityy = 10;
    world2d->listener = new mycontactlistener();
    world2d->w->SetContactListener(world2d->listener);
    world->wor.ld2d = world2d;
    _physics_SetWorldIs3D(world, 1);
    return world;
#else
    printerror("Error: Trying to create 2D physics world, but USE_PHYSICS2D is disabled.");
    return NULL;
#endif
#if defined(USE_PHYSICS2D) && defined(USE_PHYSICS3D)
    }else{
#endif
#ifdef USE_PHYSICS3D
    printerror(BW_E_NO3DYET);
    return NULL;
#else
    printerror("Error: Trying to create 3D physics world, but USE_PHYSICS3D is disabled.");
    return NULL;
#endif
#if defined(USE_PHYSICS2D) && defined(USE_PHYSICS3D)
    }
#endif
}

void physics_DestroyWorld(struct physicsworld* world) {
#if defined(USE_PHYSICS2D) && defined(USE_PHYSICS3D)
    if (not world->is3d) {
#endif
#ifdef USE_PHYSIC2D
    delete world->wor.ld2d->listener;
    delete world->wor.ld2d->w;
    free(world->wor.ld2d);
    free(world);
#endif
#if defined(USE_PHYSICS2D) && defined(USE_PHYSICS3D)
    }else{
#endif
#ifdef USE_PHYSICS3D
    printerror(BW_E_NO3DYET);
#endif
#if defined(USE_PHYSICS2D) && defined(USE_PHYSICS3D)
    }
#endif
}

int physics2d_GetStepSize(struct physicsworld2d* world) {
#if defined(ANDROID) || defined(__ANDROID__)
    // less accurate physics on Android (40 FPS)
    return (1000/40);
#else
    // more accurate physics on desktop (60 FPS)
    return (1000/60);
#endif
}

static void physics2d_DestroyObjectDo(struct physicsobject2d* obj);

void physics_Step(struct physicsworld* world) {
#if defined(USE_PHYSICS2D) && defined(USE_PHYSICS3D)
    if (world->is3d) {
#endif
#ifdef USE_PHYSICS2D
    struct physicsworld* world2d = world->wor.ld2d;
    // Do a collision step
    insidecollisioncallback = 1; // remember we are inside a step
    int i = 0;
    while (i < 2) {
        double forcefactor = (1.0/(1000.0f/physics2d_GetStepSize(world2d)))*2;
        b2Body* b = world2d->w->GetBodyList();
        while (b) {
            // obtain physics object struct from body
            struct physicsobject2d* obj = ((struct bodyuserdata*)b->GetUserData())->pobj;
            if (obj) {
                if (obj->gravityset) {
                    // custom gravity which we want to apply
                    b->ApplyLinearImpulse(b2Vec2(obj->gravityx * forcefactor, obj->gravityy * forcefactor), b2Vec2(b->GetPosition().x, b->GetPosition().y));
                }else{
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
        int it1 = 7;
        int it2 = 4;
#endif
        world2d->w->Step(1.0 /(1000.0f/physics2d_GetStepSize(world2d)), it1, it2);
        i++;
    }
    insidecollisioncallback = 0; // we are no longer inside a step

    // actually delete objects marked for deletion during the step:
    while (deletedlist) {
        // delete first object in the queue
        physics2d_DestroyObjectDo(deletedlist->obj);

        // update list pointers (-> remove object from queue)
        struct deletedphysicsobject2d* pobj = deletedlist;
        deletedlist = deletedlist->next;

        // free removed object
        free(pobj);
    }
#endif
#ifdef USE_PHYSICS3D
    printerror(BW_E_NO3DYET);
#endif
#if defined(USE_PHYSICS2D) && defined(USE_PHYSICS3D)
    }
#endif
}

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

int physics2d_Ray(struct physicsworld2d* world, double startx, double starty, double targetx, double targety, double* hitpointx, double* hitpointy, struct physicsobject2d** hitobject, double* hitnormalx, double* hitnormaly) {
    // create callback object which finds the closest impact
    mycallback callbackobj;

    // cast a ray and have our callback object check for closest impact
    world->w->RayCast(&callbackobj, b2Vec2(startx, starty), b2Vec2(targetx, targety));
    if (callbackobj.closestcollidedbody) {
        // we have a closest collided body, provide hitpoint information:
        *hitpointx = callbackobj.closestcollidedposition.x;
        *hitpointy = callbackobj.closestcollidedposition.y;
        *hitobject = ((struct bodyuserdata*)callbackobj.closestcollidedbody->GetUserData())->pobj;
        *hitnormalx = callbackobj.closestcollidednormal.x;
        *hitnormaly = callbackobj.closestcollidednormal.y;
        return 1;
    }
    // no collision found
    return 0;
}

void physics2d_SetGravity(struct physicsobject2d* obj, float x, float y) {
    if (!obj) {return;}
    obj->gravityset = 1;
    obj->gravityx = x;
    obj->gravityy = y;
}

void physics2d_UnsetGravity(struct physicsobject2d* obj) {
    if (!obj) {return;}
    obj->gravityset = 0;
}

void physics2d_ApplyImpulse(struct physicsobject2d* obj, double forcex, double forcey, double sourcex, double sourcey) {
    if (!obj->body || !obj->movable) {return;}
    obj->body->ApplyLinearImpulse(b2Vec2(forcex, forcey), b2Vec2(sourcex, sourcey));

}

static struct physicsobject2d* createobj(struct physicsworld2d* world, void* userdata, int movable) {
    struct physicsobject2d* object = (struct physicsobject2d*)malloc(sizeof(*object));
    if (!object) {return NULL;}
    memset(object, 0, sizeof(*object));

    struct bodyuserdata* pdata = (struct bodyuserdata*)malloc(sizeof(*pdata));
    if (!pdata) {
        free(object);
        return NULL;
    }
    memset(pdata, 0, sizeof(*pdata));
    pdata->userdata = userdata;
    pdata->pobj = object;

    b2BodyDef bodyDef;
    if (movable) {
        bodyDef.type = b2_dynamicBody;
    }
    object->movable = movable;
    bodyDef.userData = (void*)pdata;
    object->userdata = pdata;
    object->body = world->w->CreateBody(&bodyDef);
    object->body->SetFixedRotation(false);
    object->world = world->w;
    object->pworld = world;
    if (!object->body) {
        free(object);
        free(pdata);
        return NULL;
    }
    return object;
}

/* Shapes start here I guess */
struct physicsobjectshape* physics_CreateEmptyShapes(int count) {
    struct physicsobjectshape* shapes = (struct physicsobjectshape*)malloc((sizeof(*shapes))*count);
    // this was in earlier code so I'm doing it here as well, though tbh I don't understand the point
    if (!shapes) {
        return NULL;
    }
    return shapes;
}

size_t physics_GetShapeSize(void) {
    return sizeof(struct physicsobjectshape*);
}

#ifdef USE_PHYSICS2D
void physics_Set2dShapeRectangle(struct physicsobjectshape* shape, double width, double height) {
/*
 it is of critical importance for all of this shit (i.e. functions below as well) that the object is absolutely uninitialised
   or had its former shape data deleted when this function (or functions below) is called. otherwise:
   XXX MEMORY LEAKS XXX
*/
    struct polygonpoint* vertices = (struct polygonpoint*)malloc(sizeof(*points)*4);
    int i = 0;
    while (i < 4) {
            vertices[i].x = (-2*((i+1)%4<2)+1)*(width/2);
            vertices[i].y = (-2*(i%4<2)+1)*(height/2);
            vertices[i].next = &points[i+1];
    }
    vertices[3].next = NULL;
    
    shape->sha.pe2d->b2.polygonpoints = vertices;
    shape->sha.pe2d->type = 0;
#ifdef USE_PHYSICS3D
    shape->is3d = 0;
#endif
}
#endif

#ifdef USE_PHYSICS2D
void physics_Set2dShapeOval(struct physicsobjectshape* shape, double width, double height) {
    if (fabs(width - height) < EPSILON) {
        physics_Set2dShapeCircle(shape, width);
        return;
    }

    //construct oval shape - by manually calculating the vertices
    struct polygonpoint* vertices = (struct polygonpoint*)malloc(sizeof(*vertices)*OVALVERTICES);
    int i = 0;
    double angle = 0;

    //go around with the angle in one full circle:
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
    
    shape->sha.pe2d->b2.polygonpoints = vertices;
    shape->sha.pe2d->type = 0;
#ifdef USE_PHYSICS3D
    shape->is3d = 0;
#endif
}
#endif

#ifdef USE_PHYSICS2D
void physics_Set2dShapeCircle(struct physicsobjectshape* shape, double diameter) {
    b2CircleShape* circle = (b2CircleShape*)malloc(sizeof(*circle));
    circle.m_radius = radius - 0.01;
    
    shape->sha.pe2d->b2.circle = circle;
    shape->sha.pe2d->type = 1;
#ifdef USE_PHYSICS3D
    shape->is3d = 0;
#endif
}
#endif

#ifdef USE_PHYSICS2D
void physics_Add2dShapePolygonPoint(struct physicsobjectshape* shape, double xoffset, double yoffset) {
    if (not shape->sha.pe2d.type == 0) {
        shape->sha.pe2d->b2.polygonpoints = NULL;
    }
    struct polygonpoint* p = shape->sha.pe2d->b2.polygonpoints;
    while (p != NULL and p.next != NULL) {
        p = p.next;
    }
    struct polygonpoint* new_point = (struct polygonpoint*)malloc(sizeof(*new_point));
    new_point.x = xoffset;
    new_point.y = yoffset;
    p.next = new_point;
    
    shape->sha.pe2d->type = 0;
#ifdef USE_PHYSICS3D
    shape->is3d = 0;
#endif
}
#endif

#ifdef USE_PHYSICS2D
void physics_Add2dShapeEdgeList(struct physicsobjectshape* shape, double x1, double y1, double x2, double y2) {
    if (not shape->sha.pe2d.type == 2) {
        shape->sha.pe2d->b2.edges = NULL;
    }
    struct edge* e = shape->sha.pe2d->b2.edge;
    while (e != NULL and e.next != NULL) {
        e = e.next;
    }
    struct edge* new_edge = (struct edge*)malloc(sizeof(*new_edge));
    new_edge.x1 = x1;
    new_edge.y1 = y1;
    new_edge.x2 = x2;
    new_edge.y2 = y2;
    p.next = new_edge;
    
    shape->sha.pe2d->type = 2;
#ifdef USE_PHYSICS3D
    shape->is3d = 0;
#endif
}
#endif

// Use those commands to move your shapes around from the center:
void physics_Set2dShapeOffsetRotation(struct physicsobjectshape* shape, double xoffset, double yoffset, double rotation);
void physics_Get2dShapeOffsetRotation(struct physicsobjectshape* shape, double* xoffset, double* yoffset, double* rotation);


struct physicsobject2d* physics2d_CreateObjectRectangle(struct physicsworld2d* world, void* userdata, int movable, double friction, double width, double height) {
    struct physicsobject2d* obj = createobj(world, userdata, movable);
    if (!obj) {return NULL;}
    b2PolygonShape box;
    box.SetAsBox((width/2) - box.m_radius*2, (height/2) - box.m_radius*2);
    b2FixtureDef fixtureDef;
    fixtureDef.shape = &box;
    fixtureDef.friction = friction;
    fixtureDef.density = 1;
    obj->body->SetFixedRotation(false);
    obj->body->CreateFixture(&fixtureDef);
    physics2d_SetMass(obj, 0);
    return obj;
}

static struct physicsobject2d* physics2d_CreateObjectCircle(struct physicsworld2d* world, void* userdata, int movable, double friction, double radius) {
    struct physicsobject2d* obj = createobj(world, userdata, movable);
    if (!obj) {return NULL;}
    b2CircleShape circle;
    circle.m_radius = radius - 0.01;
    b2FixtureDef fixtureDef;
    fixtureDef.shape = &circle;
    fixtureDef.friction = friction;
    fixtureDef.density = 1;
    obj->body->SetFixedRotation(false);
    obj->body->CreateFixture(&fixtureDef);
    physics2d_SetMass(obj, 0);
    return obj;
}

#define OVALVERTICES 8

struct physicsobject2d* physics2d_CreateObjectOval(struct physicsworld2d* world, void* userdata, int movable, double friction, double width, double height) {
    if (fabs(width - height) < EPSILON) {
        return physics2d_CreateObjectCircle(world, userdata, movable, friction, width);
    }

    //construct oval shape - by manually calculating the vertices
    b2PolygonShape shape;
    b2Vec2 vertices[OVALVERTICES];
    int i = 0;
    double angle = 0;

    //go around with the angle in one full circle:
    while (angle < 2*M_PI && i < OVALVERTICES) {
        //calculate and set vertex point
        double x,y;
        ovalpoint(angle, width, height, &x, &y);
        vertices[i].Set(x, -y);

        //advance to next position
        angle -= (2*M_PI)/((double)OVALVERTICES);
        i++;
    }
    shape.Set(vertices, (int32_t)OVALVERTICES);

    //get physics object
    struct physicsobject2d* obj = createobj(world, userdata, movable);
    if (!obj) {return NULL;}

    //construct fixture def from shape
    b2FixtureDef fixtureDef;
    fixtureDef.shape = &shape;
    fixtureDef.friction = friction;
    fixtureDef.density = 1;

    //set fixture to body and finish
    obj->body->SetFixedRotation(false);
    obj->body->CreateFixture(&fixtureDef);
    physics2d_SetMass(obj, 0);
    return obj;
}

void physics2d_SetMass(struct physicsobject2d* obj, double mass) {
    if (!obj->movable) {return;}
    if (!obj->body) {return;}
    if (mass > 0) {
        if (obj->body->GetType() == b2_staticBody) {
            obj->body->SetType(b2_dynamicBody);
        }
    }else{
        mass = 0;
        if (obj->body->GetType() == b2_dynamicBody) {
            obj->body->SetType(b2_staticBody);
        }
    }
    b2MassData mdata;
    obj->body->GetMassData(&mdata);
    mdata.mass = mass;
    obj->body->SetMassData(&mdata);
}

void physics2d_SetRotationRestriction(struct physicsobject2d* obj, int restricted) {
    if (!obj->body) {return;}
    if (restricted) {
        obj->body->SetFixedRotation(true);
    }else{
        obj->body->SetFixedRotation(false);
    }
}

void physics2d_SetLinearDamping(struct physicsobject2d* obj, double damping) {
    if (!obj->body) {return;}
    obj->body->SetLinearDamping(damping);
}


void physics2d_SetAngularDamping(struct physicsobject2d* obj, double damping) {
    if (!obj->body) {return;}
    obj->body->SetAngularDamping(damping);
}

void physics2d_SetFriction(struct physicsobject2d* obj, double friction) {
    if (!obj->body) {return;}
    b2Fixture* f = obj->body->GetFixtureList();
    while (f) {
        f->SetFriction(friction);
        f = f->GetNext();
    }
    b2ContactEdge* e = obj->body->GetContactList();
    while (e) {
        e->contact->ResetFriction();
        e = e->next;
    }
}

void physics2d_SetRestitution(struct physicsobject2d* obj, double restitution) {
    if (restitution > 1) {restitution = 1;}
    if (restitution < 0) {restitution = 0;}
    if (!obj->body) {return;}
    b2Fixture* f = obj->body->GetFixtureList();
    while (f) {
        f->SetRestitution(restitution);
        f = f->GetNext();
    }
    b2ContactEdge* e = obj->body->GetContactList();
    while (e) {
        e->contact->SetRestitution(restitution);
        e = e->next;
    }
}


double physics2d_GetMass(struct physicsobject2d* obj) {
    b2MassData mdata;
    obj->body->GetMassData(&mdata);
    return mdata.mass;
}
void physics2d_SetMassCenterOffset(struct physicsobject2d* obj, double offsetx, double offsety) {
b2MassData mdata;
    obj->body->GetMassData(&mdata);
    mdata.center = b2Vec2(offsetx, offsety);
    obj->body->SetMassData(&mdata);
}
void physics2d_GetMassCenterOffset(struct physicsobject2d* obj, double* offsetx, double* offsety) {
    b2MassData mdata;
    obj->body->GetMassData(&mdata);
    *offsetx = mdata.center.x;
    *offsety = mdata.center.y;
}

static void physics2d_DestroyObjectDo(struct physicsobject2d* obj) {
    if (obj->body) {
        obj->world->DestroyBody(obj->body);
    }
    free(obj);
}

void physics_DestroyObject(struct physicsobject* obj) {
#if defined(USE_PHYSICS2D) && defined(USE_PHYSICS3D)
    if (not obj->is3d) {
#endif
#ifdef USE_PHYSIC2D
    if (!obj->obj.ect2d || obj->obj.ect2d->deleted == 1) {
        return;
    }
    if (!insidecollisioncallback) {
        physics2d_DestroyObjectDo(obj->obj.ect2d);
    }else{
        obj->obj.ect2d->deleted = 1;
        struct deletedphysicsobject2d* dobject = (struct deletedphysicsobject2d*)malloc(sizeof(*dobject));
        if (!dobject) {
            return;
        }
        memset(dobject, 0, sizeof(*dobject));
        dobject->obj = obj->obj.ect2d;
        dobject->next = deletedlist;
        deletedlist = dobject;
    }
    
    // ?
    free(obj);
#endif
#if defined(USE_PHYSICS2D) && defined(USE_PHYSICS3D)
    }else{
#endif
#ifdef USE_PHYSICS3D
    printerror(BW_E_NO3DYET);
#endif
#if defined(USE_PHYSICS2D) && defined(USE_PHYSICS3D)
    }
#endif
}


void physics2d_Warp(struct physicsobject2d* obj, double x, double y, double angle) {
    obj->body->SetTransform(b2Vec2(x, y), angle * M_PI / 180);  
}

void physics2d_GetPosition(struct physicsobject2d* obj, double* x, double* y) {
    b2Vec2 pos = obj->body->GetPosition();
    *x = pos.x;
    *y = pos.y;
}

void physics2d_GetRotation(struct physicsobject2d* obj, double* angle) {
    *angle = (obj->body->GetAngle() * 180)/M_PI;
}

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

struct physicsobject2dedgecontext* physics2d_CreateObjectEdges_Begin(struct physicsworld2d* world, void* userdata, int movable, double friction) {
    struct physicsobject2dedgecontext* context = (struct physicsobject2dedgecontext*)malloc(sizeof(*context));
    if (!context) {
        return NULL;
    }
    memset(context, 0, sizeof(*context));
    context->obj = createobj(world, userdata, movable);
    if (!context->obj) {
        free(context);
        return NULL;
    }
    context->friction = friction;
    return context;
}

int physics2d_CheckEdgeLoop(struct edge* edge, struct edge* target) {
    struct edge* e = edge;
    struct edge* eprev = NULL;
    while (e) {
        if (e == target) {return 1;}
        struct edge* nextprev = e;
        if (e->adjacent1 && e->adjacent1 != eprev) {
            e = e->adjacent1;
        }else{
            if (e->adjacent2 && e->adjacent2 != eprev) {
                e = e->adjacent2;
            }else{
                e = NULL;
            }
        }
        eprev = nextprev;
    }
    return 0;
}

void physics2d_CreateObjectEdges_Do(struct physicsobject2dedgecontext* context, double x1, double y1, double x2, double y2) {
    struct edge* newedge = (struct edge*)malloc(sizeof(*newedge));
    if (!newedge) {return;}
    memset(newedge, 0, sizeof(*newedge));
    newedge->x1 = x1;
    newedge->y1 = y1;
    newedge->x2 = x2;
    newedge->y2 = y2;
    newedge->adjacentcount = 1;
    
    //search for adjacent edges
    struct edge* e = context->edgelist;
    while (e) {
        if (!newedge->adjacent1) {
            if (fabs(e->x1 - newedge->x1) < EPSILON && fabs(e->y1 - newedge->y1) < EPSILON && e->adjacent1 == NULL) {
                if (physics2d_CheckEdgeLoop(e, newedge)) {
                    newedge->inaloop = 1;
                }else{
                    e->adjacentcount += newedge->adjacentcount;
                    newedge->adjacentcount = e->adjacentcount;
                }
                newedge->adjacent1 = e;
                e->adjacent1 = newedge;
                e = e->next;
                continue;
            }
            if (fabs(e->x2 - newedge->x1) < EPSILON && fabs(e->y2 - newedge->y1) < EPSILON && e->adjacent2 == NULL) {
                if (physics2d_CheckEdgeLoop(e, newedge)) {
                    newedge->inaloop = 1;
                }else{
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
                if (physics2d_CheckEdgeLoop(e, newedge)) {
                    newedge->inaloop = 1;
                }else{
                    e->adjacentcount += newedge->adjacentcount;
                    newedge->adjacentcount = e->adjacentcount;
                }
                newedge->adjacent2 = e;
                e->adjacent1 = newedge;
                e = e->next;
                continue;
            }
            if (fabs(e->x2 - newedge->x2) < EPSILON && fabs(e->y2 - newedge->y2) < EPSILON && e->adjacent2 == NULL) {
                if (physics2d_CheckEdgeLoop(e, newedge)) {
                    newedge->inaloop = 1;
                }else{
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
    newedge->next = context->edgelist;
    context->edgelist = newedge;
}

struct physicsobject2d* physics2d_CreateObjectEdges_End(struct physicsobject2dedgecontext* context) {
    if (!context->edgelist) {
        physics2d_DestroyObject(context->obj);
        free(context);
        return NULL;
    }

    struct edge* e = context->edgelist;
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
        }else{
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
            }else{
                varray[i] = b2Vec2(e2->x1, e2->y1);
            }

            //advance to next edge
            if (e2->adjacent1 && e2->adjacent1 != eprev) {
                e2 = e2->adjacent1;
            }else{
                if (e2->adjacent2 && e2->adjacent2 != eprev) {
                    e2 = e2->adjacent2;
                }else{
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
    
        //construct an edge shape from this
        if (e->inaloop) {
            chain.CreateLoop(varray, e->adjacentcount);
        }else{
            chain.CreateChain(varray, e->adjacentcount+1);
        }

        //add it to our body
        b2FixtureDef fixtureDef;
        fixtureDef.shape = &chain;
        fixtureDef.friction = context->friction;
        fixtureDef.density = 0;
        context->obj->body->CreateFixture(&fixtureDef);

        delete[] varray;
    }
    struct physicsobject2d* obj = context->obj;
    
    //free all edges
    e = context->edgelist;
    while (e) {
        struct edge* enext = e->next;
        free(e);
        e = enext;
    }

    free(context);

    physics2d_SetMass(obj, 0);
    return obj;
}

} //extern "C"

#endif

