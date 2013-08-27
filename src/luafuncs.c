
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

///
// @author Jonas Thiem  (jonas.thiem@gmail.com)
// @copyright 2011-2013
// @license zlib
// @module blitwizard

#include "os.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <float.h>
#include <math.h>

#include "luaheader.h"

#include "os.h"
#include "luaerror.h"
#include "luafuncs.h"
#ifdef USE_SDL_GRAPHICS
#include "SDL.h"
#endif
#include "graphicstexture.h"
#ifdef WINDOWS
#include <windows.h> // needed for HWND in graphics.h
#endif
#include "graphics.h"
#include "timefuncs.h"
#include "luastate.h"
#include "audio.h"
#include "audiomixer.h"
#include "file.h"
#include "filelist.h"
#include "main.h"
#include "win32console.h"
#include "osinfo.h"
#include "logging.h"
#include "resources.h"
#include "zipfile.h"

#if defined(ANDROID) || defined(__ANDROID__)
// required for RWops file loading for Android
#include "SDL.h"
#endif

#if defined(ANDROID) || defined(__ANDROID__)
// the lua chunk reader for Android
SDL_RWops* loadfilerwops = NULL;
struct luachunkreaderinfo {
    SDL_RWops* rwops;
    char buffer[512];
};

static const char* luastringchunkreader(lua_State *l, void *data, size_t *size) {
    struct luachunkreaderinfo* info = (struct luachunkreaderinfo*)data;
    int i = info->rwops->read(info->rwops, info->buffer, 1, sizeof(*info->buffer));
    if (i > 0) {
        *size = (size_t)i;
        return info->buffer;
    }
    return NULL;
}

#endif

void _luacfuncs_onError_internal(const char* funcname, const char* error,
int triggerOnLog) {
    char errorstr[1024];
    snprintf(errorstr, sizeof(errorstr), "Error when calling %s: %s",
    funcname, error);
    if (triggerOnLog) {
        // trigger log entry:
        printerror("%s", errorstr);
    } else {
        luastate_suspendGC();
        // send error to ingame lua console:
        lua_State* l = (lua_State*)luastate_GetStatePtr();

        // call print with suspended garbage collector:
        lua_getglobal(l, "print");
        lua_pushstring(l, errorstr);
        lua_pcall(l, 1, 0, 0);

        luastate_resumeGC();
    }
}

void luacfuncs_onError(const char* funcname, const char* error) {
    _luacfuncs_onError_internal(funcname, error, 1);
}

void luacfuncs_onLog(const char* type, const char* fmt, ...) {
    char printline[2048];
    va_list a;
    va_start(a, fmt);
    vsnprintf(printline, sizeof(printline)-1, fmt, a);
    printline[sizeof(printline)-1] = 0;
    va_end(a);

    luastate_suspendGC();
    if (!luastate_PushFunctionArgumentToMainstate_String(type)) {
        fprintf(stderr, "Error when pushing func args to blitwizard.onLog");
        main_Quit(1); 
        return;
    }
    if (!luastate_PushFunctionArgumentToMainstate_String(printline)) {
        fprintf(stderr, "Error when pushing func args to blitwizard.onLog");
        main_Quit(1);
        return;
    }

    // call blitwizard.onLog:
    char* error = NULL;
    if (!luastate_CallFunctionInMainstate("blitwizard.onLog", 2, 1, 1,
    &error, NULL, NULL)) {
        _luacfuncs_onError_internal("blitwizard.onLog", error, 0);
        if (error) {
            free(error);
        }
    }
    luastate_resumeGC();
}

int luafuncs_getTemplateDirectory(lua_State* l) {
    lua_pushstring(l, templatepath);
    return 1;
}

#ifdef USE_PHYSFS
// this lua reader reads from a zip resourcelocation:
static struct zipfilereader* zfr = NULL;
static char zfrbuf[256];
const char* luazipreader(lua_State* l, void* data, size_t* size) {
    struct resourcelocation* s = data;
    
    // get a zip file reader if we don't have any:
    if (!zfr) {
        zfr = zipfile_FileOpen(s->location.ziplocation.archive,
        s->location.ziplocation.filepath);
        if (!zfr) {
            return NULL;
        }
    }

    // read more data:
    int r = zipfile_FileRead(zfr, zfrbuf, sizeof(zfrbuf));
    if (r == 0) {
        zipfile_FileClose(zfr);
        zfr = NULL;
        return NULL;
    }
    *size = r;
    return zfrbuf;
}
#endif

