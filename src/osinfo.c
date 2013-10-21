
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
#include <stdio.h>
#include "osinfo.h"
#include "file.h"
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

static char versionbuf[512] = "";

#ifdef WINDOWS
#include <windows.h>
static char staticwindowsbuf[] = "Windows";
static int osinfo_DetectWine(void) {
    HMODULE lib = GetModuleHandle("ntdll.dll");
    if (lib) {
        void* p = (void*)GetProcAddress(lib, "wine_get_version");
        if (p) {
            return 1;
        }
        FreeLibrary(lib);
    }
    return 0;
}
static char wineversionbuf[] = "";
static const char* osinfo_GetWineVersion(void) {
    if (strlen(wineversionbuf) > 0) {
        return wineversionbuf;
    }

    return wineversionbuf;
}
const char* osinfo_GetSystemName(void) {
    return staticwindowsbuf;
}
const char* osinfo_GetSystemVersion(void) {
    if (strlen(versionbuf) > 0) {
        return versionbuf;
    }
    OSVERSIONINFO osvi;
    memset(&osvi, 0, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&osvi);

    char ntorwine[20] = "NT";
    if (osinfo_DetectWine()) {
        strcpy(ntorwine, "Wine");
    }

    snprintf(versionbuf,sizeof(versionbuf),"%s/%d.%d",ntorwine,(int)osvi.dwMajorVersion,(int)osvi.dwMinorVersion);
    versionbuf[sizeof(versionbuf)-1] = 0;
    return versionbuf;
}
#else

#ifdef MAC
#include <CoreServices/CoreServices.h>
#endif
#ifdef LINUX
#include <sys/utsname.h>
#endif

#if defined(__linux) || defined(linux)
int filecontains(const char* file, const char* name) {
    char filecontents[64];
    FILE* r = fopen(file, "r");
    if (r) {
        int i = fread(filecontents,1,sizeof(filecontents)-1,r);
        fclose(r);
        if (i > 0) {
            if (i > (int)sizeof(filecontents)-1) {
                i = (int)sizeof(filecontents)-1;
            }
            filecontents[i] = 0;
            if (i < (int)strlen(name)) {
                return 0;
            }
            int j = 0;
            while (j < i-(int)strlen(name)) {
                int r = j;
                int matched = 1;
                while (r < i && r < (int)strlen(name)+j) {
                    if (toupper(filecontents[r]) != toupper(name[r-j])) {
                        matched = 0;
                    }
                    r++;
                }
                if (matched) {
                    break;
                }
                j++;
            }
            return 1;
        }
    }
    return 0;
}
static char distribuf[64] = "";
static const char* osinfo_GetDistributionName(void) {
    if (strlen(distribuf) > 0) {
        return distribuf;
    }
    if (file_DoesFileExist("/etc/redhat-release")) {
        if (filecontains("/etc/redhat-release", "Red Hat")) {
            strcpy(distribuf, "Red Hat");
        }
        if (filecontains("/etc/redhat-release", "Fedora")) {
            strcpy(distribuf, "Fedora");
        }
    }
    if (filecontains("/etc/SuSE-release", "SUSE")) {
        strcpy(distribuf, "SUSE");
    }
    if (filecontains("/etc/SUSE-release", "SUSE")) {
        strcpy(distribuf, "SUSE");
    }
    if (file_DoesFileExist("/etc/debian_version")) {
        // check for derivates:
        if (filecontains("/etc/lsb-release", "Ubuntu")) {
            strcpy(distribuf, "Ubuntu");
        }
        if (filecontains("/etc/lsb-release", "Mint")) {
            strcpy(distribuf, "LinuxMint");
        }
        // otherwise, assume debian
        if (strlen(distribuf) <= 0) {
            strcpy(distribuf, "Debian");
        }
    }
    if (filecontains("/etc/mandrake-release", "Mandrake")) {
        strcpy(distribuf,"Mandrake");
    }
    if (strlen(distribuf) <= 0) {
        strcpy(distribuf,"Generic");
    }
    return distribuf;
}
#endif

const char* osinfo_GetSystemVersion(void) {
    // print out detailed system version
    if (strlen(versionbuf) > 0) {
        return versionbuf;
    }
#ifdef MAC
    // Mac OS X:
    SInt32 majorVersion,minorVersion,bugfixVersion;

    Gestalt(gestaltSystemVersionMajor, &majorVersion);
    Gestalt(gestaltSystemVersionMinor, &minorVersion);
    Gestalt(gestaltSystemVersionBugFix, &bugfixVersion);

    snprintf(versionbuf,sizeof(versionbuf),"%d.%d.%d",majorVersion,minorVersion,bugfixVersion);
    versionbuf[sizeof(versionbuf)-1] = 0;
    return versionbuf;
#endif
#ifdef LINUX
    // Linux:
    struct utsname b;
    memset(&b, 0, sizeof(b));
    if (uname(&b) != 0) {
        snprintf(versionbuf, sizeof(versionbuf), "%s/Unknown kernel info", osinfo_GetDistributionName());
        versionbuf[sizeof(versionbuf)-1] = 0;
        return versionbuf;
    }
    snprintf(versionbuf, sizeof(versionbuf),"%s/%s", osinfo_GetDistributionName(),b.release);
    versionbuf[sizeof(versionbuf)-1] = 0;
    return versionbuf;
#endif
#ifdef ANDROID
    // Android:
    strcpy(versionbuf,"Unknown system version");
    return versionbuf;
#endif
    // All others:
    strcpy(versionbuf,"Unknown system version");
    return versionbuf;
}

