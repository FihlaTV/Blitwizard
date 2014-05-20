
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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "os.h"

#include "audiosource.h"
#include "audiosourceresourcefile.h"

#ifdef USE_PHYSFS

struct audiosourceresourcefile_internaldata {
    struct zipfilereader* file;
    int eof;
    struct zipfile* archive;
    char* path;
};

static void audiosourceresourcefile_rewind(struct audiosource *source) {
    struct audiosourceresourcefile_internaldata *idata = source->internaldata;
    idata->eof = 0;
    if (idata->file) {
        // Close the file. it will be reopened on next read
        zipfile_FileClose(idata->file);
        idata->file = NULL;
    }
}


static int audiosourceresourcefile_read(
        struct audiosource *source, char* buffer, unsigned int bytes) {
    struct audiosourceresourcefile_internaldata *idata = source->internaldata;

    if (idata->file == NULL) {
        if (idata->eof) {
            return -1;
        }
        idata->file = zipfile_FileOpen(idata->archive, idata->path);
        if (!idata->file) {
            idata->file = NULL;
            return -1;
        }
    }

    int bytesread = zipfile_FileRead(idata->file, buffer, bytes);
    if (bytesread > 0) {
        return bytesread;
    } else {
        zipfile_FileClose(idata->file);
        idata->file = NULL;
        idata->eof = 1;
        if (bytesread < 0) {
            return -1;
        }
        return 0;
    }
}


static int audiosourceresourcefile_seek(
        struct audiosource *source, size_t pos) {
    struct audiosourceresourcefile_internaldata *idata = source->internaldata;

    // don't allow seeking beyond end of file:
    size_t length = source->length(source);
    if (pos > length) {
        pos = length;
    }

    // seek:
    if (!zipfile_FileSeek(idata->file, pos)) {
        return 0;
    }

    // update our own eof marker:
    if (pos < length) {
        idata->eof = 0;
    } else {
        idata->eof = 1;
    }
    return 1;
}

static size_t audiosourceresourcefile_length(struct audiosource *source) {
    struct audiosourceresourcefile_internaldata *idata = source->internaldata;

    return zipfile_FileGetLength(idata->archive, idata->path);
}

static size_t audiosourceresourcefile_position(struct audiosource *source) {
    struct audiosourceresourcefile_internaldata *idata = source->internaldata;
    if (idata->eof) {
        return source->length(source);
    }
    size_t pos = zipfile_FileTell(idata->file);
    return pos;
}

static void audiosourceresourcefile_close(struct audiosource *source) {
    struct audiosourceresourcefile_internaldata *idata = source->internaldata;
    if (idata) {
        // close file we might have opened
        if (idata->file) {
            zipfile_FileClose(idata->file);
        }

        // free all structs & strings
        if (idata->path) {
            free(idata->path);
        }
        free(idata);
    }
    free(source);
}

struct audiosource *audiosourceresourcefile_create(
        struct zipfile *archive, const char* path) {
    struct audiosource *a = malloc(sizeof(*a));
    if (!a) {
        return NULL;
    }

    memset(a, 0, sizeof(*a));
    a->internaldata = malloc(sizeof(
        struct audiosourceresourcefile_internaldata));
    if (!a->internaldata) {
        free(a);
        return NULL;
    }

    struct audiosourceresourcefile_internaldata *idata = a->internaldata;
    memset(idata, 0, sizeof(*idata));
    idata->path = strdup(path);
    if (!idata->path) {
        free(a->internaldata);
        free(a);
        return NULL;
    }
    idata->archive = archive;

    a->read = &audiosourceresourcefile_read;
    a->close = &audiosourceresourcefile_close;
    a->rewind = &audiosourceresourcefile_rewind;
    a->seek = &audiosourceresourcefile_seek;
    a->length = &audiosourceresourcefile_length;
    a->position = &audiosourceresourcefile_position;
    a->seekable = 1;

    return a;
}

#endif  // USE_PHYSFS