int luafuncs_loadfile(lua_State* l) {
    /* our load-file is NOT thread-safe !! */

    // obtain load file argument:
    const char* p = lua_tostring(l,1);
    if (!p) {
        return haveluaerror(l, badargument1, 1, "loadfile", "string", lua_strtype(l, 1));
    }

#if defined(ANDROID) || defined(__ANDROID__)
    // special Android file loading
    struct luachunkreaderinfo* info = malloc(sizeof(*info));
    if (!info) {
        lua_pushstring(l, "malloc failed");
        return lua_error(l);
    }
    char errormsg[512];
    memset(info, 0, sizeof(*info));
    info->rwops = SDL_RWFromFile(p, "r");
    if (!info->rwops) {
        free(info);
        snprintf(errormsg, sizeof(errormsg), "Cannot open file \"%s\"", p);
        errormsg[sizeof(errormsg)-1] = 0;
        lua_pushstring(l, errormsg);
        return lua_error(l);
    }
    int r = lua_load(l, &luastringchunkreader, info, p, NULL);
    SDL_FreeRW(info->rwops);
    free(info);
    if (r != 0) {
        if (r == LUA_ERRSYNTAX) {
            snprintf(errormsg,sizeof(errormsg),"Syntax error: %s",lua_tostring(l,-1));
            lua_pop(l, 1);
            lua_pushstring(l, errormsg);
            return lua_error(l);
        }
        return lua_error(l);
    }
    return 1;
#else
    // check our resources infrastructure for the file:
    struct resourcelocation s;
    if (!resources_LocateResource(p, &s)) {
        return haveluaerror(l, "Cannot find file \"%s\"", p);
    }

    // open with internal zip reader, or with Lua's
    // regular file loading function, depending on
    // the location type:
    int r;
    switch (s.type) {
#ifdef USE_PHYSFS
    case LOCATION_TYPE_ZIP:
        // load with our own internal zip reader:
        if (zfr) {
            zipfile_FileClose(zfr);
            zfr = NULL;
        }
        r = lua_load(l,
        (lua_Reader)luazipreader,
        &s,
        s.location.ziplocation.filepath,
        "t");
        if (zfr) {
            zipfile_FileClose(zfr);
            zfr = NULL;
        }
        break;
#endif
    case LOCATION_TYPE_DISK:
        // regular file loading done by Lua
        r = luaL_loadfile(l, p);
        break;
    default:
        // no idea how to load this??
        return haveluaerror(l,
        "Unknown resource location type for \"%s\"", p);
    }
    if (r != 0) {
        char errormsg[512];
        if (r == LUA_ERRFILE) {
            return haveluaerror(l,
            "Cannot open file \"%s\"", p);
        }
        if (r == LUA_ERRSYNTAX) {
            return haveluaerror(l,
            "Syntax error: %s", lua_tostring(l, -1));
        }
        return lua_error(l);
    }
    return 1;
#endif
}

static char printlinebuf[1024 * 50] = "";
static int luafuncs_printline(void) {
    // print a line from the printlinebuf
    unsigned int len = strlen(printlinebuf);
    if (len == 0) {
        return 0;
    }
    unsigned int i = 0;
    while (i < len) {
        if (printlinebuf[i] == '\n') {
            break;
        }
        i++;
    }
    if (i >= len-1 && printlinebuf[len-1] != '\n') {
        return 0;
    }
    printlinebuf[i] = 0;
    fprintf(stdout, "%s\n", printlinebuf);
    memmove(printlinebuf, printlinebuf+(i+1), sizeof(printlinebuf)-(i+1));
    return 1;
}


/// Load a .zip file as a resource archive.
//
// All files inside the zip file will be mapped into the local
// folder as resource files, so that you can load them
// as sounds, graphics etc.
//
// E.g. if the archive contains a folder "blubb", and inside
// "sound.ogg", you can just load "blubb/sound.ogg" as a sound
// after loading the archive with this function.
// @function loadResourceArchive
// @tparam string Path to the .zip archive to be loaded

