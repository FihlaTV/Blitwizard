
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

#ifdef USE_SDL_GRAPHICS

//  various standard headers
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#ifdef WINDOWS
#include <windows.h>
#endif
#include <stdarg.h>

#include "logging.h"
#include "imgloader.h"
#include "timefuncs.h"
#include "hash.h"
#include "file.h"
#ifdef NOTHREADEDSDLRW
#include "main.h"
#endif

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

#include "blitwizard_opengl.h"

#include "graphicstexture.h"
#include "graphics.h"
#include "graphicstexturelist.h"
#include "graphicssdlglext.h"

SDL_Window *mainwindow = NULL;
SDL_Renderer *mainrenderer = NULL;
#ifdef USE_SDL_GRAPHICS_OPENGL_EFFECTS
static SDL_GLContext _context;
SDL_GLContext *maincontext = NULL;
#endif
int sdlvideoinit = 0;
int sdlinit = 0;
int manualopengl = 0;

extern int graphicsactive;  // whether graphics are active/open (1) or not (0)
int inbackground = 0;  // whether program has focus (1) or not (0)
int graphics3d = 0;  // 1: Ogre graphics, 0: SDL graphics
int mainwindowfullscreen; // whether fullscreen (1) or not (0)

double centerx = 0;
double centery = 0;
double zoom = 1;

int graphics_haveValidWindow() {
    if (mainwindow) {
        return 1;
    }
    return 0;
}


// initialize the video sub system, returns 1 on success or 0 on error:
static int graphics_initVideoSubsystem(char **error) {
    char errormsg[512];
    // initialize SDL video if not done yet
    if (!sdlvideoinit) {
         // set scaling settings
        SDL_SetHintWithPriority(SDL_HINT_RENDER_SCALE_QUALITY,
            "1", SDL_HINT_OVERRIDE);

        // initialize SDL
        if (!sdlinit) {
            if (SDL_Init(SDL_INIT_TIMER) < 0) {
                snprintf(errormsg, sizeof(errormsg),
                    "Failed to initialize SDL: %s", SDL_GetError());
                errormsg[sizeof(errormsg)-1] = 0;
                if (error) {
                    *error = strdup(errormsg);
                }
                return 0;
            }
            sdlinit = 1;
        }
        if (SDL_VideoInit(NULL) < 0) {
            snprintf(errormsg, sizeof(errormsg),
                "Failed to initialize SDL video: %s", SDL_GetError());
            errormsg[sizeof(errormsg)-1] = 0;
            if (error) {
                *error = strdup(errormsg);
            }
            return 0;
        }
        sdlvideoinit = 1;
    }
    return 1;
}

__attribute__((constructor))
static void graphics_setInitialHints(void) {
    SDL_SetHintWithPriority(SDL_HINT_RENDER_SCALE_QUALITY,
   "1", SDL_HINT_OVERRIDE);
}

int graphics_init(char **error, int use3dgraphics) {
    char errormsg[512];
    graphics3d = (use3dgraphics ? 1 : 0);

    if (!graphics3d) {
        // set scaling settings
        SDL_SetHintWithPriority(SDL_HINT_RENDER_SCALE_QUALITY,
        "1", SDL_HINT_OVERRIDE);

        // set minimize settings on linux to fix issues with Gnome 3:
#ifdef UNIX
        SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");
#endif

        // initialize SDL
        if (SDL_Init(SDL_INIT_TIMER) < 0) {
            snprintf(errormsg, sizeof(errormsg),
                "Failed to initialize SDL: %s", SDL_GetError());
            errormsg[sizeof(errormsg)-1] = 0;
            *error = strdup(errormsg);
            return 0;
        }
        return 1;
    }

    *error = strdup("3d graphics not available");
    return 0;
}


