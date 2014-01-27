
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

// This function will be used by the image (imgloader.c). You are advised to use the imgloader, not this function directly.
int pngloader_loadRGBA(const char* pngdata, unsigned int pngdatasize,
char** imagedata, unsigned int* imagedatasize,
int (*callbackSize)(size_t imagewidth, size_t imageheight, void* userdata),
void* userdata,
int maxwidth, int maxheight);
// The size callback will be called rather early (from the same thread),
// and then lateron the function will return and you will  be
// provided with the whole data.

