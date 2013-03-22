
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

extern "C" {

#ifdef USE_OGRE_GRAPHICS

//  various standard headers
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

extern "C++" {
#include <OgreWindowEventUtilities.h>
#include <OgreMaterialManager.h>
#include <OgreMeshSerializer.h>
#include <OgreMeshManager.h>
#include "OIS.h"
}

#include "graphicstexture.h"
#include "graphics.h"
#include "graphicstexturelist.h"


Ogre::Root* mainogreroot = NULL;
Ogre::Camera* mainogrecamera = NULL;
Ogre::SceneManager* mainogrescenemanager = NULL;
Ogre::RenderWindow* mainogrewindow = NULL;
OIS::InputManager* maininput = NULL;
OIS::Mouse* mainogremouse = NULL;
OIS::Keyboard* mainogrekeyboard = NULL;

extern int graphicsactive;  // whether graphics are active/opened (1) or not (0)
int inbackground = 0;  // whether program has focus (1) or not (0)
int graphics3d = 0;  // 1: Ogre graphics, 0: SDL graphics
int mainwindowfullscreen; // whether fullscreen (1) or not (0)

int graphics_HaveValidWindow() {
    if (mainwindow) {
        return 1;
    }
    return 0;
}

// event receiver to be used by Ogre:
extern "C++" {
class EventReceiver : public Ogre::WindowEventListener, public OIS::KeyListener, public OIS::MouseListener {
public:
    EventReceiver(void) {
    }
    ~EventReceiver(void) {
    }
};
}
static EventReceiver* maineventreceiver = NULL;

void graphics_DestroyHWTexture(struct graphicstexture* gt) {

}

// initialize the video sub system, returns 1 on success or 0 on error:
static int graphics_InitVideoSubsystem(char** error) {
    return 1;
}

int graphics_Init(char** error, int use3dgraphics) {
    graphics3d = (use3dgraphics ? 1 : 0);

    if (graphics3d) {

    } else {

    }
    return 0;
}

int graphics_TextureToHW(struct graphicstexture* gt) {
    if (graphics3d) {
        if (gt->tex.ogretex || gt->threadingptr || !gt->name) {
            return 1;
        }
    }
    return 0;
}

void graphics_TextureFromHW(struct graphicstexture* gt) {
}


int graphics_GetWindowDimensions(unsigned int* width, unsigned int* height) {
    return 0;
}



void graphics_Close(int preservetextures) {
}

#ifdef ANDROID
void graphics_ReopenForAndroid() {
}
#endif

const char* graphics_GetWindowTitle() {
}

void graphics_Quit() {
    graphics_Close(0);
}

const char* graphics_GetCurrentRendererName() {
}

int* videomodesx = NULL;
int* videomodesy = NULL;

static void graphics_ReadVideoModes() {
    // free old video mode data
    if (videomodesx) {
        free(videomodesx);
        videomodesx = 0;
    }
    if (videomodesy) {
        free(videomodesy);
        videomodesy = 0;
    }

    // -> read video modes with ogre.
}

int graphics_GetNumberOfVideoModes() {
    char* error;
    if (!graphics_InitVideoSubsystem(&error)) {
        printwarning("Failed to initialise video subsystem: %s", error);
        if (error) {
            free(error);
        }
        return 0;
    }
    graphics_ReadVideoModes();
    int i = 0;
    while (videomodesx && videomodesx[i] > 0
            && videomodesy && videomodesy[i] > 0) {
        i++;
    }
    return i;
}

void graphics_GetVideoMode(int index, int* x, int* y) {
    graphics_ReadVideoModes();
    *x = videomodesx[index];
    *y = videomodesy[index];
}

void graphics_GetDesktopVideoMode(int* x, int* y) {
    char* error;
    *x = 0;
    *y = 0;
    if (!graphics_InitVideoSubsystem(&error)) {
        printwarning("Failed to initialise video subsystem: %s", error);
        if (error) {
            free(error);
        }
        return;
    }

    // -> find desktop mode using ogre

}

void graphics_MinimizeWindow() {
}

int graphics_IsFullscreen() {
}

void graphics_ToggleFullscreen() {
}

#ifdef WINDOWS
HWND graphics_GetWindowHWND() {
}
#endif

int graphics_SetMode(int width, int height, int fullscreen, int resizable, const char* title, const char* renderer, char** error) {

    return 1;
}

int lastfingerdownx,lastfingerdowny;

void graphics_CheckEvents(void (*quitevent)(void), void (*mousebuttonevent)(int button, int release, int x, int y), void (*mousemoveevent)(int x, int y), void (*keyboardevent)(const char* button, int release), void (*textevent)(const char* text), void (*putinbackground)(int background)) {

}

#endif  // USE_OGRE_GRAPHICS

} // extern "C"

