
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

#include "config.h"
#include "os.h"

#include "luaheader.h"
#include "luastate.h"
#include "luafuncs.h"
#include "physics.h"
#include "threading.h"
#include "luafuncs_graphics.h"
#include "luafuncs_object.h"
#include "luafuncs_objectphysics.h"
#include "luafuncs_physics.h"
#include "luafuncs_net.h"
#include "luafuncs_media_object.h"
#include "luafuncs_os.h"
#include "luafuncs_string.h"
#include "luafuncs_rundelayed.h"
#include "luaerror.h"
#include "luastate_functionTables.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "file.h"

static lua_State* scriptstate = NULL;

void* luastate_GetStatePtr() {
    return scriptstate;
}

void luastate_PrintStackDebug() {
    // print the contents of the Lua stack
    printf("Debug stack:\n");
    int m = lua_gettop(scriptstate);
    int i = 1;
    while (i <= m) {
        printf("%d (", i);
        switch (lua_type(scriptstate, i)) {
            case LUA_TNIL:
                printf("nil");
                break;
            case LUA_TSTRING:
                printf("string");
                break;
            case LUA_TNUMBER:
                printf("number");
                break;
            case LUA_TBOOLEAN:
                printf("boolean");
                break;
            case LUA_TUSERDATA:
                printf("userdata");
                break;
            case LUA_TFUNCTION:
                printf("function");
                break;
            default:
                printf("unknown");
                break;
        }
        printf("): ");
        switch (lua_type(scriptstate, i)) {
            case LUA_TNIL:
                printf("nil");
                break;
            case LUA_TSTRING:
                printf("\"%s\"", lua_tostring(scriptstate, i));
                break;
            case LUA_TNUMBER:
                printf("%f", lua_tonumber(scriptstate, i));
                break;
            case LUA_TBOOLEAN:
                if (lua_toboolean(scriptstate, i)) {
                    printf("true");
                }else{
                    printf("false");
                }
                break;
            case LUA_TUSERDATA:
                printf("<userdata %p>", lua_touserdata(scriptstate, i));
                break;
            case LUA_TFUNCTION:
                printf("<function>");
                break;
        }
        printf("\n");
        i++;
    }
}

// table must be on stack to which you want to set this function
void luastate_SetGCCallback(void* luastate, int tablestackindex,
int (*callback)(void*)) {
    lua_State* l = luastate;
    luaL_checkstack(l, 5,
    "insufficient stack to set gc callback on metatable");
    if (!lua_getmetatable(l, tablestackindex)) {
        lua_newtable(l);
    } else {
        if (lua_type(l, -1) != LUA_TTABLE) {
            lua_pop(l, 1);
            lua_newtable(l);
        }
    }
    lua_pushstring(l, "__gc");
    lua_pushcfunction(l, (lua_CFunction)callback);
    lua_rawset(l, -3);
    if (tablestackindex < 0) {tablestackindex--;} // metatable is now at the top
    lua_setmetatable(l, tablestackindex);
    if (tablestackindex < 0) {tablestackindex++;}
}

static int openlib_blitwizard(lua_State* l) {
    // add an empty "blitwizard" lib namespace
    static const struct luaL_Reg blitwizlib[] = { {NULL, NULL} };
    luaL_newlib(l, blitwizlib);
    return 1;
}

static int luastate_AddBlitwizFuncs(lua_State* l) {
    luaL_requiref(l, "blitwizard", openlib_blitwizard, 1);
    lua_pop(l, 1);
    return 0;
}

char* luastate_GetPreferredAudioBackend() {
    if (!scriptstate) {
        return NULL;
    }
    lua_getglobal(scriptstate, "audiobackend");
    const char* p = lua_tostring(scriptstate, -1);
    char* s = NULL;
    if (p) {
        s = strdup(p);
    }
    lua_pop(scriptstate, 1);
    return s;
}

int luastate_GetWantFFmpeg() {
    if (!scriptstate) {
        return 0;
    }
    lua_getglobal(scriptstate, "useffmpegaudio");
    if (lua_type(scriptstate, -1) == LUA_TBOOLEAN) {
        int i = lua_toboolean(scriptstate, -1);
        lua_pop(scriptstate, 1);
        if (i) {
            return 1;
        }
        return 0;
    }
    return 1;
}

