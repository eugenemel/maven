#ifndef EDITPEAKGROUPINGDIALOG_H
#define EDITPEAKGROUPINGDIALOG_H

#include "ui_editpeakgroupdialog.h"
#include "stable.h"

class EditPeakGroupingDialog : public QDialog, public Ui_editPeakGroupDialog {

    Q_OBJECT

    public:
        EditPeakGroupingDialog(QWidget *parent);
        ~EditPeakGroupingDialog();

    public slots:
        void show();

};

#endif // EDITPEAKGROUPINGDIALOG_H
