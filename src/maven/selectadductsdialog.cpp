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
    tblAdducts->setSelectionMode(QAbstractItemView::NoSelection);

    //fill out table with all available and valid adducts
    int counter = 0;
    for (int i = 0; i < DB.availableAdducts.size(); i++) {

        Adduct* adduct = DB.availableAdducts.at(i);

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
    connect(btnCancel, SIGNAL(clicked()), this, SLOT(hide()));

    tblAdducts->repaint();
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
    tblAdducts->repaint();
}

void SelectAdductsDialog::deselectAll() {
    qDebug() << "SelectAdductsDialog::deselectAll()";
    for (auto it = checkBoxAdduct.begin(); it != checkBoxAdduct.end(); ++it) {
        it->first->setCheckState(Qt::Unchecked);
    }
    tblAdducts->update();
    tblAdducts->repaint();
}

void SelectAdductsDialog::updateSelectedAdducts() {
    qDebug() << "SelectAdductsDialog::updateSelectedAdducts()";

    vector<Adduct*> updatedEnabledAdducts;
    for (auto it = checkBoxAdduct.begin(); it != checkBoxAdduct.end(); ++it) {
        if (it->first->checkState() == Qt::Checked){
            updatedEnabledAdducts.push_back(it->second);
        }
    }

    sort(updatedEnabledAdducts.begin(), updatedEnabledAdducts.end(), [](const Adduct* lhs, const Adduct* rhs){
        return lhs->name < rhs->name;
    });

    DB.adductsDB = updatedEnabledAdducts;

    mainwindow->updateAdductsInGUI();

    hide();
}

void SelectAdductsDialog::updateGUI() {
    QString enabledAdductsList = settings->value("enabledAdducts").toString();
    QStringList enabledAdductNames = enabledAdductsList.split(";", QString::SkipEmptyParts);

    for (unsigned int i = 0; i< tblAdducts->rowCount(); i++) {
        QString adductName = tblAdducts->item(static_cast<int>(i), 1)->text();

        if (enabledAdductNames.contains(adductName)) {
            tblAdducts->item(static_cast<int>(i), 0)->setCheckState(Qt::Checked);
        } else {
            tblAdducts->item(static_cast<int>(i), 0)->setCheckState(Qt::Unchecked);
        }
    }
}
