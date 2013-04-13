
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

#include "os.h"
#include "osinfo.h"
#ifdef UNIX
#include <signal.h>
#endif
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef UNIX
#include <execinfo.h>
#endif
#ifdef WINDOWS
#include <windows.h>
#endif

#define FATALERROR_MSG "************\nThe blitwizard engine has encountered a fatal error.\nTHIS SHOULD NEVER HAPPEN!\n\nPlease report this error at http://www.blitwizard.de/forum/\nand include the following error information:\n************\n\nError information:\n\n%s"

static const char* GetCrashInfo(const char* reason);
static void handleerror(const char* name);

#ifdef UNIX
char escapedstrbuf[512];
const char* escapedstr(const char* str) {
    if (!str) {return NULL;}

    size_t i = 0;
    while (*str && i < sizeof(escapedstrbuf)-2) {
        if (*str == '"') {
            escapedstrbuf[i] = '\\';
            escapedstrbuf[i+1] = '\"';
            i += 2;
        } else {
            escapedstrbuf[i] = *str;
            i++;
        }
        str++;
    }
    escapedstrbuf[i] = 0;
    return escapedstrbuf;
}

extern char* binpath;  // defined in main.c, points to our binary
char addr2linebuf[256];
// Translate pointer address to source code file/line
// for our blitwizard binary:
const char* addr2line(void* addr) {
    if (!binpath) {
        return NULL;
    }

    // we do this using the external "addr2line" tool.
    // if not present, we won't return anything:
    if (!osinfo_CheckForCmdProg("addr2line")) {
        return NULL;
    }

    // ok it is present, prepare command:
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "addr2line -e \"%s\" %p", escapedstr(binpath),
    addr);

    // run addr2info tool:
    FILE* f = popen(cmd, "r");
    if (!f) {
        return NULL;
    }

    // get result:
    addr2linebuf[0] = 0;
    char* s = fgets(addr2linebuf, sizeof(addr2linebuf), f);
    pclose(f);

    if (s && strlen(s) > 2) {
        // it shouldn't start with ??:
        if (s[0] != '?' && s[1] != '?') {
            // if it ends with a line break, remove it:
            if (s[strlen(s)-1] == '\n') {
                s[strlen(s)-1] = 0;
            }
            if (s[strlen(s)-1] == '\r') {
                s[strlen(s)-1] = 0;
            }
            return s;
        }
    }
    return NULL;
}

static void generatebacktrace(char* buffer, size_t buffersize) {
    if (buffersize <= 0) {
        return;
    }
    if (buffersize == 1) {
        buffer[buffersize] = 0;
        return;
    }

    void* backtraceptrs[50];
    int count = backtrace(backtraceptrs, sizeof(backtraceptrs));
    if (count <= 0) {
        buffer[buffersize] = 0;
        return;
    }

    char** symbols = backtrace_symbols(backtraceptrs, count);
    if (!symbols) {
        buffer[buffersize] = 0;
        return;
    }

    buffer[0] = 0;
    int i = 0;
    while (i < count) {
        // try to get a meaningful file location:
        const char* line = addr2line(backtraceptrs[i]);
        if (line) {
            // we have a nice file location:
            char appendstr[512];
            snprintf(appendstr, sizeof(appendstr), "%s [%p]\n",
            line, backtraceptrs[i]);
            strncat(buffer, appendstr, buffersize-(strlen(buffer)+1));
        } else {
            // append the (not-so-useful) backtrace_symbols output:
            strncat(buffer, symbols[i], buffersize-(strlen(buffer)+1));
            if (i < count - 1) {
                strncat(buffer, "\n", buffersize-(strlen(buffer)+1));
            }
        }
        i++;
    }
}

void signalhandler_FatalUnix(int signal) {
    switch (signal) {
    case SIGSEGV:
        handleerror("SIGSEGV");
        break;
    case SIGABRT:
        handleerror("SIGABRT");
        break;
    case SIGFPE:
        handleerror("SIGFPE");
        break;
    case SIGILL:
        handleerror("SIGILL");
        break;
    case SIGBUS:
        handleerror("SIGBUS");
        break;
    default:
        handleerror("unknown");
        break;
    }
}
#endif

static char crashinfo[10 * 4096];
static char backtracebuf[10 * 4096];
static const char* GetCrashInfo(const char* reason) {
#ifdef UNIX
    generatebacktrace(backtracebuf, sizeof(backtracebuf));
#else
    backtracebuf[0] = 0;
#endif
    snprintf(crashinfo, sizeof(crashinfo),
    "Operating system: %s (%s)\n"
    "Blitwizard version: %s\n"
    "Error reason: %s\n"
    "Backtrace:\n%s",
    osinfo_GetSystemName(), osinfo_GetSystemVersion(), VERSION,
    reason, backtracebuf);
    return crashinfo;
}

static void handleerror(const char* name) {
    char msg[4096];
    snprintf(msg, sizeof(msg), FATALERROR_MSG, GetCrashInfo(name));
    fprintf(stderr, "%s\n", msg);
    osinfo_ShowMessage(msg, 1);
    exit(1);
}

#ifdef WINDOWS
LONG signalhandler_FatalWindows(struct _EXCEPTION_POINTERS* ex) {
    switch (ex->ExceptionRecord->ExceptionCode) {
    default:
        handleerror("unknown");
        break;
    }
}
#endif

void signalhandling_Init(void) {
#ifdef UNIX
    // handler for fatal error signals:
    struct sigaction crashhandler;
    memset(&crashhandler, 0, sizeof(crashhandler));
    crashhandler.sa_handler = signalhandler_FatalUnix;
    sigemptyset(&crashhandler.sa_mask);
    crashhandler.sa_flags = 0;
    sigaction(SIGSEGV, &crashhandler, NULL);
    sigaction(SIGABRT, &crashhandler, NULL);
    sigaction(SIGFPE, &crashhandler, NULL);
    sigaction(SIGILL, &crashhandler, NULL);
    sigaction(SIGBUS, &crashhandler, NULL);
#endif
#ifdef WINDOWS
    SetUnhandledExceptionFilter(signalhandler_FatalWindows);
#endif
    return;
}
