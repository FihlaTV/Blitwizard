
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

#ifndef BLITWIZARD_FILE_H_
#define BLITWIZARD_FILE_H_

#include <stdlib.h>

int file_Cwd(const char* path);

char* file_GetCwd(void);

char* file_getCanonicalPath(const char* path);

char* file_getAbsolutePathFromRelativePath(const char* path);

int file_DoesFileExist(const char* path);

int file_IsDirectory(const char* path);

int file_IsPathRelative(const char* path);

char* file_GetDirectoryPathFromFilePath(const char* path);

char* file_getAbsoluteDirectoryPathFromFilePath(const char* path);

char* file_GetFileNameFromFilePath(const char* path);

int file_ContentToBuffer(const char* path, char** buf, size_t* buflen);

char* file_AddComponentToPath(const char* path, const char* component);

unsigned int file_CountPathComponents(const char* path);

void file_StripComponentFromPath(char* path);

void file_makeSlashesNative(char* path);

void file_makeSlashesCrossplatform(char* path);

char* file_GetUserFileDir(void);

char* file_GetTempPath(const char* name);

size_t file_GetSize(const char* name);

int file_CreateDirectory(const char* name);

int file_DeleteFile(const char* name);

void file_makePathRelative(char* path, const char* base);

void file_removeDoubleSlashes(char* path);

#endif  // BLITWIZARD_FILE_H_

