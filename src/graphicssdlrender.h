
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

#ifndef BLITWIZARD_GRAPHICSSDLRENDER_H_
#define BLITWIZARD_GRAPHICSSDLRENDER_H_

void graphicsrender_DrawRectangle();

int graphicssdlrender_DrawCropped(const char* texname, int x, int y, float alpha, unsigned int sourcex, unsigned int sourcey, unsigned int sourcewidth, unsigned int sourceheight, unsigned int drawwidth, unsigned int drawheight, int rotationcenterx, int rotationcentery, double rotationangle, int horiflipped, double red, double green, double blue);
// Draw a texture cropped. Returns 1 on success, 0 when there is no such texture.

int graphicssdlrender_Draw(const char* texname, int x, int y, float alpha, unsigned int drawwidth, unsigned int drawheight, int rotationcenterx, int rotationcentery, double rotationangle, int horiflipped, double red, double green, double blue);
// Draw a texture. Returns 1 on success, 0 when there is no such texture.

void graphicssdlrender_DrawRectangle(int x, int y, int width, int height, float r, float g, float b, float a);
// Draw a colored rectangle.

void graphicssdlrender_StartFrame();
// Begin drawing a new frame

void graphicssdlrender_CompleteFrame();
// Complete frame & show it on the screen

#endif  // BLITWIZARD_GRAPHICSSDLRENDER_H_


