
/* blitwizard game engine - unit test code

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

#include "config.h"
#include "os.h"

#ifdef USE_GRAPHICS

#include <time.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "graphicstexturemanager.h"
#include "graphics2dsprites.h"
#include "timefuncs.h"
#include "texmanassertions.c"
#include "testimagepaths.h"
#include "logging.h"
#include "graphicstexturerequeststruct.h"
#include "graphics2dspritestruct.h"
#include "graphicssdltexturestruct.h"

extern int main_startup_do(int argc, char **argv);
extern int main_initAudio(void);

struct texturerequesthandle *req = NULL;
static size_t obtainedWidth = 0;
static size_t obtainedHeight = 0;
static struct graphicstexture *tex = NULL;

static void textureDimensionInfoCallback(struct texturerequesthandle
        *request, size_t width, size_t height, void *userdata) {
    assert(request == req);
    assert(userdata == NULL);
    assert(obtainedWidth == 0 && obtainedHeight == 0);
    assert(width > 0);
    assert(height > 0);

}

static void textureSwitch(struct texturerequestandle *request,
        struct graphicstexture *texture, void* userdata) {
    assert(userdata == NULL);
    assert(obtainedWidth != 0 && obtainedHeight != 0);
    assert(req == request);
    assert(!tex);
    tex = texture;
}

static void textureHandlingDone(struct texturerequesthandle *request,
        void *userdata) {
    // this shouldn't happen.
    assert(0);
}

int main(int argc, char **argv) {
    // initialise blitwizard:
    assert(main_startup_do(argc, argv) == 0);
    main_initAudio();

    // open up graphics:
    assert(graphics_setMode(640, 480, 0, 0, "Test", NULL, NULL));

    // create a texture request:
    req = texturemanager_requestTexture(ORBLARGE,
        &textureDimensionInfoCallback,
        &textureSwitch,
        &textureHandlingDone,
        NULL);
    texturemanager_usingRequest(req, USING_AT_VISIBILITY_DETAIL);
    texturemanager_tick();
    doConsoleLog();

    // now wait for the texture to load:
    while (!tex) {
        texturemanager_tick();
    }

    // assert that the largest version is available:
#if (defined(USE_SDL_GRAPHICS) || defined(USE_NULL_GRAPHICS))
    assert(obtainedWidth == tex->width);
    assert(obtainedHeight == tex->height);
#else
#error "unsupported graphics backend - please update test2
#endif
    return 0;
}

#else  // USE_GRAPHICS

#include <stdio.h>

int main(int argc, const char **argv) {
    fprintf(stderr, "Nothing to test, no graphics available.\n");
    return 77;
}

#endif
