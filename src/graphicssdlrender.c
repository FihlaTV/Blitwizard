
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

#ifdef USE_SDL_GRAPHICS

//  various standard headers
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#ifdef WINDOWS
#include <windows.h>
#endif
#include <stdarg.h>

#include "logging.h"
#include "imgloader.h"
#include "timefuncs.h"
#include "hash.h"
#include "file.h"
#ifdef NOTHREADEDSDLRW
#include "main.h"
#endif

#include "SDL.h"
#include "SDL_syswm.h"

#include "graphicstexture.h"
#include "graphics.h"
#include "graphicstexturelist.h"
#include "graphicssdltexturestruct.h"

extern SDL_Window* mainwindow;
extern SDL_Renderer* mainrenderer;
extern int sdlvideoinit;

extern int graphicsactive;
extern int inbackground;
extern int graphics3d;
extern int mainwindowfullscreen;



void graphicsrender_DrawRectangle(int x, int y, int width, int height, float r, float g, float b, float a) {
    if (!graphics3d) {
        SDL_SetRenderDrawColor(mainrenderer, (int)((float)r * 255.0f),
        (int)((float)g * 255.0f), (int)((float)b * 255.0f), (int)((float)a * 255.0f));
        SDL_Rect rect;
        rect.x = x;
        rect.y = y;
        rect.w = width;
        rect.h = height;

        SDL_SetRenderDrawBlendMode(mainrenderer, SDL_BLENDMODE_BLEND);
        SDL_RenderFillRect(mainrenderer, &rect);
    }
}

int graphicsrender_DrawCropped(struct graphicstexture* gt, int x, int y, float alpha, unsigned int sourcex, unsigned int sourcey, unsigned int sourcewidth, unsigned int sourceheight, unsigned int drawwidth, unsigned int drawheight, int rotationcenterx, int rotationcentery, double rotationangle, int horiflipped, double red, double green, double blue) {
    if (alpha <= 0) {
        return 1;
    }
    if (alpha > 1) {
        alpha = 1;
    }

    // calculate source dimensions
    SDL_Rect src,dest;
    src.x = sourcex;
    src.y = sourcey;

    if (sourcewidth > 0) {
        src.w = sourcewidth;
    }else{
        src.w = gt->width;
    }
    if (sourceheight > 0) {
        src.h = sourceheight;
    }else{
        src.h = gt->height;
    }

    // set target dimensinos
    dest.x = x; dest.y = y;
    if (drawwidth == 0 || drawheight == 0) {
        dest.w = src.w;dest.h = src.h;
    }else{
        dest.w = drawwidth; dest.h = drawheight;
    }

    // render
    int i = (int)((float)255.0f * alpha);
    if (SDL_SetTextureAlphaMod(gt->sdltex, i) < 0) {
        printwarning("Warning: Cannot set texture alpha "
        "mod %d: %s\n",i,SDL_GetError());
    }
    SDL_Point p;
    p.x = (int)((double)rotationcenterx * ((double)drawwidth / src.w));
    p.y = (int)((double)rotationcentery * ((double)drawheight / src.h));
    if (red > 1) {
        red = 1;
    }
    if (red < 0) {
        red = 0;
    }
    if (blue > 1) {
        blue = 1;
    }
    if (blue < 0) {
        blue = 0;
    }
    if (green > 1) {
        green = 1;
    }
    if (green < 0) {
        green = 0;
    }
    SDL_SetTextureColorMod(gt->sdltex, (red * 255.0f),
    (green * 255.0f), (blue * 255.0f));
    if (horiflipped) {
        // draw rotated and flipped
        SDL_RenderCopyEx(mainrenderer, gt->sdltex, &src, &dest,
        rotationangle, &p, SDL_FLIP_HORIZONTAL);
    } else {
        if (rotationangle > 0.001 || rotationangle < -0.001) {
            // don't rotate the rendered image if it's barely rotated
            // (this helps the software renderer)
            SDL_RenderCopyEx(mainrenderer, gt->sdltex, &src, &dest,
            rotationangle, &p, SDL_FLIP_NONE);
        } else {
            // draw rotated
            SDL_RenderCopy(mainrenderer, gt->sdltex, &src, &dest);
        }
    }
    return 1;
}


void graphicsrender_StartFrame(void) {
    SDL_SetRenderDrawColor(mainrenderer, 0, 0, 0, 1);
    SDL_RenderClear(mainrenderer);
}

void graphicsrender_CompleteFrame(void) {
    SDL_RenderPresent(mainrenderer);
}

#endif  // USE_SDL_GRAPHICS


