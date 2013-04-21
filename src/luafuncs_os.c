
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

/// This is the default Lua "os" module. However, blitwizard extends
// it with a few additional functions as documented here.
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
    }else{
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

/// Returns a table array containing all the files in a directory.
// @function ls
// @tparam string directory path, empty string ("") for current directory
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
    // loop through all files:
    while ((returnvalue = filelist_GetNextFile(ctx,
    filenamebuf, sizeof(filenamebuf), &isdir)) == 1) {
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
        snprintf(errmsg,sizeof(errmsg)-1,"Failed to change directory to: %s",p);
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
int luafuncs_openConsole(lua_State* intentionally_unused) {
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

