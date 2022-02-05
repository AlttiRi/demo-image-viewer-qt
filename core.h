#pragma once

#include <QString>
#include <QDateTime>
#include <QFileInfo>
#include <QDir>
#include <QImageReader>
#include <QElapsedTimer>
#include <QPixmap>
#include <QtConcurrent>

class Timer {
    inline static QMap<QString, QElapsedTimer> map;
public:
    static void start(QString name) {
        QElapsedTimer timer;
        timer.start();
        map.insert(name, timer);
    }
    static int elapsed(QString name) {
        if (!map.contains(name)) {
            qDebug().noquote() << "[timer][" + name + "]: Not found.";
        }
        int time = map.take(name).elapsed();
        qDebug().noquote() << "[timer][" + name + "]:" << time << "ms";
        return time;
    }
};

class SortOrders {
public:
    // if `false` — an order is reversed
    inline static bool mtime = true;
    inline static bool btime = true;
    inline static bool size  = true;
    inline static QString by = "";
};

class FileEntry {
public:
    QString   name;
    QDateTime mtime;
    QDateTime btime;
    qint64    size;
    QFileInfo fileInfo; // for tests only
    FileEntry(QFileInfo fileInfo) {
        this->name  = fileInfo.fileName();  // x1, filePath() is x3 faster, absoluteFilePath() x6 times slower
        this->mtime = fileInfo.fileTime(QFileDevice::FileModificationTime);
        this->btime = fileInfo.fileTime(QFileDevice::FileBirthTime);
        this->size  = fileInfo.size();
        this->fileInfo = fileInfo;
    }
    FileEntry() {}
    friend QDebug &operator<<(QDebug &stream, const FileEntry &target) {
        return stream << target.name;
    }
};

class Cache {
    static inline int num = 0;
    static inline QMap<QString, QFuture<QPixmap>> map;
public:    
    static void add(const QString &path) {
        if (Cache::has(path)) {
            // qDebug() << "has:" << path;
            return;
        }
        qDebug() << "cache:" << path;
        int i = num++;
        QFuture<QPixmap> future = QtConcurrent::run([path, i]() {
            Timer::start("Cache QPixmap [" + QString::number(i) + "]");
            QPixmap pixmap(path);
            Timer::elapsed("Cache QPixmap [" + QString::number(i) + "]");
            return pixmap;
        });
        map.insert(path, future);
    }
    static QPixmap get(const QString &path) {
        return map.value(path).result();
    }
    static bool has(const QString &path) {
        return map.contains(path);
    }
    static void set(const QString &path, const QPixmap &pixmap) {
        map.insert(path, QtFuture::makeReadyFuture(pixmap));
    }
    static void cacheOnly(const QList<QString> &paths) {
        QList<QString> keys = map.keys();
        for (QString &path : keys) {
            if (!paths.contains(path)) {
                map.remove(path);
            }
        }
        for (const QString &path : paths) {
            add(path);
        }
    }
};

class DirState {
public:
    QString name;
    DirState(QString name) {
        this->name = name;
    }
    bool operator==(const DirState& target) {
        return name == target.name;
    }
    friend QDebug &operator<<(QDebug &stream, const DirState &target) {
        return stream << target.name;
    }
};
class DS {
public:
    static inline DirState Empty       = DirState("Empty");
    static inline DirState Ready       = DirState("Ready");
    static inline DirState NotReady    = DirState("NotReady");
    static inline DirState Preview     = DirState("Preview");
    static inline DirState Unsupported = DirState("Unsupported");
    static inline DirState NotExists   = DirState("NotExists");
};

class DirectoryFileList {
public:
    FileEntry getSelectedFileEntry() {
        return fileEntryList.at(selectedFileEntryIndex);
    };
    QString getSelectedFileEntryPath() {
        return getPath(getSelectedFileEntry());
    };
    QString getDirPath() {
        return dirPath;
    };
    int getSelectedFileEntryIndex() {
        return selectedFileEntryIndex + 1;
    };
    int getCount() {
        return fileEntryList.count();
    };
    DirState getState() {
        return state;
    }
    bool isEmpty() {
        return getCount() == 0;
    }

    QList<QString> supportedExts = getSupportedExts();

