#include "editpeakgroupingdialog.h"

EditPeakGroupDialog::EditPeakGroupDialog(QWidget *parent) : QDialog(parent) {
    setupUi(this);
    setModal(true);

    connect(brsSuggestions, SIGNAL(anchorClicked(QUrl)), this, SLOT(onAnchorClicked(QUrl)));
}

EditPeakGroupDialog::~EditPeakGroupDialog(){

}

void EditPeakGroupDialog::show(){
    QDialog::show();
}

void EditPeakGroupDialog::onAnchorClicked(const QUrl &link){
    this->txtUpdateID->setText(link.toString());
}
