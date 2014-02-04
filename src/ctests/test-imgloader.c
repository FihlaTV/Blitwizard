
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

#include <assert.h>
#include <stdio.h>
#include "imgloader/imgloader.h"
#include "testimagepaths.h"
#include "timefuncs.h"

#ifdef NDEBUG
#error "this makes no sense without asserts"
#endif

static volatile int callbackSizeHappened = 0;

static void callbackDataMustBeFailure(void *handle, char *imgdata,
        unsigned int imgdatasize, void *userdata) {
    //assert(callbackSizeHappened);  // not necessary if it failed early
    callbackSizeHappened = 0;
    assert(imgdata == NULL);
    assert(imgdatasize == 0);
}

static size_t imageDataSize;
static void callbackDataMustBeSuccess(void *handle, char *imgdata,
        unsigned int imgdatasize, void *userdata) {
    assert(callbackSizeHappened);
    callbackSizeHappened = 0;
    assert(imgdata);
    assert(imgdatasize == imageDataSize);
}

static int reportedSizeW, reportedSizeH;
static void callbackSizeMustFireBeforeData(void *handle, int imgwidth,
        int imgheight, void *userdata) {
    assert(callbackSizeHappened == 0);
    callbackSizeHappened = 1;
    assert(imgwidth == reportedSizeW);
    assert(imgheight == reportedSizeH);

    time_Sleep(500); // delay thread
}

int main(int argc, char **argv) {
    // load up npot massive with padding. the maximum size is larger than
    // the image, but smaller than the padded image. this should abort!
    callbackSizeHappened = 0;
    reportedSizeW = 4098;
    reportedSizeH = 4096;
    fprintf(stderr, "testing orb massive npot padded with restricted size, "
        "expecting failure\n");
    void *handle = img_loadImageThreadedFromFile(ORBMASSIVENPOT,
        5000, 5000, 1, "rgba", callbackSizeMustFireBeforeData,
        callbackDataMustBeFailure, NULL);
    while (!img_checkSuccess(handle)) {
        time_Sleep(500);
    }
    img_freeHandle(handle);

    // same again, but no maximum size: load up npot massive with padding.
    // now this should work.
    callbackSizeHappened = 0;
    reportedSizeW = 4098;
    reportedSizeH = 4096;
    imageDataSize = (4096 * 2 * 4096) * 4;  // padded size
    fprintf(stderr, "testing orb massive npot with unrestricted size, "
        "expecting success\n");
    handle = img_loadImageThreadedFromFile(ORBMASSIVENPOT,
        0, 0, 1, "rgba", callbackSizeMustFireBeforeData,
        callbackDataMustBeSuccess, NULL);
    assert(!img_checkSuccess(handle));
    while (!img_checkSuccess(handle)) {
        time_Sleep(500);
    }
    img_freeHandle(handle);

    // same again, but now orb npot massive with NO padding.
    // that should make it fit into the original size constraints.
    callbackSizeHappened = 0;
    reportedSizeW = 4098;
    reportedSizeH = 4096;
    imageDataSize = (4098 * 4096) * 4;  // actual size
    fprintf(stderr, "testing orb massive npot unpadded with restricted size, "
        "expecting success\n");
    handle = img_loadImageThreadedFromFile(ORBMASSIVENPOT,
        5000, 5000, 0, "rgba", callbackSizeMustFireBeforeData,
        callbackDataMustBeSuccess, NULL);
    while (!img_checkSuccess(handle)) {
        time_Sleep(500);
    }
    img_freeHandle(handle);
    fprintf(stderr, "test complete! have fun using blitwizard\n");
    return 0;
}

