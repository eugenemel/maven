#include "selectadductsdialog.h"

SelectAdductsDialog::SelectAdductsDialog(QWidget *parent, MainWindow *mw, QSettings *sets) : QDialog(parent) {
    setupUi(this);
    setModal(false);

    mainwindow = mw;
    settings = sets;

    tblAdducts->setColumnWidth(0, 75);      // enabled
    tblAdducts->setColumnWidth(1, 300);     // name
    tblAdducts->setColumnWidth(2, 75);      // nmol
    tblAdducts->setColumnWidth(3, 150);     // mass (sum = 600 px)
    tblAdducts->setColumnWidth(4, 60);      // z

    QStringList enabledAdductNames;
    if (settings->contains("enabledAdducts")) {
        QString enabledAdductsList = settings->value("enabledAdducts").toString();
        enabledAdductNames = enabledAdductsList.split(";", QString::SkipEmptyParts);
    }

    tblAdducts->setSortingEnabled(true);
    tblAdducts->setEditTriggers(QAbstractItemView::NoEditTriggers);

    //fill out table with all available and valid adducts
    int counter = 0;
    for (int i = 0; i < mainwindow->availableAdducts.size(); i++) {

        Adduct* adduct = mainwindow->availableAdducts.at(i);

        if (abs(adduct->charge) < 1e-6) continue; //require charged species

        tblAdducts->insertRow(counter);

        QTableWidgetItem *item = new QTableWidgetItem();
        if (enabledAdductNames.contains(QString(adduct->name.c_str()))) {
            item->setCheckState(Qt::Checked);
        } else {
            item->setCheckState(Qt::Unchecked);
        }

        checkBoxAdduct.insert(make_pair(item, adduct));

        tblAdducts->setItem(counter, 0, item);

        QTableWidgetItem *item2 = new QTableWidgetItem();
        item2->setText(adduct->name.c_str());
        tblAdducts->setItem(counter, 1, item2);

        QTableWidgetItem *item3 = new QTableWidgetItem();
        item3->setText(to_string(adduct->nmol).c_str());
        tblAdducts->setItem(counter, 2, item3);

        QTableWidgetItem *item4 = new QTableWidgetItem();
        item4->setText(QString::number(adduct->mass,'f', 4));
        tblAdducts->setItem(counter, 3, item4);

        QTableWidgetItem *item5 = new QTableWidgetItem();
        item5->setText(QString::number(adduct->charge));
        tblAdducts->setItem(counter, 4, item5);

        counter++;
    }

    connect(btnSelectAll, SIGNAL(clicked()), this, SLOT(selectAll()));
    connect(btnDeselectAll, SIGNAL(clicked()), this, SLOT(deselectAll()));
    connect(btnUpdate, SIGNAL(clicked()), this, SLOT(updateSelectedAdducts()));

}

SelectAdductsDialog::~SelectAdductsDialog(){}

void SelectAdductsDialog::show() {
    QDialog::show();
}

void SelectAdductsDialog::selectAll() {
    qDebug() << "SelectAdductsDialog::selectAll()";
    for (auto it = checkBoxAdduct.begin(); it != checkBoxAdduct.end(); ++it) {
        it->first->setCheckState(Qt::Checked);
    }
    tblAdducts->update();
}

void SelectAdductsDialog::deselectAll() {
    qDebug() << "SelectAdductsDialog::deselectAll()";
    for (auto it = checkBoxAdduct.begin(); it != checkBoxAdduct.end(); ++it) {
        it->first->setCheckState(Qt::Unchecked);
    }
    tblAdducts->update();
}

void SelectAdductsDialog::updateSelectedAdducts() {
    qDebug() << "SelectAdductsDialog::updateSelectedAdducts()";
    for (auto it = checkBoxAdduct.begin(); it != checkBoxAdduct.end(); ++it) {
        if (it->first->checkState() == Qt::Checked){
            qDebug() << "SELECTED ADDUCT: " << it->second->name.c_str();
        }
    }
}
