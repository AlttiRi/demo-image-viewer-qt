#pragma once

#include <QString>
#include "fileapi.h"


namespace WIN {
    QString longFilePathA(QString filepath, DWORD cchBuffer = 512);
    QString longFilePathW(QString filepath, DWORD cchBuffer = 512);
}

/*

#ifdef _WIN32
#else
#endif

https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getfullpathnamea

https://stackoverflow.com/questions/3277717/c-winapi-handling-long-file-paths-names



void FromNamespacedPath(std::string* path) {
https://github.com/nodejs/node/blob/a3970092693785f73161a80e81411fd043535eaa/src/node_file.cc#L703

  toNamespacedPath(path) {
https://github.com/nodejs/node/blob/69f487efc734f2b9bb67f46f99cd98e1c1e13d9b/lib/path.js#L618

    ret = GetFullPathNameW(req->file.pathw,
https://github.com/nodejs/node/blob/b6b65101873c32655c8d71b4d73363d624f58770/deps/uv/src/win/fs.c#L2747


https://doc.qt.io/qt-5/qdir.html#toNativeSeparators


GetFileTime
https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getfiletime






*/
