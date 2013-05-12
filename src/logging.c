
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

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include "logging.h"
#include "luaheader.h"
#include "luafuncs.h"
#include "threading.h"
#include "os.h"
#ifdef WINDOWS
#include <windows.h>
#ifdef USE_SDL_GRAPHICS
#include "SDL.h"
#include "graphicstexture.h"
#include "graphics.h"
#endif
#endif

#define MEMORYLOGBUFCHUNK (1024 * 10)
char* memorylogbuf = NULL;
int memorylogbufsize = 0;
extern int suppressfurthererrors;
mutex* memLogMutex = NULL;

#define CONSOLELOGMAXBUFFEREDLINES 1024
char* consoleloglines[CONSOLELOGMAXBUFFEREDLINES];
char* consoleloglinetypes[CONSOLELOGMAXBUFFEREDLINES];
int consoleloglinecount = 0;
int mayConsoleLog = 0;
mutex* consoleLogMutex = NULL;

__attribute__ ((constructor)) static void prepareMutexes(void) {
    consoleLogMutex = mutex_Create();
    memLogMutex = mutex_Create();
}

void consolelog(const char* type, const char* str) {
    mutex_Lock(consoleLogMutex);
    if (!mayConsoleLog) {
        printf("storing line in memory\n");
        consoleloglinecount++;
        if (consoleloglinecount > CONSOLELOGMAXBUFFEREDLINES) {
            consoleloglinecount--;
            mutex_Release(consoleLogMutex);
            return;
        }
        consoleloglines[consoleloglinecount-1] = strdup(str);
        consoleloglinetypes[consoleloglinecount-1] = strdup(type);
    } else {
        printf("calling onLog\n");
        luacfuncs_onLog(type, "%s", str);
    }
    mutex_Release(consoleLogMutex);
}

void enableConsoleLog() {
    mutex_Lock(consoleLogMutex);
    // enable console log
    mayConsoleLog = 1;

    // flush out all buffered lines:
    int i = 0;
    while (i < consoleloglinecount) {
        if (consoleloglines[i] && consoleloglinetypes[i]) {
            luacfuncs_onLog(consoleloglinetypes[i], "%s",
            consoleloglines[i]);
        }
        if (consoleloglines[i]) {
            free(consoleloglines[i]);
        }
        if (consoleloglinetypes[i]) {
            free(consoleloglinetypes[i]);
        }
        i++;
    }
    consoleloglinecount = 0;
    mutex_Release(consoleLogMutex);
}

void memorylog(const char* str) {
    mutex_Lock(memLogMutex);
    int newlen = strlen(str);
    int currlen = 0;
    if (memorylogbuf) {
        currlen = strlen(memorylogbuf);
    }

    // if the memory buffer is full, enlarge:
    if (currlen + newlen >= memorylogbufsize) {
        // check on new size
        int newsize = memorylogbufsize;
        while (currlen + newlen >= newsize) {
            newsize += MEMORYLOGBUFCHUNK;
        }
        char* p = realloc(memorylogbuf, newsize);
        // out of memory.. nothing we could sensefully do
        if (!p) {
            mutex_Release(memLogMutex);
            return;
        }
        // resizing complete:
        memorylogbuf = p;
        memorylogbufsize = newsize;
    }
    // append new log data:
    memcpy(memorylogbuf + currlen, str, newlen);
    memorylogbuf[currlen+newlen] = 0;
    mutex_Release(memLogMutex);
}

void printerror(const char* fmt, ...) {
    char printline[2048];
    va_list a;
    va_start(a, fmt);
    vsnprintf(printline, sizeof(printline)-1, fmt, a);
    printline[sizeof(printline)-1] = 0;
    va_end(a);
#ifdef ANDROID
    __android_log_print(ANDROID_LOG_ERROR, "blitwizard", "%s", printline);
#else
    fprintf(stderr, "%s\n",printline);
    fflush(stderr);
    memorylog(printline);
    memorylog("\n");
    consolelog("error", printline);
#endif
#ifdef WINDOWS
    // we want graphical error messages for windows
    if (!suppressfurthererrors || 1 == 1) {
        // minimize drawing window if fullscreen
#ifdef USE_GRAPHICS
        if (graphics_AreGraphicsRunning() && graphics_IsFullscreen()) {
            graphics_MinimizeWindow();
        }
#endif
        // show error msg
        char printerror[4096];
        snprintf(printerror, sizeof(printerror)-1,
        "The application cannot continue due to a fatal error:\n\n%s",
        printline);
#ifdef USE_SDL_GRAPHICS
        MessageBox(graphics_GetWindowHWND(), printerror, "Fatal error", MB_OK|MB_ICONERROR);
#else
        MessageBox(NULL, printerror, "Fatal error", MB_OK|MB_ICONERROR);
#endif
    }
#endif
}

void printwarning(const char* fmt, ...) {
    char printline[2048];
    va_list a;
    va_start(a, fmt);
    vsnprintf(printline, sizeof(printline)-1, fmt, a);
    printline[sizeof(printline)-1] = 0;
    va_end(a);
#if defined(ANDROID) || defined(__ANDROID__)
    __android_log_print(ANDROID_LOG_ERROR, "blitwizard", "%s", printline);
#else
    memorylog(printline);
    memorylog("\n");
    consolelog("warning", printline);
#endif
}

void printinfo(const char* fmt, ...) {
    char printline[2048];
    va_list a;
    va_start(a, fmt);
    vsnprintf(printline, sizeof(printline)-1, fmt, a);
    printline[sizeof(printline)-1] = 0;
    va_end(a);
#if defined(ANDROID) || defined(__ANDROID__)
    __android_log_print(ANDROID_LOG_INFO, "blitwizard", "%s", printline);
#else
    memorylog(printline);
    memorylog("\n");
    consolelog("info", printline);
#endif
}



