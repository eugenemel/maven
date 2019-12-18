#include "setrumsdbdialog.h"

SetRumsDBDialog::SetRumsDBDialog(QWidget *parent) : QDialog(parent) {
        setupUi(this);
        setModal(true);

        QStringList dbnames = DB.getLoadedDatabaseNames();
        for (QString db : dbnames) {
            cmbLoadedLibraries->addItem(db);
        }

        mainWindow = static_cast<MainWindow*>(parent);


        QString selectedDB = mainWindow->ligandWidget->getDatabaseName();
        cmbLoadedLibraries->setCurrentIndex(cmbLoadedLibraries->findText(selectedDB));

        connect(btnUseSelected, SIGNAL(clicked()), this, SLOT(setRumsDBDatabaseName()));
        connect(btnNoLibrary, SIGNAL(clicked()), this, SLOT(useNoRumsDBDatabaseName()));
        connect(btnCancel, SIGNAL(clicked()), this, SLOT(cancelLoading()));

        QDialog::show();
}

void SetRumsDBDialog::setRumsDBDatabaseName() {
    _isCancelled = false;
    mainWindow->rumsDBDatabaseName = cmbLoadedLibraries->currentText();
    close();
}

void SetRumsDBDialog::useNoRumsDBDatabaseName() {
    _isCancelled = false;
    mainWindow->rumsDBDatabaseName = QString("");
    close();
}

void SetRumsDBDialog::cancelLoading() {
    _isCancelled = true;
    close();
}

bool SetRumsDBDialog::isCancelled(){
    return _isCancelled;
}