int graphics_getWindowDimensions(unsigned int *width, unsigned int *height) {
    if (!graphics3d && mainwindow) {
        int w,h;
        SDL_GetWindowSize(mainwindow, &w,&h);
        if (w < 0 || h < 0) {
            return 0;
        }
        *width = w;
        *height = h;
        return 1;
    }
    return 0;
}



void graphics_close(int preservetextures) {
    if (!graphics3d) {
        // close graphics, and destroy textures if instructed to do so
        graphicsactive = 0;
        if (mainrenderer) {
            if (preservetextures) {
                // preserve textures if not instructed to destroy them
                graphicstexturelist_transferTexturesFromHW();
            }
            SDL_DestroyRenderer(mainrenderer);
            mainrenderer = NULL;
        }
#ifdef USE_SDL_GRAPHICS_OPENGL_EFFECTS
        if (maincontext) {
            SDL_GL_DeleteContext(*maincontext);
            maincontext = NULL;
        }
#endif
        if (mainwindow) {
            SDL_DestroyWindow(mainwindow);
            mainwindow = NULL;
        }
    }
}

#ifdef ANDROID
void graphics_reopenForAndroid() {
    // throw away hardware textures:
    texturemanager_deviceLost();

    // preserve old window size:
    int w,h;
    w = 0;h = 0;
    graphics_GetWindowDimensions(&w, &h);

    // preserve renderer:
    char renderer[512];
    const char *p = graphics_GetCurrentRendererName();
    int len = strlen(p)+1;
    if (len >= sizeof(renderer)) {
        len = sizeof(renderer)-1;
    }
    memcpy(renderer, p, len);
    renderer[sizeof(renderer)-1] = 0;

    // preserve window title:
    char title[512];
    p = graphics_GetWindowTitle();
    len = strlen(p)+1;
    if (len >= sizeof(title)) {
        len = sizeof(title)-1;
    }
    memcpy(title, p, len);
    title[sizeof(title)-1] = 0;

    // close window:
    graphics_close(0);

    // reopen:
    char *e;
    graphics_setMode(w, h, 1, 0, title, renderer, &e);

    // transfer textures back to hardware:
    graphicstexturemanager_DeviceReopened();
}
#endif

const char *graphics_getWindowTitle() {
#ifdef USE_SDL_GRAPHICS_OPENGL_EFFECTS
    if ((!mainrenderer && !maincontext) || !mainwindow) {
        return NULL;
    }
#else
    if (!mainrenderer || !mainwindow) {
        return NULL;
    }
#endif
    return SDL_GetWindowTitle(mainwindow);
}

void graphics_quit() {
    graphics_close(0);
    if (!graphics3d) {
        if (sdlvideoinit) {
            SDL_VideoQuit();
            sdlvideoinit = 0;
        }
        SDL_Quit();
    }
}

SDL_RendererInfo info;
#if defined(ANDROID)
static char openglstaticname[] = "opengl";
#endif
static char opengleffectsstaticname[] = "opengleffects";
const char *graphics_getCurrentRendererName() {
#ifdef USE_SDL_GRAPHICS_OPENGL_EFFECTS
    if (manualopengl) {
        return opengleffectsstaticname;
    }
#endif
    if (!mainrenderer) {
        return NULL;
    }
    SDL_GetRendererInfo(mainrenderer, &info);
#if defined(ANDROID)
    if (strcasecmp(info.name, "opengles") == 0) {
        // we return "opengl" here aswell, since we want "opengl"
        // to represent the best/default opengl renderer consistently
        // across all platforms, which is opengles on Android (and
        // regular opengl for normal desktop platforms).
        return openglstaticname;
    }
#endif
    return info.name;
}

int *videomodesx = NULL;
int *videomodesy = NULL;

