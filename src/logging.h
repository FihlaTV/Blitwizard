
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

#ifndef BLITWIZARD_LOGGING_H_
#define BLITWIZARD_LOGGING_H_

#ifdef __cplusplus
extern "C" {
#endif

extern volatile int crashed;
extern char *memorylogbuf;
void printerror_internal(const char *file, int line, const char *fmt, ...);
void printfatalerror_internal(const char *file, int line, const char *fmt,
    ...);
void printwarning_internal(const char *file, int line, const char *fmt, ...);
void printinfo(const char *fmt, ...);

#define printwithline(func, fmt, ...) print##func##_internal(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
//#define printwithline(func, fmt, ...) print##func##_internal("(" __FILE__ ":%d)", __LINE__, ##__VA_ARGS__)
#define printwarning(fmt, ...) printwithline(warning, fmt, ##__VA_ARGS__)
#define printerror(fmt, ...) printwithline(error, fmt, ##__VA_ARGS__)
#define printfatalerror(fmt, ...) printwithline(fatalerror, fmt, ##__VA_ARGS__)

// once the Lua state is ready, enable blitwizard.onLog:
void doConsoleLog(void);

#ifdef __cplusplus
}
#endif

#endif  // BLITWIZARD_LOGGING_H_


