﻿#pragma once

#include <FLHook.h>
#include <algorithm>
#include <iostream>
#include <list>
#include <map>
#include <math.h>
#include <plugin.h>
#include <sstream>
#include <stdio.h>
#include <string>
#include <time.h>
#include <windows.h>

static int set_iPluginDebug = 0;
PLUGIN_RETURNCODE returncode;

typedef bool (*_UserCmdProc)(uint, const std::wstring &, const std::wstring &,
                             const wchar_t *);

struct USERCMD {
    wchar_t *wszCmd;
    _UserCmdProc proc;
    wchar_t *usage;
};

#define IS_CMD(a) !wscCmd.compare(L##a)
