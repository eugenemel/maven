#ifndef LIBRARYDIALOG_H
#define LIBRARYDIALOG_H

#include <QDialog>
#include "database.h"
#include "mainwindow.h"
#include "ui_librarydialog.h"

class MainWindow;
extern Database DB;

class LibraryMangerDialog : public QDialog , public Ui_librarydialog {
    Q_OBJECT

public:
    LibraryMangerDialog(QWidget *parent);
    void setMainWindow(MainWindow* w) { this->mainwindow = w; }

public slots:
    void show();
    void loadCompoundsFile();
    void loadCompoundsFile(QString filename);
    void reloadMethodsFolder();

private slots:
    void deleteLibrary();
    void loadLibrary();

private:
    void updateLibraryStats();
    MainWindow *mainwindow;

};

#endif // LIBRARYDIALOG_H
