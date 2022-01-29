#include "win.h"

QString WIN::longFilePathA(QString filepath, DWORD cchBuffer)
{
    std::string stdString = filepath.toStdString();
    LPCSTR lpszShortPath = stdString.c_str();
    auto lpszLongPath = std::make_unique<CHAR[]>(cchBuffer);

    DWORD length = GetLongPathNameA(lpszShortPath, lpszLongPath.get(), cchBuffer);

    if (length > cchBuffer) {
        return WIN::longFilePathA(filepath, length);
    }

    QString result = QString(lpszLongPath.get());
    return result;
}

QString WIN::longFilePathW(QString filepath, DWORD cchBuffer)
{
    auto lpszShortPath = reinterpret_cast<LPCWSTR>(filepath.utf16());
    auto lpszLongPath = std::make_unique<WCHAR[]>(cchBuffer);

    DWORD length = GetLongPathNameW(lpszShortPath, lpszLongPath.get(), cchBuffer);

    if (length > cchBuffer) {
        return WIN::longFilePathW(filepath, length);
    }

    QString result = QString::fromStdWString(lpszLongPath.get());
    return result;
}
