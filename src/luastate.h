
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

// Run script:

int luastate_DoInitialFile(const char* file, int argcount, char** error);

// Misc:

char* luastate_GetPreferredAudioBackend(void);
int luastate_GetWantFFmpeg(void);
void luastate_PrintStackDebug(void);
void luastate_SetGCCallback(void* luastate, int tablestackindex, int (*callback)(void*));
void luastate_GCCollect(void);
void luastate_suspendGC(void);
void luastate_resumeGC(void);
void* luastate_GetStatePtr(void); // pointer of the lua state
void* internaltracebackfunc(void); // function pointer of traceback function

// Call functions:

// Arguments:
int luastate_PushFunctionArgumentToMainstate_Bool(int yesno); // 1: success, 0: failure (out of memory)
int luastate_PushFunctionArgumentToMainstate_String(const char* string); // 1: success, 0: failure (out of memory)
int luastate_PushFunctionArgumentToMainstate_Double(double i); // 1: success, 0: failure (out of memory)
// Function call:
int luastate_CallFunctionInMainstate(const char* function, int args,
int recursivetablelookup, int allownil, char** error,
int* functiondidnotexist, int* returnedboolean); // 1: success, 0: failure
// *error will be either changed to NULL or to an error string -
//   NULL means most likely out of memory)
// recursivetablelookup: 1 to allow recursive table lookup, 0 for no.
// allownil set to 1 will only report failure when the function
//   existed and ran into an error, if it simply didn't exist at all
//   it will report sucess.
// If allownil is 1 and functiondidnotexist is not NULL,
//   *functiondidnotexist will be set to 1 if the function did not exist
//   (and success was reported).
//   If the function actually existed (no matter whether the call succeeded
//   or not), it will be left untouched
// If returnedboolean is not NULL, its target will be changed to
//   a boolean return value if there was any returned by the function.
//   If the function couldn't be called or didn't exist, it will be left
//   untouched.

#define IDREF_MAGIC 373482

#define IDREF_MEDIA 1
#define IDREF_NETSTREAM 2
#define IDREF_BLITWIZARDOBJECT 3
#define IDREF_CAMERA 4
#define IDREF_TIMEOUTHANDLE 5

struct blitwizardobject;
struct mediaobject;
struct luacameralistentry;
struct luaidref {
    int magic;
    int type;
    union {
        int id;
        void* ptr;
        struct blitwizardobject* bobj;
        struct mediaobject* mobj;
        struct luacameralistentry* camera;
    } ref;
};