static int gettraceback(lua_State* l) {
    lua_checkstack(l, 5);
    char errormsg[2048] = "";

    // obtain the error first:
    const char* p = lua_tostring(l, -1);
    if (p) {
        unsigned int i = strlen(p);
        if (i >= sizeof(errormsg)) {i = sizeof(errormsg)-1;}
        memcpy(errormsg, p, i);
        errormsg[i] = 0;
    }
    lua_pop(l, 1);

    // add a line break if we can
    if (strlen(errormsg) + strlen("\n") < sizeof(errormsg)) {
        strcat(errormsg, "\n");
    }

    // call the original debug.traceback
    lua_pushstring(l, "debug_traceback_preserved");
    lua_gettable(l, LUA_REGISTRYINDEX);
    if (lua_type(l, -1) != LUA_TFUNCTION) { // make sure it is valid
        // oops.
        char invaliddebugtraceback[] = "OOPS: The debug traceback "
        "couldn't be generated:\n"
        "  Traceback function is not present or invalid (lua type is '";
        if (strlen(errormsg) + strlen(invaliddebugtraceback) <
        sizeof(errormsg)) {
            strcat(errormsg, invaliddebugtraceback);
        }

        // clarify on the invalid type of the function (which is not a func):
        char typebuf[64] = "";
        luatypetoname(lua_type(l, -1), typebuf, 16);
        strcat(typebuf, "')");
        if (strlen(errormsg) + strlen(typebuf) < sizeof(errormsg)) {
            strcat(errormsg, typebuf);
        }

        // pop non-function:
        lua_pop(l, 1);

        // push error:
        lua_pushstring(l, errormsg);
        return 1;
    }
    lua_call(l, 0, 1);

    // add the traceback as much as we can
    p = lua_tostring(l, -1);
    if (p) {
        int len = strlen(p);
        if (strlen(errormsg) + len >= sizeof(errormsg)) {
            len = sizeof(errormsg) - strlen(errormsg) - 1;
        }
        if (len > 0) {
            int offset = strlen(errormsg);
            memcpy(errormsg + offset, p, len);
            errormsg[offset + len] = 0;
        }
    }
    lua_pop(l, 1); // pop traceback

    // push the whole result
    lua_pushstring(l, errormsg);
    return 1;
}

void* internaltracebackfunc() {
    return &gettraceback;
}

void luastate_RememberTracebackFunc(lua_State* l) {
    lua_pushstring(l, "debug_traceback_preserved"); // push table index

    // obtain debug.traceback
    lua_getglobal(l, "debug");
    lua_pushstring(l, "traceback");
    lua_gettable(l, -2);

    // get rid of the table on the stack (position -2)
    lua_insert(l, -2);
    lua_pop(l, 1);

    // set it to the registry table
    lua_settable(l, LUA_REGISTRYINDEX);
}

static void luastate_VoidDebug(lua_State* l) {
    // void debug table
    lua_pushnil(l);
    lua_setglobal(l, "debug");

    // void debug library from package.loaded
    lua_getglobal(l, "package");
    lua_pushstring(l, "loaded");
    lua_gettable(l, -2);
    lua_pushstring(l, "debug");
    lua_pushnil(l);
    lua_settable(l, -3);

    // should we void binary loaders to avoid loading a debug.so?
}

static char* whitelist[] = { "string", "os", "math", "blitwizard",
"io", "_G", "_VERSION", "pairs", "ipairs", "coroutine", "next",
"tostring", "tonumber", "type", "setmetatable", "getmetatable",
"load", "table", "error", "pcall", "xpcall", "assert", "require",
"arg", NULL };

static void luastate_ApplyWhitelist(lua_State* l) {
    int repeat = 1;
    while (repeat) {
        repeat = 0;

        // get global table and iterate:
        lua_getglobal(l, "_G");
        lua_pushnil(l);
        while (lua_next(l, -2) != 0) {
            // on stack are now key, value.
            // we don't care about the value:
            lua_pop(l, 1);

            // see if this key is whitelisted:
            int i = 0;
            int whitelisted = 0;
            while (whitelist[i]) {
                if (strcmp(lua_tostring(l, -1), whitelist[i]) == 0) {
                    whitelisted = 1;
                    break;
                }
                i++;
            }

            // if not whitelisted, remove it and restart checking _G:
            if (!whitelisted) {
                lua_pushnil(l);
                lua_settable(l, -3);
                // now only _G is left on the stack. push a fake key:
                lua_pushnil(l);
                // make sure to repeat:
                repeat = 1;
                // stop _G loop and restart from beginning:
                break;
            }
        }
        lua_pop(l, 2);  // pop key, _G
    }
}