    static QList<QString> getSupportedExts() {
        QList<QString> formats;
        for (const QByteArray &format : QImageReader::supportedImageFormats()) {
            formats << QString("." + format);
        }
        // Move the popular exts to the begin
        QList<QString> popularExts {".jpg", ".png", ".jpeg", ".gif", ".webp", ".bmp"};
        int i = 0;
        for (const QString &ext : popularExts) {
            int from = formats.indexOf(ext);
            if (from != -1) {
                formats.move(from, i++);
            }
        }
        return formats;
    }

    QList<QString> pathsRange(int left, int right) {
        int from = selectedFileEntryIndex - left;
        if (from < 0) {
            from = 0;
        }
        int to = selectedFileEntryIndex + right;
        const int count = getCount();
        if (to >= count) {
            if (count > 0) {
                to = count - 1;
            } else {
                to = 0;
            }
        }
        // qDebug() << from << to;
        QList<QString> result;
        for (int i = from; i <= to; i++) {
             result << getPath(fileEntryList.at(i));
        }
        // qDebug() << result;
        return result;
    }

private:
    QString dirPath = "";
    QList<FileEntry> fileEntryList;
    int selectedFileEntryIndex = 0;
    DirState state = DS::Empty;

    QString getPath(FileEntry entry) {
        return dirPath + "/" + entry.name;
    };

    QList<QFileInfo> getFileInfoList(QString path) {
        QDir dir(path);
        dir.setFilter(QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot);
        return dir.entryInfoList();
    }

    bool isSupportedByExt(QString &fileName, QList<QString> &extensions) {
        for (const QString &ext : extensions) {
            if (fileName.endsWith(ext, Qt::CaseInsensitive)) {
                return true;
            }
        }
        return false;
    }

    QList<QFileInfo> filterByExts(QList<QFileInfo> &fileInfoList, QList<QString> &extensions) {
        QList<QFileInfo> fileInfoListFiltered;
        for (const QFileInfo &fileInfo : fileInfoList) {
            QString fileName = fileInfo.fileName();
            if (isSupportedByExt(fileName, extensions)) {
                fileInfoListFiltered << fileInfo;
            }
        }        
        return fileInfoListFiltered;
    }

    void initFileEntryList(QList<QFileInfo> fileInfoList) {
        fileEntryList = QList<FileEntry>();
        for (QFileInfo &fileInfo : fileInfoList) {
            fileEntryList << FileEntry(fileInfo);
        }
    }

    int indexOfByFileName(QString fileName) {
        int i = 0;
        for (FileEntry &fileEntry: fileEntryList) {
            if (fileEntry.name == fileName) {
                return i;
            }
            i++;
        }
        return -1;
    }

public:
    /**
     * The first step of initialization.
     *
     * If `path` leads to an image, it creates one `FileEntry` for it and puts it in the list.
     * The return value is `true` in this case, so, you can display the image immediately.
     *
     * If `path` leads to a directory, it returns `false`.
     *
     * Then use `initFileList` to parse the directory.
     */
    DirState initImage(QString path) {

        QString inputFileName = "";
        QString inputDirPath  = "";

        QFileInfo fileInfo(path);
        if (!fileInfo.exists()) {
            return DS::NotExists;
        }
        bool isDir = fileInfo.isDir();
        if (fileInfo.isDir()) {
            inputDirPath = fileInfo.absoluteFilePath();
        } else {
            inputDirPath = fileInfo.absolutePath();
            inputFileName = fileInfo.fileName();
        }

        if (state == DS::Ready && dirPath == inputDirPath) { // For 2th+ calls of `initImage` with the same dir.
            if (isDir) {
                // Way 1:
                return state; // The same dir was opened, no need actions.
                // Way 2:
                // // ...Or go next — rescan it, maybe there were some changes (new files were added).
            } else {
                int index = indexOfByFileName(inputFileName);
                if (index != -1) {
                    selectedFileEntryIndex = index;
                    return state;
                } else {
                    bool isSupported = isSupportedByExt(inputFileName, supportedExts);
                    if (!isSupported) {
                        return state; // Just ignore it
                    }
                    // Maybe a new file was added, so, let's rescan it. Go next.
                }
            }
        }

        fileEntryList = QList<FileEntry>();
        selectedFileEntryIndex = 0;
        dirPath = inputDirPath;

        if (!isDir) {
            bool isSupported = isSupportedByExt(inputFileName, supportedExts);
            if (isSupported) {
                fileEntryList << FileEntry(QFileInfo(path));
                state = DS::Preview; // You can display the opened image now. But directory was not handled, use `initFileList` then.
                return state;
            } else {
                fileEntryList << FileEntry(QFileInfo(path)); // OK, let's try to open.
                state = DS::Unsupported;
                return state;
            }
        } else {
            state = DS::NotReady; // Need to perform `initFileList`.
            return state;
        }
    }
    /**
     * The second step of initialization.
     *
     * Handles all files in a directory.
     */
    DirState initFileList() {
        FileEntry openedImage;
        bool hasPreviewImage = state == DS::Preview;
        if (hasPreviewImage) {
            openedImage = fileEntryList.at(0);
        }

        // 111 ms
        Timer::start("entryInfoList");
        QList<QFileInfo> fileInfoList = getFileInfoList(dirPath);
        Timer::elapsed("entryInfoList");

        // 6 ms
        Timer::start("filterBySupportedExts");
        QList<QFileInfo> fileInfoListFiltered = filterByExts(fileInfoList, supportedExts);
        Timer::elapsed("filterBySupportedExts");

        qDebug() << "[filterBySupportedExts] fileInfoList.size:        " << fileInfoList.size();
        qDebug() << "[filterBySupportedExts] fileInfoListFiltered.size:" << fileInfoListFiltered.size();

        // 185 ms
        Timer::start("initFileEntryList");
        initFileEntryList(fileInfoListFiltered);
        Timer::elapsed("initFileEntryList");

//        // 6 ms
//        Timer::start("sortByMtime");
//        sortByMtime();
//        Timer::elapsed("sortByMtime");

        if (fileEntryList.length() == 0) {
            state = DS::Empty;
            return state;
        }

        if (hasPreviewImage) {
            int index = indexOfByFileName(openedImage.name);
            if (index != -1) { // if an unsupported file was opened
                fileEntryList.replace(index, openedImage);
                selectedFileEntryIndex = index;
            }
        }

        state = DS::Ready;
        return state;
    }


