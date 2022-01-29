# demo-image-viewer

It's a **demonstration** only image viewer made with Qt6.
[The binaries are available](https://github.com/AlttiRi/demo-image-viewer/releases).

![Screenshot](https://user-images.githubusercontent.com/16310547/151670175-122043d2-725b-4997-9669-9a147a56d1b2.png)

---

- Opens an image in a time independent of the count of images in a directory where it is located.
- Decodes 8.3  file names to long file names with Win API (`GetLongPathNameW`). A 8.3 can be faced on a user input from Windows Exlorer:
on Drag'n'Drop and when a user opens an image with a double click (the image path is passed as a command line argument to the program.)
- Handles images of the directory in a separated thread with `QtConcurrent::run`.
- Sorts by mtime, btime, size.
- Updates the image position (in the title) on the sorting change.

Opening of the next images is for O(n) (It depends of a image size: 20 ms for 150 KB, 170 ms for 10 MB, 700 ms for 36 MB). 
So the adjacent images should be preloaded in a separate thread, but I did not implemented that in this demo program. 

---

The speed results for 350 KB image with a directory with 30000+ files (Run it with QT Creator to look at the `qDebug()` logs):
[timer][displayImage]: 84 ms — time to desplay the image
at this moment the image is displayed.

That is performed in a background thread:
- [timer][entryInfoList]: **143 ms** — time to get the list with QFileInfo of all files (31973) in the directory
- [timer][filterBySupportedExts]: **6 ms** — time filter that list to exlude unsupported files
- [filterBySupportedExts] fileInfoList.size:         **31973** 
- [filterBySupportedExts] fileInfoListFiltered.size: **30006**
- [timer][initFileEntryList]: **200 ms**  — time to create a list of data structure with all necessary information for all filtered files (30006)
- [timer][initFileList]: **352 ms** — the **total time** of the work with directory files ~(143 + 6 + 200)
- [timer][sortByMtime]: **6 ms** — to sort all files by mtime

---

Note: Do NOT use code that calls methods of QFileInfo in `std::sort`. It's incredibly slow. 

That is up to 600 times slower
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

3200-3600 ms vs 6 ms.

So, store all required fields of `QFileInfo` in your own data struct.
