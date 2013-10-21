
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

#ifndef BLITWIZARD_OS_H_

#define BLITWIZARD_OS_H_

#define USE_SOCKETS

// Detect operating system:

// We don't want Cygwin:
#if defined(__CYGWIN__) || defined(CYGWIN32) || defined(_CYGWIN32_) || defined(_CYGWIN64_)
#error "You should compile blitwizard natively for Windows, not using Cygwin."
#endif

// Android:
#if defined(__ANDROID__) && !defined(ANDROID)
#define ANDROID
#undef USE_SOCKETS
#endif

// Linux:
#if defined(linux__) || defined(__linux) || defined(__linux__) || defined(linux)
#define LINUX
#endif

// Mac:
#if defined(__APPLE__) && defined(__MACH__)
#define MAC
#endif

// Windows:
#if defined(WIN32) || defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) || defined(_MSC_VER)
#define WINDOWS
#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#define _WIN32_WINNT 0x0501
#if defined __MINGW_H
#define _WIN32_IE 0x0400
#endif
#endif

// Set SDLRW for Android:
#ifdef ANDROID
#define SDLRW
#endif

// Debugging the Android/SDL_RWops approach on Linux:
// #define SDLRW

// Force S16 audio on Android:
#ifdef ANDROID
#define FORCES16AUDIO
#endif

// We want to know if we have Unix for sockets and such:
#ifndef WINDOWS
#define UNIX
#endif

// See if we have SDL:
#if (defined(USE_SDL_AUDIO) || defined(USE_SDL_GRAPHICS))
#define HAVE_SDL
#endif

// Detect 32/64bit compilation (doesn't necessarily apply to system's 32/64bit):
#ifdef WINDOWS
#if defined(_WIN64)
#define _64BIT
#endif
#else
// gcc check:
#if (defined(_LP64) || defined(__LP64__))
#define _64BIT
#endif
#endif

// Use either SDL or OGRE graphics:
#ifdef USE_OGRE_GRAPHICS
#undef USE_SDL_GRAPHICS
#define HAVE_3D_GRAPHICS
#endif

#if defined(USE_OGRE_GRAPHICS) || defined(USE_SDL_GRAPHICS)
#define HAVE_2D_GRAPHICS
#endif

#ifdef USE_GRAPHICS
#define HAVE_GRAPHICS
#ifndef HAVE_2D_GRAPHICS
#define USE_NULL_GRAPHICS
#endif
#endif

#endif  // BLITWIZARD_OS_H_

