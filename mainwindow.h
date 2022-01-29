#pragma once

#include <QMainWindow>
#include <QWheelEvent>
#include <QDragEnterEvent>
#include "core.h"


namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    DirectoryFileList fileList;
    QString currentImagePath;
    QPixmap image;

    void handleInputPath(QString inputPath);
    void init();
    void displayImage(QString imagePath);
    void update();
    void updateTitle();
    void updateStatusBar();
    void logProgramArguments();

    void next();
    void prev();    
    void first();
    void last();

    void sortBySize();
    void sortByMtime();
    void sortByBtime();
    void setOrderDirectionInButtons();

    void wheelEvent(QWheelEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

};
