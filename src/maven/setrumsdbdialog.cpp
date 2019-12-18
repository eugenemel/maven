#include "setrumsdbdialog.h"

SetRumsDBDialog::SetRumsDBDialog(QWidget *parent) : QDialog(parent) {
        setupUi(this);
        setModal(true);

        mainWindow = static_cast<MainWindow*>(parent);

        connect(btnCancel, SIGNAL(clicked()), this, SLOT(cancelLoading()));
        connect(btnUseSelected, SIGNAL(clicked()), this, SLOT(setRumsDBDatabaseName()));
}

void SetRumsDBDialog::setRumsDBDatabaseName() {
    _isCancelled = false;
    close();
}

void SetRumsDBDialog::useNoRumsDBDatabaseName() {
    _isCancelled = false;
    mainWindow->rumsDBDatabaseName = "";
    close();
}

void SetRumsDBDialog::cancelLoading() {
    _isCancelled = true;
    close();
}

bool SetRumsDBDialog::isCancelled(){
    return _isCancelled;
}