static void luastate_InstructionCallback(lua_State* l, lua_Debug* i) {
    int (*terminationCallback)(void* stateptr) = 0;

    // obtain the callback:
    lua_pushstring(l, "terminationCallback");
    lua_gettable(l, LUA_REGISTRYINDEX);
    terminationCallback = (int (*)(void*))(lua_touserdata(l, -1));
    lua_pop(l, 1); // remove from stack again

    // call it:
    if (!terminationCallback((void*)l)) {
        // we shall instruct lua to terminate!
        lua_pushstring(l, "execution time exceeded");
        lua_error(l);
    }
}

static lua_State* luastate_New(int (*terminationCallback)(void* stateptr)) {
    lua_State* l = luaL_newstate();

    lua_gc(l, LUA_GCSTOP, 0);
    //lua_gc(l, LUA_GCSETPAUSE, 110);
    //lua_gc(l, LUA_GCSETSTEPMUL, 300);

    if (terminationCallback) {
        // allow possible script kill when script runs too long:
        lua_sethook(l, &luastate_InstructionCallback, LUA_MASKCOUNT, 5000);

        // push the pointer to the lua registry:
        lua_pushstring(l, "terminationCallback");
        lua_pushlightuserdata(l, (void*)terminationCallback);
        lua_settable(l, LUA_REGISTRYINDEX);
    }

    // standard libs
    luaL_openlibs(l);
    luastate_RememberTracebackFunc(l);
    luastate_VoidDebug(l);
    luastate_AddBlitwizFuncs(l);

    // clean up global namespace apart from whitelist:
    luastate_ApplyWhitelist(l);

    // own dofile/loadfile/print
    lua_pushcfunction(l, &luafuncs_loadfile);
    lua_setglobal(l, "loadfile");
    lua_pushcfunction(l, &luafuncs_dofile);
    lua_setglobal(l, "dofile");
    lua_pushcfunction(l, &luafuncs_print);
    lua_setglobal(l, "print");
    lua_pushcfunction(l, &luafuncs_dostring);
    lua_setglobal(l, "dostring");
    lua_pushcfunction(l, &luafuncs_dostring_returnvalues);
    lua_setglobal(l, "dostring_returnvalues");

    // obtain the blitwiz lib
    lua_getglobal(l, "blitwizard");

    // blitwizard.runDelayed:
    lua_pushstring(l, "runDelayed");
    lua_pushcfunction(l, &luafuncs_runDelayed);
    lua_settable(l, -3);

    // blitwizard.object:
    lua_pushstring(l, "object");
    luastate_CreateObjectTable(l);
    lua_settable(l, -3);

    // blitwizard.getAllObjects():
    lua_pushstring(l, "getAllObjects");
    lua_pushcfunction(l, &luafuncs_getAllObjects);
    lua_settable(l, -3);

    // blitwizard.scanFor2dObjects():
    lua_pushstring(l, "scanFor2dObjects");
    lua_pushcfunction(l, &luafuncs_scanFor2dObjects);
    lua_settable(l, -3);

    // blitwizard.setStep:
    lua_pushstring(l, "setStep");
    lua_pushcfunction(l, &luafuncs_setstep);
    lua_settable(l, -3);

    // blitwizard.loadResourceArchive
    lua_pushstring(l, "loadResourceArchive");
    lua_pushcfunction(l, &luafuncs_loadResourceArchive);
    lua_settable(l, -3);

    // blitwizard.getTemplateDirectory:
    lua_pushstring(l, "getTemplateDirectory");
    lua_pushcfunction(l, &luafuncs_getTemplateDirectory);
    lua_settable(l, -3);

    // blitwizard namespaces
    lua_pushstring(l, "graphics");
    luastate_CreateGraphicsTable(l);
    lua_settable(l, -3);

    lua_pushstring(l, "net");
    luastate_CreateNetTable(l);
    lua_settable(l, -3);

    lua_pushstring(l, "audio");
    luastate_CreateAudioTable(l);
    lua_settable(l, -3);

    lua_pushstring(l, "debug");
    luastate_CreateDebugTable(l);
    lua_settable(l, -3);

    lua_pushstring(l, "callback");
    lua_newtable(l);
        lua_pushstring(l, "event");
        lua_newtable(l);
        lua_settable(l,  -3);
    lua_settable(l, -3);

    lua_pushstring(l, "time");
    luastate_CreateTimeTable(l);
    lua_settable(l, -3);

    lua_pushstring(l, "physics");
    luastate_CreatePhysicsTable(l);
    lua_settable(l, -3);

    // we still have the module "blitwiz" on the stack here
    lua_pop(l, 1);

    // vector namespace:
    luastate_CreateVectorTable(l);
    lua_setglobal(l, "vector");

    // obtain math table
    lua_getglobal(l, "math");

    // math namespace extensions
    lua_pushstring(l, "trandom");
    lua_pushcfunction(l, &luafuncs_trandom);
    lua_settable(l, -3);

    // remove math table from stack
    lua_pop(l, 1);

    // obtain os table
    lua_getglobal(l, "os");

    // os namespace extensions
    lua_pushstring(l, "exit");
    lua_pushcfunction(l, &luafuncs_exit);
    lua_settable(l, -3);
    lua_pushstring(l, "chdir");
    lua_pushcfunction(l, &luafuncs_chdir);
    lua_settable(l, -3);
    lua_pushstring(l, "templatedir");
    lua_pushcfunction(l, &luafuncs_templatedir);
    lua_settable(l, -3);
    lua_pushstring(l, "sleep");
    lua_pushcfunction(l, &luafuncs_sleep);
    lua_settable(l, -3);
    lua_pushstring(l, "gameluapath");
    lua_pushcfunction(l, &luafuncs_gameluapath);
    lua_settable(l, -3);
    lua_pushstring(l, "forcetemplatedir");
    lua_pushcfunction(l, &luafuncs_forcetemplatedir);
    lua_settable(l, -3);
    lua_pushstring(l, "openConsole");
    lua_pushcfunction(l, &luafuncs_openConsole);
    lua_settable(l, -3);
    lua_pushstring(l, "ls");
    lua_pushcfunction(l, &luafuncs_ls);
    lua_settable(l, -3);
    lua_pushstring(l, "isdir");
    lua_pushcfunction(l, &luafuncs_isdir);
    lua_settable(l, -3);
    lua_pushstring(l, "getcwd");
    lua_pushcfunction(l, &luafuncs_getcwd);
    lua_settable(l, -3);
    lua_pushstring(l, "exists");
    lua_pushcfunction(l, &luafuncs_exists);
    lua_settable(l, -3);
    lua_pushstring(l, "sysname");
    lua_pushcfunction(l, &luafuncs_sysname);
    lua_settable(l, -3);
    lua_pushstring(l, "sysversion");
    lua_pushcfunction(l, &luafuncs_sysversion);
    lua_settable(l, -3);

    // throw table "os" off the stack
    lua_pop(l, 1);

    // get "string" table for custom string functions
    lua_getglobal(l, "string");

    // set custom string functions
    lua_pushstring(l, "startswith");
    lua_pushcfunction(l, &luafuncs_startswith);
    lua_settable(l, -3);
    lua_pushstring(l, "endswith");
    lua_pushcfunction(l, &luafuncs_endswith);
    lua_settable(l, -3);
    lua_pushstring(l, "split");
    lua_pushcfunction(l, &luafuncs_split);
    lua_settable(l, -3);

    // throw table "string" off the stack
    lua_pop(l, 1);

    // set _VERSION string:
    char vstr[512];
    char is64bit[] = " (64-bit binary)";
#ifndef _64BIT
    strcpy(is64bit, "");
#endif
    snprintf(vstr, sizeof(vstr), "Blitwizard %s based on Lua 5.2%s", VERSION, is64bit);
    lua_pushstring(l, vstr);
    lua_setglobal(l, "_VERSION");

    // now set a default blitwizard.onLog handler:
    lua_getglobal(l, "dostring");
    lua_pushstring(l, "function blitwizard.onLog(type, msg)\n"
        "    print(\"[LOG:\" .. type .. \"] \" .. msg)\n"
        "end\n");
    // coverity[unchecked_value] - we don't want to cause error message
    // logs from logging itself since that could cause an infinite loop.
    // therefore, we don't handle possible lua errors here.
    lua_pcall(l, 1, 0, 0);

    return l;
}

