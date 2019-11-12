#include "selectadductsdialog.h"

SelectAdductsDialog::SelectAdductsDialog(QWidget *parent, MainWindow *mw, QSettings *sets) : QDialog(parent) {
    setupUi(this);
    setModal(false);

    mainwindow = mw;
    settings = sets;

    tblAdducts->setColumnWidth(0, 30);      // enabled
    tblAdducts->setColumnWidth(1, 300);     // name
    tblAdducts->setColumnWidth(2, 70);      // nmol
    tblAdducts->setColumnWidth(3, 190);     // mass (sum = 600 px)
    tblAdducts->setColumnWidth(4, tblAdducts->width()-600-2);      // z (add 2 px for padding)

    //fill out table with all available
    for (int i = 0; i < mainwindow->availableAdducts.size(); i++) {

        Adduct* adduct = mainwindow->availableAdducts.at(i);

        tblAdducts->insertRow(i);
        QTableWidgetItem *item = new QTableWidgetItem();
        item->setCheckState(Qt::Unchecked);
        tblAdducts->setItem(i, 0, item);

        QTableWidgetItem *item2 = new QTableWidgetItem();
        item2->setText(adduct->name.c_str());
        tblAdducts->setItem(i, 1, item2);

        QTableWidgetItem *item3 = new QTableWidgetItem();
        item3->setText(to_string(adduct->nmol).c_str());
        tblAdducts->setItem(i, 2, item3);

        QTableWidgetItem *item4 = new QTableWidgetItem();
        item4->setText(QString::number(adduct->mass,'f', 3));
        tblAdducts->setItem(i, 3, item4);

        QTableWidgetItem *item5 = new QTableWidgetItem();
        item5->setText(QString::number(adduct->charge));
        tblAdducts->setItem(i, 4, item5);
    }

}

SelectAdductsDialog::~SelectAdductsDialog(){}

void SelectAdductsDialog::show() {
    QDialog::show();
}
