
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

void luacfuncs_onError(const char* funcname, const char* error) {
    printerror("Error when calling %s: %s", funcname, error);

    // send error to ingame lua console:


}

int luafuncs_getTemplateDirectory(lua_State* l) {
    lua_pushstring(l, templatepath);
    return 1;
}

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
    printinfo("%s", printlinebuf);
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

int luafuncs_sysname(lua_State* l) {
    lua_pushstring(l, osinfo_GetSystemName());
    return 1;
}

int luafuncs_sysversion(lua_State* l) {
    lua_pushstring(l, osinfo_GetSystemVersion());
    return 1;
}

int luafuncs_dofile(lua_State* l) {
    // obtain function name argument
    const char* p = lua_tostring(l,1);
    if (!p) {
        return haveluaerror(l, badargument1, 1, "loadfile", "string", lua_strtype(l, 1));
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

    int previoustop = lua_gettop(l)-(1+additionalargs); // minus the function on the stack which lua_call() removes and all the args to it
    lua_call(l, additionalargs, LUA_MULTRET); // call returned function by loadfile

    // return all values the function has left for us on the stack
    return lua_gettop(l)-previoustop;
}

int luafuncs_exists(lua_State* l) {
    const char* p = lua_tostring(l, 1);
    if (!p) {
        return haveluaerror(l, badargument1, 1, "loadfile", "string", lua_strtype(l, 1));
    }
    if (file_DoesFileExist(p)) {
        lua_pushboolean(l, 1);
    }else{
        lua_pushboolean(l, 0);
    }
    return 1;
}

int luafuncs_isdir(lua_State* l) {
    const char* p = lua_tostring(l, 1);
    if (!p) {
        lua_pushstring(l, "First argument is not a valid path string");
        return lua_error(l);
    }
    if (!file_DoesFileExist(p)) {
        char errmsg[500];
        snprintf(errmsg, sizeof(errmsg), "No such file or directory: %s\n", p);
        errmsg[sizeof(errmsg)-1] = 0;
        lua_pushstring(l, errmsg);
        return lua_error(l);
    }
    if (file_IsDirectory(p)) {
        lua_pushboolean(l, 1);
    }else{
        lua_pushboolean(l, 0);
    }
    return 1;
}

int luafuncs_ls(lua_State* l) {
    const char* p = lua_tostring(l, 1);
    if (!p) {
        lua_pushstring(l, "First argument is not a valid path string");
        return lua_error(l);
    }
    struct filelistcontext* ctx = filelist_Create(p);
    if (!ctx) {
        char errmsg[500];
        snprintf(errmsg, sizeof(errmsg), "Failed to ls folder: %s", p);
        errmsg[sizeof(errmsg)-1] = 0;
        lua_pushstring(l, errmsg);
        return lua_error(l);
    }

    // create file listing table
    lua_newtable(l);

    // add all files/folders to file listing table
    char filenamebuf[500];
    int isdir;
    int returnvalue;
    int i = 0;
    while ((returnvalue = filelist_GetNextFile(ctx, filenamebuf, sizeof(filenamebuf), &isdir)) == 1) {
        i++;
        lua_pushinteger(l, i);
        lua_pushstring(l, filenamebuf);
        lua_settable(l, -3);
    }

    // free file list
    filelist_Free(ctx);

    // process error during listing
    if (returnvalue < 0) {
        lua_pop(l, 1); // remove file listing table

        char errmsg[500];
        snprintf(errmsg, sizeof(errmsg), "Error while processing ls in folder: %s", p);
        errmsg[sizeof(errmsg)-1] = 0;
        lua_pushstring(l, errmsg);
        return lua_error(l);
    }

    // return file list
    return 1;
}

int luafuncs_split(lua_State* l) {
    size_t len1,len2;
    const char* src1 = luaL_checklstring(l, 1, &len1);
    const char* src2 = luaL_checklstring(l, 2, &len2);
    int maxsplits = -1;
    if (lua_type(l, 3) != LUA_TNIL && lua_gettop(l) >= 3) {
        maxsplits = luaL_checkint(l, 3);
        if (maxsplits < 0) {maxsplits = 0;}
        if (maxsplits == 0) {
            lua_pushlstring(l, src1, len1);
            return 1;
        }
    }
    if (len2 == 0) {
        lua_pushlstring(l, src1, len1);
        return 1;
    }
    unsigned int returncount = 0;
    // split off as often as we can:
    while ((maxsplits > 0 || maxsplits < 0) && len1 >= len2) {
        unsigned int i = 0;
        int match = 0;
        while (i <= len1 - len2) {
            if (memcmp(src1 + i, src2, len2) == 0) {
                // found a delimeter match!
                lua_pushlstring(l, src1, i);
                src1 += i + len2;
                len1 -= i + len2;
                returncount++;
                match = 1;
                if (!lua_checkstack(l, 10)) {
                    lua_pop(l, returncount);
                    return haveluaerror(l, "Exceeded stack space - cannot grow stack further");
                }
                break;
            }
            i++;
        }
        if (match) {
            if (maxsplits > 0) {
                maxsplits--;
            }
        }else{
            break;
        }
    }
    // return remaining string:
    lua_pushlstring(l, src1, len1);
    return returncount + 1;
}

int luafuncs_startswith(lua_State* l) {
    size_t len1,len2;
    const char* src1 = lua_tolstring(l, 1, &len1);
    const char* src2 = lua_tolstring(l, 2, &len2);
    if (!src1 || !src2 || len2 > len1) {
        lua_pushboolean(l, 0);
        return 1;
    }
    if (memcmp(src1, src2, len2) == 0) {
        lua_pushboolean(l, 1);
        return 1;
    }
    lua_pushboolean(l, 0);
    return 1;
}

int luafuncs_endswith(lua_State* l) {
    size_t len1,len2;
    const char* src1 = lua_tolstring(l, 1, &len1);
    const char* src2 = lua_tolstring(l, 2, &len2);
    if (!src1 || !src2 || len2 > len1) {
        lua_pushboolean(l, 0);
        return 1;
    }
    if (memcmp(src1+(len1-len2), src2, len2) == 0) {
        lua_pushboolean(l, 1);
        return 1;
    }
    lua_pushboolean(l, 0);
    return 1;
}


int luafuncs_getTime(lua_State* l) {
    lua_pushnumber(l, time_GetMilliseconds());
    return 1;
}

int luafuncs_sleep(lua_State* l) {
    if (lua_type(l, 1) != LUA_TNUMBER || lua_tonumber(l, 1) < 0) {
        lua_pushstring(l, "First parameter is not a valid milliseconds number");
        return lua_error(l);
    }
    unsigned int i = lua_tointeger(l, 1);
    time_Sleep(i);
    return 0;
}

/*int luafuncs_getImageSize(lua_State* l) {
#ifdef USE_GRAPHICS
    const char* p = lua_tostring(l,1);
    if (!p) {
        lua_pushstring(l, "First parameter is not a valid image name string");
        return lua_error(l);
    }
    unsigned int w,h;
    if (!graphics_GetTextureDimensions(p, &w,&h)) {
        lua_pushstring(l, "Failed to get image size");
        return lua_error(l);
    }
    lua_pushnumber(l, w);
    lua_pushnumber(l, h);
    return 2;
#else // ifdef USE_GRAPHICS
    lua_pushstring(l, compiled_without_graphics);
    return lua_error(l);
#endif
}*/

int luafuncs_getWindowSize(lua_State* l) {
#ifdef USE_GRAPHICS
    unsigned int w,h;
    if (!graphics_GetWindowDimensions(&w,&h)) {
        lua_pushstring(l, "Failed to get window size");
        return lua_error(l);
    }
    lua_pushnumber(l, w);
    lua_pushnumber(l, h);
    return 2;
#else // ifdef USE_GRAPHICS
    lua_pushstring(l, compiled_without_graphics);
    return lua_error(l);
#endif
}


int luafuncs_getcwd(lua_State* l) {
    char* p = file_GetCwd();
    lua_pushstring(l, p),
    free(p);
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

int luafuncs_chdir(lua_State* l) {
    const char* p = lua_tostring(l,1);
    if (!p) {
        lua_pushstring(l, "First parameter is not a directory string");
        return lua_error(l);
    }
    if (!file_Cwd(p)) {
        char errmsg[512];
        snprintf(errmsg,sizeof(errmsg)-1,"Failed to change directory to: %s",p);
        errmsg[sizeof(errmsg)-1] = 0;
        lua_pushstring(l, errmsg);
        return lua_error(l);
    }
    return 0;
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

int luafuncs_openConsole(lua_State* intentionally_unused) {
    win32console_Launch();
    return 0;
}

#ifdef USE_AUDIO
static int soundfromstack(lua_State* l, int index) {
    if (lua_type(l, index) != LUA_TUSERDATA) {
        return -1;
    }
    if (lua_rawlen(l, index) != sizeof(struct luaidref)) {
        return -1;
    }
    /*struct luaidref* idref = (struct luaidref*)lua_touserdata(l, index);
    if (!idref || idref->magic != IDREF_MAGIC || idref->type != IDREF_SOUND) {
        return -1;
    }
    return idref->ref.id;*/
    return -1;
}
#endif

int luafuncs_stop(lua_State* l) {
#ifdef USE_AUDIO
    main_InitAudio();
    int id = soundfromstack(l, 1);
    if (id < 0) {
        lua_pushstring(l, "First parameter is not a valid sound handle");
        return lua_error(l);
    }
    audiomixer_StopSound(id);
    return 0;
#else // ifdef USE_AUDIO
    lua_pushstring(l, compiled_without_audio);
    return lua_error(l);
#endif
}

int luafuncs_playing(lua_State* l) {
#ifdef USE_AUDIO
    main_InitAudio();
    int id = soundfromstack(l, 1);
    if (id < 0) {
        lua_pushstring(l, "First parameter is not a valid sound handle");
        return lua_error(l);
    }
    if (audiomixer_IsSoundPlaying(id)) {
        lua_pushboolean(l, 1);
    }else{
        lua_pushboolean(l, 0);
    }
    return 1;
#else // ifdef USE_AUDIO
    lua_pushstring(l, compiled_without_audio);
    return lua_error(l);
#endif
}

int luafuncs_adjust(lua_State* l) {
#ifdef USE_AUDIO
    main_InitAudio();
    int id = soundfromstack(l, 1);
    if (id < 0) {
        lua_pushstring(l, "First parameter is not a valid sound handle");
        return lua_error(l);
    }
    if (lua_type(l, 2) != LUA_TNUMBER) {
        lua_pushstring(l, "Second parameter is not a valid volume number");
        return lua_error(l);
    }
    float volume = lua_tonumber(l, 2);
    if (volume < 0) {volume = 0;}
    if (volume > 1) {volume = 1;}
    if (lua_type(l, 3) != LUA_TNUMBER) {
        lua_pushstring(l, "Third parameter is not a valid panning number");
        return lua_error(l);
    }
    float panning = lua_tonumber(l, 3);
    if (panning < -1) {panning = -1;}
    if (panning > 1) {panning = 1;}

    audiomixer_AdjustSound(id, volume, panning, 0);
    return 0;
#else // ifdef USE_AUDIO
    lua_pushstring(l, compiled_without_audio);
    return lua_error(l);
#endif
}

/*int luafuncs_play(lua_State* l) {
#ifdef USE_AUDIO
    main_InitAudio();
    const char* p = lua_tostring(l, 1);
    if (!p) {
        lua_pushstring(l, "First parameter is not a valid sound name string");
        return lua_error(l);
    }
    float volume = 1;
    float panning = 0;
    int priority = -1;
    int looping = 0;
    float fadein = -1;
    if (lua_gettop(l) >= 2 && lua_type(l, 2) != LUA_TNIL) {
        if (lua_type(l,2) != LUA_TNUMBER) {
            lua_pushstring(l, "Second parameter is not a valid volume number");
            return lua_error(l);
        }
        volume = lua_tonumber(l, 2);
        if (volume < 0) {volume = 0;}
        if (volume > 1) {volume = 1;}
    }
    if (lua_gettop(l) >= 3 && lua_type(l, 3) != LUA_TNIL) {
        if (lua_type(l, 3) != LUA_TNUMBER) {
            lua_pushstring(l, "Third parameter is not a valid panning number");
            return lua_error(l);
        }
        panning = lua_tonumber(l, 3);
        if (panning < -1) {panning = -1;}
        if (panning > 1) {panning = 1;}
    }
    if (lua_gettop(l) >= 4 && lua_type(l, 4) != LUA_TNIL) {
        if (lua_type(l, 4) != LUA_TBOOLEAN) {
            lua_pushstring(l,"Fourth parameter is not a valid loop boolean");
            return lua_error(l);
        }
        if (lua_toboolean(l, 4)) {
            looping = 1;
        }
    }
    if (lua_gettop(l) >= 5 && lua_type(l, 5) != LUA_TNIL) {
        if (lua_type(l,5) != LUA_TNUMBER) {
            lua_pushstring(l, "Fifth parameter is not a valid priority index number");
            return lua_error(l);
        }
        priority = lua_tointeger(l, 5);
        if (priority < 0) {priority = 0;}
    }
    if (lua_gettop(l) >= 6 && lua_type(l, 6) != LUA_TNIL) {
        if (lua_type(l,6) != LUA_TNUMBER) {
            lua_pushstring(l, "Sixth parameter is not a valid fade-in seconds number");
            return lua_error(l);
        }
        fadein = lua_tonumber(l,6);
        if (fadein <= 0) {
            fadein = -1;
        }
    }
    struct luaidref* iref = lua_newuserdata(l, sizeof(*iref));
    memset(iref,0,sizeof(*iref));
    iref->magic = IDREF_MAGIC;
    iref->type = IDREF_SOUND;
    iref->ref.id = audiomixer_PlaySoundFromDisk(p, priority, volume, panning, 0, fadein, looping);
    if (iref->ref.id < 0) {
        char errormsg[512];
        snprintf(errormsg, sizeof(errormsg), "Cannot play sound \"%s\"", p);
        errormsg[sizeof(errormsg)-1] = 0;
        lua_pop(l,1);
        lua_pushstring(l, errormsg);
        return lua_error(l);
    }
    return 1;
#else // ifdef USE_AUDIO
    lua_pushstring(l, compiled_without_audio);
    return lua_error(l);
#endif
}*/

int luafuncs_getDesktopDisplayMode(lua_State* l) {
#ifdef USE_GRAPHICS
    int w,h;
    graphics_GetDesktopVideoMode(&w, &h);
    lua_pushnumber(l, w);
    lua_pushnumber(l, h);
    return 2;
#else // ifdef USE_GRAPHICS
    lua_pushstring(l, compiled_without_graphics);
    return lua_error(l);
#endif
}

int luafuncs_getDisplayModes(lua_State* l) {
#ifdef USE_GRAPHICS
    int c = graphics_GetNumberOfVideoModes();
    lua_createtable(l, 1, 0);

    // first, add desktop mode
    int desktopw,desktoph;
    graphics_GetDesktopVideoMode(&desktopw, &desktoph);

    // resolution table with desktop width, height
    lua_createtable(l, 2, 0);
    lua_pushnumber(l, 1);
    lua_pushnumber(l, desktopw);
    lua_settable(l, -3);
    lua_pushnumber(l, 2);
    lua_pushnumber(l, desktoph);
    lua_settable(l, -3);

    // add table into our list
    lua_pushnumber(l, 1);
    lua_insert(l, -2);
    lua_settable(l, -3);

    int i = 1;
    int index = 2;
    while (i <= c) {
        // add all supported video modes...
        int w,h;
        graphics_GetVideoMode(i, &w, &h);

        // ...but not the desktop mode twice
        if (w == desktopw && h == desktoph) {
            i++;
            continue;
        }

        // table containing the resolution width, height
        lua_createtable(l, 2, 0);
        lua_pushnumber(l, 1);
        lua_pushnumber(l, w);
        lua_settable(l, -3);
        lua_pushnumber(l, 2);
        lua_pushnumber(l, h);
        lua_settable(l, -3);

        // add the table into our list
        lua_pushnumber(l, index);
        lua_insert(l, -2);
        lua_settable(l, -3);
        index++;
        i++;
    }
    return 1;
#else // ifdef USE_GRAPHICS
    lua_pushstring(l, compiled_without_graphics);
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

int luafuncs_exit(lua_State* l) {
    int exitcode = lua_tonumber(l,1);
    main_Quit(exitcode);
    return 0;
}
