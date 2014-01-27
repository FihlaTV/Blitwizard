
/* blitwizard game engine - source code file

  Copyright (C) 2011-2014 Jonas Thiem

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

void *img_loadImageThreadedFromFile(
    const char *path,
    int maxwidth, int maxheight, int padnpot, const char *format,
    void (*callbackSize)(void *handle, int imgwidth, int imgheight,
      void *userdata),
    void (*callbackData)(void *handle, char *imgdata,
    unsigned int imgdatasize, void *userdata),
    void *userdata
);
// Starts an asynchronous image load. You get back a job handle to
// check on the status of the job. (It is impossible to cancel it!)
// Parameters:
//   - path: path to the file
//   - maximumwidth/-height: maximum size restrictions (or 0 if any size is
//                           allowed)
//                           - this is recommended for not wasting too much
//                           memory! (e.g. so loading a 50k x 50k will be
//                           cancelled right away before you run out of memory)
//   - padnpot: whether to pad up non power of two textures to a power of two
//   - format: "rgba", "bgra" are valid parameters for now
//   - callback: if you want to be called (in a separate thread!) when stuff is done, specify a function here. otherwise NULL
//         size callback parameters:
//            - handle: the same thing this function also returns.
//            - imgwidth, imgheight: dimensions of the loaded image
//         data callback parameters:
//            - handle: the handle this function returns.
//               you might want to img_FreeHandle() it.
//            - imgdata: data area containing the raw 32bit rgba image data
//               (or NULL if loading failed!) - you need to use free
//               on this yourself if not NULL!
//            - imgdatasize: the size of the image data

void *img_loadImageThreadedFromMemory(
    const void *memdata, unsigned int memdatasize,
    int maxwidth, int maxheight, int padnpot, const char *format,
    void (*callbackSize)(void *handle, int imgwidth, int imgheight,
    void *userdata),
    void (*callbackData)(void *handle, char *imgdata,
    unsigned int imgdatasize, void *userdata),
    void *userdata
);
// Same as img_LoadImageThreadedFromFile, but takes a memory pointer & size
// instead of a path to a file on disk

void *img_loadImageThreadedFromFunction(
  int (*readfunc)(void *buffer, size_t bytes, void *userdata),
  void *readfuncuserdata, int maxwidth, int maxheight, int padnpot,
  const char *format,
  void (*callbackSize)(void *handle, int imgwidth, int imgheight,
  void *userdata),
  void (*callbackData)(void *handle, char *imgdata,
  unsigned int imgdatasize, void *userdata),
  void *userdata
);
// Same as img_LoadImageThreadedFromFile, but takes a function that will be
// called to load the file from disk

int img_checkSuccess(void *handle);
// Check on the progress of a job handle.
// Returns 1 if job is done (otherwise 0)

void img_getData(void *handle, char **path, int *imgwidth,
    int *imgheight, char **imgdata);
// Get the resulting raw image data of a _completed_ job.
// Please note the behaviour is undefined if img_CheckSuccess doesn't
// return 1 on the handle. (certainly including crashes/corruption!)
// If this function gives you a NULL pointer for img data, then
// the image could not be loaded.
// Please note you need to free() the pointer you get through
// imgdata yourself (if not NULL) - if you call img_GetData multiple
// times, you will end up with the same pointer (so not a newly
// allocated copy for each call)
// If path is not NULL, *path will be set to point to a freshly
// allocated string copy of the image path or NULL if there was none.
// You need to free this variable aswell (each call to this function
// will get you a freshly allocated copy)

void img_freeHandle(void *handle);
// free the job
// IMPORTANT: this will NOT(!) free the image data!!
//  You REALLY should fetch it through img_GetData _first_, and then
//  free that data manually when you no longer need it.
//  If you use this function before using free() on the image data,
//  you will suffer a memory leak!

void img_convertRGBAtoBGRA(char *imgdata, int datasize);
void img_convertRGBAtoABGR(char *imgdata, int datasize);
void img_convertRGBAtoARGB(char *imgdata, int datasize);
// convert image data to bgra if needed

void img_scale(int bytesize, char *imgdata, int originalwidth,
    int originalheight, int pitch, char **newdata,
    int targetwidth, int targetheight);
// Scale an image (simple linear scaling).
// parameters:
//   - bytesize: specifies whether this is 32bit rgba (-> bytesize 4)
//      or 24bit rgb (-> bytesize 3) image data
//   - imgdata: pointer to buffer which holds original (unscaled) image
//   - originalwidth, originalheight: dimensions of original image
//   - pitch: horizontal row pitch for each line after the first
//   - newdata: pointer to a pointer which will hold the target image.
//   IMPORTANT: *newdata must be either NULL, or a valid pointer to a
//              buffer which has the byte size
//              targetwidth * targetheight * bytesize.
//              If it is uninitialised, this will most likely crash!
//   - targetwidth, targetheight: the intended new size after scaling
// Return value: none, but *newdata will be either altered to point at
// a new buffer if it was NULL (in case the allocation doesn't fail),
// and the data at which *newdata points will be overwritten to contain
// the new scaled image.


void img_4to3channel(char *imgdata, int width, int height, char **newdata,
    int channeltodrop);
// Drop a channel in a 4 channel image (used for the alpha channel
// usually). channeltodrop is zero-based, so 1st channel is 0, 4th
// channel is 3.
// If *newdata is NULL, it will be changed to a buffer for the
// new RGB data (it will be sized width * height * 3) if allocation
// succeeds.
// If *newdata is not NULL, it is assumed to point to a buffer of
// that size.
// The contents of *newdata, if not NULL, will be altered to
// contain the new RGB data.

size_t imgloader_getPaddedSize(size_t size);

