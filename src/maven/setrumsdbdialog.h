#ifndef SETRUMSDBDIALOG_H
#define SETRUMSDBDIALOG_H

#include "stable.h"
#include "database.h"
#include "ui_setrumsDBdialog.h"
#include "mainwindow.h"

class MainWindow;
extern Database DB;

class SetRumsDBDialog : public QDialog, public Ui_setrumsDBDialog {

    Q_OBJECT

    public:
        SetRumsDBDialog(QWidget *parent);
        bool isCancelled();

    public slots:
        void setRumsDBDatabaseName();
        void useNoRumsDBDatabaseName();
        void cancelLoading();

    private:
        MainWindow* mainWindow;
        bool _isCancelled = true;
};

#endif // SETRUMSDBDIALOG_H
