#ifndef EDITPEAKGROUPINGDIALOG_H
#define EDITPEAKGROUPINGDIALOG_H

#include "ui_editpeakgroupdialog.h"
#include "stable.h"

class EditPeakGroupDialog : public QDialog, public Ui_editPeakGroupDialog {

    Q_OBJECT

    public:
        EditPeakGroupDialog(QWidget *parent);
        ~EditPeakGroupDialog();

    public slots:
        void show();

};

#endif // EDITPEAKGROUPINGDIALOG_H
