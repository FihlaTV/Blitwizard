
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

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#ifndef __BIG_ENDIAN
#define __BIG_ENDIAN 1
#endif
#ifndef __LITTLE_ENDIAN
#define __LITTLE_ENDIAN 2
#endif

// check endianness:
#ifdef __BIG_ENDIAN__
#define __BYTE_ORDER __BIG_ENDIAN
#elif __LITTLE_ENDIAN__
#define __BYTE_ORDER __LITTLE_ENDIAN
#else
#ifndef __BYTE_ORDER
// "guess" byte order:
#define __BYTE_ORDER __LITTLE_ENDIAN
#endif
#endif

#include "pngloader.h"
#include "imgloader.h"

#if defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64) || defined(__WIN32__) || defined(WINDOWS)
#define WIN
#include <windows.h>
#include <process.h>
#endif

#ifndef WIN
#include <errno.h>
#include <pthread.h>
#endif

struct loaderthreadinfo {
    int padnpot;
    char *path;
    int (*readfunc)(void *buffer, size_t bytes, void *userdata);
    void *readfuncptr;
    void *memdata;
    unsigned int memdatasize;
    char *data;
    char *format;
    unsigned int datasize;
    int imagewidth,imageheight;
    int maxsizex,maxsizey;
    void (*callbackSize)(void *handle, int imgwidth, int imgheight,
    void *userdata);
    void (*callbackData)(void *handle, char *imgdata,
    unsigned int imgdatasize, void *userdata);
    void *userdata;
#ifdef WIN
    // windows threads stuff
    HANDLE threadhandle;
#else
    // pthreads for unix
    pthread_t threadhandle;
    pthread_mutex_t threadeventmutex;
    int threadeventobject;
#endif
};

size_t imgloader_getPaddedSize(size_t size) {
    size_t potsize = 2;
    while (potsize < size) {
        potsize *= 2;
    }
    return potsize;
}

static int loaderthreadsizecallback(size_t imagewidth,
size_t imageheight, void *data) {
    struct loaderthreadinfo* i = data;

    i->imagewidth = imagewidth;
    i->imageheight = imageheight;
    size_t finalwidth = i->imagewidth;
    size_t finalheight = i->imageheight;
    if (i->padnpot) {
        finalwidth = imgloader_getPaddedSize(finalwidth);
        finalheight = imgloader_getPaddedSize(finalheight);
    }
    // abort early if final size is too lage:
    if ((finalwidth > (size_t)i->maxsizex && (size_t)i->maxsizex > 0)
            || (finalheight > (size_t)i->maxsizey &&
                (size_t)i->maxsizey > 0)) {
        return 0;
    }
    if (i->callbackSize) {
        i->callbackSize(i, imagewidth, imageheight, i->userdata);
    }
    return 1;
}

