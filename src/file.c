
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

#include "os.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

#ifdef WINDOWS
#include <windows.h>
#include "shlwapi.h"
#else
#include <limits.h>
#include <sys/stat.h>
#endif

#include "file.h"

char *file_getCanonicalPath(const char *path) {
    char *s = file_getAbsolutePathFromRelativePath(path);
    if (!s) {
        return NULL;
    }
    file_makeSlashesNative(s);
    // remove double slashes
    file_removeDoubleSlashes(s);
    return s;
}

static char file_NativeSlash(void) {
#ifdef WINDOWS
        return '\\';
#else
        return '/';
#endif
}

static int file_IsDirectorySeparator(char c) {
#ifdef WINDOWS
    if (c == '/' || c == '\\') {
        return 1;
    }
#else
    if (file_NativeSlash() == c
    || c == '\\'
    // this is actually wrong for mac/linux,
    // but we want to support
    // paths written by thoughtless windows people aswell.
    ) {
        return 1;
    }
#endif
    return 0;
}

void file_makeSlashesNative(char *path) {
    unsigned int i = 0;
    char nativeslash = file_NativeSlash();
    while (i < strlen(path)) {
        if (file_IsDirectorySeparator(path[i])) {
            path[i] = nativeslash;
        }
        i++;
    }
}

void file_makeSlashesCrossplatform(char *path) {
    unsigned int i = 0;
    while (i < strlen(path)) {
        if (file_IsDirectorySeparator(path[i])) {
            path[i] = '/';
        }
        i++;
    }
}

size_t file_GetSize(const char *name) {
#ifdef UNIX
    // use stat to get file size:
    struct stat info;
    int r = stat(name, &info);
    if (r < 0) {
        return 0;
    }
    return info.st_size;
#else
#ifdef WINDOWS
    // use GetFileAttributesEx for file size:
    // query file attributes for file:
    WIN32_FILE_ATTRIBUTE_DATA fad;
    if (!GetFileAttributesEx(name, GetFileExInfoStandard, &fad)) {
        return 0;
    }

    // combine lower and higher order bytes to large int:
    LARGE_INTEGER size;
    size.HighPart = fad.nFileSizeHigh;
    size.LowPart = fad.nFileSizeLow;
    return size.QuadPart;
#else
    // use standard libc functions for file size:
    // open file
    FILE* r = fopen(name, "rb");
    if (!r) {
        return 0;
    }

    // obtain file size by seeking to end
    fseek(r, 0L, SEEK_END);
    size_t size = ftell(r);

    // close file again
    fclose(r);

    return size;
#endif
#endif
}

int file_Cwd(const char *path) {
    while (path[0] == '.' && strlen(path) > 1 && file_IsDirectorySeparator(path[1])) {
        path += 2;
    }
    if (strcasecmp(path, "") == 0 || strcasecmp(path, ".") == 0) {
        return 1;
    }
    char *pathcopy = strdup(path);
    if (!pathcopy) {
        return 0;
    }
    file_makeSlashesNative(pathcopy);
#ifdef WINDOWS
    if (SetCurrentDirectory(pathcopy) == 0) {
        free(pathcopy);
        return 0;
    }
#else
    if (chdir(pathcopy) != 0) {
        free(pathcopy);
        return 0;
    }
#endif
    free(pathcopy);
    return 1;
}

char *file_getCwd() {
    char cwdbuf[2048] = "";
#ifdef WINDOWS
    if (GetCurrentDirectory(sizeof(cwdbuf), cwdbuf) <= 0) {
        return 0;
    }
    // turn all paths like C:\blubb\ into C:/blubb/ (blitwizard-style paths)
    unsigned int i = 0;
    while (i <= strlen(cwdbuf)) {
        if (cwdbuf[i] == '\\') {
            cwdbuf[i] = '/';
        }
        i++;
    }
#else
    if (!getcwd(cwdbuf, sizeof(cwdbuf))) {
        return 0;
    }
#endif
    return strdup(cwdbuf);
}