    void sortByMtime(bool asc = true) {
        if (fileEntryList.length() == 0) {
            return;
        }
        FileEntry selected = getSelectedFileEntry();
        std::sort(fileEntryList.begin(), fileEntryList.end(),
                  [asc](const FileEntry& a, const FileEntry& b) {
                      bool result = false;
                      if (a.mtime < b.mtime) {
                          result = true;
                      } else if (a.mtime == b.mtime) {
                          result = a.btime < b.btime;
                      }
                      return asc ? result : !result;

                      /** Try this slow shit. 3200-3600 ms vs 6 ms — up to 600 times slower! **/
                      //return a.fileInfo.lastModified() < b.fileInfo.lastModified();
                      /** That is OK: **/
                      //return a.mtime < b.mtime;
                  }
        );
        selectedFileEntryIndex = indexOfByFileName(selected.name);
    }
    void sortByBtime(bool asc = true) {
        if (fileEntryList.length() == 0) {
            return;
        }
        FileEntry selected = getSelectedFileEntry();
        std::sort(fileEntryList.begin(), fileEntryList.end(),
                  [asc](const FileEntry& a, const FileEntry& b) {
                      return asc ? a.btime < b.btime : a.btime > b.btime;
                  }
        );
        selectedFileEntryIndex = indexOfByFileName(selected.name);
    }
    void sortBySize(bool asc = true) {
        if (fileEntryList.length() == 0) {
            return;
        }
        FileEntry selected = getSelectedFileEntry();
        std::sort(fileEntryList.begin(), fileEntryList.end(),
                  [asc](const FileEntry& a, const FileEntry& b) {
                      return asc ? a.size < b.size : a.size > b.size;
                  }
        );
        selectedFileEntryIndex = indexOfByFileName(selected.name);
    }


    bool isFirst() {
        return selectedFileEntryIndex == 0;
    }
    bool isLast() {
        if (fileEntryList.length() == 0) {
            return true;
        }
        return selectedFileEntryIndex == fileEntryList.length() - 1;
    }

    bool goNext() {
        if (!isLast()) {
            selectedFileEntryIndex++;
            return true;
        }
        return false;
    }
    bool goBack() {
        if (!isFirst()) {
            selectedFileEntryIndex--;
            return true;
        }
        return false;
    }
    bool goFirst() {
        if (!isFirst()) {
            selectedFileEntryIndex = 0;
            return true;
        }
        return false;
    }
    bool goLast() {
        if (!isLast()) {
            selectedFileEntryIndex = fileEntryList.length() - 1;
            return true;
        }
        return false;
    }
};
