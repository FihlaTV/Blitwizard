
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
#include "resources.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <inttypes.h>
#ifdef WINDOWS
#include <windows.h>
#define _WINDOWS_
#endif
#ifdef ANDROID
#include <android/log.h>
#endif

// physics2d callback we will need later when setting up the physics simulation
struct physicsobject2d;
int luafuncs_globalcollision2dcallback_unprotected(void* userdata, struct physicsobject2d* a, struct physicsobject2d* b, double x, double y, double normalx, double normaly, double force);

// media cleanup callback:
void checkAllMediaObjectsForCleanup(void);

int wantquit = 0; // set to 1 if there was a quit event
int suppressfurthererrors = 0; // a critical error was shown, don't show more
int windowisfocussed = 0;
int appinbackground = 0; // app is in background (mobile/Android)
static int sdlinitialised = 0; // sdl was initialised and needs to be quit
char* templatepath = NULL; // global template directory path as determined at runtime

char* binpath = NULL;  // path to blitwizard binary

#include "luastate.h"
#include "file.h"
#include "timefuncs.h"
#include "audio.h"
#include "main.h"
#include "audiomixer.h"
#include "logging.h"
#include "audiosourceffmpeg.h"
#include "signalhandling.h"
#include "physics.h"
#include "connections.h"
#include "listeners.h"
#ifdef USE_SDL_GRAPHICS
#include "SDL.h"
#endif
#include "graphicstexture.h"
#include "graphicstexturemanager.h"
#include "graphics.h"
#include "graphicsrender.h"

int TIMESTEP = 16;
int MAXLOGICITERATIONS = 50;  // 50 * 16 = 800ms

void main_SetTimestep(int timestep) {
    if (timestep < 16) {
        timestep = 16;
    }
    TIMESTEP = timestep;
    MAXLOGICITERATIONS = 800 / timestep;  // never do logic ..
            // ... for longer than 800ms
    if (MAXLOGICITERATIONS < 2) {
        MAXLOGICITERATIONS = 2;
    }
}

struct physicsworld* physics2ddefaultworld = NULL;
void* main_DefaultPhysics2dPtr() {
    return physics2ddefaultworld;
}

void main_Quit(int returncode) {
    listeners_CloseAll();
    if (sdlinitialised) {
#ifdef USE_SDL_AUDIO
        // audio_Quit(); // FIXME: workaround for
        // http://bugzilla.libsdl.org/show_bug.cgi?id=1396
        // (causes an unclean shutdown)
#endif
#ifdef USE_GRAPHICS
        graphics_Quit();
#endif
    }
    exit(returncode);
}

void fatalscripterror(void) {
    wantquit = 1;
    suppressfurthererrors = 1;
}

int simulateaudio = 0;
int audioinitialised = 0;
void main_InitAudio(void) {
#ifdef USE_AUDIO
    if (audioinitialised) {
        return;
    }
    audioinitialised = 1;

    // get audio backend
    char* p = luastate_GetPreferredAudioBackend();

    // load FFmpeg if we happen to want it
    if (luastate_GetWantFFmpeg()) {
        audiosourceffmpeg_LoadFFmpeg();
    }else{
        audiosourceffmpeg_DisableFFmpeg();
    }

#if defined(USE_SDL_AUDIO) || defined(WINDOWS)
    char* error;

    // initialise audio - try 32bit first
    s16mixmode = 0;
#ifndef FORCES16AUDIO
    if (!audio_Init(&audiomixer_GetBuffer, 0, p, 0, &error)) {
        if (error) {
            free(error);
        }
#endif
        // try 16bit now
        s16mixmode = 1;
        if (!audio_Init(&audiomixer_GetBuffer, 0, p, 1, &error)) {
            printwarning("Warning: Failed to initialise audio: %s", error);
            if (error) {
                free(error);
            }
            // non-fatal: we will simulate audio manually:
            simulateaudio = 1;
            s16mixmode = 0;
        }
#ifndef FORCES16AUDIO
    }
#endif
    if (p) {
        free(p);
    }
#else  // USE_SDL_AUDIO || WINDOWS
    // simulate audio:
    simulateaudio = 1;
    s16mixmode = 0;
#endif  // USE_SDL_AUDIO || WINDOWS
#else // ifdef USE_AUDIO
    // we don't support any audio
    return;
#endif  // ifdef USE_AUDIO
}


