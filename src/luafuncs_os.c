
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

/// This is the Lua "os" standard module, as extended by blitwizard.
// (<a href="http://www.lua.org/manual/5.2/manual.html#6.9">Check here for the documentation of the Lua "os" standard functions</a>)
//
// Blitwizard extends Lua's "os" module with
// a few additional functions as documented here.
// Hopefully they'll be useful with your game building!
// @author Jonas Thiem  (jonas.thiem@gmail.com)
// @copyright 2011-2013
// @license zlib
// @module os

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
#include <SDL2/SDL.h>
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

/// Return the name of the operating system your game is
// currently running on.
// @function sysname
// @treturn string Operating system name
int luafuncs_sysname(lua_State* l) {
    lua_pushstring(l, osinfo_GetSystemName());
    return 1;
}

/// Return the detailed version of the operating system
// your game is currently running on.
// @function sysversion
// @treturn string Operating system version
int luafuncs_sysversion(lua_State* l) {
    lua_pushstring(l, osinfo_GetSystemVersion());
    return 1;
}

/// Check if a given path to a file or directory exists
// (it points to a valid file or directory).
// Returns true if that is the case, or false if not.
// @function exists
// @tparam string path file or directory path
// @treturn boolean true if target exists, false if not
int luafuncs_exists(lua_State* l) {
    const char* p = lua_tostring(l, 1);
    if (!p) {
        return haveluaerror(l, badargument1, 1, "os.exists", "string", lua_strtype(l, 1));
    }
    if (file_DoesFileExist(p)) {
        lua_pushboolean(l, 1);
    } else {
#ifdef USE_PHYSFS
        if (resources_LocateResource(p, NULL)) {
            lua_pushboolean(l, 1);
            return 1;
        }
#endif
        lua_pushboolean(l, 0);
    }
    return 1;
}

/// Check if the given path points to a directory or not.
// If it points to a file or to nothing, this function returns false.
// If it points to a directory, it returns true.
// @function isdir
// @tparam string path directory path
// @treturn boolean true if target is directory, false if not
int luafuncs_isdir(lua_State* l) {
    const char* p = lua_tostring(l, 1);
    if (!p) {
        lua_pushstring(l, "First argument is not a valid path string");
        return lua_error(l);
    }
    if (!file_DoesFileExist(p)) {
#ifdef USE_PHYSFS
        if (resource_IsFolderInZip(p)) {
            lua_pushboolean(l, 1);
            return 1;
        }
#endif
        char errmsg[500];
        snprintf(errmsg, sizeof(errmsg), "No such file or directory: %s\n", p);
        errmsg[sizeof(errmsg)-1] = 0;
        lua_pushstring(l, errmsg);
        return lua_error(l);
    }
    if (file_IsDirectory(p)) {
        lua_pushboolean(l, 1);
    } else {
        lua_pushboolean(l, 0);
    }
    return 1;
}

