
/* blitwizard game engine - source code file

  Copyright (C) 2014 Jonas Thiem

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

#ifndef BLITWIZARD_GRAPHICSTEXTUREMANAGER_INTERNAL_HELPERS_H_
#define BLITWIZARD_GRAPHICSTEXTUREMANAGER_INTERNAL_HELPERS_H_

#include "config.h"
#include "os.h"

#ifdef USE_GRAPHICS

#include "graphics.h"
#include "graphicstexturemanager.h"
#include "logging.h"
#include "graphicstexturerequeststruct.h"
#include "graphicstexturemanagermembudget.h"

static int ispoweroftwo(int number) {
    if (number < 0) {
        number = -number;
    }
    if (number < 2) {
        return 0;
    }
    while ((number % 2) == 0 && number >= 2) {
        number /= 2;
    }
    if (number == 1) {
        return 1;
    }
    return 0;
}

static int texturemanager_badTextureDimensions(struct graphicstexturemanaged
        *gtm) {
    // get texture dimensions if possible:
    if (!gtm) {
        return 0;
    }
    if (gtm->beingInitiallyLoaded) {
        return 0;
    }
    if (gtm->origscale < 0 || gtm->origscale >= gtm->scalelistcount) {
        return 0;
    }
    size_t width = gtm->scalelist[gtm->origscale].paddedWidth;
    size_t height = gtm->scalelist[gtm->origscale].paddedHeight;
    if (width == 0 || height == 0) {
        return 0;
    }
    // avoid excessively large textures:
    if (width > 4096 || height > 4096) {
        return 1;
    }
    // avoid non power of two:
    //if (!ispoweroftwo(width) || !ispoweroftwo(height)) {
    //    return 1;
    //}
    return 0;
}

#endif  // USE_GRAPHICS

#endif  // BLITWIZARD_GRAPHICSTEXTUREMANAGER_INTERNAL_HELPERS_H
