
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

#include "config.h"
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
#include "graphics2dsprites.h"
#include "graphicssdl.h"
#include "graphicssdlrender.h"

extern SDL_Window* mainwindow;
extern SDL_Renderer* mainrenderer;
extern int sdlvideoinit;

extern int graphicsactive;
extern int inbackground;
extern int graphics3d;
extern int mainwindowfullscreen;

void graphics_SetCamera2DZoom(int index, double newzoom) {
    if (index == 0) {
        zoom = newzoom;
    }
}

double graphics_GetCamera2DZoom(int index) {
    if (index != 0) {return 0;}
    return zoom;
}

void graphicsrender_DrawRectangle(int x, int y, int width, int height,
float r, float g, float b, float a) {
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
    } else {
        src.w = gt->width;
    }
    if (sourceheight > 0) {
        src.h = sourceheight;
    } else {
        src.h = gt->height;
    }

    // set target dimensinos
    dest.x = x; dest.y = y;
    if (drawwidth == 0 || drawheight == 0) {
        dest.w = src.w;dest.h = src.h;
    } else {
        dest.w = drawwidth; dest.h = drawheight;
    }

    // render
    int i = (int)((float)255.0f * alpha);
    if (SDL_SetTextureAlphaMod(gt->sdltex, i) < 0) {
        printwarning("Warning: Cannot set texture alpha "
        "mod %d: %s\n", i, SDL_GetError());
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

void graphicssdlrender_StartFrame(void) {
    SDL_SetRenderDrawColor(mainrenderer, 0, 0, 0, 1);
    SDL_RenderClear(mainrenderer);
}

void graphicssdlrender_CompleteFrame(void) {
    SDL_RenderPresent(mainrenderer);
}

static void graphicssdlrender_spriteCallback(
const char* path, struct graphicstexture* tex,
double x, double y,
double width, double height,
size_t texWidth, size_t texHeight,
double angle,
double alpha, double r, double g, double b,
size_t sourceX, size_t sourceY,
size_t sourceWidth, size_t sourceHeight,
int visible, int cameraId) {
    if (!tex) {
        return;
    }
    if (!visible) {
        return;
    }

    // get actual texture size (texWidth, texHeight are theoretical
    // texture size of full sized original texture)
    size_t actualTexW, actualTexH;
    graphics_GetTextureDimensions(tex, &actualTexW, &actualTexH);

    // if the actual texture is upscaled or downscaled,
    // we need to take this into account:
    if ((texWidth != actualTexW || texHeight != actualTexH) &&
    (texWidth != 0 && texHeight != 0)) {
        double scalex = (double)actualTexW / (double)texWidth;
        double scaley = (double)actualTexH / (double)texHeight;

        // scale all stuff according to this:
        sourceX = scalex * (double)sourceX;
        sourceY = scaley * (double)sourceY;
        sourceWidth = scalex * (double)sourceWidth;
        sourceHeight = scaley * (double)sourceHeight;
    }

    // now override texture size:
    texWidth = actualTexW;
    texHeight = actualTexH;

    // if a camera is specified for pinning,
    // the sprite will be pinned to the screen:
    int pinnedToCamera = 0;
    if (cameraId >= 0) {
        pinnedToCamera = 1;
    }

    // evaluate special width/height:
    // negative for flipping, zero'ed etc
    int horiflip = 0;
    if (width == 0 && height == 0) {
        width = ((double)texWidth) / UNIT_TO_PIXELS;
        height = ((double)texHeight) / UNIT_TO_PIXELS;
        if (sourceWidth > 0) {
            width = ((double)sourceWidth) / UNIT_TO_PIXELS;
            height = ((double)sourceHeight) / UNIT_TO_PIXELS;
        }
    }
    if (width < 0) {
        width = -width;
        horiflip = 1;
    }
    if (height < 0) {
        angle += 180;
        horiflip = !horiflip;
    }

    // scale position according to zoom:
    if (!pinnedToCamera) {
        x *= UNIT_TO_PIXELS * zoom;
        y *= UNIT_TO_PIXELS * zoom;
    } else {
        // ignore zoom when pinned to screen
        x *= UNIT_TO_PIXELS;
        y *= UNIT_TO_PIXELS;
    }

    if (!pinnedToCamera) {
        // image center offset (when not pinned to screen):
        x -= (width/2.0) * UNIT_TO_PIXELS * zoom;
        y -= (height/2.0) * UNIT_TO_PIXELS * zoom;

        // screen center offset (if not pinned):
        unsigned int winw,winh;
        graphics_GetWindowDimensions(&winw, &winh);
        x += (winw/2.0);
        y += (winh/2.0);

        // move according to zoom, cam pos etc:
        width *= UNIT_TO_PIXELS * zoom;
        height *= UNIT_TO_PIXELS * zoom;
        x -= centerx * UNIT_TO_PIXELS * zoom;
        y -= centery * UNIT_TO_PIXELS * zoom;
    } else {
        // only adjust width/height to proper units when pinned
        width *= UNIT_TO_PIXELS;
        height *= UNIT_TO_PIXELS;
    }

    // render:
    graphicsrender_DrawCropped(tex, x, y, alpha,
    sourceX, sourceY, sourceWidth, sourceHeight, width, height,
    sourceWidth/2, sourceHeight/2, angle, horiflip,
    r, g, b);
}

void graphicsrender_Draw(void) {
    graphicssdlrender_StartFrame();

    // render sprites:
    graphics2dsprites_doForAllSprites(
    &graphicssdlrender_spriteCallback);

    graphicssdlrender_CompleteFrame();
}

#endif  // USE_SDL_GRAPHICS


