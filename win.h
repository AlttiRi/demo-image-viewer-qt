#pragma once

#include <QString>
#include "fileapi.h"


namespace WIN {
    QString longFilePathA(QString filepath, DWORD cchBuffer = 512);
    QString longFilePathW(QString filepath, DWORD cchBuffer = 512);
}