static void graphics_readVideoModes(void) {
    // free old video mode data
    if (videomodesx) {
        free(videomodesx);
        videomodesx = 0;
    }
    if (videomodesy) {
        free(videomodesy);
        videomodesy = 0;
    }

    // allocate space for video modes
    int d = SDL_GetNumVideoDisplays();
    if (d < 1) {
        return;
    }
    int c = SDL_GetNumDisplayModes(0);
    int i = 0;
    videomodesx = (int*)malloc((c+1)*sizeof(int));
    videomodesy = (int*)malloc((c+1)*sizeof(int));
    if (!videomodesx || !videomodesy) {
        if (videomodesx) {
            free(videomodesx);
        }
        if (videomodesy) {
            free(videomodesy);
        }
        return;
    }
    memset(videomodesx, 0, (c+1) * sizeof(int));
    memset(videomodesy, 0, (c+1) * sizeof(int));

    // read video modes
    int lastusedindex = -1;
    while (i < c) {
        SDL_DisplayMode m;
        if (SDL_GetDisplayMode(0, i, &m) == 0) {
            if (m.w > 0 && m.h > 0) {
                // first, check for duplicates
                int isduplicate = 0;
                int j = 0;
                while (j <= lastusedindex && videomodesx[j] > 0 && videomodesy[j] > 0) {
                    if (videomodesx[j] == m.w && videomodesy[j] == m.h) {
                        isduplicate = 1;
                        break;
                    }
                    j++;
                }
                if (isduplicate) {
                    i++;
                    continue;
                }

                // add mode
                lastusedindex++;
                videomodesx[lastusedindex] = m.w;
                videomodesy[lastusedindex] = m.h;
            }
        }
        i++;
    }
}

int graphics_getNumberOfVideoModes() {
    char *error;
    if (!graphics_initVideoSubsystem(&error)) {
        printwarning("Failed to initialise video subsystem: %s", error);
        if (error) {
            free(error);
        }
        return 0;
    }
    graphics_readVideoModes();
    int i = 0;
    while (videomodesx && videomodesx[i] > 0
            && videomodesy && videomodesy[i] > 0) {
        i++;
    }
    return i;
}

void graphics_getVideoMode(int index, int *x, int *y) {
    graphics_readVideoModes();
    *x = videomodesx[index];
    *y = videomodesy[index];
}

void graphics_getDesktopVideoMode(int *x, int *y) {
    char *error;
    *x = 0;
    *y = 0;
    if (!graphics_initVideoSubsystem(&error)) {
        printwarning("Failed to initialise video subsystem: %s", error);
        if (error) {
            free(error);
        }
        return;
    }

    // -> do this with SDL per default
    SDL_DisplayMode m;
    if (SDL_GetDesktopDisplayMode(0, &m) == 0) {
        *x = m.w;
        *y = m.h;
    } else {
        printwarning("Unable to determine desktop video mode: %s", SDL_GetError());
    }
}

void graphics_minimizeWindow(void) {
    if (!mainwindow) {
        return;
    }
    SDL_MinimizeWindow(mainwindow);
}

int graphics_isFullscreen(void) {
    if (mainwindow) {
        return mainwindowfullscreen;
    }
    return 0;
}

void graphics_toggleFullscreen(void) {
    if (!graphics3d) {
        if (!mainwindow) {
            return;
        }
        SDL_bool wantfull = SDL_TRUE;
        if (mainwindowfullscreen) {
            wantfull = SDL_FALSE;
        }
        if (SDL_SetWindowFullscreen(mainwindow, wantfull) == 0) {
            if (wantfull == SDL_TRUE) {
                mainwindowfullscreen = 1;
            } else {
                mainwindowfullscreen = 0;
            }
        }
    }
}

#ifdef WINDOWS
HWND graphics_getWindowHWND() {
    if (!mainwindow) {
        return NULL;
    }
    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);
    if (SDL_GetWindowWMInfo(mainwindow, &info)) {
        if (info.subsystem == SDL_SYSWM_WINDOWS) {
            return info.info.win.window;
        }
    }
    return NULL;
}
#endif

