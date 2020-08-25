#include "editpeakgroupingdialog.h"

EditPeakGroupDialog::EditPeakGroupDialog(QWidget *parent) : QDialog(parent) {
    setupUi(this);
    setModal(true);

    //Issue 278: force window to top
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);

    brsSuggestions->setOpenLinks(false);
    brsSuggestions->setOpenExternalLinks(false);
    connect(brsSuggestions, SIGNAL(anchorClicked(QUrl)), this, SLOT(onAnchorClicked(QUrl)));
}

EditPeakGroupDialog::~EditPeakGroupDialog(){

}

void EditPeakGroupDialog::show(){
    if (QDialog::isVisible()){
        QDialog::show();
        QDialog::raise();
        QDialog::activateWindow();
    } else {
        QDialog::show();
    }
}

void EditPeakGroupDialog::onAnchorClicked(const QUrl &link){
    this->txtUpdateID->setText(link.path());
}
