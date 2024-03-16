/*
 * A small crossplatform set of file manipulation functions.
 * All input/output strings are UTF-8 encoded, even on Windows!
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

#include "files.h"
#include <stdio.h>
#include <locale>

#ifdef _WIN32
#include <windows.h>
#include <shlwapi.h>
#include <algorithm>

static std::wstring Str2WStr(const std::string &path)
{
    std::wstring wpath;
    wpath.resize(path.size());
    int newlen = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), static_cast<int>(path.length()), &wpath[0], static_cast<int>(path.length()));
    wpath.resize(newlen);
    return wpath;
}

static std::string WStr2Str(const std::wstring &wstr)
{
    std::string dest;
    dest.resize((wstr.size() * 2));
    int newlen = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.length()), &dest[0], static_cast<int>(dest.size()), NULL, NULL);
    dest.resize(newlen);
    return dest;
}
#else
#if defined(__ANDROID__)
#   include <SDL2/SDL_rwops.h>
#endif
#include <unistd.h>
#include <fcntl.h>         // open
#include <string.h>
#include <sys/stat.h>      // fstat
#include <sys/types.h>     // fstat
#include <cstdio>          // BUFSIZ
#include <dirent.h>        // opendir
#endif

#if defined(__CYGWIN__) || defined(__DJGPP__) || defined(__MINGW32__)
#   define IS_PATH_SEPARATOR(c) (((c) == '/') || ((c) == '\\'))
#else
#   define IS_PATH_SEPARATOR(c) ((c) == '/')
#endif

static char fi_path_dot[] = ".";
static char fi_path_root[] = "/";

static char *fi_basename(char *s)
{
    char *rv;

    if(!s || !*s)
        return fi_path_dot;

    rv = s + strlen(s) - 1;

    do
    {
        if(IS_PATH_SEPARATOR(*rv))
            return rv + 1;
        --rv;
    }
    while(rv >= s);

    return s;
}

static char *fi_dirname(char *path)
{
    char *p;

    if(path == NULL || *path == '\0')
        return fi_path_dot;

    p = path + strlen(path) - 1;
    while(IS_PATH_SEPARATOR(*p))
    {
        if(p == path)
            return path;
        *p-- = '\0';
    }

    while(p >= path && !IS_PATH_SEPARATOR(*p))
        p--;

    if(p < path)
        return fi_path_dot;

    if(p == path)
        return fi_path_root;

    *p = '\0';
    return path;
}

FILE *Files::utf8_fopen(const char *filePath, const char *modes)
{
#ifndef _WIN32
    return ::fopen(filePath, modes);
#else
    wchar_t wfile[MAX_PATH + 1];
    wchar_t wmode[21];
    int wfile_len = (int)strlen(filePath);
    int wmode_len = (int)strlen(modes);
    wfile_len = MultiByteToWideChar(CP_UTF8, 0, filePath, wfile_len, wfile, MAX_PATH);
    wmode_len = MultiByteToWideChar(CP_UTF8, 0, modes, wmode_len, wmode, 20);
    wfile[wfile_len] = L'\0';
    wmode[wmode_len] = L'\0';
    return ::_wfopen(wfile, wmode);
#endif
}

int Files::skipBom(FILE* file, const char** charset)
{
    char buf[4];
    auto pos = ::ftell(file);

    // Check for a BOM marker
    if(::fread(buf, 1, 4, file) == 4)
    {
        if(::memcmp(buf, "\xEF\xBB\xBF", 3) == 0) // UTF-8 is only supported
        {
            if(charset)
                *charset = "[UTF-8 BOM]";
            ::fseek(file, pos + 3, SEEK_SET);
            return CHARSET_UTF8;
        }
        // Unsupported charsets
        else if(::memcmp(buf, "\xFE\xFF", 2) == 0)
        {
            if(charset)
                *charset = "[UTF16-BE BOM]";
            ::fseek(file, pos + 2, SEEK_SET);
            return CHARSET_UTF16BE;
        }
        else if(::memcmp(buf, "\xFF\xFE", 2) == 0)
        {
            if(charset)
                *charset = "[UTF16-LE BOM]";
            ::fseek(file, pos + 2, SEEK_SET);
            return CHARSET_UTF16LE;
        }
        else if(::memcmp(buf, "\x00\x00\xFE\xFF", 4) == 0)
        {
            if(charset)
                *charset = "[UTF32-BE BOM]";
            ::fseek(file, pos + 4, SEEK_SET);
            return CHARSET_UTF32BE;
        }
        else if(::memcmp(buf, "\x00\x00\xFF\xFE", 4) == 0)
        {
            if(charset)
                *charset = "[UTF32-LE BOM]";
            ::fseek(file, pos + 4, SEEK_SET);
            return CHARSET_UTF32LE;
        }
    }

    if(charset)
        *charset = "[NO BOM]";

    // No BOM detected, seek to begining of the file
    ::fseek(file, pos, SEEK_SET);

    return CHARSET_UTF8;
}

bool Files::fileExists(const std::string &path)
{
#if defined(_WIN32)
    std::wstring wpath = Str2WStr(path);
    return PathFileExistsW(wpath.c_str()) == TRUE;

#elif defined(__ANDROID__)
    SDL_RWops *ops = SDL_RWFromFile(path.c_str(), "rb");
    if(ops)
    {
        SDL_RWclose(ops);
        return true;
    }

    return false;

#else
    FILE *ops = fopen(path.c_str(), "rb");
    if(ops)
    {
        fclose(ops);
        return true;
    }
    return false;
#endif
}

bool Files::dirExists(const std::string& dirPath)
{
#if _WIN32
    DWORD ftyp = GetFileAttributesW(Str2WStr(dirPath).c_str());
    if(ftyp == INVALID_FILE_ATTRIBUTES)
        return false;   //something is wrong with your path!
    if(ftyp & FILE_ATTRIBUTE_DIRECTORY)
        return true;    // this is a directory!
    return false;       // this is not a directory!
#else
    DIR *dir = opendir(dirPath.c_str());
    if(dir)
    {
        closedir(dir);
        return true;
    }
    else
        return false;
#endif
}

bool Files::deleteFile(const std::string &path)
{
#ifdef _WIN32
    std::wstring wpath = Str2WStr(path);
    return (DeleteFileW(wpath.c_str()) == TRUE);
#else
    return ::unlink(path.c_str()) == 0;
#endif
}

bool Files::copyFile(const std::string &to, const std::string &from, bool override)
{
    if(!override && fileExists(to))
        return false;// Don't override exist target if not requested

    bool ret = true;

#ifdef _WIN32

    std::wstring wfrom  = Str2WStr(from);
    std::wstring wto    = Str2WStr(to);
    ret = (bool)CopyFileW(wfrom.c_str(), wto.c_str(), !override);

#else

    char    buf[BUFSIZ];
    ssize_t size;
    ssize_t sizeOut;

    int source  = open(from.c_str(), O_RDONLY, 0);
    if(source == -1)
        return false;

    int dest    = open(to.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0640);
    if(dest == -1)
    {
        close(source);
        return false;
    }

    while((size = read(source, buf, BUFSIZ)) > 0)
    {
        sizeOut = write(dest, buf, static_cast<size_t>(size));
        if(sizeOut != size)
        {
            ret = false;
            break;
        }
    }

    close(source);
    close(dest);
#endif

    return ret;
}

bool Files::moveFile(const std::string& to, const std::string& from, bool override)
{
    bool ret = copyFile(to, from, override);
    if(ret)
        ret &= deleteFile(from);
    return ret;
}


std::string Files::dirname(std::string path)
{
    char *p = strdup(path.c_str());
    char *d = ::fi_dirname(p);
    path = d;
    free(p);
    return path;
}

#if _WIN32
template<class CHAR>
static inline void delEnd(std::basic_string<CHAR> &dirPath, CHAR ch)
{
    if(!dirPath.empty())
    {
        CHAR last = dirPath[dirPath.size() - 1];
        if(last == ch)
            dirPath.resize(dirPath.size() - 1);
    }
}
#endif

std::string Files::real_dirname(std::string path)
{
#if _WIN32
    std::wstring pathw = Str2WStr(path);
    wchar_t fullPath[MAX_PATH];
    GetFullPathNameW(pathw.c_str(), MAX_PATH, fullPath, NULL);
    pathw = fullPath;
    //Force UNIX paths
    std::replace(pathw.begin(), pathw.end(), L'\\', L'/');
    path = WStr2Str(pathw);
    delEnd(path, '/');
#else
#   ifdef FILES_HAS_REALPATH
    char rp[PATH_MAX];
    memset(rp, 0, PATH_MAX);
    char* realPath = realpath(path.c_str(), rp);
    (void)realPath;

    if(strlen(rp) > 0)
        path = rp;
    // If failed, keep as-is
#   endif
#endif

    return Files::dirname(path);
}

std::string Files::basename(std::string path)
{
    char *p = strdup(path.c_str());
    char *d = ::fi_basename(p);
    path = d;
    free(p);
    return path;
}

std::string Files::basenameNoSuffix(std::string path)
{
    char *p = strdup(path.c_str());
    char *d = ::fi_basename(p);
    path = d;
    free(p);
    std::string::size_type dot = path.find_last_of('.');
    if(dot != std::string::npos)
        path.resize(dot);
    return path;
}


std::string Files::changeSuffix(std::string path, const std::string &suffix)
{
    size_t pos = path.find_last_of('.');// Find dot
    if((path.size() < suffix.size()) || (pos == std::string::npos))
        path.append(suffix);
    else
        path.replace(pos, suffix.size(), suffix);
    return path;
}

bool Files::hasSuffix(const std::string &path, const std::string &suffix)
{
    if(suffix.size() > path.size())
        return false;

    std::locale loc;
    std::string f = path.substr(path.size() - suffix.size(), suffix.size());
    for(char &c : f)
        c = std::tolower(c, loc);
    return (f.compare(suffix) == 0);
}


bool Files::isAbsolute(const std::string& path)
{
    bool firstCharIsSlash = (path.size() > 0) ? path[0] == '/' : false;
#ifdef _WIN32
    bool containsWinChars = (path.size() > 2) ? (path[1] == ':') && ((path[2] == '\\') || (path[2] == '/')) : false;
    if(firstCharIsSlash || containsWinChars)
    {
        return true;
    }
    return false;
#else
    return firstCharIsSlash;
#endif
}

void Files::getGifMask(std::string& mask, const std::string& front)
{
    mask = front;
    //Make mask filename
    size_t dotPos = mask.find_last_of('.');
    if(dotPos == std::string::npos)
        mask.push_back('m');
    else
        mask.insert(mask.begin() + dotPos, 'm');
}

bool Files::dumpFile(const std::string& inPath, std::string& outData)
{
    off_t end;
    bool ret = true;
    FILE *in = Files::utf8_fopen(inPath.c_str(), "rb");
    if(!in)
        return false;

    outData.clear();

    std::fseek(in, 0, SEEK_END);
    end = std::ftell(in);
    std::fseek(in, 0, SEEK_SET);

    outData.resize(end);

    if(std::fread((void*)outData.data(), 1, outData.size(), in) != outData.size())
        ret = false;

    std::fclose(in);

    return ret;
}
