
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
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef WINDOWS
#include <windows.h>
#endif
#ifdef UNIX
#ifndef MAC
#include <sys/file.h>
#endif
#include <sys/types.h>
#endif

#include "threading.h"
#include "diskcache.h"
#include "file.h"

#define RANDOM_FOLDER_CHARS 8
#define RANDOM_FILE_CHARS 10

static mutex *cachemutex = NULL;
static char *cachefolder = NULL;

static void diskcache_randomLetters(char *buffer, size_t len) {
    unsigned int i = 0;
    while (i < len) {
#ifdef WINDOWS
        double d = (double)((double)rand())/RAND_MAX;
#else
        double d = drand48();
#endif
        int c = (int)((double)d*9.99);
        buffer[i] = '0' + c;
        i++;
    }
}

static char *diskcache_generateCacheFolderPath(void) {
    // generate folder name:
    const char basename[] = "blitwizardcache";
    char folderbuf[64];
    memcpy(folderbuf, basename, strlen(basename));
    diskcache_randomLetters(folderbuf + strlen(basename),
        RANDOM_FOLDER_CHARS);
    folderbuf[strlen(basename) + RANDOM_FOLDER_CHARS] = 0;

    // return temporary folder item path with this folder name:
    return file_GetTempPath(folderbuf);
}

__attribute__((constructor)) static void diskcache_init(void) {
#ifdef UNIX
    // initialise drand48():
    srand48(time(NULL)+(int)getpid());
#else
    // initialise rand():
#ifdef WINDOWS
    srand(time(NULL)+GetCurrentProcessId());
#else
    srand(time(NULL));
#endif
#endif

    // create mutex and lock it instantly:
    cachemutex = mutex_create();
    mutex_lock(cachemutex);

    // get folder path & create folder:
    cachefolder = diskcache_generateCacheFolderPath();
    if (!cachefolder) {
        // oops, not good
        return;
    }
    if (!file_CreateDirectory(cachefolder)) {
        // we failed to create the cache dir.
        free(cachefolder);
        cachefolder = NULL;
        return;
    }

    // release mutex again:
    mutex_release(cachemutex);
}

struct diskcache_StoreThreadInfo {
    char *resourcepath;
    char *data;
    size_t datalength;
};

static void diskcache_closeLockedFile(FILE *f) {
#ifdef UNIX
#ifndef MAC
    // for non-Mac OS X Unix (Linux/BSD), we use flock
    flock(fileno(f), LOCK_UN);
#else
    // for Mac OS X, we currently use nothing!
#endif
#else
    // on windows, we use LockFileEx
    HANDLE h = (HANDLE)_get_osfhandle(fileno(f));
    UnlockFile(h, 0, 0, 0, 0);
    _close((int)h);
#endif
    fclose(f);
}

static FILE *diskcache_openLockedFile(const char *path, int write) {
    FILE *f;
    if (!file_doesFileExist(path)) {
        // if we don't want to write, this means failure.
        if (!write) {
            return 0;
        }

        // create file.
        f = fopen(path, "wb");
        if (!f) {
            return 0;
        }
    } else {
        if (write) {
            // open up file in append mode for writing
            f = fopen(path, "ab");
        } else {
            // .. otherwise in read mode
            f = fopen(path, "rb");
        }
        if (!f) {
            return 0;
        }
    }
#ifdef UNIX
#ifndef MAC
    // for non-Mac OS X Unix (Linux/BSD), we use flock
    int operation = LOCK_SH;
    if (write) {
        operation = LOCK_EX;
    }
    if (flock(fileno(f), operation) == 0) {
        return f;
    }
    fclose(f);
    return 0;
#else
    // for Mac OS X, we currently use nothing!
    return f;
#endif
#else
    // on windows, we use LockFileEx
    HANDLE h = (HANDLE)_get_osfhandle(fileno(f));
    int flags = 0;
    if (write) {
        flags |= LOCKFILE_EXCLUSIVE_LOCK;
    }
    OVERLAPPED ov;
    memset(&ov, 0, sizeof(ov));
    if (LockFileEx(h, flags, 0, 0, 0, &ov)) {
        _close((int)h);
        return f;
    }
    _close((int)h);
    fclose(f);
    return 0;
#endif
}

static void diskcache_storeThread(void *userdata) {
    // this thread simply writes the given data to disk.

    // retrieve data & path to write to:
    struct diskcache_StoreThreadInfo *sti =
    (struct diskcache_StoreThreadInfo*)userdata;

    // open file for writing with proper file locking:
    FILE *f = diskcache_openLockedFile(sti->resourcepath, 1);
    if (!f) {
        free(sti->resourcepath);
        free(sti->data);    
        free(sti);
        return;
    }

    // write data:
    fwrite(sti->data, 1, sti->datalength, f);

    // close file and free resources:
    diskcache_closeLockedFile(f);
    free(sti->resourcepath);
    free(sti->data);
    free(sti);
}

