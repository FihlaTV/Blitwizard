
/* blitwizard 2d engine - source code file

  Copyright (C) 2011 Jonas Thiem

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

int audio_Init(void*(*samplecallback)(unsigned int bytes), unsigned int buffersize, const char* backend, char** error);
//Initialise audio. Returns 1 on success, 0 on error (in which case
//error will be modified so *error points at an error message you
//must free() yourself).
//Pass either 0/NULL to buffersize/backend, or:
// buffersize: preferred sound buffer size (lower = less stable but less latency)
// backend: available alternative backends like "waveout" on windows.
//Wants to be fed with 16bit signed int stereo audio.
//The raw audio data will be requested through the callback.
//The amount of requested bytes is not guaranteed to be fitting to the samples.

void audio_441to480(const char* inputbuf, char* outputbuf, int channels);
//Write from a 441*2*channels byte buffer converted to a 480*2*channels byte buffer
//with a linear sample rate conversion. Assumes 16bit signed int audio.