static void quitevent(void) {
    char* error;
    if (!luastate_CallFunctionInMainstate("blitwizard.onClose",
    0, 1, 1, &error, NULL)) {
        printerror("Error when calling blitwizard.onClose: %s",error);
        if (error) {
            free(error);
        }
        fatalscripterror();
        return;
    }
    exit(0);
}

static void mousebuttonevent(int button, int release, int x, int y) {
    char* error;
    char onmouseup[] = "blitwizard.onMouseUp";
    const char* funcname = "blitwizard.onMouseDown";
    if (release) {
        funcname = onmouseup;
    }
    if (!luastate_PushFunctionArgumentToMainstate_Double(x)) {
        printerror("Error when pushing func args to %s", funcname);
        fatalscripterror();
        main_Quit(1);
        return;
    }
    if (!luastate_PushFunctionArgumentToMainstate_Double(y)) {
        printerror("Error when pushing func args to %s", funcname);
        fatalscripterror();
        main_Quit(1);
        return;
    }
    if (!luastate_PushFunctionArgumentToMainstate_Double(button)) {
        printerror("Error when pushing func args to %s", funcname);
        fatalscripterror();
        main_Quit(1);
        return;
    }
    if (!luastate_CallFunctionInMainstate(funcname, 3, 1, 1, &error, NULL)) {
        printerror("Error when calling %s: %s", funcname, error);
        if (error) {
            free(error);
        }
        fatalscripterror();
        main_Quit(1);
        return;
    }
}
static void mousemoveevent(int x, int y) {
    char* error;
    if (!luastate_PushFunctionArgumentToMainstate_Double(x)) {
        printerror("Error when pushing func args to blitwizard.onMouseMove");
        fatalscripterror();
        main_Quit(1);
        return;
    }
    if (!luastate_PushFunctionArgumentToMainstate_Double(y)) {
        printerror("Error when pushing func args to blitwizard.onMouseMove");
        fatalscripterror();
        main_Quit(1);
        return;
    }
    if (!luastate_CallFunctionInMainstate("blitwizard.onMouseMove",
    2, 1, 1, &error, NULL)) {
        printerror("Error when calling blitwizard.onMouseMove: %s", error);
        if (error) {
            free(error);
        }
        fatalscripterror();
        main_Quit(1);
        return;
    }
}
static void keyboardevent(const char* button, int release) {
    char* error;
    char onkeyup[] = "blitwizard.onKeyUp";
    const char* funcname = "blitwizard.onKeyDown";
    if (release) {
        funcname = onkeyup;
    }
    if (!luastate_PushFunctionArgumentToMainstate_String(button)) {
        printerror("Error when pushing func args to %s", funcname);
        fatalscripterror();
        main_Quit(1);
        return;
    }
    if (!luastate_CallFunctionInMainstate(funcname, 1, 1, 1, &error, NULL)) {
        printerror("Error when calling %s: %s", funcname, error);
        if (error) {
            free(error);
        }
        fatalscripterror();
        main_Quit(1);
        return;
    }
}
static void textevent(const char* text) {
    char* error;
    if (!luastate_PushFunctionArgumentToMainstate_String(text)) {
        printerror("Error when pushing func args to blitwizard.onText");
        fatalscripterror();
        return;
    }
    if (!luastate_CallFunctionInMainstate("blitwizard.onText",
    1, 1, 1, &error, NULL)) {
        printerror("Error when calling blitwizard.onText: %s", error);
        if (error) {
            free(error);
        }
        fatalscripterror();
        return;
    }
}

static void putinbackground(int background) {
    if (background) {
        // remember we are in the background
        appinbackground = 1;
    } else {
        // restore textures and wipe old ones
#ifdef ANDROID
        graphics_ReopenForAndroid();
#endif
        // we are back in the foreground! \o/
        appinbackground = 0;
    }
}

