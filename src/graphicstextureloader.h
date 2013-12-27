
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

#ifndef BLITWIZARD_GRAPHICSTEXTURELOADER_H_
#define BLITIZARD_GRAPHICSTEXTURELOADER_H_

#include "os.h"

#ifdef USE_GRAPHICS

#include "graphics.h"
#include "graphicstexturelist.h"

// For a texture which needs to be loaded completely from the original
// .png files (-> it is not even in the disk cache), use this function.
void graphicstextureloader_doInitialLoading(struct graphicstexturemanaged* gtm,
void (*callbackDimensions)(struct graphicstexturemanaged* gtm, size_t width,
size_t height, int success,
void* userdata),
void (*callbackData)(struct graphicstexturemanaged* gtm, int success,
void* userdata),
void* userdata);

#endif  // USE_GRAPHICS

#endif  // BLITWIZARD_GRAPHICSTEXTURELOADER_H_
