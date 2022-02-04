#ifdef _WIN32
    #include "win.h"
#endif

#include "core.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QPixmap>
#include <QMimeData>
#include <QImageReader>
#include <QWheelEvent>
#include <QtConcurrent>


MainWindow::~MainWindow() {
    delete ui;
}
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    ui = new Ui::MainWindow();
    ui->setupUi(this);

    setAcceptDrops(true);
    QImageReader::setAllocationLimit(512);

    connect(ui->pushButton_First, &QPushButton::clicked, this, &MainWindow::first);
    connect(ui->pushButton_Last,  &QPushButton::clicked, this, &MainWindow::last);
    connect(ui->pushButton_Next,  &QPushButton::clicked, this, &MainWindow::next);
    connect(ui->pushButton_Prev,  &QPushButton::clicked, this, &MainWindow::prev);

    connect(ui->pushButton_SZ, &QPushButton::clicked, this, &MainWindow::sortBySize);
    connect(ui->pushButton_MT, &QPushButton::clicked, this, &MainWindow::sortByMtime);
    connect(ui->pushButton_BT, &QPushButton::clicked, this, &MainWindow::sortByBtime);

    init();
}

void MainWindow::init() {
    logProgramArguments();
    qDebug() << "[supportedExts]:" << DirectoryFileList::getSupportedExts();

    if (qApp->arguments().count() > 1) {
        QString argv1 = qApp->arguments().at(1);
        handleInputPath(argv1);
    } else {
        // [Note]: Comment it on the release.
        //QString debugPath = "M:\\Downloads\\";
        //handleInputPath(debugPath);
    }
}

void MainWindow::handleInputPath(QString inputPath) {
    qDebug().noquote() << "[handleInputPath]:" << inputPath;

#ifdef _WIN32
    QString longInputPath = WIN::longFilePathW(inputPath);
    if (inputPath.length() != longInputPath.length()) {
        qDebug() << "[longFilePathW][before]:" << inputPath;
        qDebug() << "[longFilePathW][after]: " << longInputPath;
        inputPath = longInputPath;
    }
#endif

    DirState state = fileList.initImage(inputPath);
    qDebug() << "[state]" << state;

    if (state == DS::NotExists) {
        return;
    }
    
    if (state == DS::Ready) { // Opening a new file from the same directory, for example.
        update();
        return;
    }

    if (state == DS::Unsupported) { // Let't try to display it. // Or just remove that to ignore them.
        ui->label_Image->setText("[Unsupported]");
        update();
        return;
    }

    if (state == DS::Preview) { // `true` if it's a file, not directory
        update();
    } else {
        ui->label_Image->setText("Parsing...");
        setWindowTitle(inputPath);
    }

    QtConcurrent::run([this]() {
        Timer::start("initFileList");
        DirState state = fileList.initFileList();
        Timer::elapsed("initFileList");

        sortByMtime();

        return state;
    }).then([&](DirState state) {
        if (state == DS::Ready) {
            update();
            cacheAdjacentImages();
        } else if (state == DS::Empty) {
            ui->label_Image->setText("[No Images]");
        }
    });
}


void MainWindow::update() {
    if (fileList.isEmpty()) {
        return;
    }

    QString imgPath = fileList.getSelectedFileEntryPath();
    if (currentImagePath != imgPath) {

        Timer::start("displayImage");
        displayImage(imgPath);
        Timer::elapsed("displayImage");

        currentImagePath = imgPath;
    }

    updateTitle();
    updateStatusBar();
    setOrderDirectionInButtons();
}
void MainWindow::updateTitle() {
    if (fileList.getState() == DS::Preview) {
        setWindowTitle("[ ... ] " + fileList.getSelectedFileEntry().name);
        return;
    }
    QString index = QString::number(fileList.getSelectedFileEntryIndex());
    QString total = QString::number(fileList.getCount());


    setWindowTitle("[" + index + "/" + total + "] " + fileList.getSelectedFileEntry().name);
}
void MainWindow::updateStatusBar() {
    FileEntry entry = fileList.getSelectedFileEntry();
    QLocale locale = this->locale();
    QString size = locale.formattedDataSize(entry.size);
    ui->statusbar->showMessage(
                "Size: "  + size                                            + ",   " +
                "mtime: " + entry.mtime.toTimeSpec(Qt::OffsetFromUTC).toString("yyyy.MM.dd hh:mm:ss.zzz") + "Z,   " +
                "btime: " + entry.btime.toTimeSpec(Qt::OffsetFromUTC).toString("yyyy.MM.dd hh:mm:ss.zzz") + "Z,   " +
                QString::number(image.width()) + "x" + QString::number(image.height())
    );
}


