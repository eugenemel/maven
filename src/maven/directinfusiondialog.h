#ifndef DIRECTINFUSIONDIALOG_H
#define DIRECTINFUSIONDIALOG_H

#include "ui_directinfusiondialog.h"
#include "stable.h"
#include "mainwindow.h"

class DirectInfusionDialog : public QDialog, public Ui_directInfusionDialog{

    Q_OBJECT

    public:
        DirectInfusionDialog(QWidget* parent);
        ~DirectInfusionDialog();
        void setSettings(QSettings* settings) { this->settings = settings; }
        void setMainWindow(MainWindow* w) { this->mainwindow = w; }

    public slots:
        void setProgressBar(QString text, int progress, int totalSteps);
        void show();

    private:
        QSettings *settings;
        MainWindow *mainwindow;
};

#endif