int attemptTemplateLoad(const char* path) {
#ifdef WINDOWS
    // check for invalid absolute unix paths:
    if (path[0] == '/' || path[0] == '\\') {
        return 0;
    }
#endif

    // path to init.lua:
    char* p = malloc(strlen(path) + 1 + strlen("init.lua") + 1);
    if (!p) {
        printerror("Error: failed to allocate string when "
        "attempting to run templates init.lua");
        main_Quit(1);
        return 0;
    }
    memcpy(p, path, strlen(path));
    p[strlen(path)] = '/';
    memcpy(p+strlen(path)+1, "init.lua", strlen("init.lua"));
    p[strlen(path)+1+strlen("init.lua")] = 0;
    file_MakeSlashesNative(p);

    // if file doesn't exist, report failure:
    if (!file_DoesFileExist(p)) {
        free(p);
        return 0;
    }

    // update global template path:
    if (templatepath) {
        free(templatepath);
    }
    templatepath = strdup(path);

    // run file:
    char outofmem[] = "Out of memory";
    char* error;
    if (!luastate_DoInitialFile(p, 0, &error)) {
        if (error == NULL) {
            error = outofmem;
        }
        printerror("Error: An error occured when running "
        "templates init.lua: %s", error);
        if (error != outofmem) {
            free(error);
        }
        fatalscripterror();
        free(p);
        main_Quit(1);
        return 0;
    }
    free(p);
    return 1;
}


int luafuncs_ProcessNetEvents(void);

#define MAXSCRIPTARGS 1024

