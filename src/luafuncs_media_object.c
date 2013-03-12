
/* blitwizard game engine - source code file

  Copyright (C) 2012-2013 Jonas Thiem

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

/// The blitwizard audio namespace allows you to create
// sound objects to play sounds and music.
// Use @{blitwizard.audio.simpleSound|simpleSound} objects for
// background music, @{blitwizard.audio.pannedSound|pannedSound}
// for a sound with left/right panning and
// @{blitwizard.audio.positionedSound|positionedSound} for a sound with
// positioning in 2d or 3d space inside the game world.
//
// Please note blitwizard will NEVER preload any sounds into memory,
// so it is perfectly fine to create many sound objects from the same
// sound file, it won't cause the file to be loaded into memory
// many times.
//
// Supported audio formats are .ogg and .flac, and, if a usable FFmpeg
// version is present on the system, many more like .mp3, .mp4 and others.
//
// @author Jonas Thiem  (jonas.thiem@gmail.com)
// @copyright 2012-2013
// @license zlib
// @module blitwizard.audio

#include "luafuncs_media_object.h"
#include "luaheader.h"
#include "luastate.h"
#include "luaerror.h"
#include "audio.h"
#include "audiomixer.h"

#include <stdlib.h>
#include <string.h>

struct mediaobject* mediaObjects = NULL;

static void mediaobject_UpdateIsPlaying(struct mediaobject* o);
static int garbagecollect_mediaobjref(lua_State* l);
struct mediaobject* tomediaobject(lua_State* l,
int type, int index, int arg, const char* func);

int luafuncs_media_object_new(lua_State* l, int type) {
#ifdef USE_AUDIO
    // check which function called us:
    char funcname_simple[] = "blitwizard.audio.simpleSound:new";
    char funcname_panned[] = "blitwizard.audio.pannedSound:new";
    char funcname_positioned[] = "blitwizard.audio.positionedSound:new";
    char funcname_unknown[] = "???";
    char* funcname = funcname_unknown;
    switch (type) {
    case MEDIA_TYPE_AUDIO_SIMPLE:
        funcname = funcname_simple;
        break;
    case MEDIA_TYPE_AUDIO_PANNED:
        funcname = funcname_panned;
        break;
    case MEDIA_TYPE_AUDIO_POSITIONED:
        funcname = funcname_positioned;
        break;
    }

    // technical first argument is the object table,
    // which we don't care about in the :new function.
    // actual specified first argument is the second one
    // on the lua stack.

    // first argument needs to be media file name
    if (lua_type(l, 2) != LUA_TSTRING) {
        return haveluaerror(l, badargument1, 1, funcname,
        "string", lua_strtype(l, 2));
    }
    const char* p = lua_tostring(l, 2);

    // check if the given resource exists:
    if (!resources_LocateResource(p, NULL)) {
        return haveluaerror(l, "sound file \"%s\" not found", p);
    }

    // generate new sound object:
    struct luaidref* iref = lua_newuserdata(l, sizeof(*iref));
    memset(iref, 0, sizeof(*iref));
    iref->magic = IDREF_MAGIC;
    iref->type = IDREF_MEDIA;
    iref->ref.mobj = malloc(sizeof(struct mediaobject));
    if (!iref->ref.mobj) {
        lua_pop(l, 1);
        return haveluaerror(l, "failed to allocate media object");
    }
    luastate_SetGCCallback(l, -1, (int (*)(void*))&garbagecollect_mediaobjref);
    struct mediaobject* m = iref->ref.mobj;
    memset(m, 0, sizeof(*m));
    m->type = type;
    m->refcount++;
    m->mediainfo.sound.soundid = -1;

    // set proper default priority:
    switch (type) {
    case MEDIA_TYPE_AUDIO_SIMPLE:
        m->mediainfo.sound.priority = 5;
        break;
    case MEDIA_TYPE_AUDIO_PANNED:
        m->mediainfo.sound.priority = 2;
        break;
    case MEDIA_TYPE_AUDIO_POSITIONED:
        m->mediainfo.sound.priority = 2;
        break;
    }

    // remember sound path:
    m->mediainfo.sound.soundname = strdup(p);
    if (!m->mediainfo.sound.soundname) {
        // string alloc failed
        lua_pop(l, 1);
        return haveluaerror(l, "allocating sound path failed");
    }

    // set meta table __index field to class table
    lua_checkstack(l, 8);  // ensure some stack space
    lua_getmetatable(l, -1);  // on stack now: metatable
    lua_getglobal(l, "blitwizard");
    lua_pushstring(l, "audio");
    lua_gettable(l, -2);  // on stack now: metatable, blitwizard, audio
    lua_pushstring(l, "simpleSound");
    lua_gettable(l, -2);  // on stack now: metatable, blitwizard, audio, simpleSound
    lua_insert(l, -3);  // on stack now: metatable, simpleSound, blitwizard, audio
    lua_pop(l, 2);  // on stack now: metatable, simpleSound
    lua_pushstring(l, "__index");  // on stack now: metatable, simpleSound, "__index"
    lua_insert(l, -2);  // on stack now: metatable, "__index", simpleSound
    lua_settable(l, -3);  // on stack now: metatable
    lua_pop(l, 1);  // done!

    // add to media object list:
    if (mediaObjects) {
        mediaObjects->prev = m;
    }
    m->next = mediaObjects;
    mediaObjects = m;
    return 1;
#else
    return haveluaerror(l, compiled_without_audio);
#endif
}

int luafuncs_media_object_play(lua_State* l, int type) {
#ifdef USE_AUDIO
    // check which function called us:
    char funcname_simple[] = "blitwizard.audio.simpleSound:play";
    char funcname_panned[] = "blitwizard.audio.pannedSound:play";
    char funcname_positioned[] = "blitwizard.audio.positionedSound:play";
    char funcname_unknown[] = "???";
    char* funcname = funcname_unknown;
    switch (type) {
    case MEDIA_TYPE_AUDIO_SIMPLE:
        funcname = funcname_simple;
        break;
    case MEDIA_TYPE_AUDIO_PANNED:
        funcname = funcname_panned;
        break;
    case MEDIA_TYPE_AUDIO_POSITIONED:
        funcname = funcname_positioned;
        return haveluaerror("positioned audio objects are not implemented yet");
        break;
    }

    // obtain sound object:
    struct mediaobject* m = tomediaobject(l,
    type, 1, 0, funcname);

    // update current playing state:
    mediaobject_UpdateIsPlaying(m);
    if (m->isPlaying) {return 0;}

    float volume = 1;
    int noamplify = 1;
    float panning = 0;
    float fadeinseconds = 0;
    int loop = 0;

    // extract volume parameter:
    if (lua_type(l, 2) != LUA_TNUMBER &&
    lua_type(l, 2) != LUA_TNIL) {
        return haveluaerror(badargument1, 1, funcname,
        "number", lua_strtype(l, 2));
    }
    if (lua_type(l, 2) == LUA_TNUMBER) {
        volume = lua_tonumber(l, 2);
        if (volume < 0) {volume = 0;}
        if (volume > 1.5) {volume = 1.5;}
        if (volume > 1 &&
        type == MEDIA_TYPE_AUDIO_SIMPLE) {
            volume = 1,
        }
    }

    // extract panning parameter:
    if (type == MEDIA_TYPE_AUDIO_PANNED) {
        if (lua_type(l, 3) != LUA_TNUMBER &&
        lua_type(l, 3) != LUA_TNIL) {
            return haveluaerror(badargument1, 2, funcname,
            "number", lua_strtype(l, 3));
        }
        if (lua_type(l, 3) == LUA_TNUMBER) {
            panning = lua_tonumber(l, 3);
            if (panning < -1) {panning = -1;}
            if (panning > 1) {panning = 1;}
        }
    }

    // play sound:
    m->mediainfo.sound.soundid = audiomixer_PlaySoundFromDisk(
    m->mediainfo.sound.soundname, m->mediainfo.sound.priority,
    volume, panning, noamplify, fadeinseconds, loop);
    return 0;
#else
    return haveluaerror(l, compiled_without_audio);
#endif
}

int luafuncs_media_object_stop(lua_State* l, int type) {
    return 0;
}

int luafuncs_media_object_setPriority(lua_State* l, int type) {
    return 0;
}

int luafuncs_media_object_adjust(lua_State* l, int type) {
    return 0;
}

/// Implements a simple sound which has no
// stereo left/right panning or room positioning features.
// This is the sound object suited best for background music.
// It deactivates some of the postprocessing which would allow positioning
// or stereo panning which results in slightly better and
// unaltered sound.
// 
// The default @{blitwizard.audio.simpleSound:setPriority|sound priority}
// for simple sound objects is 5.
//
// @usage -- play sound file "blubber.ogg"
// mysound = blitwizard.audio.simpleSound:new("blubber.ogg")
// mysound:play()
// @type simpleSound

/// Create a new simple sound object.
// @function new
// @tparam string filename Filename of the audio file you want to play

int luafuncs_media_simpleSound_new(lua_State* l) {
    return luafuncs_media_object_new(l, MEDIA_TYPE_AUDIO_SIMPLE);
}

/// Play the sound represented by the simple sound object.
//
// If the sound file doesn't exist or if it is in an unsupported format,
// an error will be thrown.
//
// Does nothing if the sound object is already playing: to play a sound
// multiple times at once, use multiple sound objects.
// @function play
// @tparam number volume (optional) Volume at which the sound plays from 0 (quiet) to 1 (full volume). Defaults to 1
// @tparam boolean loop (optional) If set to true, the sound will loop until explicitely stopped. If set to false or if not specified, it will play once
// @tparam number fadein (optional) Fade in from silence to the specified volume in the given amount of seconds, instead of playing at full volume right from the start
// @usage -- play sound file "blubber.ogg" at 80% volume and
// -- fade in for 5 seconds
// mysound2 = blitwizard.audio.simpleSound:new("blubber.ogg")
// mysound2:play(0.8, 8)

int luafuncs_media_simpleSound_play(lua_State* l) {
    return luafuncs_media_object_play(l, MEDIA_TYPE_AUDIO_SIMPLE);
}

/// Stop the sound represented by the simple sound object.
// Does nothing if the sound doesn't currently play
// @function stop
// @tparam number fadeout (optional) If specified, the sound will fade out for the specified amount in seconds. Otherwise it will stop instantly

int luafuncs_media_simpleSound_stop(lua_State* l) {
    return luafuncs_media_object_stop(l, MEDIA_TYPE_AUDIO_SIMPLE);
}

/// Set the sound priority of the simple sound object.
//
// In blitwizard, only a fixed maximum number of sounds can play
// at a time. If you attempt to @{blitwizard.audio.simpleSound:play|play}
// a sound object when the maximum number of sounds simultaneously
// is reached, it will kill off playing sounds with
// lower priority. If no lower or same priority sound is available
// to kill, it won't start playing at all.
//
// Changing the priority won't affect the sound object's current
// playing, it will only have an effect the next time you use
// play.
//
// Background music should have a high priority, and short,
// frequent sounds like gun shots or footsteps should have a low
// priority.
//
// Simple sound objects default to a priority of 5.
// @function setPriority
// @tparam number priority Priority from 0 (lowest) to 20 (highest), values will be rounded down to have no decimal places (0.5 becomes 0, 1.7 becomes 1, etc)
// @usage -- play sound file "blubber.ogg" with lowest priotiy
// mysound = blitwizard.audio.simpleSound:new("blubber.ogg")
// mysound:setPriority(0)
// mysound:play()

int luafuncs_media_simpleSound_setPriority(lua_State* l) {
    return luafuncs_media_object_setPriority(l, MEDIA_TYPE_AUDIO_SIMPLE);
}

/// Adjust the volume of a simple sound while it is playing
// (does nothing if it's not)
// @function adjust
// @tparam number volume New volume from 0 (quiet) to 1 (full volume)

int luafuncs_media_simpleSound_adjust(lua_State* l) {
    return luafuncs_media_object_adjust(l, MEDIA_TYPE_AUDIO_SIMPLE);
}

/// Implements a sound with simple left/right stereo panning.
// If you want to make a sound emit from a specific location,
// you should probably use a
// @{blitwizard.audio.positionedSound|positionedSound} instead
// @type pannedSound

/// Create a new panned sound object.
// @function new
// @tparam string filename Filename of the audio file you want to play

int luafuncs_media_pannedSound_new(lua_State* l) {
    return luafuncs_media_object_new(l, MEDIA_TYPE_AUDIO_PANNED);
}

/// Play the sound represented by the panned sound object
// @function play
// @tparam number volume (optional) Volume at which the sound plays from 0 (quiet), through 1 (full volume) to 1.5 (over-amplified). Defaults to 1, values >1 can cause distortion
// @tparam number panning (optional) Stereo panning which alters the left/right placement of the sound from 1 (left) through 0 (center) to -1 (right). Default is 0
// @tparam boolean loop (optional) If set to true, the sound will loop until explicitely stopped. If set to false or if not specified, it will play once
// @tparam number fadein (optional) Fade in from silence to the specified volume in the given amount of seconds, instead of playing at full volume right from the start
// @usage -- Play a sound at full volume, slightly panned to the left
// mysound = blitwizard.audio.pannedSound("song.ogg")
// mysound:play(1, 0.3)

int luafuncs_media_pannedSound_play(lua_State* l) {
    return luafuncs_media_object_play(l, MEDIA_TYPE_AUDIO_PANNED);
}

/// Stop the sound represented by the panned sound object.
// Does nothing if the sound doesn't currently play
// @function stop
// @tparam number fadeout (optional) If specified, the sound will fade out for the specified amount in seconds. Otherwise it will stop instantly

int luafuncs_media_pannedSound_stop(lua_State* l) {
    return luafuncs_media_object_stop(l, MEDIA_TYPE_AUDIO_PANNED);
}

/// Set the sound priority of the panned sound,
// see explanation of @{blitwizard.audio.simpleSound:setPriority}.
//
// Panned sounds default to a priority of 2.
// @function setPriority
// @tparam number priority Priority from 0 (lowest) to 20 (highest), values will be rounded down to have no decimal places (0.5 becomes 0, 1.7 becomes 1, etc)

int luafuncs_media_pannedSound_setPriority(lua_State* l) {
    return luafuncs_media_object_setPriority(l, MEDIA_TYPE_AUDIO_SIMPLE);
}

/// Adjust the volume or panning of a panned sound
// (does nothing if the sound is not playing)
// @function adjust
// @tparam number volume New volume from 0 to 1.5 (values higher than 1 can cause distortions)
// @tparam number panning (optional) New panning from 1 (left) to 0 (center) to -1 (right)

int luafuncs_media_pannedSound_adjust(lua_State* l) {
    return luafuncs_media_object_adjust(l, MEDIA_TYPE_AUDIO_SIMPLE);
}

/// Implements a positioned sound which can either follow a
// specific @{blitwizard.object|blitwizard object} or which
// you can move to any position you like. It will alter volume
// based on distance and channels based on direction to simulate
// its room placement.
// @type positionedSound


/// Create a new positioned sound object.
// @function new
// @tparam string filename Filename of the audio file you want to play
// @tparam userdata object (optional) Specify a @{blitwizard.object|blitwizard object} to which the sound should stick. If you don't specify an object to stick to, the sound can be freely placed with @{position}

int luafuncs_media_positionedSound_new(lua_State* l) {
    return luafuncs_media_object_new(l, MEDIA_TYPE_AUDIO_POSITIONED);
}


/// Stop the sound represented by the positioned sound object.
// Does nothing if the sound doesn't currently play
// @function stop
// @tparam number fadeout (optional) If specified, the sound will fade out for the specified amount in seconds. Otherwise it will stop instantly

int luafuncs_media_positionedSound_stop(lua_State* l) {
    return luafuncs_media_object_stop(l, MEDIA_TYPE_AUDIO_POSITIONED);
}



// Various cleanup and management functions:

void cleanupMediaObject(struct mediaobject* o) {
    
}

void deleteMediaObject(struct mediaobject* o) {
    cleanupMediaObject(o);

    // remove object from the list
    if (o->prev) {
        // adjust prev object to have new next pointer
        o->prev->next = o->next;
    } else {
        // was first object in list -> adjust list start pointer
        mediaObjects = o->next;
    }
    if (o->next) {
        // adjust next object to have new prev pointer
        o->next->prev = o->prev;
    }

    // free object
    free(o);
}

static void mediaobject_UpdateIsPlaying(struct mediaobject* m) {
    if (m->type == MEDIA_TYPE_AUDIO_SIMPLE ||
    m->type == MEDIA_TYPE_AUDIO_PANNED ||
    m->type == MEDIA_TYPE_AUDIO_POSITIONED) {
        // it is a sound object.
        if (m->mediainfo.sound.soundid < 0) {
            m->isPlaying = 0;
        } else {
            m->isPlaying = audiomixer_IsSoundPlaying(m->mediainfo.sound.soundid);
        }
    }
}

void checkAllMediaObjectsForCleanup() {
    struct mediaobject* m = mediaObjects;
    while (m) {
        struct mediaobject* mnext = m->next;
        if (m->refcount <= 0) {
            mediaobject_UpdateIsPlaying(m);
            if (!m->isPlaying) {
                deleteMediaObject(m);
            }
        }
        m = mnext;
    } 
}

static int garbagecollect_mediaobjref(lua_State* l) {
    // we need to decrease our reference count of the
    // media object we referenced to.

    // get id reference to object
    struct luaidref* idref = lua_touserdata(l, -1);

    if (!idref || idref->magic != IDREF_MAGIC
    || idref->type != IDREF_MEDIA) {
        // either wrong magic (-> not a luaidref) or not a media object
        lua_pushstring(l, "internal error: invalid media object ref");
        lua_error(l);
        return 0;
    }

    // it is a valid media object, decrease ref count:
    struct mediaobject* o = (struct mediaobject*)idref->ref.mobj;
    o->refcount--;

    // if it's not playing and ref count is zero, remove it
    // entirely and free it:
    if (o->refcount <= 0) {
        mediaobject_UpdateIsPlaying(o);
        if (!o->isPlaying) {
            deleteMediaObject(o);
        }
    }
    return 0;
}

struct mediaobject* tomediaobject(lua_State* l, int type, int index, int arg, const char* func) {
    if (lua_type(l, index) != LUA_TUSERDATA) {
        haveluaerror(l, badargument1, arg, func, "media object", lua_strtype(l, index));
        return NULL;
    }
    if (lua_rawlen(l, index) != sizeof(struct luaidref)) {
        haveluaerror(l, badargument2, arg, func, "not a valid media object (not luaidref)");
        return NULL;
    }
    struct luaidref* idref = lua_touserdata(l, index);
    if (!idref || idref->magic != IDREF_MAGIC
    || idref->type != IDREF_MEDIA) {
        haveluaerror(l, badargument2, arg, func, "not a valid media object");
        return NULL;
    }
    struct mediaobject* o = idref->ref.mobj;
    if (o->type != type) {
        haveluaerror(l, badargument2, arg, func, "media object is of wrong type");
        return NULL;
    }
    return o;
}

