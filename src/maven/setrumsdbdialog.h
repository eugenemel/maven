#ifndef SETRUMSDBDIALOG_H
#define SETRUMSDBDIALOG_H

#include "stable.h"
#include "ui_setrumsDBdialog.h"

class SetRumsDBDialog : public QDialog, public Ui_setrumsDBDialog {

    Q_OBJECT

    public:
        SetRumsDBDialog(QWidget *parent);
};

#endif // SETRUMSDBDIALOG_H
