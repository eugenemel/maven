#ifndef SELECTADDUCTSDIALOG_H
#define SELECTADDUCTSDIALOG_H

#include "ui_selectadductsdialog.h"
#include "stable.h"
#include "mainwindow.h"

class MainWindow;

class SelectAdductsDialog : public QDialog, public Ui_selectAdductsDialog {

    Q_OBJECT

    public:
       SelectAdductsDialog(QWidget* parent);
       ~SelectAdductsDialog();
       void setSettings(QSettings* settings) { this->settings = settings;}
       void setMainWindow(MainWindow* w) { this->mainwindow = w;}

    public slots:
       void show();

    private:
       QSettings *settings;
       MainWindow *mainwindow;

};

#endif // SELECTADDUCTSDIALOG_H