static int luastate_DoFile(lua_State* l, int argcount, const char* file, char** error) {
    int previoustop = lua_gettop(l);
    lua_pushcfunction(l, &gettraceback);
    lua_getglobal(l, "dofile"); // first, push function
    lua_pushstring(l, file); // then push file name as argument

    // in case of additional user-passed arguments, we need them too:
    if (argcount > 0) {
        // move file name argument in front of all arguments:
        lua_insert(l, -(argcount+3));

        // move function in front of all arguments (including file name):
        lua_insert(l, -(argcount+3));

        // move error function in front of all arguments + function:
        lua_insert(l, -(argcount+3));
    }

    int ret = lua_pcall(l, 1+argcount, 0, -(argcount+3)); // call returned function by loadfile

    int returnvalue = 1;
    // process errors
    if (ret != 0) {
        const char* e = lua_tostring(l, -1);
        *error = NULL;
        if (e) {
            *error = strdup(e);
        }
        returnvalue = 0;
    }

    // clean up stack (e.g. from return values or so)
    if (lua_gettop(scriptstate) > previoustop) {
        lua_pop(scriptstate, lua_gettop(scriptstate) - previoustop);
    }

    return returnvalue;
}

// declared in main.c:
int terminateCurrentScript(void* userdata);

int luastate_DoInitialFile(const char* file, int argcount, char** error) {
    if (!scriptstate) {
        scriptstate = luastate_New(terminateCurrentScript);
        if (!scriptstate) {
            *error = strdup("Failed to initialize state");
            return 0;
        }
    }
    return luastate_DoFile(scriptstate, argcount, file, error);
}