#ifdef USE_SDL_GRAPHICS_OPENGL_EFFECTS
static int graphics_setModeWithOpenGL(int width, int height, int fullscreen,
        int resizable, const char *title, const char *renderer, char **error) {
    // we want OpenGL 2.1:
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 2 );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 1 );

    _context = SDL_GL_CreateContext(mainwindow);
    maincontext = &_context;
    if (!maincontext) {
        *error = strdup("failed to create OpenGL 2.1 context");
        return 0;
    }

    // load up OpenGL extension function pointers:
    if (!graphicssdlglext_init(1)) {
        *error = strdup("failed to get all extension functions - "
            "you may need a newer graphics card driver or your "
            "hardware may not support all required functionality");
        return 0;
    }


    // attempt to enable vsync:
    SDL_GL_SetSwapInterval(1);

    // set up projection and model view matrix:
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glClearColor(0, 0, 0, 0);

    // various:
    glEnable(GL_TEXTURE_2D);

    return 1;
}
#endif

static int graphics_setModeWithRenderer(int width, int height, int fullscreen,
        int resizable, const char *title, const char *renderer,
        int softwarerendering, char **error) {
    // get renderer index
    int rendererindex = -1;
    if (strlen(renderer) > 0 && !softwarerendering) {
        int count = SDL_GetNumRenderDrivers();
        if (count > 0) {
            int r = 0;
            while (r < count) {
                SDL_RendererInfo info;
                SDL_GetRenderDriverInfo(r, &info);
                if (strcasecmp(info.name, renderer) == 0) {
                    rendererindex = r;
                    break;
                }
                r++;
            }
        }
    }

    // Create renderer
    if (!softwarerendering) {
        mainrenderer = SDL_CreateRenderer(mainwindow, rendererindex,
            SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
        if (!mainrenderer) {
            softwarerendering = 1;
        }
    }
    if (softwarerendering) {
        mainrenderer = SDL_CreateRenderer(mainwindow, -1,
            SDL_RENDERER_SOFTWARE);
    }
    if (!mainrenderer) {
        // we failed to create the renderer
        if (mainwindow) {
            // destroy window aswell in case it is open
            SDL_DestroyWindow(mainwindow);
            mainwindow = NULL;
        }
        char errormsg[128];
        if (softwarerendering) {
            snprintf(errormsg, sizeof(errormsg),
                "Failed to create SDL renderer (backend software): %s",
                SDL_GetError());
        } else {
            SDL_RendererInfo info;
            SDL_GetRenderDriverInfo(rendererindex, &info);
            snprintf(errormsg, sizeof(errormsg),
                "Failed to create SDL renderer (backend %s): %s",
                info.name, SDL_GetError());
        }
        errormsg[sizeof(errormsg)-1] = 0;
        *error = strdup(errormsg);
        return 0;
    } else {
        SDL_RendererInfo info;
        SDL_GetRendererInfo(mainrenderer, &info);
    }
    return 1;
}

int graphics_setMode(int width, int height, int fullscreen,
        int resizable, const char *title, const char *renderer, char **error) {
    graphics_calculateUnitToPixels(width, height);

#if defined(ANDROID)
    if (!fullscreen) {
        // do not use windowed on Android
        *error = strdup("windowed mode is not supported on Android");
        return 0;
    }
#endif

    char errormsg[512];

    // initialize SDL video if not done yet
    if (!graphics_initVideoSubsystem(error)) {
        return 0;
    }

    // think about the renderer we want
#ifndef WINDOWS
#ifdef ANDROID
    char preferredrenderer[20] = "opengles";
#else
#ifdef USE_SDL_GRAPHICS_OPENGL_EFFECTS
    char preferredrenderer[20] = "opengleffects";
#else
    char preferredrenderer[20] = "opengl";
#endif
#endif
#else
#ifdef USE_SDL_GRAPHICS_OPENGL_EFFECTS
    char preferredrenderer[20] = "opengleffects";
#else
    char preferredrenderer[20] = "direct3d";
#endif
#endif
    int softwarerendering = 0;
    if (renderer) {
        if (strcasecmp(renderer, "software") == 0) {
#ifdef ANDROID
            // we don't want software rendering on Android
#else
            softwarerendering = 1;
            strcpy(preferredrenderer, "software");
#endif
        } else {
            if (strcasecmp(renderer, "opengl") == 0) {
#ifdef ANDROID
                // opengles is the opengl we want for android :-)
                strcpy(preferredrenderer, "opengles");
#else
                // regular opengl on desktop platforms
                strcpy(preferredrenderer, "opengl");
#endif
            }
#ifdef WINDOWS
            // only windows knows direct3d obviously
            if (strcasecmp(renderer,"direct3d") == 0) {
                strcpy(preferredrenderer, "direct3d");
            }
#endif
        }
    }

    //  see if anything changes at all
    unsigned int oldw = 0;
    unsigned int oldh = 0;
    graphics_getWindowDimensions(&oldw,&oldh);
    if (mainwindow &&
    width == (int)oldw && height == (int)oldh) {
        int rendererchanged = 1;

#ifdef USE_SDL_GRAPHICS_OPENGL_EFFECTS
        if (!mainrenderer && strcasecmp(preferredrenderer,
                "opengleffects") == 0) {
            if (maincontext) {
                rendererchanged = 0;
            }
        }
#endif

        SDL_RendererInfo info;
        if (mainrenderer) {
            SDL_GetRendererInfo(mainrenderer, &info);
            if (strcasecmp(preferredrenderer, info.name) == 0) {
                rendererchanged = 0;
            }
        }

        if (rendererchanged) {
            //  same renderer and resolution
            if (strcmp(SDL_GetWindowTitle(mainwindow), title) != 0) {
                SDL_SetWindowTitle(mainwindow, title);
            }
            //  toggle fullscreen if desired
            if (graphics_isFullscreen() != fullscreen) {
                graphics_toggleFullscreen();
            }
            return 1;
        }
    }

    //  Check if we support the video mode for fullscreen -
    //   This is done to avoid SDL allowing impossible modes and
    //   giving us a fake resized/padded/whatever output we don't want.
    if (fullscreen) {
        //  check all video modes in the list SDL returns for us
        int count = graphics_getNumberOfVideoModes();
        int i = 0;
        int supportedmode = 0;
        while (i < count) {
            int w,h;
            graphics_getVideoMode(i, &w, &h);
            if (w == width && h == height) {
                supportedmode = 1;
                break;
            }
            i++;
        }
        if (!supportedmode) {
            //  check for desktop video mode aswell
            int w,h;
            graphics_getDesktopVideoMode(&w,&h);
            if (w == 0 || h == 0 || width != w || height != h) {
                *error = strdup("video mode is not supported");
                return 0;
            }
        }
    }

    // notify texture manager of device shutdown
    texturemanager_deviceLost();

    // destroy old window/renderer if we got one
    graphics_close(1);

    // set minimize settings on linux to fix issues with Gnome 3:
#ifdef UNIX
    SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");
#endif

    // create window
    if (fullscreen) {
        uint32_t flags = SDL_WINDOW_FULLSCREEN;
#ifdef USE_SDL_GRAPHICS_OPENGL_EFFECTS
        if (strcmp(preferredrenderer, "opengleffects") == 0) {
            flags |= SDL_WINDOW_OPENGL;
        }
#endif
        mainwindow = SDL_CreateWindow(title, 0, 0, width, height,
            flags);
        mainwindowfullscreen = 1;
    } else {
        uint32_t flags = 0;
#ifdef USE_SDL_GRAPHICS_OPENGL_EFFECTS
        if (strcmp(preferredrenderer, "opengleffects") == 0) {
            flags |= SDL_WINDOW_OPENGL;
        }
#endif
        mainwindow = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED, width, height, flags);
        mainwindowfullscreen = 0;
    }

    if (!mainwindow) {
        snprintf(errormsg, sizeof(errormsg),
            "Failed to open SDL window: %s", SDL_GetError());
        errormsg[sizeof(errormsg)-1] = 0;
        *error = strdup(errormsg);
        return 0;
    }

    // see if we actually ended up with the resolution we wanted:
    int actualwidth, actualheight;
    SDL_GetWindowSize(mainwindow, &actualwidth, &actualheight);
    if (actualwidth != width || actualheight != height) {
        if (fullscreen) {  // we failed to get the requested resolution:
            SDL_DestroyWindow(mainwindow);
            snprintf(errormsg, sizeof(errormsg), "Failed to open "
                "SDL window: ended up with other resolution than requested");
            *error = strdup(errormsg);
            return 0;
        }
    }

#ifdef USE_SDL_GRAPHICS_OPENGL_EFFECTS
    if (strcasecmp(preferredrenderer, "opengleffects") == 0 &&
            !softwarerendering) {
        if (!graphics_setModeWithOpenGL(width, height, fullscreen,
                resizable, title, preferredrenderer, error)) {
            return 0;
        }
    } else {
#endif
        if (!graphics_setModeWithRenderer(width, height, fullscreen,
                resizable, title, preferredrenderer, softwarerendering,
                error)) {
            return 0;
        }
#ifdef USE_SDL_GRAPHICS_OPENGL_EFFECTS
    }
#endif

    // notify texture manager that device is back
    texturemanager_deviceRestored();

    // Re-focus window if previously focussed
    if (!inbackground) {
        SDL_RaiseWindow(mainwindow);
    }

    graphicsactive = 1;
    return 1;
}

