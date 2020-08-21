#include "setrumsdbdialog.h"

SetRumsDBDialog::SetRumsDBDialog(QWidget *parent) : QDialog(parent) {
        setupUi(this);
        setModal(true);
        _isCancelled = true;

        updateComboBox();

        mainWindow = static_cast<MainWindow*>(parent);


        QString selectedDB = mainWindow->ligandWidget->getDatabaseName();
        cmbLoadedLibraries->setCurrentIndex(cmbLoadedLibraries->findText(selectedDB));

        connect(btnUseSelected, SIGNAL(clicked()), this, SLOT(setRumsDBDatabaseName()));
        connect(btnNoLibrary, SIGNAL(clicked()), this, SLOT(useNoRumsDBDatabaseName()));
        connect(btnCancel, SIGNAL(clicked()), this, SLOT(cancelLoading()));
        connect(btnSelectLibrary, SIGNAL(clicked()), mainWindow->libraryDialog, SLOT(show()));
        connect(chkmzrollDBLoadLibs, SIGNAL(clicked()), this, SLOT(setAttemptToLoadDBFlag()));
        connect(chkmzrollDBLoadPGCompounds, SIGNAL(clicked()), this, SLOT(setLoadPGCompoundMatchesFlag()));

        btnUseSelected->setFocus();

        QDialog::show();
}

void SetRumsDBDialog::updateComboBox() {
    cmbLoadedLibraries->clear();
    QStringList dbnames = DB.getLoadedDatabaseNames();
    for (QString db : dbnames) {
        cmbLoadedLibraries->addItem(db);
    }
}

void SetRumsDBDialog::setRumsDBDatabaseName() {
    _isCancelled = false;
    if (!cmbLoadedLibraries->currentText().isEmpty()) {
        mainWindow->rumsDBDatabaseName = cmbLoadedLibraries->currentText();
        close();
    }
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

void SetRumsDBDialog::setAttemptToLoadDBFlag(){
    mainWindow->isAttemptToLoadDB = chkmzrollDBLoadLibs->checkState() != Qt::Unchecked;
}

void SetRumsDBDialog::setLoadPGCompoundMatchesFlag(){
    mainWindow->isLoadPGCompoundMatches = chkmzrollDBLoadPGCompounds->checkState() != Qt::Unchecked;
}
