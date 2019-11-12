#include "selectadductsdialog.h"

SelectAdductsDialog::SelectAdductsDialog(QWidget *parent) : QDialog(parent) {
    setupUi(this);
    setModal(true);

}

SelectAdductsDialog::~SelectAdductsDialog(){

}

void SelectAdductsDialog::show() {
    QDialog::show();
}
