#include "editpeakgroupingdialog.h"

EditPeakGroupingDialog::EditPeakGroupingDialog(QWidget *parent) : QDialog(parent) {
    setupUi(this);
    setModal(true);
}

EditPeakGroupingDialog::~EditPeakGroupingDialog(){

}

void EditPeakGroupingDialog::show(){
    QDialog::show();
}
