
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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "zipdecryption.h"

struct zipdecrypternone_data {
    struct zipdecryptionfileaccess f;
};

size_t zipdecryption_Read(struct zipdecrypter* d, char* buffer, size_t bytes) {
    struct zipdecrypternone_data* data = d->internaldata;

    return data->f.read(data->f.userdata, buffer, bytes);
}

int zipdecryption_Eof(struct zipdecrypter* d) {
    struct zipdecrypternone_data* data = d->internaldata;

    return data->f.eof(data->f.userdata);
}

size_t zipdecryption_Length(struct zipdecrypter* d) {
    struct zipdecrypternone_data* data = d->internaldata;

    return data->f.length(data->f.userdata);
}

int zipdecryption_Seek(struct zipdecrypter* d, size_t pos) {
    struct zipdecrypternone_data* data = d->internaldata;

    return data->f.seek(data->f.userdata, pos);
}

size_t zipdecryption_Tell(struct zipdecrypter* d) {
    struct zipdecrypternone_data* data = d->internaldata;

    return data->f.tell(data->f.userdata);
}

void zipdecryption_Close(struct zipdecrypter* d) {
    struct zipdecrypternone_data* data = d->internaldata;

    data->f.close(data->f.userdata);
    free(data);
    free(d);
}

struct zipdecrypter* zipdecryption_Duplicate(struct zipdecrypter* d) {
    // allocate new zip decrypter struct:
    struct zipdecrypter* d2 = malloc(sizeof(*d2));
    if (!d2) {
        return NULL;
    }

    // allocate new internal data struct:
    struct zipdecrypternone_data* data2 = malloc(sizeof(*data2));
    if (!data2) {
        free(d2);
        return NULL;
    }
    
    // initialise internal data with new file access copy:
    memset(data2, 0, sizeof(*data2));
    struct zipdecrypternone_data* data = d->internaldata;
    if (!data->f.duplicate(data->f.userdata, &(data2->f))) {
        // duplication failed
        free(d2);
        free(data2);
        return NULL;
    }

    // do an easy shortcut to copy over all functions:
    memcpy(d2, d, sizeof(*d2));
    // set new internal data:
    d2->internaldata = data2;

    return d2;
}

struct zipdecrypter* zipdecryption_None(
struct zipdecryptionfileaccess* fileaccess) {
    if (!fileaccess) {
        return NULL;
    }

    // allocate decrypter struct:
    struct zipdecrypter* d = malloc(sizeof(*d));
    if (!d) {
        return NULL;
    }
    memset(d, 0, sizeof(*d));

    // allocate internal data struct:
    struct zipdecrypternone_data* data = malloc(sizeof(*data));
    if (!data) {
        free(d);
        return NULL;
    }
    memset(data, 0, sizeof(*data));

    // copy over file access:
    memcpy(&(data->f), fileaccess, sizeof(*fileaccess));

    // populate our decrypter struct with all the neat things:
    d->internaldata = data;
    d->read = zipdecryption_Read;
    d->eof = zipdecryption_Eof;
    d->length = zipdecryption_Length;
    d->seek = zipdecryption_Seek;
    d->tell = zipdecryption_Tell;
    d->close = zipdecryption_Close;
    d->duplicate = zipdecryption_Duplicate;
    return d;
}