static char osbuf[64] = "";
const char* osinfo_GetSystemName(void) {
    if (strlen(osbuf) > 0) {
        return osbuf;
    }
#ifdef MAC
    strcpy(osbuf, "Mac OS X");
#endif
#if defined(__FreeBSD__)
    strcpy(osbuf, "FreeBSD");
#endif
#if defined(__NetBSD__)
    strcpy(osbuf, "NetBSD");
#endif
#if defined(__OpenBSD__)
    strcpy(osbuf, "OpenBSD");
#endif
#ifdef LINUX
    strcpy(osbuf, "Linux");
#endif
#if defined(sun) || defined(__sun)
    strcpy(osbuf, "Solaris");
#endif
#if defined(__BEOS__)
    strcpy(osbuf, "BeOS");
#endif
#ifdef ANDROID
    strcpy(osbuf, "Android");
#endif
    if (strlen(osbuf) <= 0) {
        strcpy(osbuf,"Unknown Unix/POSIX");
    }
    return osbuf;
}
#endif

int osinfo_CheckForCmdProg(const char* name) {
    // cancel if supplied name is weird:
    if (strstr(name, " ") || strstr(name, "\n") ||
    strstr(name, "\"") || strstr(name, "\r") || strstr(name, "'")
    || strstr(name, "\t") || strstr(name, ":") || strstr(name, "/")
    || strstr(name, "\\") || strstr(name, "?") || strstr(name, "$")
    || strstr(name, "`") || strstr(name, "(") || strstr(name, ")")) {
        return 0;
    }

    // check if command line tool is present:
#ifdef WINDOWS
    if (!file_DoesFileExist(name) || file_IsDirectory(name)) {
        // require tool to be present in current folder for windows for now
        return 0;
    }
    return 1;
#else
    // check whereis output for slash (=some path found):
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "whereis \"%s\"", name);
    cmd[strlen(cmd)-1] = '"';
    FILE* f = popen(cmd, "r");
    if (!f) {
        return 0;
    }
    char resultbuf[256];
    char* s = fgets(resultbuf, sizeof(resultbuf), f);
    fclose(f);
    if (s && strstr(s, "/")) {
        return 1;
    }
    return 0;
#endif
}

const char* osinfo_ShowMessageTool(int error) {
#ifdef UNIX
    if (osinfo_CheckForCmdProg("gxmessage")) {
        if (error) {
            return "gxmessage -ontop -title \"Error\" ";
        } else {
            return "gxmessage -ontop -title \"Information\" ";
        }
    } else {
        if (osinfo_CheckForCmdProg("zenity")) {
            if (error) {
                return "zenity --error --no-markup --no-wrap --height=300 --text=";
            } else {
                return "zenity --info --no-markup --no-wrap --height=300 --text=";
            }
        } else {
            if (osinfo_CheckForCmdProg("xmessage")) {
                return "xmessage ";
            }
            return NULL;
        }
    }
#else
    return NULL;
#endif
}

// we have this function just locally so we can use a static buffer.
// while that would be a bad idea normally, this function
// might be used from osinfo_ShowMessage(), which might be used
// under conditions where malloc() might not work well (segfault handler).
static char escapedstrbuf[4096];
static const char* escapedosinfostr(const char* str) {
    if (!str) {
        return NULL;
    }

    size_t i = 0;
    while (*str && i < sizeof(escapedstrbuf)-2) {
        if (*str == '"') {
            escapedstrbuf[i] = '\\';
            escapedstrbuf[i+1] = '\"';
            i += 2;
        } else if (*str == '\\') {
            escapedstrbuf[i] = '\\';
            escapedstrbuf[i+1] = '\\';
            i += 2;
        } else {
            escapedstrbuf[i] = *str;
            i++;
        }
        str++;
    }
    escapedstrbuf[i] = 0;
    return escapedstrbuf;
}

void osinfo_ShowMessage(const char* msg, int error) {
#ifndef WINDOWS
    // for linux, we use some external command line tools
    // to do this. this allows us to avoid linking GTK+
    // or other GUI libs for simply showing error dialogs.

    // examine if we got a useful message showing tool:
    const char* tool = osinfo_ShowMessageTool(error);
    if (!tool) {
        // we cannot show a message.
        return;
    }

    // compose cmd beginning:
    char cmdbegin[64];
    snprintf(cmdbegin, sizeof(cmdbegin), "%s\"", tool);

    // copy message to stack to avoid strdup/malloc:
    char s[4096];
    memcpy(s, cmdbegin, strlen(cmdbegin));
    const char* escapedmsg = escapedosinfostr(msg);
    size_t i = strlen(escapedmsg);
    if (i >= sizeof(s)-(strlen(cmdbegin)+2)) {
        i = sizeof(s)-(3+strlen(cmdbegin));
    }
    memcpy(s+strlen(cmdbegin), escapedmsg, i);
    s[i+strlen(cmdbegin)] = 0;

    // make sure it ends with \"\n:
    i = strlen(s);
    s[i] = '"';
    s[i+1] = '\n';
    s[i+2] = 0;

    // run command:
    system(s);
#else
    // on windows, use a simple message box:
    if (error) {
        MessageBox(NULL, msg, "Error", MB_ICONERROR);
    } else {
        MessageBox(NULL, msg, "Information", MB_ICONINFORMATION);
    }
#endif
}