void MainWindow::displayImage(QString imagePath) {
    if (Cache::has(imagePath)) {
        image = Cache::get(imagePath);
    } else {
        image = QPixmap(imagePath);
        Cache::set(imagePath, image);
    }

    const int maxWidth = 1024;
    const int maxHeight = 728;
    int width  = image.width()  < maxWidth  ? image.width()  : maxWidth;
    int height = image.height() < maxHeight ? image.height() : maxHeight;
    ui->label_Image->setPixmap(image.scaled(width, height, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    cacheAdjacentImages();
}

void MainWindow::cacheAdjacentImages() {
    QList<QString> paths = fileList.pathsRange(1, 1);
    Cache::cacheOnly(paths);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event) {
    const QMimeData* mimeData = event->mimeData();
    if (mimeData->hasUrls()) {
        event->acceptProposedAction();
    }
}
void MainWindow::dropEvent(QDropEvent *event) {
//    qDebug() << "dropEvent" << event->mimeData()->text();
//    for (const QUrl &url : event->mimeData()->urls()) {
//        QString fileName = url.toLocalFile();
//        qDebug() << "Dropped file:" << fileName;
//    }
    QString fileName = event->mimeData()->urls().at(0).toLocalFile();
    handleInputPath(fileName);
}



void MainWindow::wheelEvent(QWheelEvent *event) {
    if (event->angleDelta().y() > 0){
        prev();
    } else {
        next();
    }
}
void MainWindow::next() {
    if (fileList.goNext()) {
        update();
    }
}
void MainWindow::prev() {
    if (fileList.goBack()) {
        update();
    }
}
void MainWindow::first() {
    if (fileList.goFirst()) {
        update();
    }
}
void MainWindow::last() {
    if (fileList.goLast()) {
        update();
    }
}


void MainWindow::setOrderDirectionInButtons() {
    ui->pushButton_MT->setText("MT");
    ui->pushButton_BT->setText("BT");
    ui->pushButton_SZ->setText("SZ");

    if (SortOrders::by.length()) {
        QString direction;
        if (SortOrders::by == "mtime") {
            direction = SortOrders::mtime ? "↑" : "↓";
            ui->pushButton_MT->setText("MT" + direction);
        } else
        if (SortOrders::by == "btime") {
            direction = SortOrders::btime ? "↑" : "↓";
            ui->pushButton_BT->setText("BT" + direction);
        } else
        if (SortOrders::by == "size") {
            direction = SortOrders::size ? "↑" : "↓";
            ui->pushButton_SZ->setText("SZ" + direction);
        }
    }
}

// Pretty fast, no need to use QtConcurrent
void MainWindow::sortByMtime() {
    bool asc = SortOrders::mtime;
    if (SortOrders::by == "mtime") {
        asc = !asc;
    }
    SortOrders::by = "mtime";
    SortOrders::mtime = asc;

    Timer::start("sortByMtime");
    fileList.sortByMtime(asc);
    Timer::elapsed("sortByMtime");

    update();
}
void MainWindow::sortByBtime() {
    bool asc = SortOrders::btime;
    if (SortOrders::by == "btime") {
        asc = !asc;
    }
    SortOrders::by = "btime";
    SortOrders::btime = asc;

    Timer::start("sortByBtime");
    fileList.sortByBtime(asc);
    Timer::elapsed("sortByBtime");

    update();
}
void MainWindow::sortBySize() {
    bool asc = SortOrders::size;
    if (SortOrders::by == "size") {
        asc = !asc;
    }
    SortOrders::by = "size";
    SortOrders::size = asc;

    Timer::start("sortBySize");
    fileList.sortBySize(asc);
    Timer::elapsed("sortBySize");

    update();
}


void MainWindow::logProgramArguments() {
    int     argc  = qApp->arguments().count();
    QString argv0 = qApp->arguments().at(0);

    qDebug().noquote() << "argc:   " << QString::number(argc);
    qDebug().noquote() << "argv[0]:" << argv0;

    if (argc > 1) {
        QString argv1 = qApp->arguments().at(1);
        qDebug().noquote() << "argv[1]:" << argv1;
    }
}
