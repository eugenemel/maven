#ifndef SEARCHPARAMSDIALOG_H
#define SEARCHPARAMSDIALOG_H

#include "ui_searchparamsdialog.h"

class TableDockWidget;

class SearchParamsDialog : public QDialog, public Ui_SearchParamsDialog{

    Q_OBJECT

public:
    SearchParamsDialog(QWidget *parent);
};

#endif // SEARCHPARAMSDIALOG_H
