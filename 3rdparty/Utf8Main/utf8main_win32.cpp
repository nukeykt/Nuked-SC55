/*
 * utf8main - a small wrapper over entry points
 * to provide the UTF8 encoded main function
 *
 * Copyright (c) 2017-2024 Vitaly Novichkov <admin@wohlnet.ru>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies
 * or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
 * FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#include <string>
#include <vector>

#ifdef PGE_ENGINE
#   include <SDL2/SDL_main.h>
#else
#   include "utf8main.h"
#endif

#ifndef _In_
#   define _In_
#endif
#ifndef _In_opt_
#   define _In_opt_
#endif

extern int  main(int argc, char *argv[]);

static void buildUtf8Args(std::vector<std::string> &utf8_args)
{
    //Get raw UTF-16 command line
    wchar_t    *cmdLineW = GetCommandLineW();
    int         argc     = 0;
    //Convert it into array of strings
    wchar_t   **argvW    = CommandLineToArgvW(cmdLineW, &argc);

    utf8_args.reserve(argc);
    //Convert every argument into UTF-8
    for(int i = 0; i < argc; i++)
    {
        wchar_t *argW = argvW[i];
        size_t argWlen = wcslen(argW);
        std::string arg;
        arg.resize(argWlen * 2);
        size_t newLen = WideCharToMultiByte(CP_UTF8, 0, argW, static_cast<int>(argWlen), &arg[0], static_cast<int>(arg.size()), 0, 0);
        arg.resize(newLen);
        utf8_args.push_back(arg);
    }
}

#ifdef WIN32_CONSOLE
#undef main
int main()
#   define main UTF8_Main
#else
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ PSTR, _In_ int)
#endif
{
    //! Storage of UTF8-encoded command line arguments
    std::vector<std::string>  g_utf8_args;
    //! Storage of the pointers to begin of every argument
    std::vector<char *>       g_utf8_argvV;

    buildUtf8Args(g_utf8_args);

#ifdef UTF8Main_Debug
    printf("UTF8 ARGS RAN!\n");
    fflush(stdout);
#endif

    size_t argc = g_utf8_args.size();
    g_utf8_argvV.reserve(argc);
    for(size_t i = 0; i < argc; i++)
        g_utf8_argvV.push_back(&g_utf8_args[i][0]);

    return  main((int)argc, g_utf8_argvV.data());
}
#endif