int luafuncs_loadResourceArchive(lua_State* l) {
    const char* p = lua_tostring(l, 1);
    if (!p) {
        return haveluaerror(l, badargument1, 1,
        "blitwizard.loadResourceArchive",
        "string", lua_strtype(l, 1));
    }
    if (!file_DoesFileExist(p) || file_IsDirectory(p)) {
        return haveluaerror(l,
        "specified resource archive file \"%s\" does not exist", p);
    }
    if (!resources_LoadZipFromFile(p, 1)) {
        if (!resources_LoadZipFromFile(p, 0)) {
            return haveluaerror(l,
            "failed to load specified resource archive \"%s\"", p);
        }
    }
    return 1;
}

int luafuncs_print(lua_State* l) { // not threadsafe
    int args = lua_gettop(l);
    int i = 1;
    while (i <= args) {
        switch (lua_type(l, i)) {
            case LUA_TSTRING: {
                // add a space char first
                if (strlen(printlinebuf) > 0) {
                    if (strlen(printlinebuf) < sizeof(printlinebuf)-1) {
                        strcat(printlinebuf, " ");
                    }
                }

                // add string
                unsigned int plen = strlen(printlinebuf);
                unsigned int len = (sizeof(printlinebuf)-1) - plen;
                const char* p = lua_tostring(l, i);
                if (len > strlen(p)) {
                    len = strlen(p);
                }
                if (len > 0) {
                    memcpy(printlinebuf + plen, p, len);
                    printlinebuf[plen + len] = 0;
                }
                break;
            }
            case LUA_TNUMBER: {
                // add a space char first
                if (strlen(printlinebuf) > 0) {
                    if (strlen(printlinebuf) < sizeof(printlinebuf)-1) {
                        strcat(printlinebuf, " ");
                    }
                }

                // add number
                unsigned int plen = strlen(printlinebuf);
                unsigned int len = (sizeof(printlinebuf)-1) - plen;
                char number[50];
                snprintf(number, sizeof(number)-1, "%f", lua_tonumber(l, i));
                number[sizeof(number)-1] = 0;
                if (len >= strlen(number)) {
                    len = strlen(number);
                }
                if (len > 0) {
                    memcpy(printlinebuf + plen, number, len);
                    printlinebuf[plen + len] = 0;
                }
                break;
            }
            default:
                break;
        }
        while (luafuncs_printline()) { }
        i++;
    }

    // add a line break
    if (strlen(printlinebuf) > 0) {
        if (strlen(printlinebuf) < sizeof(printlinebuf)-1) {
            strcat(printlinebuf, "\n");
        }
    }

    while (luafuncs_printline()) { }
    return 0;
}

int luafuncs_dostring(lua_State* l) {
    // a nice dostring which emits syntax error info
    const char* p = lua_tostring(l,1);
    if (!p) {
        return haveluaerror(l, badargument1, 1, "dostring",
        "string", lua_strtype(l, 1));
    }

    int r = luaL_loadstring(l, p);
    if (r != 0) { // got an error, throw it
        return lua_error(l);
    }

    // run string:
    int before = lua_gettop(l)-1;  // minus function itself
    lua_call(l, 0, LUA_MULTRET);
    return lua_gettop(l)-before;
}

int luafuncs_dostring_returnvalues(lua_State* l) {
    // a nice dostring which emits syntax error info
    const char* p = lua_tostring(l,1);
    if (!p) {
        return haveluaerror(l, badargument1, 1,
        "dostring_returnvalues",
        "string", lua_strtype(l, 1));
    }

    int r = luaL_loadstring(l, p);
    if (r != 0) { // got an error, throw it
        return lua_error(l);
    }

    // run string:
    int before = lua_gettop(l)-1;  // minus function itself
    lua_call(l, 0, LUA_MULTRET);
    int returncount = lua_gettop(l)-before;
    lua_pushnumber(l, returncount);
    lua_insert(l, -(returncount+1));
    return returncount + 1;
}

