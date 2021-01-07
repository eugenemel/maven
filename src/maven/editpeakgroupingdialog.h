#ifndef EDITPEAKGROUPINGDIALOG_H
#define EDITPEAKGROUPINGDIALOG_H

#include "ui_editpeakgroupdialog.h"
#include "stable.h"
#include "mainwindow.h"

class EditPeakGroupDialog : public QDialog, public Ui_editPeakGroupDialog {

    Q_OBJECT

    MainWindow *_mainwindow = nullptr;

    public:
        EditPeakGroupDialog(QWidget *parent);
        void setMainWindow(MainWindow *mainwindow){_mainwindow = mainwindow;}
        ~EditPeakGroupDialog();

    public slots:
        void show();
        void onAnchorClicked(const QUrl &link);
        void setPeakGroup(PeakGroup *peakGroup);
};

#endif // EDITPEAKGROUPINGDIALOG_H
