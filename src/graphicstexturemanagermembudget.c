
/* blitwizard game engine - source code file

  Copyright (C) 2014 Jonas Thiem

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

#include <stdint.h>

// texture system memory budget in megabyte:
uint64_t textureSysMemoryBudgetMin = 100;
uint64_t texturesysMemoryBudgetMax = 200;

// texture gpu memory budget in megabyte:
uint64_t textureGpuMemoryBudgetMin = 50;
uint64_t textureGpuMemoryBudgetMax = 100;

// actual resource use:
uint64_t sysMemUse = 0;
uint64_t gpuMemUse = 0;

int texturemanager_saveGPUMemory(void) {
    return 0;
    double current = (gpuMemUse / (1024 * 1024));
    double m1 = textureGpuMemoryBudgetMin;
    double m2 = textureGpuMemoryBudgetMax;
    if (current > m1 * 0.8) {
        if (current > m1 + (m2-m1)/2) {
            // we should save memory more radically,
            // approaching the upper limit
            return 2;
        }
        return 1;
    }
    return 0;
}

int texturemanager_saveSystemMemory(void) {
    return 0;
}