int luafuncs_dofile(lua_State* l) {
    // obtain function name argument
    const char* p = lua_tostring(l,1);
    if (!p) {
        return haveluaerror(l, badargument1, 1, "loadfile", "string",
        lua_strtype(l, 1));
    }

    // check additional arguments we might have received
    int additionalargs = lua_gettop(l)-1;

    // load function and call it
    lua_getglobal(l, "loadfile"); // first, push function
    lua_pushvalue(l, 1); // then push given file name as argument

    lua_call(l, 1, 1); // call loadfile

    // the stack should now look like this:
    //   [additional arg 1] ... [additional arg n] [loaded function]

    // if we have additional args on the stack, move the function in front:
    if (additionalargs > 0) {
        lua_insert(l, -(additionalargs+1));
    }

    int previoustop = lua_gettop(l)-(1+additionalargs); // minus the
        // function on the stack which lua_call() removes and all the args to it
    lua_call(l, additionalargs, LUA_MULTRET); // call returned function
        // by loadfile

    // return all values the function has left for us on the stack
    return lua_gettop(l)-previoustop;
}

int luafuncs_getTime(lua_State* l) {
    lua_pushnumber(l, time_GetMilliseconds());
    return 1;
}

int luafuncs_trandom(lua_State* l) {
    // try to get higher quality random numbers here
    // (although still not cryptographically safe)
#if defined(UNIX)
    // on linux, we use /dev/urandom
    uint64_t d;
    FILE* f = fopen("/dev/urandom", "rb");
    if (!f) {
        // fallback to something simple
        lua_pushnumber(l, drand48());
        return 1;
    }
    fread(&d, sizeof(d), 1, f);
    fclose(f);

    // limit to meaningful range:
    // a double has 16 meaningful digits.
    // that should cover 10^15 safely, which is roughly more than 2^49.
    // therefore, null 64-49 leading bits:
    int i = sizeof(d)*8;
    while (i > 49) {
        uint64_t nullmask = ~(0x8000000000000000 >> (64-i));
        d = d & nullmask;
        i--;
    }
    // we lose entropy here,
    // but we could no longer safely distinguish such high numbers
    // in a double anyway

    // convert to double in a 0, 1 range:
    double d2 = d;
    d2 /= (double)pow(2, 49);

    lua_pushnumber(l, d2);
    return 1;
#else
#ifdef WINDOWS
    // on windows, use cheap random numbers for now
    double d = (double)rand() / ((double)RAND_MAX+1);
    lua_pushnumber(l, d);
    return 1;
#else
    // unknown system
    lua_pushnumber(l, drand48());
    return 1;
#endif
#endif
}


int luafuncs_getRendererName(lua_State* l) {
#ifdef USE_GRAPHICS
    const char* p = graphics_GetCurrentRendererName();
    if (!p) {
        lua_pushnil(l);
        return 1;
    }else{
        lua_pushstring(l, p);
        return 1;
    }
#else // ifdef USE_GRAPHICS
    lua_pushstring(l, compiled_without_graphics);
    return lua_error(l);
#endif
}

int luafuncs_getBackendName(lua_State* l) {
#ifdef USE_AUDIO
    main_InitAudio();
    const char* p = audio_GetCurrentBackendName();
    if (p) {
        lua_pushstring(l, p);
    }else{
        lua_pushstring(l, "null driver");
    }
    return 1;
#else // ifdef USE_AUDIO
    lua_pushstring(l, compiled_without_audio);
    return lua_error(l);
#endif
}


/// Set the stepping frequency of the game logic.
// The default is 16ms. Physics are unaffected.
//
// Please note changing this is a very invasive change
// - don't do this if you don't know what you're doing.
//
// Also, do NOT use this to speed up/slow down your game
// while it runs. While it may work, it will lead to low
// frames per second and you should simply use a speed factor
// in all your logic functions instead.
//
// Generally, bigger stepping times to 16ms can cause
// all sorts of problems (low frames per second, physics
// callbacks triggering oddly late, ...), so this function
// is only useful for some rare cases.
// @function setStep
// @tparam number step_time the amount of time between steps in milliseconds. 16 is minimum (and default). HIGH STEPPING TIME MAY CAUSE SLOPPY FRAMERATE AND OTHER PROBLEMS

int luafuncs_setstep(lua_State* l) {
    int newtimestep = 16;
    int type = LUA_TNIL;
    if (lua_gettop(l) >= 1) {type = lua_type(l, 1);}
    if (type != LUA_TNUMBER) {
        return haveluaerror(l, badargument1, 1,
        "blitwizard.setStep", "number", lua_strtype(l, 1));
    }
    newtimestep = lua_tointeger(l, 1);
    if (newtimestep < 16) {
        return haveluaerror(l, badargument2, 1,
        "blitwizard.setStep", "time step cannot be smaller than 16");
    }
    main_SetTimestep(newtimestep);
    return 0;
}


