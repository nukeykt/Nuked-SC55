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

#ifndef FILES_H
#define FILES_H

#include <string>

namespace Files
{
    FILE *utf8_fopen(const char *filePath, const char *modes);
    enum Charsets
    {
        CHARSET_UTF8 = 0,
        CHARSET_UTF16BE,
        CHARSET_UTF16LE,
        CHARSET_UTF32BE,
        CHARSET_UTF32LE
    };

    int skipBom(FILE *file, const char **charset = nullptr);
    bool fileExists(const std::string &path);
    bool dirExists(const std::string &dirPath);
    bool deleteFile(const std::string &path);
    bool copyFile(const std::string &to, const std::string &from, bool override = false);
    bool moveFile(const std::string &to, const std::string &from, bool override = false);
    bool isAbsolute(const std::string &path);
    std::string basename(std::string path);
    std::string basenameNoSuffix(std::string path);
    std::string dirname(std::string path);
    std::string real_dirname(std::string path);
    std::string changeSuffix(std::string path, const std::string& suffix);
    bool hasSuffix(const std::string &path, const std::string &suffix);
    //Appends "m" into basename of the file name before last dot
    void getGifMask(std::string &mask, const std::string &front);
    bool dumpFile(const std::string &inPath, std::string &outData);
}

#endif // FILES_H
