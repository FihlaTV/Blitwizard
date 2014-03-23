
/* blitwizard game engine - source code file

  Copyright (C) 2013-2014 Jonas Thiem

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
#include <stdlib.h>
#include <stdio.h>

#include "config.h"
#include "os.h"

#ifdef USE_PHYSFS

#include "main.h"
#include "file.h"
#include "physfs.h"
#include "zipfile.h"
#include "zipdecryption.h"


#define MOUNT_POINT_LEN 20

struct zipfile {
    // physio handle:
    struct PHYSFS_Io* physio;

    // PhysIO random mount point for our archive:
    char mountpoint[MOUNT_POINT_LEN+1];

    // zip decrypter:
    struct zipdecrypter* zipdecrypter;
};

struct zipfileaccess {
    // archive disk file info:
    FILE* f;  // file handle
    char* fname;  // file name, required for duplication
    size_t offsetinfile;
    size_t sizeinfile;
    size_t posinfile;
};

struct PHYSFS_Io* physio = NULL;

__attribute__((constructor)) void initialisezip(void) {
    // initialize physfs and just pass some random stuff for binary name:
    PHYSFS_init("./ourrandomnonexistingbinaryname");
}

static PHYSFS_sint64 zipfile_ReadPhysFS(struct PHYSFS_Io *io, void *buf,
PHYSFS_uint64 len) {
    // get our zip file info:
    struct zipfile* zf = (struct zipfile*)(io->opaque);

    // read from our zip decryption source:
    if (len == 0) {
        return 0;
    }
    size_t r = zf->zipdecrypter->read(zf->zipdecrypter,
    buf, len);
    if (r == 0) {
        // check if EOF or read error:
        if (zf->zipdecrypter->eof(zf->zipdecrypter)) {
            return 0;  // EOF
        }
        return -1;  // error
    }
    return r;
}

static size_t zipfile_ReadUndecrypted(void* userdata, char* buffer,
size_t bytes) {
    // get our zip file info:
    struct zipfileaccess* zf = userdata;

    // simply read from our file handle, but no further than the limit:
    size_t readlen = bytes;
    if (readlen > zf->sizeinfile - zf->posinfile) {
        readlen = zf->sizeinfile - zf->posinfile;
    }
    if (readlen == 0) {
        return 0;
    }

    // do read:
    int r = fread(buffer, 1, readlen, zf->f);
    if (r <= 0) {  // eof or error
        return 0;
    }
    zf->posinfile += (size_t)r;
    return r;
}

static PHYSFS_sint64 zipfile_TellPhysFS(struct PHYSFS_Io *io) {
    // get our zip file info:
    struct zipfile* zf = (struct zipfile*)io->opaque;

    // query position from our decrypter:
    return zf->zipdecrypter->tell(zf->zipdecrypter);
}

static size_t zipfile_TellUndecrypted(void* userdata) {
    // get our zip file info:
    struct zipfileaccess* zf = userdata;

    // return position:
    return zf->posinfile;
}

static PHYSFS_sint64 zipfile_LengthPhysFS(struct PHYSFS_Io *io) {
    // get our zip file info:
    struct zipfile* zf = (struct zipfile*)io->opaque;

    return zf->zipdecrypter->length(zf->zipdecrypter);
}

static size_t zipfile_LengthUndecrypted(void* userdata) {
    // get our zip file info:
    struct zipfileaccess* zf = userdata;

    // return length:
    return zf->sizeinfile;
}

static int zipfile_SeekPhysFS(struct PHYSFS_Io *io,
PHYSFS_uint64 offset) {
    // get our zip file info:
    struct zipfile* zf = (struct zipfile*)io->opaque;

    // seek through our zip decrypter:
    return zf->zipdecrypter->seek(zf->zipdecrypter,
    offset);
}

static int zipfile_SeekUndecrypted(void* userdata, size_t offset) {
    // get our zip file info:
    struct zipfileaccess* zf = userdata;

    // check if seek is valid:
    if (offset > zf->sizeinfile) {
        return 0;  // out of bounds
    }

    // attempt to seek:
    int i = fseek(zf->f, offset + zf->offsetinfile, SEEK_SET);
    if (i == 0) {
        // success
        zf->posinfile = offset;
        return 1;
    }
    // error
    return 0;
}

static void zipfile_DestroyPhysFS(struct PHYSFS_Io *io) {
    // close decrypter, destroy our structs:
    struct zipfile* zf = (struct zipfile*)(io->opaque);
    zf->zipdecrypter->close(zf->zipdecrypter);
    free(zf->physio);
}

static int zipfile_EofUndecrypted(void* userdata) {
    // get our zip file access info:
    struct zipfileaccess* zf = userdata;

    if (zf->offsetinfile >= zf->sizeinfile) {
        return 1;
    }
    return 0;
}

static void zipfile_DestroyUndecrypted(void* userdata) {
    // get our zip file access info:
    struct zipfileaccess* zf = userdata;

    // close the file:
    fclose(zf->f);

    // free file name:
    free(zf->fname);

    // free file access info itself:
    free(zf);
}

struct PHYSFS_Io* zipfile_DuplicatePhysFS(struct PHYSFS_Io* io) {
    // duplicate zipfile struct:
    struct zipfile* zf = (struct zipfile*)(io->opaque);
    struct zipfile* newzip = malloc(sizeof(*newzip));
    if (!newzip) {
        return NULL;
    }
    memcpy(newzip, zf, sizeof(*newzip));

    // duplicate physio struct:
    newzip->physio = malloc(sizeof(*(newzip->physio)));
    if (!newzip->physio) {
        free(newzip);
        return NULL;
    }
    memcpy(newzip->physio, zf->physio, sizeof(*(newzip->physio)));
    newzip->physio->opaque = newzip;

    // duplicate zip decrypter:
    newzip->zipdecrypter = newzip->zipdecrypter->duplicate(newzip->zipdecrypter);
    if (!newzip->zipdecrypter) {
        free(newzip->physio);
        free(newzip);
        return NULL;
    }
    return newzip->physio;
}

static int zipfile_DuplicateUndecrypted(
void* userdata, struct zipdecryptionfileaccess* zdf) {
    // get our zip file info:
    struct zipfileaccess* zf = userdata;

    // duplicate zip file access info:
    struct zipfileaccess* zf2 = malloc(sizeof(*zf2));
    if (!zf2) {return 0;}
    memset(zf2, 0, sizeof(*zf2));

    // duplicate file name:
    zf2->fname = strdup(zf->fname);
    if (!zf2->fname) {
        free(zf2);
        return 0;
    }

    // "duplicate" file handle by simply reopening the file:
    zf2->f = fopen(zf2->fname, "rb");
    if (!zf2->f) {
        free(zf2->fname);
        free(zf2);
        return 0;
    }
    // seek to position in old handle:
    int64_t oldpos = ftell(zf->f);
    if (oldpos >= 0) {
        if (fseek(zf2->f, oldpos, SEEK_SET) != 0) {
            fclose(zf2->f);
            free(zf2->fname);
            free(zf2);
            return 0;
        }
    } else {
        fclose(zf2->f);
        free(zf2->fname);
        free(zf2);
        return 0;
    }

    // copy offset info:
    zf2->posinfile = zf->posinfile;
    zf2->offsetinfile = zf->offsetinfile;
    zf2->sizeinfile = zf->sizeinfile; 

    // return new zipdecryptionfileaccess info:
    memset(zdf, 0, sizeof(*zdf));
    zdf->read = &zipfile_ReadUndecrypted;
    zdf->eof = &zipfile_EofUndecrypted;
    zdf->length = &zipfile_LengthUndecrypted;
    zdf->seek = &zipfile_SeekUndecrypted;
    zdf->tell = &zipfile_TellUndecrypted;
    zdf->close = &zipfile_DestroyUndecrypted;
    zdf->duplicate = &zipfile_DuplicateUndecrypted;
    zdf->userdata = zf2;
    return 1;
}

static void zipfile_RandomMountPoint(char* buffer) {
    //buffer[0] = '/';
    int i = 0;
    while (i < MOUNT_POINT_LEN-1) {
#ifdef WINDOWS
        double d = (double)((double)rand())/RAND_MAX;
#else
        double d = drand48();
#endif
        int c = (int)((double)d*9.99);
        buffer[i] = '0' + c;
        i++;
    }
    buffer[MOUNT_POINT_LEN-1] = '/';
    buffer[MOUNT_POINT_LEN] = 0;
}

PHYSFS_sint64 zipfile_WritePhysFS(struct PHYSFS_Io* io,
const void* buffer, PHYSFS_uint64 len) {
    return -1;
}

struct zipfile* zipfile_Open(const char* file, size_t offsetinfile,
size_t sizeinfile, int encrypted) {
    if (!file || strlen(file) <= 0) {
        // no file given. this is not valid
        return NULL;
    }

    // allocate our basic zip info struct:
    struct zipfile* zf = malloc(sizeof(*zf));
    if (!zf) {
        return NULL;
    }
    memset(zf, 0, sizeof(*zf));

    // allocate file access info struct:
    struct zipfileaccess* zfa = malloc(sizeof(*zfa));
    if (!zfa) {
        free(zf);
        return NULL;
    }
    memset(zfa, 0, sizeof(*zfa));
    zfa->offsetinfile = offsetinfile;
    zfa->sizeinfile = sizeinfile;

    // prepare file name:
    zfa->fname = strdup(file);
    if (!zfa->fname) {
        free(zf);
        free(zfa);
        return NULL;
    }
    file_makeSlashesNative(zfa->fname);

    // open file:
    zfa->f = fopen(zfa->fname, "rb");
    if (!zfa->f) {
        free(zfa->fname);
        free(zfa);
        free(zf);
        return NULL;
    }

    // initialise sizeinfile to something senseful if zero:
    if (zfa->sizeinfile == 0) {
        // initialise to possible file size:
        zfa->sizeinfile = file_getSize(zfa->fname)-zfa->offsetinfile;
        if (zfa->sizeinfile == 0) {
            fclose(zfa->f);
            free(zfa->fname);
            free(zfa);
            free(zf);
            return NULL;
        }
    }

    // seek to proper position in file:
    if (fseek(zfa->f, zfa->offsetinfile, SEEK_SET) != 0) {
        fclose(zfa->f);
        free(zfa->fname);
        free(zfa);
        free(zf);
        return NULL;
    }

    // allocate physfs io struct:
    zf->physio = malloc(sizeof(*physio));
    if (!zf->physio) {
        fclose(zfa->f);
        free(zfa->fname);
        free(zfa);
        free(zf);
        return NULL;
    }

    memset(zf->physio, 0, sizeof(*(zf->physio)));
    zf->physio->opaque = zf;  // make sure we can find our zip info again

    // initialise our custom file access functions:
    zf->physio->read = &zipfile_ReadPhysFS;
    zf->physio->length = &zipfile_LengthPhysFS;
    zf->physio->tell = &zipfile_TellPhysFS;
    zf->physio->seek = &zipfile_SeekPhysFS;
    zf->physio->duplicate = &zipfile_DuplicatePhysFS;
    zf->physio->destroy = &zipfile_DestroyPhysFS;
    zf->physio->write = &zipfile_WritePhysFS;

    // initialise our decryption:
    struct zipdecryptionfileaccess zdf;
    memset(&zdf, 0, sizeof(zdf));
    zdf.read = &zipfile_ReadUndecrypted;
    zdf.eof = &zipfile_EofUndecrypted;
    zdf.length = &zipfile_LengthUndecrypted;
    zdf.seek = &zipfile_SeekUndecrypted;
    zdf.tell = &zipfile_TellUndecrypted;
    zdf.close = &zipfile_DestroyUndecrypted;
    zdf.duplicate = &zipfile_DuplicateUndecrypted;
    zdf.userdata = zfa;
    if (!encrypted) {
        zf->zipdecrypter = zipdecryption_None(&zdf);
    } else {
        zf->zipdecrypter = DEFAULT_DECRYPTER(&zdf);
    }

    // make sure our decrypter works:
    if (!zf->zipdecrypter) {
        fclose(zfa->f);
        free(zfa->fname);
        free(zfa);
        free(zf);
        return NULL;
    }

    // mount the file to a random mount point:
    PHYSFS_permitSymbolicLinks(1);
    zipfile_RandomMountPoint(zf->mountpoint);
    if (!PHYSFS_mountIo(zf->physio, NULL, zf->mountpoint, 1)) {
        zf->physio->destroy(zf->physio);
        free(zf);
        return 0;
    }
    return zf;
}

void zipfile_ClosePhysFS(struct zipfile* zf) {
    PHYSFS_unmount(zf->mountpoint);
}

// get the proper internal path with prepended mountpoint:
static char* getfilepath(struct zipfile* f, const char* path) {
    // get prepended mount point:
    char* pathWithMountpoint = malloc(strlen(f->mountpoint) +
        strlen(path) + 1);
    if (!pathWithMountpoint) {
        return NULL;
    }

    // ensure given original file path is relative:
    char* pathRelative = strdup(path);
    if (!pathRelative) {
        free(pathWithMountpoint);
        return NULL;
    }
    file_removeDoubleSlashes(pathRelative);
    file_makeSlashesCrossplatform(pathRelative);
    if (!file_IsPathRelative(pathRelative)) {
        file_makePathRelative(pathRelative, main_getRunDir());
        if (!file_IsPathRelative(pathRelative)) {
            free(pathRelative);
            free(pathWithMountpoint);
            return NULL;
        }
    }

    // combine mountpoint + path given to us:
    memcpy(pathWithMountpoint, f->mountpoint, strlen(f->mountpoint));
    memcpy(pathWithMountpoint + strlen(f->mountpoint),
        pathRelative, strlen(pathRelative));
    pathWithMountpoint[strlen(f->mountpoint) + strlen(pathRelative)] = 0;
    file_removeDoubleSlashes(pathWithMountpoint);
    if (strlen(pathWithMountpoint) > 0) {
        if (pathWithMountpoint[strlen(pathWithMountpoint) - 1] == '/') {
            pathWithMountpoint[strlen(pathWithMountpoint) - 1] = 0;
        }
    }
    free(pathRelative);
    return pathWithMountpoint;
}

int zipfile_PathExists(struct zipfile* zf, const char* path) {
    if (!zf || !path) {
        return 0;
    }

    // get proper file path:
    char* p = getfilepath(zf, path);
    if (!p) {
        return 0;
    }

    // check if path exists in archive:
    int pathexists = 0;

    // try to open the file:
    PHYSFS_Stat s;
    if (PHYSFS_stat(p, &s)) {
        pathexists = 1;
    }

    // free sanitized path:
    free(p);

    return pathexists;
}

int zipfile_IsDirectory(struct zipfile* zf, const char* path) {
    if (!zf || !path) {
        return 0;
    }

    // get proper file path:
    char* p = getfilepath(zf, path);
    if (!p) {
        return 0;
    }

    // check if path is directory in archive:
    int isdirectory = 0;

    // try to open the file:
    PHYSFS_Stat s;
    if (PHYSFS_stat(p, &s)) {
        if (s.filetype == PHYSFS_FILETYPE_DIRECTORY) {
            isdirectory = 1;
        }
    }    

    // free sanitized path:
    free(p);

    return isdirectory;
}

int64_t zipfile_FileGetLength(struct zipfile* zf, const char* path) {
    if (!zf || !path) {
        return -1;
    }

    // get proper file path:
    char* p = getfilepath(zf, path);
    if (!p) {
        return -1;
    }

    int64_t size = -1;

    // try to open the file:
    PHYSFS_Stat s;
    if (PHYSFS_stat(p, &s)) {
        size = s.filesize;
    }

    // free sanitized path:
    free(p);

    return size;
}

struct zipfilereader {
    PHYSFS_File* f;
};

struct zipfilereader* zipfile_FileOpen(struct zipfile* f, const char* path) {
    if (!f || !path) {
        return NULL;
    }

    // get proper file path:
    char* p = getfilepath(f, path);
    if (!p) {
        return 0;
    }

    // allocate struct
    struct zipfilereader* zfr = malloc(sizeof(*zfr));
    if (!zfr) {
        free(p);
        return NULL;
    }

    // open file:
    zfr->f = PHYSFS_openRead(p);
    if (!zfr->f) {
        free(zfr);
        free(p);
        return NULL;
    }

    // free sanitized path:
    free(p);

    return zfr;
}

int zipfile_FileSeek(struct zipfilereader* f, size_t pos) {
    if (!f) {
        return 0;
    }

    return (PHYSFS_seek(f->f, pos) != 0);
}

size_t zipfile_FileTell(struct zipfilereader* f) {
    if (!f) {
        return 0;
    }

    return (size_t)PHYSFS_tell(f->f);
}

void zipfile_FileClose(struct zipfilereader* f) {
    if (!f) {
        return;
    }

    // Close file:
    PHYSFS_close(f->f);
    // not for icculus: this is not a well-written program :-)

    // free struct:
    free(f);
}

size_t zipfile_FileRead(struct zipfilereader* f, char* buffer,
size_t bytes) {
    if (!f) {
        return 0;
    }

    int r = PHYSFS_readBytes(f->f, buffer, bytes);
    if (r < 0) {
        r = 0;
    }
    return r;
}

int zipfile_FileEof(struct zipfilereader* f) {
    if (!f) {
        return 0;
    }

    return PHYSFS_eof(f->f);
}

struct zipfileiter {
    int index;
    char** filelist;
};
struct zipfileiter* zipfile_Iterate(struct zipfile* f,
        const char* dir) {
    if (!f) {
        return NULL;
    }
    struct zipfileiter* iter = malloc(sizeof(*iter));
    if (!iter) {
        return NULL;
    }
    memset(iter, 0, sizeof(*iter));
    char* path = getfilepath(f, dir);
    if (!path) {
        free(iter);
        return NULL;
    }
    char* path2 = path;
    if (path[strlen(path)-1] == '/') {
        path2 = malloc(strlen(path) + 1);
        memcpy(path2, path, strlen(path));
        path2[strlen(path)] = 0;
        free(path);
    }
    PHYSFS_Stat s;
    memset(&s, 0, sizeof(s));
    if (PHYSFS_stat(path2, &s)) {
        if (s.filetype != PHYSFS_FILETYPE_DIRECTORY) {
            free(iter);
            free(path);
            return 0;
        }
    }
    iter->filelist = PHYSFS_enumerateFiles(path2);
    free(path2);
    return iter;
}

const char* zipfile_NextFile(struct zipfileiter* f) {
    if (!f) {
        return NULL;
    }
    if (f->filelist[f->index]) {
        char* s = f->filelist[f->index];
        f->index++;
        return s;
    }
    return NULL;
}

void zipfile_FinishIteration(struct zipfileiter* f) {
    if (f) {
        PHYSFS_freeList(f->filelist);
        free(f);
    }
}

#endif  // USE_PHYSFS

