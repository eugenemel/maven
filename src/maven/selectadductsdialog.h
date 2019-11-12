#ifndef SELECTADDUCTSDIALOG_H
#define SELECTADDUCTSDIALOG_H

#include "ui_selectadductsdialog.h"
#include "stable.h"

class SelectAdductsDialog : public QDialog, public Ui_selectAdductsDialog {

    Q_OBJECT

    public:
       SelectAdductsDialog(QWidget* parent);
       ~SelectAdductsDialog();

    public slots:
       void show();

};

#endif // SELECTADDUCTSDIALOG_H
