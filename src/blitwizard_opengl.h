
/* blitwizard game engine - source code file

  Copyright (C) 2014 Jonas Thiem

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

#ifndef BLITWIZARD_OPENGL_H_
#define BLITWIZARD_OPENGL_H_

// include basic compile time options:
#include "config.h"
#include "os.h"

// check if we need OpenGL at all:
#undef NEED_OPENGL
#if (defined(USE_SDL_GRAPHICS_OPENGL_EFFECTS))
#define NEED_OPENGL
#endif

// if we need OpenGL, include it now:
#ifdef NEED_OPENGL
#undef GL_GLEXT_PROTOTYPES
#if (defined(WINDOWS) || defined(MAC))
// windows, mac: get regular gl.h + glext.h
#include <GL/gl.h>
#include "src/wglheaders/glext.h"
#else
// linux/other X11 systems: get glx.h
#include <GL/glu.h>
#include <GL/glext.h>
#include <GL/glx.h>
#include <GL/glxext.h>
#endif
#endif

// if using SDL graphics, get the SDL OpenGL header as well:
#if (defined(USE_SDL_GRAPHICS) && defined(NEED_OPENGL))
#include <SDL2/SDL_opengl.h>
#endif

// finally, define the glext stuff:
#ifdef NEED_OPENGL
#include "graphicssdlglext.h"
#endif

// on platforms with GLU, get OpenGL error messages with that
#ifdef LINUX
#define glGetErrorString gluErrorString
#else
#ifdef NEED_OPENGL
__attribute__((unused)) static const char* glGetErrorString(
        __attribute__((unused)) GLenum error) {
    return "no OpenGL error string support for this platform provided "
        "(by blitwizard)";
}
#endif
#endif

#endif  // BLITWIZARD_OPENGL_H_