int file_IsDirectory(const char *path) {
#ifdef UNIX
    struct stat info;
    int r = stat(path, &info);
    if (r < 0) {
        return 0;
    }
    if (S_ISDIR(info.st_mode) != 0) {
        return 1;
    }
    return 0;
#else
#ifdef WINDOWS
    if (PathIsDirectory(path) != FALSE) {
        return 1;
    }
    return 0;
#else
#error "No file_IsDirectory implementation available"
#endif
#endif
}

int file_DoesFileExist(const char *path) {
#ifndef WINDOWS
        struct stat st;
        if (stat(path,&st) == 0) {
            return 1;
        }
        return 0;
#else
        DWORD fileAttr;
        fileAttr = GetFileAttributes(path);
        if (0xFFFFFFFF == fileAttr) {
            return 0;
        }
        return 1;
#endif
}


static int file_LatestSlash(const char *path) {
    int i = strlen(path)-1;
    while (!file_IsDirectorySeparator(path[i]) && i > 0) {
        i--;
    }
    if (i <= 0) {
        i = -1;
    }
    return i;
}

static void file_CutOffOneElement(char *path) {
    while (1) {
        int i = file_LatestSlash(path);
        // check if there is nothing left to cut off for absolute paths
#ifdef WINDOWS
        if (i == 2 && path[1] == ':' && (tolower(path[0]) >= 'a' && tolower(path[0]) <= 'z')) {
            return;
        }
#else
        if (i == 0) {
            return;
        }
#endif
        // see what we can cut off
        if (i < 0) {
            // just one relative item left -> empty to current dir ""
            path[0] = 0;
            return;
        } else {
            if (i == (int)strlen(path)-1) {
                // slash is at the end (directory path).
                path[i] = 0;
                if (strlen(path) > 0) {
                    if (path[strlen(path)-1] == '.') {
                        // was this a trailing ./ or ../?
                        if (strlen(path) > 1) {
                            if (path[strlen(path)-2] == '.') {
                                // this is ../, so we're done when the .. is gone
                                path[strlen(path)-2] = 0;
                                return;
                            }
                        }
                        // it is just ./ so we need to carry on after removing it
                        path[strlen(path)-1] = 0;
                    }
                }
            } else {
                path[i+1] = 0;
                return;
            }
        }
    }
}

char *file_AddComponentToPath(const char *path, const char *component) {
    int addslash = 0;
    if (strlen(path) > 0) {
        if (!file_IsDirectorySeparator(path[strlen(path)-1])) {
            addslash = 1;
        }
    }
    char *newpath = malloc(strlen(path)+addslash+1+strlen(component));
    if (!newpath) {
        return NULL;
    }
    memcpy(newpath, path, strlen(path));
    if (addslash) {
        newpath[strlen(path)] = file_NativeSlash();
    }
    memcpy(newpath + strlen(path) + addslash, component, strlen(component));
    newpath[strlen(path) + addslash + strlen(component)] = 0;
    return newpath;
}

void file_StripComponentFromPath(char *path) {
    file_makeSlashesNative(path);

    int repeat = 1;
    while (repeat) {
        repeat = 0;
        if (strlen(path) > 0) {
            unsigned int slashpos = file_LatestSlash(path);
            if (slashpos > 0) {
                if (slashpos >= strlen(path)-1) {
                    repeat = 1;
                }
                path[slashpos] = 0;
            } else {
                path[0] = 0;
            }
        }
    }
}

char *file_getAbsolutePathFromRelativePath(const char *path) {
    // cancel for absolute paths
    if (!file_IsPathRelative(path)) {
        return strdup(path);
    }

    // cut off unrequired clutter
    while (path[0] == '.' && path[1] == file_NativeSlash()) {
        path += 2;
    }

    // get current working directory
    char *currentdir = file_getCwd();
    if (!currentdir) {
        return NULL;
    }

    // process ../
    while (path[0] == '.' && path[1] == '.' && path[2] == file_NativeSlash()) {
        file_CutOffOneElement(currentdir);
        path += 3;
    }

    // allocate space for new path
    char *newdir = malloc(strlen(currentdir) + 1 + strlen(path) + 1);
    if (!newdir) {
        free(currentdir);
        return NULL;
    }

    // assemble new path
    memcpy(newdir, currentdir, strlen(currentdir));
    char slash = file_NativeSlash();
    newdir[strlen(currentdir)] = slash;
    memcpy(newdir + strlen(currentdir) + 1, path, strlen(path));
    newdir[strlen(currentdir) + 1 + strlen(path)] = 0;

    free(currentdir);
    return newdir;
}

