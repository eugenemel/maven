#include "selectadductsdialog.h"

SelectAdductsDialog::SelectAdductsDialog(QWidget *parent) : QDialog(parent) {
    setupUi(this);
    setModal(false);

    tblAdducts->setColumnWidth(0, 70);      // enabled
    tblAdducts->setColumnWidth(1, 300);     // name
    tblAdducts->setColumnWidth(2, 70);      // nmol
    tblAdducts->setColumnWidth(3, 190);     // mass (sum = 630 px)
    tblAdducts->setColumnWidth(4, tblAdducts->width()-630-2);      // z (add 2 px for padding)

}

SelectAdductsDialog::~SelectAdductsDialog(){}

void SelectAdductsDialog::show() {
    QDialog::show();
}
