
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

#define SPRITECOUNT 6
struct graphics2dsprite *spr[SPRITECOUNT];
        void createsprites(void) {
    // create a few sprites:
    spr[0] = graphics2dsprites_create(
        ORBMASSIVENPOT, 0, 0, 0, 0);
    spr[1] = graphics2dsprites_create(
        ORBLARGE, 0, 0, 0, 0);
    spr[2] = graphics2dsprites_create(
        ORBVERYLARGE, 0, 0, 0, 0);
    spr[3] = graphics2dsprites_create(
        ORBMASSIVE, 0, 0, 0, 0);
    spr[4] = graphics2dsprites_create(
        ORB, 0, 0, 0, 0);
    spr[5] = graphics2dsprites_create(
        NOSUCHIMAGE, 0, 0, 0, 0);
}

void destroysprites(void) {
    int i = 0;
    while (i < SPRITECOUNT) {
        graphics2dsprites_destroy(spr[i]);
        i++;
    }
}

static struct texturerequesthandle *textureRequestList;
static struct texturerequesthandle *unhandledRequestList;

void texturemanagerassertionsjustcreated(void) {

}

void texturemanagerassertionscreatedonetick(void) {

}

void texturemanagerassertions(void) {
    int i = 0;
    while (i < SPRITECOUNT) {
        if (i != 5) {
            assert(!spr[i]->loadingError);
        }
        i++;
    }
}

void texturemanagerassertionsallloaded(void) {
    int i = 0;
    while (i < SPRITECOUNT) {
        if (i != 5) {
            assert(spr[i]->tex);
        } else {
            assert(spr[i]->loadingError);
        }
        i++;
    }
}

extern int main_startup_do(int argc, char **argv);
extern int main_initAudio(void);

static double randomvalue() {
    srand(time(NULL));
    double d = (double)rand() / ((double)RAND_MAX + 1);
    return d;
}

int main(int argc, char **argv) {
    // initialise blitwizard:
    assert(main_startup_do(argc, argv) == 0);
    main_initAudio();
    // open up graphics:
    assert(graphics_setMode(640, 480, 0, 0, "Test", NULL, NULL));
    // create a few sprites and see what happens:
    createsprites();
    texturemanagerassertionsjustcreated();
    texturemanagerassertions();
    texturemanager_tick();
    texturemanagerassertionscreatedonetick();
    doConsoleLog();
    // destroy them again:
    destroysprites();
    // now continuously create and destroy sprites in a loop:
    int i = 0;
    while (i < 10) {
        // creation:
        createsprites();
        doConsoleLog();
        texturemanagerassertionsjustcreated();
        texturemanagerassertions();
        // wait a bit:
        uint64_t start = time_getMilliseconds();
        uint64_t randomwait = (1000.0 * randomvalue());
        if (i == 5) {  // wait a bit longer in the 5th loop:
            randomwait = randomwait + 7000;
        }
        while (time_getMilliseconds() < start + 1000 + randomwait) { 
            texturemanager_tick();
            texturemanagerassertionscreatedonetick();
            texturemanagerassertions();
            if (i == 5) {
                // ensure textures remain loaded in the 5th loop by
                // reporting usage:
                int k = 0;
                while (k < SPRITECOUNT) {
                    texturemanager_usingRequest(spr[k]->request,
                        USING_AT_VISIBILITY_NORMAL);
                    k++;
                }
            }
            doConsoleLog();
        }
        if (i == 5) {
            // All textures should be available at this point in some size:
            // we waited a bit longer, we reported usage, and the initial
            // loading (which takes the longest) had 4 previous loops to
            // complete.
            // Assert this actually happened and everything is loaded up:
            texturemanagerassertionsallloaded();
        }
        destroysprites();
        doConsoleLog();
        i++;
    }
    texturemanagerassertions();
    return 0;
}

#else  // USE_GRAPHICS

#include <stdio.h>

int main(int argc, const char **argv) {
    fprintf(stderr, "Nothing to test, no graphics available.\n");
    return 0;
}

#endif