char *file_GetDirectoryPathFromFilePath(const char *path) {
    if (file_IsDirectory(path)) {
        return strdup(path);
    } else {
        char *pathcopy = strdup(path);
        if (!pathcopy) {
            return NULL;
        }
        int i = file_LatestSlash(path);
        if (i < 0) {
            free(pathcopy);
            return strdup("");
        } else {
            pathcopy[i+1] = 0;
            return pathcopy;
        }
    }
}

int file_IsPathRelative(const char *path) {
#ifdef WINDOWS
    if (PathIsRelative(path) == TRUE) {
        return 1;
    }
    return 0;
#else
    if (file_IsDirectorySeparator(path[0])) {
        return 0;
    }
    return 1;
#endif
}

char *file_getAbsoluteDirectoryPathFromFilePath(const char *path) {
    char *p = file_GetDirectoryPathFromFilePath(path);
    if (!p) {
        return NULL;
    }

    if (!file_IsPathRelative(p)) {
        return p;
    }

    char *p2 = file_getAbsolutePathFromRelativePath(p);
    free(p);
    if (!p2) {
        return NULL;
    }
    return p2;
}

char *file_GetFileNameFromFilePath(const char *path) {
    int i = file_LatestSlash(path);
    if (i < 0) {
        return strdup(path);
    } else {
        char *filename = malloc(strlen(path)-i+1);
        if (!filename) {
            return NULL;
        }
        memcpy(filename, path + i + 1, strlen(path)-i);
        filename[strlen(path)-i] = 0;
        return filename;
    }
}

int file_ContentToBuffer(const char *path, char **buf, size_t *buflen) {
    FILE* r = fopen(path, "rb");
    if (!r) {
        return 0;
    }
    // obtain file size
    fseek(r, 0L, SEEK_END);
    long long pos = ftell(r);
    if (pos < 0) {
        fclose(r);
        return 0;
    }
    uint64_t size = pos;
    fseek(r, 0L, SEEK_SET);
    // allocate buf
    char *fbuf = malloc(size + 1);
    if (!fbuf) {
        fclose(r);
        return 0;
    }
    // read data
    int i = 0;
    if (size > 0) {
        i = fread(fbuf, 1, size, r);
    }
    fclose(r);
    // check data
    if (i != (int)size) {
        free(fbuf);
        return 0;
    }
    *buflen = size;
    fbuf[size] = 0;
    *buf = fbuf;
    return 1;
}

char *file_GetTempPath(const char *name) {
#ifdef WINDOWS
    return NULL;
#else
    size_t namelen = 0;
    if (name) {
        namelen = strlen(name);
    }

    char *tmppath = malloc(strlen("/tmp/") + 1 + namelen);
    if (!tmppath) {
        return NULL;
    }

    memcpy(tmppath, "/tmp/", strlen("/tmp/"));
    if (namelen > 0) {
        memcpy(tmppath + strlen("/tmp/"), name, namelen);
    }
    tmppath[strlen("/tmp/") + namelen] = 0;
    return tmppath;
#endif
}

#ifdef WINDOWS
#include <shlobj.h>
char *file_GetUserFileDir(void) {
    char programsdirbuf[MAX_PATH+1];
    SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, 0, programsdirbuf);
    programsdirbuf[MAX_PATH] = 0;
    return strdup(programsdirbuf);
}
#else
char *filesystem_GetUserFileDir(void) {
    char programsdirbuf[300];
    strncpy(programsdirbuf, getenv("HOME"), 299);
    programsdirbuf[299] = 0;
    return strdup(programsdirbuf);
}
#endif

