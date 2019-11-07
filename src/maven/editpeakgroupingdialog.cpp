#include "editpeakgroupingdialog.h"

EditPeakGroupDialog::EditPeakGroupDialog(QWidget *parent) : QDialog(parent) {
    setupUi(this);
    setModal(true);
}

EditPeakGroupDialog::~EditPeakGroupDialog(){

}

void EditPeakGroupDialog::show(){
    QDialog::show();
}