int lastfingerdownx,lastfingerdowny;

void graphics_checkEvents(
        void (*quitevent)(void),
        void (*mousebuttonevent)(int button, int release, int x, int y),
        void (*mousemoveevent)(int x, int y),
        void (*keyboardevent)(const char *button, int release),
        void (*textevent)(const char *text),
        void (*putinbackground)(int background)) {
    // initialize SDL video if not done yet
    if (!graphics_initVideoSubsystem(NULL)) {
        return;
    }
    
    if (!graphics3d) {
        SDL_Event e;
        memset(&e, 0, sizeof(e));
        while (SDL_PollEvent(&e) == 1) {
            if (e.type == SDL_QUIT) {
                if (quitevent) {
                    quitevent();
                }
            }
            if (e.type == SDL_MOUSEMOTION) {
                if (mousemoveevent) {
                    mousemoveevent(e.motion.x, e.motion.y);
                }
            }
            if (e.type == SDL_WINDOWEVENT &&
                (e.window.event == SDL_WINDOWEVENT_MINIMIZED
                || e.window.event == SDL_WINDOWEVENT_FOCUS_LOST)) {
                // app was put into background
                if (!inbackground) {
                    if (putinbackground) {
                        putinbackground(1);
                    }
                    inbackground = 1;
                }
            }
            if (e.type == SDL_WINDOWEVENT &&
                (e.window.event == SDL_WINDOWEVENT_RESTORED
                || e.window.event == SDL_WINDOWEVENT_FOCUS_GAINED)) {
                // app is pulled back into foreground
                if (inbackground) {
                    if (putinbackground) {
                        putinbackground(0);
                    }
                    inbackground = 0;
                }
            }
            if (e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP) {
                int release = 0;
                if (e.type == SDL_MOUSEBUTTONUP) {
                    release = 1;
                }
                int button = e.button.button;
                if (mousebuttonevent) {
                    mousebuttonevent(button, release, e.button.x, e.button.y);
                }
            }
            if (e.type == SDL_FINGERDOWN || e.type == SDL_FINGERUP) {
                int release = 0;
                int x,y;
                if (e.type == SDL_FINGERUP) {
                    // take fingerdown coordinates on fingerup
                    x = lastfingerdownx;
                    y = lastfingerdowny;
                    release = 1;
                } else {
                    // remember coordinates on fingerdown
                    x = e.tfinger.x;
                    y = e.tfinger.y;
                    lastfingerdownx = x;
                    lastfingerdowny = y;
                }
                // swap coordinates for landscape mode
                int temp = x;
                x = y;
                y = temp;
                int button = SDL_BUTTON_LEFT;
                if (mousebuttonevent) {
                    mousebuttonevent(button, release, x, y);
                }
            }
            if (e.type == SDL_TEXTINPUT) {
                if (textevent) {
                    textevent(e.text.text);
                }
            }
            if (e.type == SDL_WINDOWEVENT) {
                if (e.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
                }
            }
            if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
                int release = 0;
                if (e.type == SDL_KEYUP) {
                    release = 1;
                }
                char keybuf[30] = "";
                if (e.key.keysym.sym >= SDLK_F1 &&
                    e.key.keysym.sym <= SDLK_F12) {
                    sprintf(keybuf, "f%d", (e.key.keysym.sym+1)-SDLK_F1);
                }
                if (e.key.keysym.sym >= SDLK_0 && e.key.keysym.sym <= SDLK_9) {
                    sprintf(keybuf, "%d", e.key.keysym.sym - SDLK_0);
                }
                if (e.key.keysym.sym >= SDLK_a && e.key.keysym.sym <= SDLK_z) {
                    sprintf(keybuf, "%c", e.key.keysym.sym - SDLK_a + 'a');
                }
                switch (e.key.keysym.sym) {
                    case SDLK_SPACE:
                        sprintf(keybuf, "space");break;
                    case SDLK_BACKSPACE:
                        sprintf(keybuf, "backspace");break;
                    case SDLK_RETURN:
                        sprintf(keybuf, "return");break;
                    case SDLK_ESCAPE:
                        sprintf(keybuf, "escape");break;
                    case SDLK_TAB:
                        sprintf(keybuf, "tab");break;
                    case SDLK_LSHIFT:
                        sprintf(keybuf, "lshift");break;
                    case SDLK_RSHIFT:
                        sprintf(keybuf, "rshift");break;
                    case SDLK_UP:
                        sprintf(keybuf,"up");break;
                    case SDLK_DOWN:
                        sprintf(keybuf,"down");break;
                    case SDLK_LEFT:
                        sprintf(keybuf,"left");break;
                    case SDLK_RIGHT:
                        sprintf(keybuf,"right");break;
                    default:
                        if (strlen(keybuf) == 0) {
                            snprintf(keybuf, sizeof(keybuf)-1,
                            "unknown-%d", e.key.keysym.sym);
                        }
                        break;
                }
                if (strlen(keybuf) > 0) {
                    if (keyboardevent) {
                        keyboardevent(keybuf, release);
                    }
                }
            }
            memset(&e, 0, sizeof(e));
        }
    }
}

int graphics_getCameraCount() {
    // we only support one fake camera
    return 1;
}

int graphics_getCameraWidth(int index) {
    unsigned int w = 0;
    unsigned int h = 0;
    graphics_getWindowDimensions(&w, &h);    
    return w;
}

int graphics_getCameraHeight(int index) {
    unsigned int w = 0;
    unsigned int h = 0;
    graphics_getWindowDimensions(&w, &h);
    return h;
}

void graphics_setCamera2DCenterXY(int index, double x, double y) {
    centerx = x;
    centery = y;
}

double graphics_getCamera2DAspectRatio(int index) {
    return 1.0;
}

double graphics_getCamera2DCenterX(int index) {
    return centerx;
}

double graphics_getCamera2DCenterY(int index) {
    return centery;
}

#endif  // USE_SDL_GRAPHICS