int file_CreateDirectory(const char *name) {
    if (file_DoesFileExist(name)) {
        // file or directory already present, we cannot create it!
        return 0;
    }
#ifdef UNIX
    return (mkdir(name, 0755) == 0);
#else
#ifdef WINDOWS
    return (CreateDirectory(name, NULL) != 0);
#else
#error "No file_CreateDirectory implementation present"
#endif
#endif
}

int file_DeleteFile(const char *name) {
#ifdef UNIX
    return (unlink(name) == 0);
#else
#ifdef WINDOWS
    return (DeleteFile(name) != 0);
#else
#error "No file_DeleteFile implementation present"
#endif
#endif
}

void file_removeDoubleSlashes(char *path) {
    unsigned int i = 0;
    int previousslash = 0;
    unsigned int len = strlen(path);
    while (i < len) {
        if (file_IsDirectorySeparator(path[i])) {
            if (previousslash) {
                memmove(path+i, path+i+1, len-i);
                len--;
                continue;
            }
            previousslash = 1;
        } else {
            previousslash = 0;
        }
        i++;
    }
}

unsigned int file_CountPathComponents(const char *path) {
    unsigned int c = 0;
    unsigned int i = 0;
    int previousslash = 0;
    int absolutepathslash = 0;
    while (i < strlen(path)) {
        if (file_IsDirectorySeparator(path[i])) {
            if (!previousslash) {
#ifdef WINDOWS
                // don't increment for first absolute path slash
                if (c > 0 || (i > 0 && path[i-1] != ':')) {
                    c++;
                } else {
                    absolutepathslash = 1;
                }
#else
                if (i == 0) {
                    absolutepathslash = 1;
                } else {
                    c++;
                }
#endif
            }
            previousslash = 1;
        } else {
            previousslash = 0;
        }
        i++;
    }
    if (!previousslash) {
        // add folder at the very end:
        if (c > 1 || absolutepathslash) {
            c++;
        }
    }
    return c;
}

void file_makePathRelative(char *path, const char *base) {
    if (file_IsPathRelative(base)) {
        return;
    }
    if (file_IsPathRelative(path)) {
        return;
    }
    char *path_unified = strdup(path);
    char *base_unified = strdup(base);
    if (!path_unified || !base_unified) {
        free(path_unified);
        free(base_unified);
        return;
    }
    file_removeDoubleSlashes(path_unified);
    file_makeSlashesCrossplatform(path_unified);
    file_removeDoubleSlashes(base_unified);
    file_makeSlashesCrossplatform(base_unified);
    if (strlen(path_unified) < strlen(base_unified)) {
        // not a sub dir.
        free(path_unified);
        free(base_unified);
        return;
    }
    if (memcmp(path_unified, base_unified, strlen(base_unified)) == 0) {
        // path is indeed a sub-path of our base
        // see how many components we need to strip:
        unsigned int c = file_CountPathComponents(base_unified);
        c++; // drive letter
        // strip the components from the beginning of the path:
        while (c > 0) {
            // check for next directory separator
            char *slashptr1 = strstr(path, "/");
            int slashpos1 = -1;
            if (slashptr1) {
                slashpos1 = slashptr1 - path;
            }
            char *slashptr2 = NULL;
#ifdef WINDOWS
            slashptr2 = strstr(path, "\\");
#endif
            int slashpos2 = -1;
            if (slashptr2) {
                slashpos2 = slashptr2 - path;
            }
            // see which one is closest:
            int nextslash = slashpos1;
            if (slashpos2 >= 0 && (slashpos2 < nextslash || nextslash < 0)) {
                nextslash = slashpos2;
            }
            // strip off:
            if (nextslash >= 0) {
                memmove(path, path+nextslash+1,
                strlen(path)-(nextslash));
            } else {
                break;
            }
            c--;
        }
        // done! path should now be relative to base.
        free(path_unified);
        free(base_unified);
    } else {
        // path is not a sub-path of base, do nothing:
        free(base_unified);
        free(path_unified);
    }
}