/// Returns a table array containing all the file names of the
// files in the specified directory.
//
// <b>Important</b>: this function lists both files from actual directories on
// hard disk, and virtual directories loaded through
// @{blitwizard.loadResourceArchive|loaded zip resource archives}.
// Virtual files will take precedence over real files (you won't
// receive duplicates).
//
// If you don't want virtual files listed (e.g. because you want
// to examine any directory supplied by the user and not part of your
// project)
// @function ls
// @tparam string directory path, empty string ("") for current directory
// @tparam boolean virtual_files (optional) Specify true to list virtual files inside virtual directories aswell (the default behaviour), false to list only files in the actual file system on disk
// @usage
//   -- list all files in current directory:
//   for i,file in ipairs(os.ls("")) do
//       print("file name: " .. file)
//   end
int luafuncs_ls(lua_State* l) {
    const char* p = lua_tostring(l, 1);
    if (!p) {
        lua_pushstring(l, "First argument is not a valid path string");
        return lua_error(l);
    }
    int list_virtual = 1;
    if (lua_gettop(l) >= 2 && lua_type(l, 2) != LUA_TNIL) {
        if (lua_type(l, 2) != LUA_TNUMBER) {
            return haveluaerror(l, badargument1, 2, "os.ls", "boolean or nil",
            lua_strtype(l, 2));
        }
        list_virtual = lua_toboolean(l, 2);
    }

    // get virtual filelist:
    char** filelist = NULL;
#ifdef USE_PHYSFS
    if (list_virtual) {
        filelist = resource_FileList(p);
    }
#endif

    // get iteration context for "real" on disk directory:
    struct filelistcontext* ctx = filelist_Create(p);

    if (!ctx && (!list_virtual || !filelist)) {
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
    // loop through all files:
    while ((returnvalue = filelist_GetNextFile(ctx,
    filenamebuf, sizeof(filenamebuf), &isdir)) == 1) {
        i++;
        lua_checkstack(l, 3);

        int duplicate = 0;
        // if we list against virtual folders too,
        // check for this being a duplicate:
        if (filelist) {
            size_t i = 0;
            while (filelist[i]) {
                if (strcasecmp(filelist[i], filenamebuf) == 0) {
                    duplicate = 1;
                    break;
                }
                i++;
            }
        }
        if (duplicate) {
            // don't add this one.
            i--;
            continue;
        }
        lua_pushinteger(l, i);
        lua_pushstring(l, filenamebuf);
        lua_settable(l, -3);
    }

    // free file list
    filelist_Free(ctx);

    // free virtual file list:
    if (filelist) {
        size_t i = 0;
        while (filelist[i]) {
            free(filelist[i]);
            i++;
        }
        free(filelist);
    }

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

/// Get an absolute path to the current working directory
// (sometimes(!) this is where the blitwizard binary is located
// - but not necessarily)
// @function getcwd
// @treturn string absolute directory path
int luafuncs_getcwd(lua_State* l) {
    char* p = file_GetCwd();
    lua_pushstring(l, p),
    free(p);
    return 1;
}

/// Change the current working directory to the specified directory
// @function chdir
// @tparam string directory path
int luafuncs_chdir(lua_State* l) {
    const char* p = lua_tostring(l,1);
    if (!p) {
        lua_pushstring(l, "First parameter is not a directory string");
        return lua_error(l);
    }
    if (!file_Cwd(p)) {
        char errmsg[512];
        snprintf(errmsg,sizeof(errmsg)-1,
            "Failed to change directory to: %s",p);
        errmsg[sizeof(errmsg)-1] = 0;
        lua_pushstring(l, errmsg);
        return lua_error(l);
    }
    return 0;
}

/// Open a visible console window (in addition to the regular blitwizard
// graphics output window)
// where the standard output of blitwizard can be read. This function
// only has an effect on Windows, where applications are rarely launched
// from a terminal.
//
// If you write a console application on Windows, you will want to call
// this on program startup.
// @function openConsole
int luafuncs_openConsole(__attribute__ ((unused))
lua_State* intentionally_unused) {
    win32console_Launch();
    return 0;
}

// This is just a different implementation to the
// default lua os.exit(), so no need to document
// this.
int luafuncs_exit(lua_State* l) {
    int exitcode = lua_tonumber(l,1);
    main_Quit(exitcode);
    return 0;
}

/// Get the absolute folder path of the template directory.
// Returns nil if no template directory was found.
//
// The template directory contains common Lua extensions
// for blitwizard which offer font rendering and more.
// @function templatedir
// @treturn string absolute folder path of template directory (or nil if none).
// Can be a relative path if loaded from a .zip archive
extern char* templatepath;
int luafuncs_templatedir(lua_State* l) {
    if (!templatepath) {
        lua_pushnil(l);
    } else {
        lua_pushstring(l, templatepath);
    }
    return 1;
}

/// Set the template folder to a given path if not previously set.
// You will most likely never use this function, the templates need
// them to work in some cases.
// @function forcetemplatedir
// @tparam string path template directory
int luafuncs_forcetemplatedir(lua_State* l) {
    if (lua_type(l, 1) != LUA_TSTRING) {
        return haveluaerror(l, badargument1, 1,
        "os.forcetemplatedir", "string", lua_strtype(l, 1));
    }

    if (templatepath) {
        return haveluaerror(l, "template path already detected");
    } else {
        templatepath = file_GetAbsolutePathFromRelativePath(
        lua_tostring(l, 1));
    }
    return 0;
}

/// Get the path of the game.lua file which has been loaded at startup.
// @function gameluapath
// @treturn string absolute file path to game.lua if a file on disk,
// relative path to game.lua if loaded from a .zip archive
int luafuncs_gameluapath(lua_State* l) {
    if (!gameluapath) {
        lua_pushnil(l);
    } else {
        lua_pushstring(l, gameluapath);
    }
    return 1;
}

/// This function reloads the game.lua file that has been initially loaded
// by blitwizard on startup.
//
// For a well-written game.lua, this will reload the game code without affecting
// the game state (the game will continue to run from where it left off).
//
// <b>This function is provided by the templates and only present if you
// use them.</b>
// @function reload


/// This function lets the whole blitwizard process freeze for the given
// amount of milliseconds.
//
// The only useful application of this is probably sleeping just a few
// milliseconds sometimes to use less cpu time (but obviously, this slows
// down the game and it might cause it to run notably worse on slow computers).
//
// If you're just looking into running a function after some time has passed,
// you most likely want to use @{blitwizard.runDelayed|runDelayed} instead.
// @function sleep
// @tparam number milliseconds the amount of milliseconds to sleep the whole process
int luafuncs_sleep(lua_State* l) {
    if (lua_type(l, 1) != LUA_TNUMBER) {
        return haveluaerror(l, badargument1, 1, "os.sleep",
        "number", lua_strtype(l, 1));
    }
    double ms = lua_tonumber(l, 1);
    if (ms < 0) {
        return haveluaerror(l, badargument2, 1, "os.sleep",
        "amount of milliseconds to sleep needs to be positive");
    }
    time_Sleep(ms);
    return 0;
}

