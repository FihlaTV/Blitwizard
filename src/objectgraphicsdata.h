
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

#ifndef BLITWIZARD_OBJECTGRAPHICSDATA_H_
#define BLITWIZARD_OBJECTGRAPHICSDATA_H_

#include "config.h"
#include "os.h"

struct objectgraphicsdata {
    double alpha;  // object alpha (0 invisible, 1 solid)
    int geometryCallbackDone;  // set to 1 once geometry callback was fired
    int visibilityCallbackDone;  // set to 1 once visibility callback was fired
    int pinnedToCamera;  // copy of currently set 'pinned state'
#ifdef USE_GRAPHICS
    // explicit sprite dimensions we set (or 0,0 if texture dimensions)
    double width, height;
    struct graphics2dsprite* sprite;
#endif
};

#endif  // BLITWIZARD_OBJECTGRAPHICSDATA_H_