#ifdef WIN
unsigned __stdcall loaderthreadfunction(void *data) {
#else
void *loaderthreadfunction(void *data) {
#endif
    struct loaderthreadinfo* i = data;
    // first, we probably need to load the image from a file first
    if (!i->memdata && i->path) {
        FILE* r = fopen(i->path, "rb");
        if (r) {
            char buf[512];
            int k = fread(buf, 1, sizeof(buf), r);
            while (k > 0) {
                void *newp = realloc(i->memdata, i->memdatasize + k);
                if (!newp) {
                    if (i->memdata) {free(i->memdata);}
                    i->memdata = NULL;
                    break;
                }
                i->memdata = newp;
                memcpy(i->memdata + i->memdatasize, buf, k);
                i->memdatasize += k;
                k = fread(buf, 1, sizeof(buf), r);
            }
            fclose(r);
        }
        free(i->path);
        i->path = NULL;
    }

    // load from a byte reading function
    if (i->readfunc) {
        void *p = NULL;
        size_t currentsize = 0;
        char buf[1024];

        // read byte chunks:
        int k = i->readfunc(buf, sizeof(buf), i->readfuncptr);
        while (k > 0) {
            currentsize += k;
            void *pnew = realloc(p, currentsize);
            if (!pnew) {
                // allocation failed, simply finish reading
                // and don't do anything with it:
                while (k > 0) {
                    k = i->readfunc(buf, sizeof(buf), i->readfuncptr);
                }
                k = -1;
                break;
            }
            p = pnew;
            // copy read bytes
            memcpy(p + (currentsize - k), buf, k);
            k = i->readfunc(buf, sizeof(buf), i->readfuncptr);
        }
        // handle error
        if (k < 0) {
            if (p) {
                free(p);
            }
        } else {
            i->memdata = p;
            i->memdatasize = currentsize;
        }
    }
    // now try to load the image!
    if (i->memdata) {
        if (i->memdatasize > 0) {
            if (!pngloader_loadRGBA(i->memdata, i->memdatasize,
            &i->data, &i->datasize, &loaderthreadsizecallback,
            i, i->maxsizex, i->maxsizey)) {
                i->data = NULL;
                i->datasize = 0;
            }
        }
        free(i->memdata);
        i->memdata = NULL;
        
        if (i->data) {
#if __BYTE_ORDER == __LITTLE_ENDIAN
            // our current format is ABGR (since png outputs big endian
            // RGBA, but we assume little endian/intel byte order)
            // convert if needed
            if (strcasecmp(i->format, "rgba") == 0) {
                img_convertIntelABGRtoRGBA(i->data, i->datasize);
            }
            if (strcasecmp(i->format, "bgra") == 0) {
                img_convertIntelABGRtoBGRA(i->data, i->datasize);
            }
            if (strcasecmp(i->format, "argb") == 0) {
                img_convertIntelABGRtoARGB(i->data, i->datasize);
            }
#elif __BYTE_ORDER == __BIG_ENDIAN
            // convert if needed
            if (strcasecmp(i->format, "bgra") == 0) {
                img_convertRGBAtoBGRA(i->data, i->datasize);
            }
            if (strcasecmp(i->format, "abgr") == 0) {
                img_convertRGBAtoABGR(i->data, i->datasize);
            }
            if (strcasecmp(i->format, "argb") == 0) {
                img_convertRGBAtoARGB(i->data, i->datasize);
            }
#else
#error "unsupported byte order"
#endif
            // pad up if needed:
            if (i->padnpot) {
                size_t finalwidth = imgloader_getPaddedSize(i->imagewidth);
                size_t finalheight = imgloader_getPaddedSize(i->imageheight);
                if (finalwidth != (size_t)i->imagewidth || finalheight !=
                        (size_t)i->imageheight) {
                    // we need to pad up:
                    char *newdata = malloc(finalwidth * finalheight * 4);
                    if (!newdata) {
                        free(i->data);
                        i->data = NULL;
                        i->datasize = 0;
                    } else {
                        // copy each line:
                        size_t k = 0;
                        char *p = newdata;
                        char *p2 = i->data;
                        while (k < finalheight) {
                            // copy partial old row:
                            if (k < (size_t)i->imageheight) {
                                memcpy(p, p2, i->imagewidth * 4);
                                // null remaining line:
                                if (finalwidth > (size_t)i->imagewidth) {
                                    memset(p + (i->imagewidth * 4),
                                        0, (finalwidth - i->imagewidth) * 4
                                    );
                                }
                            } else {
                                // simply null the line:
                                memset(p, 0, finalwidth * 4);
                            }
                            p += (finalwidth * 4);
                            p2 += (i->imagewidth * 4);
                            k++;
                        }
                        free(i->data);
                        i->data = newdata;
                        i->datasize = finalwidth * finalheight * 4;
                    }
                }
            }
        }
    }

    // enter callback if we got one
#ifndef WIN
    pthread_mutex_t cachedthreadeventmutex;
    memcpy(&cachedthreadeventmutex, &i->threadeventmutex,
        sizeof(cachedthreadeventmutex));
#endif
    if (i->callbackData) {
        i->callbackData(data, i->data, i->datasize, i->userdata);
    }
    
#ifdef WIN
    return 0;
#else
    pthread_mutex_lock(&cachedthreadeventmutex);
    i->threadeventobject = 1;
    pthread_mutex_unlock(&cachedthreadeventmutex);
    pthread_exit(NULL);
    return NULL;
#endif
}

void startthread(struct loaderthreadinfo *i) {
#ifdef WIN
    i->threadhandle = (HANDLE)_beginthreadex(NULL, 0,
    loaderthreadfunction, i, 0, NULL);
#else
    pthread_mutex_init(&i->threadeventmutex, NULL);
    i->threadeventobject = 0;
    pthread_create(&i->threadhandle, NULL,
    loaderthreadfunction, i);
    pthread_detach(i->threadhandle);
#endif
}

void *img_loadImageThreadedFromFile(const char *path, int maxwidth,
        int maxheight, int padnpot, const char *format,
        void (*callbackSize)(void *handle, int imgwidth, int imgheight,
        void *userdata),
        void (*callbackData)(void *handle, char *imgdata,
        unsigned int imgdatasize, void *userdata),
        void *userdata) {
    struct loaderthreadinfo* t = malloc(sizeof(struct loaderthreadinfo));
    if (!t) {return NULL;}
    memset(t, 0, sizeof(*t));
    t->path = malloc(strlen(path) + 1);
    if (!t->path) {
        free(t);
        return NULL;
    }
    t->format = malloc(strlen(format) + 1);
    if (!t->format) {
        free(t->path);free(t);
        return NULL;
    }
    strcpy(t->path, path);
    strcpy(t->format, format);
    t->maxsizex = maxwidth;
    t->maxsizey = maxheight;
    t->callbackSize = callbackSize;
    t->callbackData = callbackData;
    t->userdata = userdata;
    t->padnpot = padnpot;
    startthread(t);
    return t;
}

void *img_loadImageThreadedFromFunction(
    int (*readfunc)(void *buffer, size_t bytes, void *userdata),
    void *readfuncuserdata, int maxwidth, int maxheight,
    int padnpot,
    const char *format,
    void (*callbackSize)(void *handle, int imgwidth, int imgheight,
    void *userdata),
    void (*callbackData)(void *handle, char *imgdata,
    unsigned int imgdatasize, void *userdata),
    void *callbackuserdata
) {
    struct loaderthreadinfo *t = malloc(sizeof(struct loaderthreadinfo));
    if (!t) {
        return NULL;
    }
    memset(t, 0, sizeof(*t));
    t->readfunc = readfunc;
    t->readfuncptr = readfuncuserdata;
    t->userdata = callbackuserdata;
    t->format = malloc(strlen(format) + 1);
    if (!t->format) {
        free(t->memdata);
        free(t);
        return NULL;
    }
    strcpy(t->format, format);
    t->maxsizex = maxwidth;
    t->maxsizey = maxheight;
    t->callbackSize = callbackSize;
    t->callbackData = callbackData;
    t->padnpot = padnpot;
    startthread(t);
    return t;
}

void *img_loadImageThreadedFromMemory(const void *memdata,
        unsigned int memdatasize, int maxwidth, int maxheight, int padnpot,
        const char *format,
        void (*callbackSize)(void *handle, int imgwidth, int imgheight,
        void *userdata),
        void (*callbackData)(void *handle, char *imgdata,
        unsigned int imgdatasize, void *userdata),
        void *userdata) {
    struct loaderthreadinfo* t = malloc(sizeof(struct loaderthreadinfo));
    if (!t) {return NULL;}
    memset(t, 0, sizeof(*t));
    t->userdata = userdata;
    t->memdata = malloc(memdatasize);
    if (!t->memdata) {
        free(t);
        return NULL;
    }
    t->format = malloc(strlen(format) + 1);
    if (!t->format) {
        free(t->memdata);
        free(t);
        return NULL;
    }
    strcpy(t->format, format);
    memcpy(t->memdata, memdata, memdatasize);
    t->memdatasize = memdatasize;
    t->maxsizex = maxwidth;
    t->maxsizey = maxheight;
    t->callbackData = callbackData;
    t->callbackSize = callbackSize;
    t->padnpot = padnpot;
    startthread(t);
    return t;
}

int img_checkSuccess(void *handle) {
    if (!handle) {
        return 1;
    }
    struct loaderthreadinfo *i = handle;
#ifdef WIN
    if (WaitForSingleObject(i->threadhandle, 0) == WAIT_OBJECT_0) {
#else
    pthread_mutex_lock(&i->threadeventmutex);
    int u = i->threadeventobject;
    pthread_mutex_unlock(&i->threadeventmutex);
    if (u == 1) {
#endif
        //request is no longer running
        return 1;
    } else {
        return 0;
    }
}

void img_GetData(void *handle, char **path, int *imgwidth, int *imgheight,
        char **imgdata) {
    if (!handle) {
        *imgwidth = 0;
        *imgheight = 0;
        *imgdata = NULL;
        *path = NULL;
        return;
    }
    struct loaderthreadinfo *i = handle;
    *imgwidth = i->imagewidth;
    *imgheight = i->imageheight;
    *imgdata = i->data;
    if (path) {
        if (i->path) {
            *path = strdup(i->path);
        } else {
            *path = NULL;
        }
    }
}

#ifdef WIN
static unsigned __stdcall img_freeHandleThread(void *handle) {
#else
static void *img_freeHandleThread(void *handle) {
#endif
    if (!handle) {
#ifdef WIN
        return 0;
#else
        return NULL;
#endif
    }
    struct loaderthreadinfo *i = handle;
    while (!img_checkSuccess(handle)) {
#ifdef WIN
        Sleep(100);
#else
        usleep(100 * 1000);
#endif
    }
    if (i->memdata) {
        free(i->memdata);
    }
    if (i->format) {
        free(i->format);
    }
    if (i->path) {
        free(i->path);
    }
    //if (i->data) {free(i->data);} //the user needs to do that!
    free(handle);
#ifdef WIN
    CloseHandle(i->threadhandle);
    return 0;
#else
    //should be done by pthread_exit()
    return NULL;
#endif
}

void img_freeHandle(void *handle) {
#ifdef WIN
    HANDLE h = (HANDLE)_beginthreadex(NULL, 0,
        img_freeHandleThread, handle, 0, NULL);
    CloseHandle(h);
#else
    pthread_t thread;
    while (pthread_create(&thread, NULL, img_freeHandleThread, handle) != 0) {
        assert(errno == EAGAIN);
    }
    pthread_detach(thread);
#endif    
}


static void img_convert(char *data, int datasize,
        unsigned char firstchannelto, unsigned char secondchannelto,
        unsigned char thirdchannelto, unsigned char fourthchannelto) {
    if (!data) {
        return;
    }
    int r = 0;
    while (r < datasize) {
        char oldlayout[4];
        memcpy(oldlayout, data + r, 4);
        *(char*)(data + r + firstchannelto) = oldlayout[0];
        *(char*)(data + r + secondchannelto) = oldlayout[1];
        *(char*)(data + r + thirdchannelto) = oldlayout[2];
        *(char*)(data + r + fourthchannelto) = oldlayout[3];
        r += 4;
    }
}

void img_convertIntelABGRtoRGBA(char *imgdata, int datasize) {
    img_convert(imgdata, datasize, 3, 2, 1, 0);
}

void img_convertIntelABGRtoBGRA(char *imgdata, int datasize) {
    img_convert(imgdata, datasize, 1, 2, 3, 0);
}

void img_convertIntelABGRtoARGB(char *imgdata, int datasize) {
    img_convert(imgdata, datasize, 2, 1, 0, 3);
}

void img_convertRGBAtoBGRA(char *imgdata, int datasize) {
    img_convert(imgdata, datasize, 2, 1, 0, 3);
}

void img_convertRGBAtoABGR(char *imgdata, int datasize) {
    img_convert(imgdata, datasize, 3, 2, 1, 0);
}

void img_convertRGBAtoARGB(char *imgdata, int datasize) {
    img_convert(imgdata, datasize, 1, 2, 3, 0);
}

// Simple linear scaler:
void img_scale(int bytesize, char *imgdata, int originalwidth,
        int originalheight, int pitch, char **newdata,
        int targetwidth, int targetheight) {
    // without proper new data we cannot scale:
    if (*newdata == NULL) {
        *newdata = malloc(targetwidth * targetheight * bytesize);
    }
    if (!(*newdata)) {
        return;
    }

    // do scaling:
    int r, k;
    r = 0;
    float scalex = ((float)originalwidth / (float)targetwidth);
    float scaley = ((float)originalheight / (float)targetheight);
    while (r < targetwidth) {
        k = 0;
        while (k < targetheight) {
            // copy over each 'bytesize' pixel:

            // get X source coordinate:
            int fromx = (float)r * scalex;
            if (fromx < 0) {fromx = 0;}
            if (fromx >= originalwidth) {fromx = originalwidth - 1;}

            // get Y source coordinate:
            int fromy = (float)k * scaley;
            if (fromy < 0) {fromy = 0;}
            if (fromy >= originalheight) {fromy = originalheight - 1;}

            // do pixel copy:
            memcpy(
                (*newdata) + (r + (k * targetwidth)) * bytesize,
                imgdata + (fromx + (fromy * (originalwidth + pitch))) * 4,
                4
            );
            
            k++;
        }
        r++;
    }
}


