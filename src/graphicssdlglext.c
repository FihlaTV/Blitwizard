
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

#include "os.h"
#include "graphicssdlglext.h"

#ifdef NEED_OPENGL

#if !defined(MAC)
PFNGLGENBUFFERSARBPROC glGenBuffers = NULL;
PFNGLBUFFERDATAARBPROC glBufferData = NULL;
PFNGLBINDBUFFERARBPROC glBindBuffer = NULL;
PFNGLDELETEBUFFERSARBPROC glDeleteBuffers = NULL;
#ifdef WINDOWS
PFNGLACTIVETEXTUREPROC glActiveTexture = NULL;
#endif
#endif

#ifdef WINDOWS
#define glGetProcAddress wglGetProcAddress
typedef const unsigned char* glPStr;
#else
#ifndef MAC
#define glGetProcAddress glXGetProcAddress
typedef const unsigned char* glPStr;
#else
// nothing to load on Mac OS X
#endif
#endif

static int glextinit = 0;

int graphicssdlglext_init(int forcereload) {
    if (glextinit && !forcereload) {
        return 1;
    }
    glextinit = 1;
#ifndef MAC
    // PBO functions:
    glGenBuffers = (PFNGLGENBUFFERSARBPROC)glGetProcAddress(
        (glPStr)"glGenBuffers");
    if (!glGenBuffers) {
        return 0;
    }
    glBufferData = (PFNGLBUFFERDATAARBPROC)glGetProcAddress(
        (glPStr)"glBufferData");
    if (!glBufferData) {
        return 0;
    }
    glBindBuffer = (PFNGLBINDBUFFERARBPROC)glGetProcAddress(
        (glPStr)"glBindBuffer");
    if (!glBindBuffer) {
        return 0;
    }
    glDeleteBuffers = (PFNGLDELETEBUFFERSARBPROC)glGetProcAddress(
        (glPStr)"glDeleteBuffers");
    if (!glDeleteBuffers) {
        return 0;
    }
#ifdef WINDOWS
    glActiveTexture = (PFNGLACTIVETEXTUREPROC)glGetProcAddress(
        "glActiveTexture");
    if (!glActiveTexture) {
        return 0;
    }
#endif
#endif
    return 1;
}

#endif