static void preparepush(void) {
    if (!scriptstate) {
        scriptstate = luastate_New(terminateCurrentScript);
        if (!scriptstate) {
            return;
        }
    }
    lua_checkstack(scriptstate, 2);
}

int luastate_PushFunctionArgumentToMainstate_Bool(int yesno) {
    preparepush();
    lua_pushboolean(scriptstate, yesno);
    return 1;
}

int luastate_PushFunctionArgumentToMainstate_String(const char* string) {
    preparepush();
    lua_pushstring(scriptstate, string);
    return 1;
}

int luastate_PushFunctionArgumentToMainstate_Double(double i) {
    preparepush();
    lua_pushnumber(scriptstate, i);
    return 1;
}

int luastate_CallFunctionInMainstate(const char* function,
int args, int recursivetables, int allownil, char** error,
int* functiondidnotexist, int *returnedbool) {
    // push error function
    lua_pushcfunction(scriptstate, &gettraceback);
    if (args > 0) {
        lua_insert(scriptstate, -(args+1));
    }

    // look up table components of our function name (e.g. namespace.func())
    int tablerecursion = 0;
    while (recursivetables && tablerecursion < 5) {
        unsigned int r = 0;
        int recursed = 0;
        while (r < strlen(function)) {
            if (function[r] == '.') {
                if (tablerecursion > 0) {
                    // check if what we got on the stack as a base table
                    // is actually a table
                    if (lua_type(scriptstate, -1) != LUA_TTABLE) {
                        // not a table.
                        lua_pop(scriptstate, 1);  // error func
                        lua_pop(scriptstate, args);

                        // remove recursive table
                        lua_pop(scriptstate, 1);

                        *error = strdup("part of recursive call path "
                        "is not a table");
                        return 0;
                    }
                }

                recursed = 1;
                // extract the component
                char* fp = malloc(r+1);
                if (!fp) {
                    *error = NULL;
                    // clean up stack again:
                    lua_pop(scriptstate, 1); // error func
                    lua_pop(scriptstate, args);
                    if (recursivetables > 0 && tablerecursion > 0) {
                        // clean up recursive table left on stack
                        lua_pop(scriptstate, 1);
                    }
                    *error = strdup("failed to allocate memory "
                    "for component string");
                    return 0;
                }
                memcpy(fp, function, r);
                fp[r] = 0;
                lua_pushstring(scriptstate, fp);
                function += r+1;
                free(fp);

                // lookup
                if (tablerecursion == 0) {
                    // lookup on global table
                    lua_getglobal(scriptstate, lua_tostring(scriptstate, -1));
                    lua_insert(scriptstate, -2);
                    lua_pop(scriptstate, 1);
                }else{
                    // lookup nested on previous table
                    lua_gettable(scriptstate, -2);

                    // dispose of previous table
                    lua_insert(scriptstate, -2);
                    lua_pop(scriptstate, 1);
                }
                break;
            }
            r++;
        }
        if (recursed) {
            tablerecursion++;
        } else {
            break;
        }
    }

    // lookup function normally if there was no recursion lookup:
    if (tablerecursion <= 0) {
        lua_getglobal(scriptstate, function);
    } else {
        // check if what we got on the stack as a base table
        // is actually a table
        if (lua_type(scriptstate, -1) != LUA_TTABLE) {
            // not a table.
            lua_pop(scriptstate, 1);  // error func
            lua_pop(scriptstate, args);

            // remove recursive table
            lua_pop(scriptstate, 1);

            *error = strdup("part of recursive call path is not a table");
            return 0;
        }

        // get the function from our recursive lookup
        lua_pushstring(scriptstate, function);
        lua_gettable(scriptstate, -2);

        // wipe out the table we got it from
        lua_insert(scriptstate,-2);
        lua_pop(scriptstate, 1);
    }

    // quit sanely if function is nil and we allowed this
    if (allownil && lua_type(scriptstate, -1) == LUA_TNIL) {
        // clean up stack again:
        lua_pop(scriptstate, 1); // error func
        lua_pop(scriptstate, args);
        if (recursivetables > 0) {
            // clean up recursive origin table left on stack
            lua_pop(scriptstate, 1);
        }
        // give back the info that it did not exist:
        if (functiondidnotexist) {
            *functiondidnotexist = 1;
        }
        return 1;
    }

    if (lua_type(scriptstate, -1) != LUA_TFUNCTION) {
        lua_pop(scriptstate, 1); // error func
        lua_pop(scriptstate, args); // arguments
        if (recursivetables > 0) {
            // clean up recursive origin table left on stack
            lua_pop(scriptstate, 1);
        }
        *error = strdup("tried to call something which is not a function");
        return 0;
    }

    // function needs to be first, then arguments. -> correct order
    if (args > 0) {
        lua_insert(scriptstate, -(args+1));
    }

    int previoustop = lua_gettop(scriptstate)-(args+2);
    // 2 = 1 (error func) + 1 (called func)

    // call function
    int i = lua_pcall(scriptstate, args, (returnedbool != NULL)*1, -(args+2));

    // process errors
    int returnvalue = 1;
    if (i != 0) {
        *error = NULL;
        if (i == LUA_ERRRUN || i == LUA_ERRERR) {
            const char* e = lua_tostring(scriptstate, -1);
            *error = NULL;
            if (e) {
                *error = strdup(e);
            }
        } else {
            if (i == LUA_ERRMEM) {
                *error = strdup("Out of memory");
            } else {
                *error = strdup("Unknown error");
            }
        }
        returnvalue = 0;
    } else {
        // process return value:
        if (returnedbool) {
            *returnedbool = lua_toboolean(scriptstate, -1);
            lua_pop(scriptstate, 1);
        }
    }

    // clean up stack
    if (lua_gettop(scriptstate) > previoustop) {
        lua_pop(scriptstate, lua_gettop(scriptstate) - previoustop);
    }

    return returnvalue;
}

void luastate_GCCollect() {
    lua_gc(scriptstate, LUA_GCSTEP, 0);
}

int wassuspended = 0;
int suspendcount = 0;
mutex* suspendLock = NULL;
__attribute__((constructor)) void luastate_lockForSuspendGC(void) {
    suspendLock = mutex_create();
}

void luastate_suspendGC() {
    mutex_lock(suspendLock);
    if (suspendcount <= 0) {
        // check current gc state:
        wassuspended = 1;
        if (lua_gc(scriptstate, LUA_GCISRUNNING, 0)) {
            wassuspended = 0;
        }

        // stop it:
        if (!wassuspended) {
            lua_gc(scriptstate, LUA_GCSTOP, 0);
        }
    }
    // increase count:
    suspendcount++;
    mutex_release(suspendLock);
}

void luastate_resumeGC() {
    mutex_lock(suspendLock);
    suspendcount--;
    if (suspendcount <= 0) {
        suspendcount = 0;
        if (!wassuspended) {
            lua_gc(scriptstate, LUA_GCRESTART, 0);
        }
    }
    mutex_release(suspendLock);
}

