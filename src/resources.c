
/* blitwizard game engine - source code file

  Copyright (C) 2013 Jonas Thiem

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

#ifdef WINDOWS
#include <windows.h>
#endif
#include <string.h>
#include <stdio.h>

#include "resources.h"
#include "file.h"
#include "zipfile.h"

#ifdef USE_PHYSFS
struct resourcearchive {
    struct zipfile* z;
    struct resourcearchive* prev, *next;
};
struct resourcearchive* resourcearchives = NULL;
#endif

int resources_LoadZipFromFilePart(const char* path,
size_t offsetinfile, size_t sizeinfile, int encrypted) {
#ifdef USE_PHYSFS
    // allocate resource location struct:
    struct resourcearchive* rl = malloc(sizeof(*rl));
    if (!rl) {
        return 0;
    }
    memset(rl, 0, sizeof(*rl));

    // open archive:
    rl->z = zipfile_Open(path, offsetinfile, sizeinfile, 0);
    if (!rl->z) {
        // opening the archive failed.
        free(rl);
        return 0;
    }

    // add ourselves to the resourcelist:
    if (resourcearchives) {
        resourcearchives->prev = rl;
    }
    rl->next = resourcearchives;
    resourcearchives = rl;
    return 1;
#endif
    return 0;
}

int resources_LoadZipFromFile(const char* path, int encrypted) {
#ifdef USE_PHYSFS
    return resources_LoadZipFromFilePart(path, 0, 0, encrypted);
#else  // USE_PHYSFS
    // no PhysFS -> no .zip support
    return 0;
#endif  // USE_PHYSFS
}

int resources_LoadZipFromExecutable(const char* path, int encrypted) {
#ifdef USE_PHYSFS
#ifdef WINDOWS
    // on windows and using PhysFS
    // open file:
    FILE* r = fopen(path, "rb");
    if (!r) {
        return 0;
    }

    // read DOS header:
    IMAGE_DOS_HEADER h;
    int k = fread(&h, 1, sizeof(h), r);
    if (k != sizeof(h)) {
        fclose(r);
    }

    // locate PE header
    IMAGE_NT_HEADERS nth;
    fseek(r, h.e_lfanew, SEEK_SET);
    if (fread(&nth, 1, sizeof(nth), r) != sizeof(nth)) {
        fclose(r);
    }

    // check header signatures:
    if (h.e_magic != IMAGE_DOS_SIGNATURE ||
    nth.Signature != IMAGE_NT_SIGNATURE) {
        fclose(r);
        return 0;
    }

    // find the section that has the highest offset in the file
    // (= latest section)
    IMAGE_SECTION_HEADER sections;
    if (fread(&sections, 1, sizeof(sections), r) != sizeof(sections)) {
        fclose(r);
        return 0;
    }
    void* latestSection = NULL;
    size_t latestSectionSize = -1;
    int i = 0;
    while (i < nth.FileHeader.NumberOfSections) {
        // check out current section:
        if ((void*)sections.PointerToRawData > (void*)latestSection) {
            latestSection = (void*)sections.PointerToRawData;
            latestSectionSize = sections.SizeOfRawData;
        }
        i++;

        // read next section if this wasn't the last one:
        if (i < nth.FileHeader.NumberOfSections) {
            if (fread(&sections, 1, sizeof(sections), r) !=
            sizeof(sections)) {
                fclose(r);
                return 0;
            }
        }
    }

    if (!latestSection) {
        // no latest section. weird!
        return 0;
    }

    // the size of the PE section data:
    size_t size = (size_t)latestSection + latestSectionSize;

    // now we have all information we want.
    return resources_LoadZipFromFilePart(path, size, 0, encrypted);
#else  // WINDOWS
    // on some unix with PhysFS
    // open file:
    FILE* r = fopen(path, "rb");

    // see if this is an ELF file:
    char magic[4];
    if (fread(magic, 1, 4, r) != 4) {
        fclose(r);
        return 0;
    }
    if (magic[0] != 127 || memcmp("ELF", magic+1, 3) != 0) {
        // not an ELF binary
        fclose(r);
        return 0;
    }

    // read whether this is a 32bit or 64bit bin:
    int is64bit = 0;
    unsigned char i;
    if (fread(&i, 1, 1, r) != 1) {
        fclose(r);
        return 0;
    }
    if (i == 1) {  // 32bit bin
        is64bit = 0;
    } else if (i == 2) {  // 64bit bin
        is64bit = 1;
    } else {  // invalid!
        fclose(r);
        return 0;
    }

    // seek to section header offset:
    if (fseek(r, (long)
    16 +  // magic string
    2+  // type
    2+  // machine
    4+  // version
    4+(4*is64bit)+  // entry offset,
    4+(4*is64bit)+  // program header offset
    0, SEEK_SET) != 0) {
        fclose(r);
        return 0;
    }

    // read section header offset:
    uint64_t t;
    if (is64bit) {
        if (fread(&t, sizeof(t), 1, r) != 1) {
            fclose(r);
            return 0;
        }
    } else {
        uint32_t t2;
        if (fread(&t2, sizeof(t2), 1, r) != 1) {
            fclose(r);
            return 0;
        }
        t = t2;
    }

    // we don't support ELF binaries with no section header:
    if (t == 0) {
        fclose(r);
        return 0;
    }

    // seek to size of a single section header entry:
    if (fseek(r, (long)
    16 +  // magic string
    2+  // type
    2+  // machine
    4+  // version
    4+(4*is64bit)+  // entry offset,
    4+(4*is64bit)+  // program header offset
    4+(4*is64bit)+  // section header offset
    4+  // flags
    2+  // ? size
    2+  // program header entry size
    2+  // program header count
    0, SEEK_SET) != 0) {
        fclose(r);
        return 0;
    }

    // read section header entry size:
    uint16_t entrysize;
    if (fread(&entrysize, sizeof(entrysize), 1, r) != 1) {
        fclose(r);
        return 0;
    }

    // read the (directly following) number of entries:
    uint16_t entrynumber;
    if (fread(&entrynumber, sizeof(entrynumber), 1, r) != 1) {
        fclose(r);
        return 0;
    }

    // no section header entries is invalid:
    if (entrynumber == 0) {
        fclose(r);
        return 0;
    }

    // a section header entry must have a minimum size to be useful:
    if (entrysize < 4+4+(4+4*is64bit)*4) {
        // ^ corresponds to name, type, flags, addr, offset, size
        // entry is too small to keep those things, so it's invalid:
        fclose(r);
        return 0;
    }

    // calculate size from section header offset + size:
    uint64_t filesize = t + entrynumber * entrysize;

    // now check all sections for extending beyond this size:
    // seek to section header:
    if (fseek(r, t, SEEK_SET) != 0) {
        fclose(r);
        return 0;
    }

    char readblock[4+4+8+8+8+8];
    int k = 0;
    while (k < entrynumber) {
        int readsize = 4+4+(4+4*is64bit)*4;

        // read section header block:
        if (fread(readblock, readsize, 1, r) != 1) {
            fclose(r);
            return 0;
        }
        // seek forward the remaining entry size:
        if (fseek(r, entrysize - readsize, SEEK_CUR) != 0) {
            fclose(r);
            return 0;
        }

        // check whether it is not an empty section type:
        uint32_t sectiontype = *(uint32_t*)((char*)readblock+4);
        if (sectiontype != 8 && sectiontype != 0) {
            // excluded sections: 8 = SHT_NOBITS, 1 = SHT_NULL

            // this section occupies an actual size in the file
            // obtain its offset and size:
            uint64_t sectionoffset;
            if (is64bit) {
                sectionoffset = *((uint64_t*)((char*)readblock+4+4+8+8));
            } else {
                uint32_t sectionoffset32 =
                *(uint32_t*)((char*)readblock+4+4+4+4);
                sectionoffset = sectionoffset32;
            }
            uint64_t sectionsize;
            if (is64bit) {
                sectionsize = *(uint64_t*)((char*)readblock+4+4+8+8+8);
            } else {
                uint32_t sectionsize32 =
                *(uint32_t*)((char*)readblock+4+4+4+4+4);
                sectionsize = sectionsize32;
            }

            // now if this section is longer than our current
            // file size estimation, correct that estimation:
            if (sectionoffset + sectionsize > filesize) {
                filesize = sectionoffset + sectionsize;
            }
        }
        k++;
    }
    return resources_LoadZipFromFilePart(path, filesize, 0, encrypted);
#endif  // WINDOWS
#else  // USE_PHYSFS
    // no PhysFS -> no .zip support
    return 0;
#endif  // USE_PHYSFS
}

int resources_LoadZipFromOwnExecutable(const char* first_commandline_arg,
int encrypted) {
#ifdef WINDOWS
    // locate our exe:
    char fname[512];
    if (!GetModuleFileName(NULL, fname, sizeof(fname))) {
        return 0;
    }

    // get an absolute path just in case it isn't one:
    char* path = file_GetAbsolutePathFromRelativePath(fname);
    if (!path) {
        return 0;
    }

    // load from our executable:
    int result = resources_LoadZipFromExecutable(fname, encrypted);
    free(path);
    return result;
#else
    // locate our own binary using the first command line arg
    if (!first_commandline_arg) {
        // no way of locating it.
        return 0;
    }

    // check if the given argument is a valid file:
    if (!file_DoesFileExist(first_commandline_arg)
    || file_IsDirectory(first_commandline_arg)) {
        // not pointing at a file as expected
        return 0;
    }

    // get an absolute path:
    char* path = file_GetAbsolutePathFromRelativePath(
    first_commandline_arg);
    if (!path) {
        return 0;
    }

    // load from our executable:
    int result = resources_LoadZipFromExecutable(path,
    encrypted);
    free(path);
    return result;
#endif
}

int resource_IsFolderInZip(const char* path) {
    if (!file_IsPathRelative(path)) {
        return 0;
    }

#ifdef USE_PHYSFS
    struct resourcearchive* a = resourcearchives;
    while (a) {
        // check if path maps to a directory in this archive:
        if (zipfile_PathExists(a->z, path)) {
            if (zipfile_IsDirectory(a->z, path)) {
                return 1;
            }
        }
        a = a->next;
    }
#endif
    // nope, no such directory in our archives.
    return 0;
}

char** resource_FileList(const char* path) {
    char** p = NULL;
    size_t filecount = 0;
    int directoryexists = 0;

#ifdef USE_PHYSFS
    struct resourcearchive* a = resourcearchives;
    while (a) {
        // check if path maps to a directory in this archive:
        if (zipfile_PathExists(a->z, path)) {
            if (zipfile_IsDirectory(a->z, path)) {
                directoryexists = 1;

                // get file list:
                struct zipfileiter* iter = zipfile_Iterate(
                a->z, path);
                if (!iter) {  // cannot iterate directory.
                    a = a->next;
                    continue;
                }
                // go through all files (if there are any):
                const char* fname = zipfile_NextFile(iter);
                while (fname) {
                    filecount++;
                    // add file name to list:
                    char** p2 = realloc(p, sizeof(char*) * (filecount+1));
                    if (!p2) {
                        // whoops.
                        zipfile_FinishIteration(iter);
                        free(p);
                        return NULL;
                    }
                    p = p2;
                    p[filecount-1] = strdup(fname);
                    p[filecount] = NULL;

                    fname = zipfile_NextFile(iter);
                }
                zipfile_FinishIteration(iter);
            }
        }
        a = a->next;
    }
#endif
    if (filecount == 0) {
        if (p) {
            // throw away whatever we have here
            free(p);
        }
        if (directoryexists) {
            // return an empty list:
            p = malloc(sizeof(char*)*1);
            if (p) {
                p[0] = 0;
            }
            return p;
        }
        // return failure:
        return NULL;
    }
    // construct a new list without duplicates:
    char** p2 = malloc(sizeof(char*) * (filecount+1));
    size_t filecount2 = 0;
    if (!p2) {
        size_t i = 0;
        while (i < filecount) {
            free(p[i]);
            i++;
        }
        free(p);
        return NULL;
    }
    size_t i = 0;
    while (i < filecount) {
        int duplicate = 0;
        // check if file is already in list:
        size_t i2 = 0;
        while (i2 < filecount2) {
            if (strcasecmp(p2[i2], p[i]) == 0) {
                // already present.
                duplicate = 1;
                break;
            }
            i2++;
        }
        // add if not a duplicate:
        if (!duplicate) {
            filecount2++;
            char** p3 = malloc(sizeof(char*) * (filecount2+1));
            if (!p3) {
                i = 0;
                while (i < filecount) {
                    free(p[i]);
                    i++;
                }
                free(p);
                free(p2);
                return NULL;
            }
            p2 = p3;
            p2[filecount2-1] = p[i];
            p2[filecount] = NULL;
        } else {
            // free duplicate entry, we won't use it:
            free(p[i]);
            p[i] = NULL;
        }
        i++;
    }
    // free old list:
    free(p);
    // return result:
    return p2;
}

int resources_LocateResource(const char* path,
struct resourcelocation* location) {
#ifdef USE_PHYSFS
    if (file_IsPathRelative(path)) {
        char* archivepath = strdup(path);
        if (!archivepath) {
            return 0;
        }
        file_MakeSlashesCrossplatform(archivepath);

        // check resource archives:
        struct resourcearchive* a = resourcearchives;
        while (a) {
            // check if path maps to a file in this archive:
            if (zipfile_PathExists(a->z, archivepath)) {
                if (zipfile_IsDirectory(a->z, archivepath)) {
                    // we want a file, not a directory.
                    a = a->next;
                    continue;
                }

                // it is a file! hooray!

                if (!location) {
                    // the callee apparently doesn't care about
                    // location details.
                    free(archivepath);
                    return 1;
                }

                // store location details:
                location->type = LOCATION_TYPE_ZIP;
                int i = strlen(archivepath);
                if (i >= MAX_RESOURCE_PATH) {
                    i = MAX_RESOURCE_PATH-1;
                }
                location->location.ziplocation.archive = a->z;
                memcpy(location->location.ziplocation.filepath,
                archivepath, i);
                location->location.ziplocation.filepath[i] = 0;
                free(archivepath);
                return 1;
            }
            // if path doesn't exist, continue our search:
            a = a->next;
        }
        free(archivepath);
    }
#endif
    // check the hard disk as last location:
    if (file_DoesFileExist(path) && !file_IsDirectory(path)) {
        // file exists on disk!
        if (location) {
            location->type = LOCATION_TYPE_DISK;

            // get an absolute path to the resource:
            char* apath = file_GetAbsolutePathFromRelativePath(path);
            if (!apath) {
                return 0;
            }

            // copy path to location info struct:
            int i = strlen(apath);
            if (i >= MAX_RESOURCE_PATH) {
                i = MAX_RESOURCE_PATH-1;
            }
            memcpy(location->location.disklocation.filepath, apath, i);
            location->location.disklocation.filepath[i] = 0;

            // turn path to native style (since it's a disk path)
            file_MakeSlashesNative(
            location->location.disklocation.filepath);

            // free temp path string
            free(apath);
        }
        return 1;
    }
    return 0;
}


