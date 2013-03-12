
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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "os.h"
#include "audiosource.h"
#include "audiosourcefadepanvol.h"

struct audiosourcefadepanvol_internaldata {
    struct audiosource* source;
    int sourceeof;
    int eof;
    int returnerroroneof;

    int fadesamplestart;
    int fadesampleend;
    float fadevaluestart;
    float fadevalueend;
    int terminateafterfade;

    float pan;
    float vol;

    char processedsamplesbuf[512];
    unsigned int processedsamplesbytes;

    int noamplify; // don't amplify with soft clipping
};

static size_t audiosourcefadepanvol_Position(struct audiosource* source) {
    struct audiosourcefadepanvol_internaldata* idata = source->internaldata;
    return idata->source->position(idata->source);
}

static size_t audiosourcefadepanvol_Length(struct audiosource* source) {
    struct audiosourcefadepanvol_internaldata* idata = source->internaldata;
    return idata->source->position(idata->source);
}

static int audiosourcefadepanvol_Seek(struct audiosource* source, size_t pos) {
    struct audiosourcefadepanvol_internaldata* idata = source->internaldata;
    if (idata->eof && idata->returnerroroneof) {
        return 0;
    }
    if (idata->source->seekable) {
        // source supports seeking -> attempt to seek:
        if (idata->source->seek(idata->source, pos)) {
            // it worked!
            idata->eof = 0;
            // reset our buffer to empty:
            idata->processedsamplesbytes = 0;
            return 1;
        }
        return 0;
    } else {
        return 0;
    }
}

static void audiosourcefadepanvol_Rewind(struct audiosource* source) {
    struct audiosourcefadepanvol_internaldata* idata = source->internaldata;
    if (!idata->eof || !idata->returnerroroneof) {
        idata->source->rewind(idata->source);
        idata->sourceeof = 0;
        idata->eof = 0;
        idata->returnerroroneof = 0;
        idata->processedsamplesbytes = 0;
    }
}

static float amplify(float value, float amplification) {
    // amplifies with soft clipping applied
    float maxamplify = 2;
    float clippingstart = 0.95;
    value *= amplification;
    if (abs(value) > clippingstart) {
        float excess = abs(value)-clippingstart;
        if (value > 0) {
            value += (excess/(maxamplify-clippingstart))*(1-clippingstart);
        } else {
            value -= (excess/(maxamplify-clippingstart))*(1-clippingstart);
        }
    }
    return value;
}

static int audiosourcefadepanvol_Read(struct audiosource* source, char* buffer, unsigned int bytes) {
    struct audiosourcefadepanvol_internaldata* idata = source->internaldata;
    if (idata->eof) {
        return -1;
    }

    unsigned int byteswritten = 0;
    while (bytes > 0) {
        // see how many samples we want to have minimum
        int stereosamples = bytes / (sizeof(float) * 2);
        if (stereosamples * sizeof(float) * 2 < bytes) {
            stereosamples++;
        }

        // get new unprocessed samples
        int unprocessedstart = idata->processedsamplesbytes;
        if (!idata->sourceeof) {
            while (idata->processedsamplesbytes + sizeof(float) * 2 <= sizeof(idata->processedsamplesbuf) && stereosamples > 0) {
                int i = idata->source->read(idata->source, idata->processedsamplesbuf + idata->processedsamplesbytes, sizeof(float) * 2);
                if (i < (int)sizeof(float)*2) {
                    if (i < 0) {
                        // read function returned error
                        idata->returnerroroneof = 1;
                    }
                    idata->sourceeof = 1;
                    break;
                }else{
                    idata->processedsamplesbytes += sizeof(float)*2;
                }
                stereosamples--;
            }
        }

        // process unprocessed samples
        unsigned int i = unprocessedstart;
        float faderange = (-idata->fadesamplestart + idata->fadesampleend);
        float fadeprogress = idata->fadesampleend;
        while ((int)i <= ((int)idata->processedsamplesbytes - ((int)sizeof(float) * 2))) {
            float leftchannel = *((float*)((char*)idata->processedsamplesbuf+i));
            float rightchannel = *((float*)((float*)((char*)idata->processedsamplesbuf+i))+1);

            if (idata->fadesamplestart < 0 || idata->fadesampleend > 0) {
                // calculate fade volume
                idata->vol = idata->fadevaluestart + (idata->fadevalueend - idata->fadevaluestart)*(1 - fadeprogress/faderange);

                // increase fade progress
                idata->fadesamplestart--;
                idata->fadesampleend--;
                fadeprogress = idata->fadesampleend;

                if (idata->fadesampleend < 0) {
                    // fade ended
                    idata->vol = idata->fadevalueend;
                    idata->fadesamplestart = 0;
                    idata->fadesampleend = 0;

                    if (idata->terminateafterfade) {
                        idata->sourceeof = 1;
                        i = idata->processedsamplesbytes;
                    }
                }
            }

            // apply volume
            if (!idata->noamplify) {
                leftchannel = amplify(leftchannel, idata->vol);
                rightchannel = amplify(rightchannel, idata->vol);
            } else {
                leftchannel *= idata->vol;
                rightchannel *= idata->vol;
            }

            // calculate panning
            if (idata->pan < 0) {
                leftchannel *= (1+idata->pan);
            }
            if (idata->pan > 0) {
                rightchannel *= (1-idata->pan);
            }

            // amplify channels when closer to edges:
            float panningamplifyfactor = abs(idata->pan);
            float amplifyamount = 0.3;
            if (!idata->noamplify) {
                if (idata->pan > 0) {
                    leftchannel = amplify(leftchannel,
                    0.7 + panningamplifyfactor * amplifyamount);
                } else {
                    rightchannel = amplify(rightchannel,
                    0.7 + panningamplifyfactor * amplifyamount);
                }
            }

            // write floats back
            memcpy(idata->processedsamplesbuf+i, &leftchannel, sizeof(float));
            memcpy(idata->processedsamplesbuf+i+sizeof(float), &rightchannel, sizeof(float));

            i += sizeof(float)*2;
        }

        // return from our processed samples
        unsigned int returnbytes = bytes;
        if (returnbytes > idata->processedsamplesbytes) {
            returnbytes = idata->processedsamplesbytes;
        }
        if (returnbytes == 0) {
            if (byteswritten == 0) {
                idata->eof = 1;
                if (idata->returnerroroneof) {
                    return -1;
                }
                return 0;
            }else{
                return byteswritten;
            }
        }else{
            byteswritten += returnbytes;
            memcpy(buffer, idata->processedsamplesbuf, returnbytes);
            buffer += returnbytes;
            bytes -= returnbytes;
        }
        // move away processed & returned samples
        if (returnbytes > 0) {
            if (idata->processedsamplesbytes - returnbytes > 0) {
                memmove(idata->processedsamplesbuf, idata->processedsamplesbuf + returnbytes, sizeof(idata->processedsamplesbuf) - returnbytes);
            }
            idata->processedsamplesbytes -= returnbytes;
        }
    }
    return byteswritten;
}

