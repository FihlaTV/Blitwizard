
/* blitwizard game engine - source code file

  Copyright (C) 2011-2014 Jonas Thiem

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
#include <assert.h>

#include "logging.h"
#include "imgloader.h"
#include "timefuncs.h"
#include "hash.h"
#include "file.h"
#ifdef NOTHREADEDSDLRW
#include "main.h"
#endif

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

#include "blitwizard_opengl.h"

#include "graphicstexture.h"
#include "graphics.h"
#include "graphicstexturelist.h"
#include "graphicssdltexturestruct.h"
#include "graphics2dsprites.h"
#include "graphicssdl.h"
#include "graphicssdlrender.h"
#include "graphics2dspritestruct.h"
#include "graphics2dspriteslist.h"
#include "graphics2dspritestree.h"

#ifdef USE_SDL_GRAPHICS_OPENGL_EFFECTS
SDL_GLContext *maincontext;
#endif
extern SDL_Window *mainwindow;
extern SDL_Renderer *mainrenderer;
extern int sdlvideoinit;

extern int graphicsactive;
extern int inbackground;
extern int graphics3d;
extern int mainwindowfullscreen;

static uint64_t renderts = 0;

void graphics_setCamera2DZoom(int index, double newzoom) {
    if (index == 0) {
        zoom = newzoom;
    }
}

double graphics_getCamera2DZoom(int index) {
    if (index != 0) {return 0;}
    return zoom;
}

void graphicsrender_drawRectangle(
        int x, int y, int width, int height,
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

static int graphicsrender_drawCropped_GL(
        struct graphicstexture *gt, int x, int y, float alpha,
        unsigned int sourcex, unsigned int sourcey,
        unsigned int sourcewidth, unsigned int sourceheight,
        unsigned int drawwidth, unsigned int drawheight,
        int rotationcenterx, int rotationcentery, double rotationangle,
        int horiflipped,
        double red, double green, double blue, int textureFiltering) {
    // source UV coords:
    double sx = sourcex / (double)gt->width;
    double sy = sourcey / (double)gt->height;
    double sw = sourcewidth / (double)gt->width;
    double sh = sourceheight / (double)gt->height;

    glPushMatrix();
    //glRotated((rotationangle / M_PI) * 180, x + drawwidth / 2,
    //    0, y + drawwidth / 2);
    if (graphicstexture_bindGl(gt, renderts)) {
        glBegin(GL_QUADS);
        //glColor3f(1, 0, 0);
        glVertex2d(x, y);
        glVertex2d(x, y + drawheight);
        glVertex2d(x + drawwidth, y + drawheight);
        glVertex2d(x + drawwidth, y);       
        glEnd();
    }
    glPopMatrix();
    return 1;
}

static int graphicsrender_drawCropped_SDL(
        struct graphicstexture *gt, int x, int y, float alpha,
        unsigned int sourcex, unsigned int sourcey,
        unsigned int sourcewidth, unsigned int sourceheight,
        unsigned int drawwidth, unsigned int drawheight,
        int rotationcenterx, int rotationcentery, double rotationangle,
        int horiflipped,
        double red, double green, double blue, int textureFiltering) {
    // calculate source dimensions
    SDL_Rect src,dest;
    src.x = sourcex;
    src.y = sourcey;
    src.w = sourcewidth;
    src.h = sourceheight;

    // set target dimensinos
    dest.x = x;
    dest.y = y;
    dest.w = drawwidth;
    dest.h = drawheight;    

    // set alpha mod:
    int i = (int)((float)255.0f * alpha);
    if (SDL_SetTextureAlphaMod(gt->sdltex, i) < 0) {
        printwarning("Warning: Cannot set texture alpha "
        "mod %d: %s\n", i, SDL_GetError());
    }

    // calculate rotation center:
    SDL_Point p;
    p.x = (int)((double)rotationcenterx * ((double)drawwidth / src.w));
    p.y = (int)((double)rotationcentery * ((double)drawheight / src.h));

    // set draw color:
    SDL_SetTextureColorMod(gt->sdltex, (red * 255.0f),
        (green * 255.0f), (blue * 255.0f));

    // set texture filter:
    if (!textureFiltering) {
        // disable texture filter
        /* SDL_SetHintWithPriority(SDL_HINT_RENDER_SCALE_QUALITY,
        "0", SDL_HINT_OVERRIDE); */

        // this doesn't work. we will need to wait for:
        // https://bugzilla.libsdl.org/show_bug.cgi?id=2167
    }

    // actual draw call:
    if (horiflipped) {
        // draw rotated and flipped
        SDL_RenderCopyEx(mainrenderer, gt->sdltex, &src, &dest,
            rotationangle, &p, SDL_FLIP_HORIZONTAL);
    } else {
        if (rotationangle > 0.001 || rotationangle < -0.001) {
            // draw rotated
            SDL_RenderCopyEx(mainrenderer, gt->sdltex, &src, &dest,
            rotationangle, &p, SDL_FLIP_NONE);
        } else {
            // don't rotate the rendered image if it's barely rotated
            // (this helps the software renderer)
            SDL_RenderCopy(mainrenderer, gt->sdltex, &src, &dest);
        }
    }
    if (!textureFiltering) {
        // re-enable texture filter
        SDL_SetHintWithPriority(SDL_HINT_RENDER_SCALE_QUALITY,
        "1", SDL_HINT_OVERRIDE);
    }
    return 1;
}