#if (defined(__ANDROID__) || defined(ANDROID))
int SDL_main(int argc, char** argv) {
#else
#ifdef WINDOWS
int CALLBACK WinMain(HINSTANCE hInstance,
HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
#else
int main(int argc, char** argv) {
#endif
#endif

#if defined(ANDROID) || defined(__ANDROID__)
    printinfo("Blitwizard %s starting", VERSION);
#endif

    // set signal handlers:
    signalhandling_Init();

    // set path to blitwizard binary:
#ifdef UNIX
    if (argc > 0) {
        binpath = file_GetAbsolutePathFromRelativePath(argv[0]);
    }
#endif

    // test crash handling:
    //*((int*)5) = 2;

    // evaluate command line arguments:
    const char* script = "game.lua";
    int scriptargfound = 0;
    int option_changedir = 0;
    char* option_templatepath = NULL;
    int nextoptionistemplatepath = 0;
    int nextoptionisscriptarg = 0;
    int gcframecount = 0;

#ifdef WINDOWS
    // obtain command line arguments a special way on windows:
    int argc = __argc;
    char** argv = __argv;
#endif

    // we want to store the script arguments so we can pass them to lua:
    char** scriptargs = malloc(sizeof(char*) * MAXSCRIPTARGS);
    if (!scriptargs) {
        printerror("Error: failed to allocate script args space");
        return 1;
    }
    int scriptargcount = 0;

    // parse command line arguments:
    int i = 1;
    while (i < argc) {
        if (!scriptargfound) { // pre-scriptname arguments
            // process template path option parameter:
            if (nextoptionistemplatepath) {
                nextoptionistemplatepath = 0;
                if (option_templatepath) {
                    free(option_templatepath);
                }
                option_templatepath = strdup(argv[i]);
                if (!option_templatepath) {
                    printerror("Error: failed to strdup() template "
                    "path argument");
                    main_Quit(1);
                    return 1;
                }
                file_MakeSlashesNative(option_templatepath);
                i++;
                continue;
            }

            // various options:
            if ((argv[i][0] == '-' || strcasecmp(argv[i],"/?") == 0)
            && !nextoptionisscriptarg) {
                if (strcasecmp(argv[i], "--") == 0) {
                    // this enforces the next arg to be the script name:
                    nextoptionisscriptarg = 1;
                    i++;
                    continue;
                }

                if (strcasecmp(argv[i],"--help") == 0 || strcasecmp(argv[i], "-help") == 0
                || strcasecmp(argv[i], "-?") == 0 || strcasecmp(argv[i],"/?") == 0
                || strcasecmp(argv[i],"-h") == 0) {
                    printf("blitwizard %s (C) 2011-2013 Jonas Thiem et al\n",VERSION);
                    printf("Usage:\n   blitwizard [blitwizard options] [script name] [script options]\n\n");
                    printf("The script name should be a .lua file containing\n"
                    "Lua source code for use with blitwizard.\n\n");
                    printf("The script options (optional) are passed through\n"
                    "to the script.\n\n");
                    printf("Supported blitwizard options:\n");
                    printf("   -changedir             Change working directory to "
                           "the\n"
                           "                          folder of the script\n");
                    printf("   -help                  Show this help text and quit\n");
                    printf("   -templatepath [path]   Check another place for "
                           "templates\n"
                           "                          (not the default "
                           "\"templates/\")\n");
                    printf("   -version               Show extended version info and quit\n");
                    return 0;
                }
                if (strcasecmp(argv[i],"-changedir") == 0) {
                    option_changedir = 1;
                    i++;
                    continue;
                }
                if (strcasecmp(argv[i],"-templatepath") == 0) {
                    nextoptionistemplatepath = 1;
                    i++;
                    continue;
                }
                if (strcmp(argv[i], "-v") == 0 || strcasecmp(argv[i], "-version") == 0
                || strcasecmp(argv[i], "--version") == 0) {
                    printf("blitwizard %s (C) 2011-2013 Jonas Thiem et al\n",VERSION);
                    printf("\nSupported features of this build:\n");

                    #ifdef USE_SDL_AUDIO
                    printf("  Audio device: SDL 2\n");
                    #else
                    #ifdef USE_AUDIO
                    #ifdef WINDOWS
                    printf("  Audio device: waveOut\n");
                    #else
                    printf("  Audio device: only virtual (not audible)\n");
                    #endif
                    #else
                    printf("  Audio device: no\n");
                    printf("     Playback support: none, audio disabled\n");
                    printf("     Resampling support: none, audio disabled\n");
                    #endif
                    #endif

                    #if (defined(USE_SDL_AUDIO) || defined(USE_AUDIO))
                    printf("     Playback support: Ogg (libogg)%s%s\n",
                    #if defined(USE_FLAC_AUDIO)
                    ", FLAC (libFLAC)"
                    #else
                    ""
                    #endif
                    ,
                    #if defined(USE_FFMPEG_AUDIO)
                    #ifndef USE_FLAC_AUDIO
                    ", FLAC (FFmpeg),\n      mp3 (FFmpeg), WAVE (FFmpeg), mp4 (FFmpeg),\n      many more.. (FFmpeg)\n     (Please note FFmpeg can fail to load at runtime,\n     resulting in FFmpeg playback support not working)"
                    #else
                    ",\n      mp3 (FFmpeg), WAVE (FFmpeg), mp4 (FFmpeg),\n      many more.. (FFmpeg)\n     (Please note FFmpeg can fail to load at runtime,\n      resulting in FFmpeg playback support not working)"
                    #endif
                    #else
                    ""
                    #endif
                    );
                    #if defined(USE_SPEEX_RESAMPLING)
                    printf("     Resampling: libspeex\n");
                    #else
                    printf("     Resampling: none (non-48kHz audio will sound wrong!)\n");
                    #endif
                    #endif

                    #ifdef USE_GRAPHICS
                    #ifdef USE_SDL_GRAPHICS
                    #ifdef USE_OGRE_GRAPHICS
                    printf("  Graphics device: SDL 2, Ogre\n");
                    printf("     2d graphics support: SDL 2, Ogre\n");
                    printf("     3d graphics support: Ogre\n");
                    #else
                    printf("  Graphics device: SDL 2\n");
                    printf("     2d graphics support: SDL 2\n");
                    printf("     3d graphics support: none\n");
                    #endif
                    #else
                    printf("  Graphics device: only virtual (not visible)\n");
                    printf("     2d graphics support: virtual\n");
                    printf("     3d graphics support: none\n");
                    #endif
                    #else
                    printf("  Graphics device: none\n");
                    printf("     2d graphics support: none, graphics disabled\n");
                    printf("     3d graphics support: none, graphics disabled\n");
                    #endif
                    #if defined(USE_PHYSICS2D) || defined(USE_PHYSICS3D)
                    printf("  Physics: yes\n");
                    #else
                    printf("  Physics: no\n");
                    #endif
                    #if defined(USE_PHYSICS2D)
                    printf("     2d physics: Box2D\n");
                    #else
                    printf("     2d physics: none\n");
                    #endif
                    #if defined(USE_PHYSICS3D)
                    printf("     3d physics: bullet\n");
                    #else
                    printf("     3d physics: none\n");
                    #endif
                    #if defined(USE_PHYSFS)
                    printf("  .zip archive resource loading: yes\n");
                    #else
                    printf("  .zip archive resource loading: no\n");
                    #endif     

                    printf("\nVarious build options:\n");
                    printf("  SYSTEM_TEMPLATE_PATH:\n   %s\n",
                    SYSTEM_TEMPLATE_PATH);
                    printf("  FINAL_USE_LIB_FLAGS:\n   %s\n",
                    USE_LIB_FLAGS);

                    printf("\nCheck out http://www.blitwizard.de/"
                    " for info about blitwizard.\n");

                    fflush(stdout);
                    exit(0);
                }
                printwarning("Warning: Unknown Blitwizard option: %s", argv[i]);
            }else{
                scriptargfound = 1;
                script = argv[i];
            }
        }else{
            // post-scriptname arguments -> store them for Lua
            if (scriptargcount < MAXSCRIPTARGS) {
                scriptargs[scriptargcount] = strdup(argv[i]);
                scriptargcount++;
            }
        }
        i++;
    }

#ifdef USE_AUDIO
    // This needs to be done at some point before we actually 
    // initialise audio so that the mixer is ready for use then
    audiomixer_Init();
#endif

    // check the provided path:
    char outofmem[] = "Out of memory";
    char* error;
    char* filenamebuf = NULL;

#if defined(ANDROID) || defined(__ANDROID__)
    printinfo("Blitwizard startup: locating lua start script...");
#endif

    // if no template path was provided, default to "templates/"
    if (!option_templatepath) {
        option_templatepath = strdup("templates/");
        if (!option_templatepath) {
            printerror("Error: failed to allocate initial template path");
            main_Quit(1);
            return 1;
        }
        file_MakeSlashesNative(option_templatepath);
    }

    // load internal resources appended to this binary,
    // so we can load the game.lua from it if there is any inside:
#ifdef WINDOWS
    // windows
    // try encrypted first:
    if (!resources_LoadZipFromOwnExecutable(NULL, 1)) {
        // ... ok, then attempt unencrypted:
        resources_LoadZipFromOwnExecutable(NULL, 0);
    }
#else
#ifndef ANDROID
    // unix systems
    // encrypted first:
    if (!resources_LoadZipFromOwnExecutable(argv[0], 1)) {
        // ... ok, then attempt unencrypted:
        resources_LoadZipFromOwnExecutable(argv[0], 0);
    }
#endif
#endif

    // check if provided script path is a folder:
    if (file_IsDirectory(script)) {
        // make sure it isn't inside a resource file as a proper file:
        if (!resources_LocateResource(script, NULL)) {
            // it isn't, so we can safely assume it is a folder.
            // -> append "game.lua" to the path
            filenamebuf = file_AddComponentToPath(script, "game.lua");
            if (!filenamebuf) {
                printerror("Error: failed to add component to script path");
                main_Quit(1);
                return 1;
            }
            script = filenamebuf;
        }
    }

    // check if we want to change directory to the provided script path:
    if (option_changedir) {
        char* p = file_GetAbsoluteDirectoryPathFromFilePath(script);
        if (!p) {
            printerror("Error: NULL returned for absolute directory");
            main_Quit(1);
            return 1;
        }
        char* newfilenamebuf = file_GetFileNameFromFilePath(script);
        if (!newfilenamebuf) {
            free(p);
            printerror("Error: NULL returned for file name");
            main_Quit(1);
            return 1;
        }
        if (filenamebuf) {
            free(filenamebuf);
        }
        filenamebuf = newfilenamebuf;
        if (!file_Cwd(p)) {
            free(filenamebuf);
            printerror("Error: Cannot cd to \"%s\"", p);
            free(p);
            main_Quit(1);
            return 1;
        }
        free(p);
        script = filenamebuf;
    }

/*#if defined(ANDROID) || defined(__ANDROID__)
    printinfo("Blitwizard startup: Preparing graphics framework...");
#endif

    // initialise graphics
#ifdef USE_GRAPHICS
    if (!graphics_Init(&error)) {
        printerror("Error: Failed to initialise graphics: %s",error);
        free(error);
        fatalscripterror();
        main_Quit(1);
        return 1;
    }
    sdlinitialised = 1;
#endif*/

#if defined(ANDROID) || defined(__ANDROID__)
    printinfo("Blitwizard startup: Initialising physics...");
#endif

#ifdef USE_PHYSICS2D
    // initialise physics
    physics2ddefaultworld = physics_CreateWorld(0);
    if (!physics2ddefaultworld) {
        printerror("Error: Failed to initialise Box2D physics");
        fatalscripterror();
        main_Quit(1);
        return 1;
    }
    physics_Set2dCollisionCallback(physics2ddefaultworld, &luafuncs_globalcollision2dcallback_unprotected, NULL);
#endif

#if defined(ANDROID) || defined(__ANDROID__)
    printinfo("Blitwizard startup: Reading templates if present...");
#endif

    // Search & run templates. Separate code for desktop/android due to
    // android having the templates in embedded resources (where cwd'ing to
    // isn't supported), while for the desktop it is a regular folder.
#if !defined(ANDROID)
    int checksystemwidetemplate = 1;
    // see if there is a template directory & file:
    if (file_DoesFileExist(option_templatepath)
    && file_IsDirectory(option_templatepath)) {
        checksystemwidetemplate = 0;

        // now run template file:
        if (!attemptTemplateLoad(option_templatepath)) {
            checksystemwidetemplate = 1;
        }
    }
#if defined(SYSTEM_TEMPLATE_PATH)
    if (checksystemwidetemplate) {
        attemptTemplateLoad(SYSTEM_TEMPLATE_PATH);
    }
#endif
#else // if !defined(ANDROID)
    // on Android, we only allow templates/init.lua.
    // see if we can read the file:
    int exists = 0;
    SDL_RWops* rwops = SDL_RWFromFile("templates/init.lua", "rb");
    if (rwops) {
        exists = 1;
        rwops->close(rwops);
    }
    if (exists) {
        // run the template file:
        attemptTemplateLoad("templates/");
    }
#endif

    // free template dir now that we've loaded things:
    free(option_templatepath);


#if defined(ANDROID) || defined(__ANDROID__)
    printinfo("Blitwizard startup: Executing lua start script...");
#endif

    // push command line arguments into script state:
    i = 0;
    int pushfailure = 0;
    while (i < scriptargcount) {
        if (!luastate_PushFunctionArgumentToMainstate_String(scriptargs[i])) {
            pushfailure = 1;
            break;
        }
        i++;
    }
    if (pushfailure) {
        printerror("Error: Couldn't push all script arguments into script state");
        main_Quit(1);
        return 1;
    }

    // open and run provided script file and pass the command line arguments:
    if (!luastate_DoInitialFile(script, scriptargcount, &error)) {
        if (error == NULL) {
            error = outofmem;
        }
        printerror("Error: an error occured when running \"%s\": %s", script, error);
        if (error != outofmem) {
            free(error);
        }
        fatalscripterror();
        main_Quit(1);
        return 1;
    }

#if defined(ANDROID) || defined(__ANDROID__)
    printinfo("Blitwizard startup: Calling blitwiz.on_init...");
#endif

    // call init
    if (!luastate_CallFunctionInMainstate("blitwizard.on_init", 0, 1, 1, &error, NULL)) {
        printerror("Error: An error occured when calling blitwizard.on_init: %s",error);
        if (error != outofmem) {
            free(error);
        }
        fatalscripterror();
        main_Quit(1);
        return 1;
    }

    // when graphics or audio is open, run the main loop
#if defined(ANDROID) || defined(__ANDROID__)
    printinfo("Blitwizard startup: Entering main loop...");
#endif

    // Initialise audio when it isn't
    main_InitAudio();

    // If we failed to initialise audio, we want to simulate it
#ifdef USE_AUDIO
    uint64_t simulateaudiotime = 0;
    if (simulateaudio) {
        simulateaudiotime = time_GetMilliseconds();
    }
#endif

    uint64_t logictimestamp = time_GetMilliseconds();
    uint64_t lastdrawingtime = 0;
    uint64_t physicstimestamp = time_GetMilliseconds();
    while (!wantquit) {
        uint64_t time = time_GetMilliseconds();

        // this is a hack for SDL bug http://bugzilla.libsdl.org/show_bug.cgi?id=1422

#ifdef USE_AUDIO
        // simulate audio
        if (simulateaudio) {
            while (simulateaudiotime < time_GetMilliseconds()) {
                char buf[48 * 4 * 2];
                audiomixer_GetBuffer(buf, 48 * 4 * 2);
                simulateaudiotime += 1; // 48 * 1000 times * 4 bytes * 2 channels per second = simulated 48kHz 32bit stereo audio
            }
        }
#endif // ifdef USE_AUDIO

        // check for unused, no longer playing media objects:
        checkAllMediaObjectsForCleanup();

        // slow sleep: check if we can safe some cpu by waiting longer
        unsigned int deltaspan = 16;
#ifndef USE_GRAPHICS
        int nodraw = 1;
#else
        int nodraw = 1;
        if (graphics_AreGraphicsRunning()) {
            nodraw = 0;
        }
#endif
        uint64_t delta = time_GetMilliseconds()-lastdrawingtime;
        if (nodraw) {
            // we can sleep as long as our timeste allows us to
            deltaspan = ((double)TIMESTEP)/2.1f;
        }

        // sleep/limit FPS as much as we can
        if (delta < deltaspan) {
            if (connections_NoConnectionsOpen() && !listeners_HaveActiveListeners()) {
                time_Sleep(deltaspan-delta);
                connections_SleepWait(0);
            } else {
                connections_SleepWait(deltaspan-delta);
            }
        } else {
            connections_SleepWait(0);
        }

        // Remember drawing time and process net events
        lastdrawingtime = time_GetMilliseconds();
        if (!luafuncs_ProcessNetEvents()) {
            // there was an error processing the events
            main_Quit(1);
        }

#ifdef USE_GRAPHICS
        // check and trigger all sort of input events
        graphics_CheckEvents(&quitevent, &mousebuttonevent, &mousemoveevent, &keyboardevent, &textevent, &putinbackground);
#endif

        // call the step function and advance physics
        int iterations = 0;
        while ((logictimestamp < time || physicstimestamp < time) && iterations < MAXLOGICITERATIONS) {
            if (logictimestamp < time && logictimestamp <= physicstimestamp) {
                // call logic functions of all objects:
                
                logictimestamp += TIMESTEP;
            }
#ifdef USE_PHYSICS2D
            if (physicstimestamp < time && physicstimestamp <= logictimestamp) {
                int psteps = ((float)TIMESTEP/(float)physics_GetStepSize(physics2ddefaultworld));
                if (psteps < 1) {psteps = 1;}
                while (psteps > 0) {
                    physics_Step(physics2ddefaultworld);
                    physicstimestamp += physics_GetStepSize(physics2ddefaultworld);
                    psteps--;
                }
            }
#else
            physicstimestamp = time + 2000;
#endif
            iterations++;
        }

        // check if we ran out of iterations:
        if (iterations >= MAXLOGICITERATIONS) {
            if (
#ifdef USE_PHYSICS2D
                    physicstimestamp < time ||
#endif
                 logictimestamp < time) {
                // we got a problem: we aren't finished,
                // but we hit the iteration limit
                if (physicstimestamp < time || logictimestamp < time) {
                    physicstimestamp = time_GetMilliseconds();
                    logictimestamp = time_GetMilliseconds();
                    printwarning("Warning: logic is too slow, maximum logic iterations have been reached (%d)", (int)MAXLOGICITERATIONS);
                }
            } else {
                // we don't need to iterate anymore -> everything is fine
            }
        }

        // texture manager tick:
        texturemanager_Tick();

#ifdef USE_GRAPHICS
        if (graphics_AreGraphicsRunning()) {
#ifdef ANDROID
            if (!appinbackground) {
#endif
                // draw a frame
                graphicsrender_Draw();
#ifdef ANDROID
            }
#endif
        }
#endif

        // we might want to quit if there is nothing else to do
#ifdef USE_AUDIO
        if (!graphics_AreGraphicsRunning() &&
        connections_NoConnectionsOpen() &&
        !listeners_HaveActiveListeners() && audiomixer_NoSoundsPlaying()) {
#else
        if (!graphics_AreGraphicsRunning() &&
        connections_NoConnectionsOpen() &&
        !listeners_HaveActiveListeners()) {
#endif
            main_Quit(1);
        }

#ifdef USE_GRAPHICS
        // be very sleepy if in background
        if (appinbackground) {
            time_Sleep(20);
        }
#endif

        // do some garbage collection:
        gcframecount++;
        if (gcframecount > 100) {
            // do a gc step once in a while
            luastate_GCCollect();
        }
    }
    main_Quit(0);
    return 0;
}

