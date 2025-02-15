#pragma once

#include <sal.h> // just for styding, not used globally in the project
#include <string>

// target Windows 7 or later
#define _WIN32_WINNT 0x0601
#include <sdkddkver.h>
// The following #defines disable a bunch of unused windows stuff.
#ifndef FULL_WINTARD
#define WIN32_LEAN_AND_MEAN
#define NOGDICAPMASKS
#define NOSYSMETRICS
#define NOMENUS
#define NOICONS
#define NOSYSCOMMANDS
#define NORASTEROPS
#define OEMRESOURCE
#define NOATOM
#define NOCLIPBOARD
#define NOCOLOR
#define NOCTLMGR
#define NODRAWTEXT
#define NOKERNEL
//#define NONLS
//#define NOMEMMGR
//#define NOMETAFILE
//#define NOOPENFILE
//#define NOSCROLL
//#define NOSERVICE
//#define NOSOUND
//#define NOTEXTMETRIC
//#define NOWH
//#define NOCOMM
//#define NOKANJI
//#define NOHELP
//#define NOPROFILER
//#define NODEFERWINDOWPOS
//#define NOMCX
//#define NORPC
//#define NOPROXYSTUB
//#define NOIMAGE
//#define NOTAPE
//#define STRICT
#endif
#define NOMINMAX

#include <Windows.h>

static std::wstring StringToWString(const std::string& s) {
    int len;
    int slength = static_cast<int>(s.length()) + 1;
    len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0);
    std::wstring r(len, L'\0');
    MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, &r[0], len);
    return r;
}