int graphicsrender_drawCropped(
        struct graphicstexture *gt, int x, int y, float alpha,
        unsigned int sourcex, unsigned int sourcey,
        unsigned int sourcewidth, unsigned int sourceheight,
        unsigned int drawwidth, unsigned int drawheight,
        int rotationcenterx, int rotationcentery, double rotationangle,
        int horiflipped,
        double red, double green, double blue, int textureFiltering) {
    if (alpha <= 0) {
        return 1;
    }
    if (alpha > 1) {
        alpha = 1;
    }
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
    if (sourcewidth == 0) {
        sourcewidth = gt->width;
    }
    if (sourceheight == 0) {
        sourceheight = gt->height;
    }
    if (drawwidth == 0 || drawheight == 0) {
        drawwidth = sourcewidth;
        drawheight = sourceheight;
    }
#ifdef USE_SDL_GRAPHICS_OPENGL_EFFECTS
    if (maincontext) {
        return graphicsrender_drawCropped_GL(gt, x, y, alpha,
            sourcex, sourcey, sourcewidth, sourceheight,
            drawwidth, drawheight, rotationcenterx, rotationcentery,
            rotationangle, horiflipped, red, green, blue, textureFiltering);
    }
#endif
    return graphicsrender_drawCropped_SDL(gt, x, y, alpha,
        sourcex, sourcey, sourcewidth, sourceheight,
        drawwidth, drawheight, rotationcenterx, rotationcentery,
        rotationangle, horiflipped, red, green, blue, textureFiltering);
}

void graphicssdlrender_startFrame(void) {
#ifdef USE_SDL_GRAPHICS_OPENGL_EFFECTS
    if (maincontext) {
        renderts = time_getMilliseconds();

        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);

        glDisable(GL_DEPTH_TEST);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();

        int actualwidth, actualheight;
        SDL_GetWindowSize(mainwindow, &actualwidth, &actualheight);
        glOrtho(0, actualwidth, actualheight, 0, -1, 1);
        return;
    }
#endif
    SDL_SetRenderDrawColor(mainrenderer, 0, 0, 0, 1);
    SDL_RenderClear(mainrenderer);
}

void graphicssdlrender_completeFrame(void) {
#ifdef USE_SDL_GRAPHICS_OPENGL_EFFECTS
    if (maincontext) {
        SDL_GL_SwapWindow(mainwindow);
        return;
    }
#endif
    SDL_RenderPresent(mainrenderer);
}

static int graphicssdlrender_spriteCallback(
        struct graphics2dsprite *sprite,
        void *userdata) {
    struct graphicstexture *tex = sprite->tex;
    if (!tex) {
        return 1;
    }
    if (!sprite->visible) {
        return 1;
    }

    double x, y, width, height;
    double sourceX, sourceY, sourceWidth, sourceHeight;
    double angle;
    int horiflip;
    graphics2dsprite_calculateSizeOnScreen(
        sprite, 0, &x, &y, &width, &height, &sourceX, &sourceY,
        &sourceWidth, &sourceHeight, &angle, &horiflip, 0);

    // render:
    graphicsrender_drawCropped(tex, x, y, sprite->alpha,
        sourceX, sourceY, sourceWidth, sourceHeight, width, height,
        sourceWidth/2, sourceHeight/2, angle, horiflip,
        sprite->r, sprite->g, sprite->b,
        sprite->textureFiltering);
    return 1;
}

void graphicsrender_draw(void) {
    graphicssdlrender_startFrame();

    graphics2dsprites_lockListOrTreeAccess();
    // render world sprites:
    double w = graphics_getCameraWidth(0);
    double h = graphics_getCameraHeight(0);
    double z = graphics_getCamera2DZoom(0);
    double cwidth = (w/UNIT_TO_PIXELS) * z;
    double cheight = (w/UNIT_TO_PIXELS) * z;
    double ctopleftx = graphics_getCamera2DCenterX(0) - cwidth/2;
    double ctoplefty = graphics_getCamera2DCenterY(0) - cheight/2;
    graphics2dspritestree_doForAllSpritesSortedBottomToTop(
        ctopleftx, ctoplefty, cwidth, cheight,
        &graphicssdlrender_spriteCallback, NULL);
    // render camera-pinned sprites:
    graphics2dspriteslist_doForAllSpritesBottomToTop(
        &graphicssdlrender_spriteCallback, NULL);
    graphics2dsprites_releaseListOrTreeAccess();

    graphicssdlrender_completeFrame();
}

#endif  // USE_SDL_GRAPHICS


