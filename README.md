# demo-image-viewer

It's a **demonstration** only image viewer made with Qt6.
[The binaries are available](https://github.com/AlttiRi/demo-image-viewer/releases).

![Screenshot](https://user-images.githubusercontent.com/16310547/151670175-122043d2-725b-4997-9669-9a147a56d1b2.png)

---

- Opens an image in a time independent of the count of images in a directory where it is located.
- Decodes 8.3  file names to long file names with Win API (`GetLongPathNameW`). A 8.3 can be faced on a user input from Windows Exlorer:
on Drag'n'Drop and when a user opens an image with a double click (the image path is passed as a command line argument to the program.)
- Handles images of the directory in a separated thread with `QtConcurrent::run`.
- Handles the directory only once, until it is changes. (For example, drag'n'dropping a file from the same directore will not trigger the directory parsing, since it's no needed to do, just find the file by name in the list structure.)
- Sorts by mtime, btime, size.
- Updates the image position (in the title) on the sorting change.
- All long time taking operations log the time in the console with `qDebug()`.

Opening of the next images is for O(n) (It depends of a image size: 20 ms for 150 KB, 170 ms for 10 MB, 700 ms for 36 MB). 
So the adjacent images should be preloaded in a separate thread, but I did not implemented that in this _demo_ program. 

---

Here is the example of the console log, after the program is started (~70 ms):

The speed results for 350 KB image with a directory with 30000+ files (Run it with QT Creator to look at the `qDebug()` logs):
- _[timer][displayImage]_: **84 ms** — time to display the image.

**At this moment the image is visible.**

That is performed in a background thread:
- _[timer][entryInfoList]_: **143 ms** — time to get the list with QFileInfo of all files (31973) in the directory
- _[timer][filterBySupportedExts]_: **6 ms** — time filter that list to exlude unsupported files
- _[filterBySupportedExts] fileInfoList.size_:         **31973** 
- _[filterBySupportedExts] fileInfoListFiltered.size_: **30006**
- _[timer][initFileEntryList]_: **200 ms**  — time to create a list of data structure with all necessary information for all filtered files (30006)
- _[timer][initFileList]_: **352 ms** — the **total time** of the work with directory files ~(143 + 6 + 200)
- _[timer][sortByMtime]_: **6 ms** — to sort all files by mtime

---

Note: Do **NOT** use code that calls methods of `QFileInfo` in `std::sort`. **It's incredibly slow. **

That is up to **600 times slower**
```c++
        std::sort(fileEntryList.begin(), fileEntryList.end(),
                  [](const FileEntry& a, const FileEntry& b) {
                      return a.fileInfo.lastModified() < b.fileInfo.lastModified();
                  }
        );
```
than that
```c++
        std::sort(fileEntryList.begin(), fileEntryList.end(),
                  [](const FileEntry& a, const FileEntry& b) {
                      return a.mtime < b.mtime;
                  }
        );
```

**3200-3600 ms** vs **6 ms**.

So, store all required fields of `QFileInfo` in your own data struct.

Try by yourself (uncomment the line 307 and commend lines 298-304): https://github.com/AlttiRi/demo-image-viewer/blob/ad9ac9b1c9d78244462064983e676542700e96d9/core.h#L296-L311
