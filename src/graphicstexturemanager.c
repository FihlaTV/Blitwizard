
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

#include "os.h"
#include <stdlib.h>

#ifdef USE_GRAPHICS

#include "graphics.h"
#include "graphicstexturemanager.h"

struct texturerequesthandle {

};

struct texturerequesthandle* texturemanager_RequestTexture(
const char* path,
void (*textureDimensionInfo)(struct texturerequesthandle* request,
size_t width, size_t height, void* userdata),
void (*textureSwitch)(struct texturerequesthandle* request,
struct graphicstexture* texture, void* userdata),
void* userdata) {
    struct texturerequesthandle* request = malloc(sizeof(*request));
    if (!request) {
        return NULL;
    }
    return request;
}

void texturemanager_UsingRequest(
struct texturerequesthandle* request, int visibility) {

}

void texturemanager_DestroyRequest(
struct texturerequesthandle* request) {
    if (!request) {
        return;
    }

    free(request);
}

void texturemanager_InvalidateTextures() {

}

#endif  // USE_GRAPHICS


