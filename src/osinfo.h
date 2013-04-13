
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

// get various system info:
const char* osinfo_GetSystemName(void);
const char* osinfo_GetSystemVersion(void);

// check whether a specific command line program is present
// (= in the environment/PATH and can be used)
int osinfo_CheckForCmdProg(const char* name);

// get a command line tool that shows a simple message string
// as graphical message box (mainly useful on Linux):
const char* osinfo_ShowMessageTool(int error);
// specify 1 if you want to show an error, 0 for an info message.

// show a simple message:
void osinfo_ShowMessage(const char* msg, int error);
// specify 1 for error message, 0 for info message.

