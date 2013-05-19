
/* blitwizard game engine - source code file

  Copyright (C) 2013 Jonas Thiem

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
    struct physicsobjectshape* shapes;
    int shapecount;
    
    int massisset;
    double mass;
    
    int masscenterisset;
    double masscenterx, masscentery, masscenterz;
    
    int gravityisset;
    double gravityx, gravity, gravityz;
    
    int rotationrestriction2d;
    int rotationrestriction3daxis;
    double rotationrestriction3dx, rotationrestriction3dy,
    rotationrestriction3dy;
    double rotationrestriction3dfull;
    
    int frictionisset;
    double friction;
    
    int angulardampingset;
    double angulardamping;
    
    int lineardampingset;
    double lineardamping;
    
    int restitutionset;
    double restitution;
    
    int positionset;
    double positionx, positiony, positionz;
    
    int rotation2dset;
    double rotation2d;
    
    int rotation3dset;
    double rotation3dx, rotation3dy, rotation3dz, rotation3dr;
    
    int velocityset;
    double velocityx, velocityy, velocityz;
    
    int angularvelocityset;
    double angularvelocity;
    
    int impulsecount;
    struct cachedimpulse* impulses;
};

struct cachedphysicsobject* cachedObjects = NULL;

static void physics_createCachedObjects() {
    
}
