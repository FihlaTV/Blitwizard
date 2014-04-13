
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

#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "imgloader/imgloader.h"
#include "testimagepaths.h"
#include "timefuncs.h"

#ifdef NDEBUG
#error "this makes no sense without asserts"
#endif

static volatile int callbackSizeHappened = 0;
int format = 0;
char *formats[] = { "rgba", "bgra" };

static size_t imageDataSize;
static void callbackDataCheckColor(void *handle, char *imgdata,
        unsigned int imgdatasize, void *userdata) {
    // The test image contains 4 horizontal pixels,
    // having those color values (left to right):
    //  r, g, b, a:
    //  0, 255, 255, 255
    //  255, 0, 0, 255
    //  255, 0, 255, 255
    //  0, 255, 0, 127
    assert(callbackSizeHappened);
    callbackSizeHappened = 0;
    assert(imgdata);
    assert(imgdatasize == imageDataSize);

    unsigned char *imgdatau = (unsigned char*)imgdata;

    // Now verify the colors in the image:
    if (strcasecmp(formats[format], "rgba") == 0) {
        // pixel 1/4: 0xffffff00
        printf("first pixel/rgba: %u %u %u %u\n",
            imgdatau[0], imgdatau[1], imgdatau[2],
            imgdatau[3]);
        assert(imgdatau[0] == 0xff); //a
        assert(imgdatau[1] == 0xff); //b
        assert(imgdatau[2] == 0xff); //g
        assert(imgdatau[3] == 0x00); //r
        // pixel 2/4: 0xff0000ff
        assert(imgdatau[4] == 0xff); //a
        assert(imgdatau[5] == 0x00); //b
        assert(imgdatau[6] == 0x00); //g
        assert(imgdatau[7] == 0xff); //r
        // pixel 3/4: 0xffff00ff
        assert(imgdatau[8] == 0xff); //a
        assert(imgdatau[9] == 0xff); //b
        assert(imgdatau[10] == 0x00); //g
        assert(imgdatau[11] == 0xff); //r
        // pixel 4/4: 0x7F00ff00
        assert(imgdatau[12] == 0x7F); //a
        assert(imgdatau[13] == 0x00); //b
        assert(imgdatau[14] == 0xff); //g
        assert(imgdatau[15] == 0x00); //r
    } else if (strcasecmp(formats[format], "bgra") == 0) {
        // pixel 1/4: 0xff00ffff
        assert(imgdatau[0] == 0xff); //a
        assert(imgdatau[1] == 0x00); //r
        assert(imgdatau[2] == 0xff); //g
        assert(imgdatau[3] == 0xff); //b
        // pixel 2/4: 0xffff0000
        assert(imgdatau[4] == 0xff); //a
        assert(imgdatau[5] == 0xff); //r
        assert(imgdatau[6] == 0x00); //g
        assert(imgdatau[7] == 0x00); //b
        // pixel 3/4: 0xffff00ff
        assert(imgdatau[8] == 0xff); //a
        assert(imgdatau[9] == 0xff); //r
        assert(imgdatau[10] == 0x00); //g
        assert(imgdatau[11] == 0xff); //b
        // pixel 4/4: 0x7F00FF00
        assert(imgdatau[12] == 0x7F); //a
        assert(imgdatau[13] == 0x00); //r
        assert(imgdatau[14] == 0xff); //g
        assert(imgdatau[15] == 0x00); //b
    } else {
        // invalid format!
        assert(0);
    }
}

static int reportedSizeW, reportedSizeH;
static void callbackSizeMustFireBeforeData(void *handle, int imgwidth,
        int imgheight, void *userdata) {
    assert(callbackSizeHappened == 0);
    callbackSizeHappened = 1;
    assert(imgwidth == reportedSizeW);
    assert(imgheight == reportedSizeH);
}

int main(int argc, char **argv) {
    // test various formats:
    int i = 0;
    while (i < 2) {
        format = i;
        callbackSizeHappened = 0;
        reportedSizeW = 4;
        reportedSizeH = 1;
        imageDataSize = (4) * 4;
        fprintf(stderr, "testing image format %s\n", formats[i]);
        void *handle = img_loadImageThreadedFromFile(COLORSIMAGE,
            5, 5, 0, formats[i], callbackSizeMustFireBeforeData,
            callbackDataCheckColor, NULL);
        while (!img_checkSuccess(handle)) {
            time_sleep(500);
        }
        img_freeHandle(handle);
        i++;
    }
    fprintf(stderr, "test complete! have fun using blitwizard\n");
    return 0;
}

