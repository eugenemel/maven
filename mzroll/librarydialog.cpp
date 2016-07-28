#include "librarydialog.h"

LibraryMangerDialog::LibraryMangerDialog(QWidget *parent) : QDialog(parent) {
        setupUi(this);
        setModal(true);
        connect(deleteButton,SIGNAL(clicked(bool)),SLOT(deleteLibrary()));
        connect(importButton,SIGNAL(clicked()),SLOT(loadCompoundsFile()));
        connect(reloadAllButton,SIGNAL(clicked()),SLOT(reloadMethodsFolder()));
}



void LibraryMangerDialog::show() {
    updateLibraryStats();
    QDialog::show();
}

void LibraryMangerDialog::updateLibraryStats() {

    map<string,int> dbnames;
    for(Compound* c: DB.compoundsDB)  dbnames[c->db]++;

    treeWidget->clear();
    for(auto& pair: dbnames ) {
        QTreeWidgetItem* item = new QTreeWidgetItem(treeWidget);
        item->setText(0,pair.first.c_str());
        item->setText(1,QString::number(pair.second));
        treeWidget->addTopLevelItem(item);
    }
}

void LibraryMangerDialog::deleteLibrary() {

    foreach(QTreeWidgetItem* item, treeWidget->selectedItems() ) {
        QString libraryName = item->text(0);
        qDebug() << "Delete Library" << libraryName;
        DB.deleteCompoundsSQL(libraryName);
    }
    DB.loadCompoundsSQL("ALL");
    updateLibraryStats();
}

void LibraryMangerDialog::loadCompoundsFile(QString filename){
    string dbfilename = filename.toStdString();
    string dbname = mzUtils::cleanFilename(dbfilename);

    int compoundCount = DB.loadCompoundsFile(filename);
    DB.loadCompoundsSQL("ALL");

    if (compoundCount > 0 && mainwindow->ligandWidget) {
        mainwindow->ligandWidget->updateDatabaseList();
        mainwindow->massCalcWidget->updateDatabaseList();

        if( mainwindow->ligandWidget->isVisible() )
            mainwindow->ligandWidget->setDatabase(QString(dbname.c_str()));
    }


    QSettings* settings = mainwindow->getSettings();
    settings->setValue("lastDatabaseFile",filename);
    updateLibraryStats();
}

void LibraryMangerDialog::loadCompoundsFile() {
    QSettings* settings = mainwindow->getSettings();
    QString methodsFolder =      settings->value("methodsFolder").value<QString>();

    QStringList filelist = QFileDialog::getOpenFileNames(this,
            "Select Compounds File To Load",
             methodsFolder,
            "All Known Formats(*.csv *.tab *.txt *.msp *.sptxt *.pepXML);;Tab Delimited(*.tab);;CSV File(*.csv);;NIST Library(*.msp);;SpectraST(*.sptxt) pepXML(*.pepXML)");

    if ( filelist.size() == 0 || filelist[0].isEmpty() ) return;
    loadCompoundsFile(filelist[0]);
}

void LibraryMangerDialog::reloadMethodsFolder() {
    QSettings* settings = mainwindow->getSettings();
    QString methodsFolder =      settings->value("methodsFolder").value<QString>();
    methodsFolder = QFileDialog::getExistingDirectory(this,methodsFolder);

    if (not methodsFolder.isEmpty()) {
        DB.deleteAllCompoundsSQL();
        DB.loadMethodsFolder(methodsFolder);
        mainwindow->ligandWidget->updateDatabaseList();
        mainwindow->massCalcWidget->updateDatabaseList();
    }
    updateLibraryStats();
}
