#ifndef SELECTADDUCTSDIALOG_H
#define SELECTADDUCTSDIALOG_H

#include "ui_selectadductsdialog.h"
#include "stable.h"
#include "mainwindow.h"

class MainWindow;

class SelectAdductsDialog : public QDialog, public Ui_selectAdductsDialog {

    Q_OBJECT

    public:
       SelectAdductsDialog(QWidget* parent, MainWindow* w, QSettings *settings);
       ~SelectAdductsDialog();

    public slots:
       void show();
       void selectAll();
       void deselectAll();
       void updateSelectedAdducts();
       void updateGUI();

    private:
       QSettings *settings;
       MainWindow *mainwindow;
       map<QTableWidgetItem*, Adduct*> checkBoxAdduct = {};

};

#endif // SELECTADDUCTSDIALOG_H
