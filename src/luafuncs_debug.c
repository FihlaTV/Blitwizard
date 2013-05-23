
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

/// This namespace contains various functions to obtain information
// about the many complex subsystems working in blitwizard.
//
// This is mainly useful for blitwizard developers or if you want
// to report a back and intend to add some additional information
// to your blitwizard bug report.
//
// Your regular blitwizard game will probably not need any of the information
// available here.
// @author Jonas Thiem (jonas.thiem@gmail.com)
// @copyright 2013
// @license zlib
// @module blitwizard.debug

#include "config.h"
#include "os.h"

#include "luaheader.h"
#include "luaerror.h"
#include "graphicstexturemanager.h"
#include "luafuncs_debug.h"
#include "luafuncs_object.h"

/// Get GPU memory used for all textures loaded by blitwizard in bytes.
// @function gpuMemoryUse
// @treturn number GPU memory used up in bytes
int luafuncs_debug_getGpuMemoryUse(lua_State* l) {
    lua_pushnumber(l, texturemanager_getGpuMemoryUse());
    return 1;
}

/// Get usage information on a given texture.
// This represents an indication of how much a texture
// is currently drawn on screen.
//
// If it's not used at all, you'll get -1.
// Otherwise you'll get a number >= 0, and a smaller
// number indicates higher usage/more detail usage.
//
// This usage information is used by blitwizard to decide
// on which textures to unload.
// @function getTextureUsageInfo
// @tparam string name the file name which was used for loading the texture, e.g. "myImage.png"
int luafuncs_debug_getTextureUsageInfo(lua_State* l) {
    if (lua_type(l, 1) != LUA_TSTRING) {
        return haveluaerror(l, badargument1, 1,
        "blitwizard.debug.getTextureUsageInfo", "string",
        lua_strtype(l, 1));
    }
    lua_pushnumber(l, texturemanager_getTextureUsageInfo(lua_tostring(l, -1)));
    return 1;
}

/// Get information of whether a texture is currently
// loaded into GPU memory, and at which size.
//
// Returns -1 if the texture isn't in GPU memory at all,
// 0 if it's loaded in original size, otherwise
// a number from 1 upwards with 1 being tiny, larger number
// being larger versions.
//
// There is currently no way to influence the size
// of the loaded texture. If you use a texture prominently
// and it only shows in a scaled down version or not at all,
// please report a bug: https://github.com/JonasT/blitwizard/issues
// @function getTextureGpuSizeInfo
// @tparam string name the file name which was used when loading the texture, e.g. "myImage.png"
int luafuncs_debug_getTextureGpuSizeInfo(lua_State* l) {
    if (lua_type(l, 1) != LUA_TSTRING) {
        return haveluaerror(l, badargument1, 1,
        "blitwizard.debug.getTextureUsageInfo", "string",
        lua_strtype(l, 1));
    }
    lua_pushnumber(l,
        texturemanager_getTextureGpuSizeInfo(lua_tostring(l, -1)));
    return 1;
}

/// Get some metrics of the logic processing pipeline
// of blitwizard (see return values).
// @function getLogicStats
// @treturn number processedUniqueImportantObjects
// @treturn number processedTotalImportantObjects
// @treturn number processedUniqueNormalObjects
// @treturn number processedTotalNormalObjects
// @treturn number processedUniqueBoringObjects
// @treturn number processedTotalBoringObjects
int luafuncs_debug_getLogicStats(lua_State* l) {
    lua_pushnumber(l, processedUniqueImportantObjects);
    lua_pushnumber(l, processedTotalImportantObjects);
    lua_pushnumber(l, processedUniqueNormalObjects);
    lua_pushnumber(l, processedTotalNormalObjects);
    lua_pushnumber(l, processedUniqueBoringObjects);
    lua_pushnumber(l, processedTotalBoringObjects);
    return 6;
}