char *diskcache_store(char *data, size_t datalength) {
    // no disk cache directory, no disk cache storing:
    if (!cachefolder) {
        return NULL;
    }

    // we don't store empty data:
    if (!data || datalength == 0) {
        return NULL;
    }

    // create a copy of the data so we can operate in another thread:
    char *newdata = malloc(datalength);
    if (!newdata) {
        return NULL;
    }
    memcpy(newdata, data, datalength);

    // generate a resource path:
    char resourcefilename[RANDOM_FILE_CHARS + 1];
    diskcache_randomLetters(resourcefilename, RANDOM_FILE_CHARS);
    resourcefilename[RANDOM_FILE_CHARS] = 0;
    char *resourcepath = file_AddComponentToPath(cachefolder, resourcefilename);
    if (!resourcepath) {
        free(newdata);
        return NULL;
    }

    // prepare thread info struct:
    struct diskcache_StoreThreadInfo *sti = malloc(sizeof(*sti));
    if (!sti) {
        free(resourcepath);
        free(newdata);
        return NULL;
    }
    memset(sti, 0, sizeof(*sti));
    sti->resourcepath = strdup(resourcepath);
    if (!sti->resourcepath) {
        free(resourcepath);
        free(sti);
        free(newdata);
        return NULL;
    }
    sti->data = newdata;
    sti->datalength = datalength;

    // spawn store thread and return resource path:
    threadinfo *ti = thread_createInfo();
    if (!ti) {
        free(sti->resourcepath);
        free(sti);
        free(resourcepath);
        free(newdata);
        return NULL;
    }
    thread_spawnWithPriority(ti, 0, diskcache_storeThread, sti);
    thread_freeInfo(ti);
    return resourcepath;
}

struct diskcache_RetrieveThreadInfo {
    char *resourcepath;
    void (*callback)(void *data, size_t datalength, void *userdata);
    void *userdata;
};

static void diskcache_retrieveThread(void *userdata) {
    struct diskcache_RetrieveThreadInfo *rti = userdata;

    // peek on file size:
    size_t size = file_getSize(rti->resourcepath);

    // open file for reading with proper file locking:
    FILE *f = diskcache_openLockedFile(rti->resourcepath, 0);
    if (!f) {
        // we couldn't open the file.
        rti->callback(NULL, 0, rti->userdata);
        free(rti->resourcepath);
        free(rti);
        return;
    }

    // see if file size has changed until we got that lock:
    size_t newsize = file_getSize(rti->resourcepath);
    if (newsize < size) {
        // we don't like shrinking files.
        // is it just being deleted?
        diskcache_closeLockedFile(f);
        rti->callback(NULL, 0, rti->userdata);
        free(rti->resourcepath);
        free(rti);
        return;
    }
    size = newsize;

    // allocate data buffer:
    char *data = malloc(size);
    if (!data) {
        // no point in continuing.
        diskcache_closeLockedFile(f);
        rti->callback(NULL, 0, rti->userdata);
        free(rti->resourcepath);
        free(rti);
        return;
    } 

    // read data:
    if (fread(data, 1, size, f) != size) {
        // failed to read all (any?) data.
        diskcache_closeLockedFile(f);
        rti->callback(NULL, 0, rti->userdata);
        free(rti->resourcepath);
        free(rti);
        free(data);
        return;
    }

    // close file:
    diskcache_closeLockedFile(f);

    // issue callback:
    rti->callback(data, size, rti->userdata);

    // free resources:
    free(rti->resourcepath);
    free(rti);
}

void diskcache_retrieve(const char *path, void (*callback)(void *data,
        size_t datalength, void *userdata), void *userdata) {
    // rule out some obvious error conditions:
    if (!callback) {
        return;
    }
    if (!path) {
        callback(NULL, 0, userdata);
        return;
    }
    if (!cachefolder) {
        callback(NULL, 0, userdata);
        return;
    }

    // prepare info struct:
    struct diskcache_RetrieveThreadInfo *rti = malloc(sizeof(*rti));
    if (!rti) {
        callback(NULL, 0, userdata);
        return;
    }
    memset(rti, 0, sizeof(*rti));
    rti->resourcepath = strdup(path);
    if (!rti->resourcepath) {
        free(rti);
        callback(NULL, 0, userdata);
        return;
    }
    rti->callback = callback;
    rti->userdata = userdata;

    // spawn retrieving thread:
    threadinfo *ti = thread_createInfo();
    if (!ti) {
        free(rti->resourcepath);
        free(rti);
        callback(NULL, 0, userdata);
        return;
    }
    thread_spawnWithPriority(ti, 0, diskcache_retrieveThread, rti);
    thread_freeInfo(ti);
    return;
}

void diskcache_delete(const char *path) {
    if (!file_deleteFile(path) && file_doesFileExist(path)) {
        int attempts = 0;
        while (attempts < 10) {
            // attempt to open file with an exclusive lock, then retry:
            FILE *f = diskcache_openLockedFile(path, 0);
            if (f) {
                diskcache_closeLockedFile(f);
                if (file_deleteFile(path)) {
                    break;
                }
                // if file is not deleted, keep retrying
                attempts++;
            }
        }
        // we are unable to delete it. sadface :(
    }
}

