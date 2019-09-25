#ifndef DIRECTINFUSIONDIALOG_H
#define DIRECTINFUSIONDIALOG_H

#include "ui_directinfusiondialog.h"
#include "stable.h"
#include "mainwindow.h"
#include "background_directinfusion_update.h"

class MainWindow;
class BackgroundDirectInfusionUpdate;
extern Database DB;

class DirectInfusionDialog : public QDialog, public Ui_directInfusionDialog{

    Q_OBJECT

    public:
        DirectInfusionDialog(QWidget* parent);
        ~DirectInfusionDialog();
        void setSettings(QSettings* settings) { this->settings = settings; }
        void setMainWindow(MainWindow* w) { this->mainwindow = w; }

    public slots:
        void analyze();
        void setProgressBar(QString text, int progress, int totalSteps);
        void show();

    private:
        QSettings *settings;
        MainWindow *mainwindow;
        BackgroundDirectInfusionUpdate *directInfusionUpdate;
};

#endif