static void audiosourcefadepanvol_Close(struct audiosource* source) {
    struct audiosourcefadepanvol_internaldata* idata = source->internaldata;
    if (idata) {
        // close the processed source
        if (idata->source) {
            idata->source->close(idata->source);
        }

        // free all structs
        free(idata);
    }
    free(source);
}

struct audiosource* audiosourcefadepanvol_Create(struct audiosource* source) {
    if (!source) {
        // no source given
        return NULL;
    }
    if (source->channels != 2) {
        // we only support stereo audio
        source->close(source);
        return NULL;
    }

    // allocate visible data struct
    struct audiosource* a = malloc(sizeof(*a));
    if (!a) {
        source->close(source);
        return NULL;
    }

    // allocate internal data struct
    memset(a,0,sizeof(*a));
    a->internaldata = malloc(sizeof(struct audiosourcefadepanvol_internaldata));
    if (!a->internaldata) {
        free(a);
        source->close(source);
        return NULL;
    }

    // remember various things
    struct audiosourcefadepanvol_internaldata* idata = a->internaldata;
    memset(idata, 0, sizeof(*idata));
    idata->source = source;
    idata->vol = 1; // run at full volume if not changed
    a->samplerate = source->samplerate;
    a->channels = source->channels;
    a->format = source->format;

    // function pointers
    a->read = &audiosourcefadepanvol_Read;
    a->close = &audiosourcefadepanvol_Close;
    a->rewind = &audiosourcefadepanvol_Rewind;
    a->position = &audiosourcefadepanvol_Position;
    a->length = &audiosourcefadepanvol_Length;
    a->seek = &audiosourcefadepanvol_Seek;

    // if our source is seekable, we are so too:
    a->seekable = source->seekable;

    return a;
}

void audiosourcefadepanvol_SetPanVol(struct audiosource* source, float vol, float pan, int noamplify) {
    struct audiosourcefadepanvol_internaldata* idata = source->internaldata;
    // limit panning range:
    if (pan < -1) {
        pan = -1;
    }
    if (pan > 1) {
        pan = 1;
    }
    // limit volume range:
    if (vol < 0) {
        vol = 0;
    }
    if (vol > 1.5) {
        vol = 1.5;
    }
    // the user wants to avoid excess amplification:
    if (noamplify) {
        if (vol > 1) {
            vol = 1;
        }
    }

    idata->noamplify = noamplify;
    idata->pan = pan;
    idata->vol = vol;

    idata->fadesamplestart = 0;
    idata->fadesampleend = 0;
    idata->fadevalueend = vol;
}

void audiosourcefadepanvol_StartFade(struct audiosource* source, float seconds, float targetvol, int terminate) {
    // start a fade to a specified volume
    // terminate: stop sound when fade is done
    struct audiosourcefadepanvol_internaldata* idata = source->internaldata;

    // if seconds <= 0, terminate current fade:
    if (seconds <= 0) {
        idata->fadevaluestart = 0;
        idata->fadevalueend = 0;
        idata->fadesamplestart = 0;
        idata->fadesampleend = 0;
        return;
    }
    // check target volume bounds:
    if (targetvol < 0) {
        targetvol = 0;
    }
    if (targetvol > 1.5) {
        targetvol = 1.5;
    }
    if (idata->noamplify && targetvol > 1) {
        targetvol = 1;
    }

    // set fade info:
    idata->terminateafterfade = terminate;
    idata->fadevaluestart = idata->vol;
    idata->fadevalueend = targetvol;
    idata->fadesamplestart = 0;
    idata->fadesampleend = (int)((double)((double)idata->source->samplerate) * ((double)seconds));
}
