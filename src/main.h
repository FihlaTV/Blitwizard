
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

void main_initAudio(void);
void main_Quit(int returncode);
void* main_DefaultPhysics2dPtr(void);  // pointer to 2d physics world
void* main_DefaultPhysics3dPtr(void);  // pointer to 3d physics world
void main_SetTimestep(int timestep);
extern char* templatepath;  // template path as determined at runtime
extern char* gameluapath;  // loaded game.lua path as determined at runtime