/// Specify this function if you want to get notified
// when your blitwizard game gets closed (close button,
// ALT+F4, ...).
//
// <b>This function does not exist</b> unless you specify/override
// it with blitwizard.onClose = ...
//
// This event is useful if you want to save the game
// or do other things on shutdown.
// @function onClose
// @usage
// -- specify our own close event function:
// function blitwizard.onClose()
//     print("Shutting down!")
// end

/// Specify this function if you want to get notified
// when the mouse is moved over the @{blitwizard.graphics|graphics output}.
//
// The coordinates you get are the same that would be used
// for @{blitwizard.graphics.camera:screenPosTo2dWorldPos|
// camera:screenPosTo2dWorldPos}.
//
// <b>This function does not exist</b> unless you specify/override
// it.
// @function onMouseMove
// @tparam number x_pos mouse X position in game units
// @tparam number y_pos mouse Y position in game units
// @tparam userdata camera the @{blitwizard.graphics.camera|camera} the mouse moved over (most likely the default camera)
// @usage
// -- Listen for mouse evens:
// function blitwizard.onMouseMove(x, y, camera)
//     print("Mouse position: x: " .. x .. ", y: " .. y)
// end

/// Specify this function if you want to get notified
// when a mouse button is pressed down
// on a @{blitwizard.graphics.camera|camera} in the
// @{blitwizard.graphics|graphics output}.
// (Usually, one camera covers the whole graphics output)
// 
// If you just want to create a button to be clicked on (or register
// clicks on objects generally), take a look at
// @{blitwizard.object:onMouseClick|object:onMouseClick} instead.
//
// The coordinates and the @{blitwizard.graphics.camera|camera}
// are specified in the same manner as for
// @{blitwizard.onMouseMove}.
//
// <b>This function does not exist</b> unless you specify/override
// it.
// @function onMouseDown
// @tparam number x_pos mouse X position
// @tparam number y_pos mouse Y position
// @tparam number button mouse button (1 for left, 2 for right, 3 for middle)
// @tparam camera the @{blitwizard.graphics.camera|camera} the mouse was clicked on
// @usage
// -- Listen for mouse evens:
// function blitwizard.onMouseDown(x, y, button, camera)
//     print("Mouse down: button: " .. button .. ", x: " .. x .. ", y: " .. y)
// end

/// Specify this function if you want to get notified
// when a mouse button (which was previously pressed down)
// is released
// on the @{blitwizard.graphics|graphics output}.
//
// The coordinates and the @{blitwizard.graphics.camera|camera}
// are specified in the same manner as for
// @{blitwizard.onMouseMove}.
//
// <b>This function does not exist</b> unless you specify/override
// it.
// @function onMouseUp
// @tparam number x_pos mouse X position
// @tparam number y_pos mouse Y position
// @tparam number button mouse button (1 for left, 2 for right, 3 for middle)
// @tparam camera the @{blitwizard.graphics.camera|camera} the mouse was clicked on
// @usage
// -- Listen for mouse evens:
// function blitwizard.onMouseUp(x, y, button)
//     print("Mouse up: button: " .. button .. ", x: " .. x .. ", y: " .. y)
// end

/// Specify this function if you want to receive internal
// blitwizard log messages.
//
// <b>This function is defined with a default function that
// simply prints() all log messages, which sends them to the
// @{blitwizard.console|developer console}.</b>
//
// Feel free to redefine it to do something more useful.
// @function onLog
// @tparam string msgtype the type of log message. Currently supported types: "info", "warning", "error"
// @tparam string msg the actual log message
// @usage
// -- only show warnings and errors:
// function blitwizard.onLog(msgtype, msg)
//     if type == "info" then
//         return
//     end
//     print("[LOG:" .. msgtype .. "] " .. msg)
// end

// an additional helper function:
size_t lua_tosize_t(lua_State* l, int index) {
    int i = lua_tonumber(l, index);
    if (i < 0) {
        i = 0;
    }
    return i;
}


