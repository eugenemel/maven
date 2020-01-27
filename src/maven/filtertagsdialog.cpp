#include "filtertagsdialog.h"

FilterTagsDialog::FilterTagsDialog(QWidget *parent) : QDialog(parent) {
    setupUi(this);
    setModal(true);

    tblTags->setColumnWidth(0, 75);     // visible
    tblTags->setColumnWidth(1, 100);    // name
    tblTags->setColumnWidth(2, 50);     // label
    tblTags->setColumnWidth(3, 50);     // hotkey
    tblTags->setColumnWidth(4, 50);     // icon
    tblTags->setColumnWidth(5, 1000);    // description

    tblTags->setSortingEnabled(true);
    tblTags->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tblTags->setSelectionMode(QAbstractItemView::NoSelection);

    //start counter for building table
    int counter = 0;

    //Special cases: does not correspond to a PeakGroupTag* object

    //No Tags

    tblTags->insertRow(counter);

    QTableWidgetItem *u_item0 = new QTableWidgetItem();
    u_item0->setCheckState(Qt::Checked);
    tblTags->setItem(counter, 0, u_item0);

    noTags = u_item0;

    QTableWidgetItem *u_item1 = new QTableWidgetItem();
    u_item1->setText("No Tags");
    tblTags->setItem(counter, 1, u_item1);

    QTableWidgetItem *u_item2 = new QTableWidgetItem();
    u_item2->setText("");
    tblTags->setItem(counter, 2, u_item2);

    QTableWidgetItem *u_item3 = new QTableWidgetItem();
    u_item3->setText("u");
    tblTags->setItem(counter, 3, u_item3);

    QTableWidgetItem *u_item4 = new QTableWidgetItem();
    u_item4->setIcon(QIcon());
    tblTags->setItem(counter, 4, u_item4);

    QTableWidgetItem *u_item5 = new QTableWidgetItem();
    u_item5->setText("Peak Group has not been tagged.");
    tblTags->setItem(counter, 5, u_item5);

    counter++;

    // Good Tag

    tblTags->insertRow(counter);

    QTableWidgetItem *g_item0 = new QTableWidgetItem();
    g_item0->setCheckState(Qt::Checked);
    tblTags->setItem(counter, 0, g_item0);

    goodTag = g_item0;

    QTableWidgetItem *g_item1 = new QTableWidgetItem();
    g_item1->setText("Good");
    tblTags->setItem(counter, 1, g_item1);

    QTableWidgetItem *g_item2 = new QTableWidgetItem();
    g_item2->setText("g");
    tblTags->setItem(counter, 2, g_item2);

    QTableWidgetItem *g_item3 = new QTableWidgetItem();
    g_item3->setText("g");
    tblTags->setItem(counter, 3, g_item3);

    QTableWidgetItem *g_item4 = new QTableWidgetItem();
    g_item4->setIcon(QIcon(":/images/good.png"));
    tblTags->setItem(counter, 4, g_item4);

    QTableWidgetItem *g_item5 = new QTableWidgetItem();
    g_item5->setText("Peak Group is good.");
    tblTags->setItem(counter, 5, g_item5);

    counter++;

    // Bad Tag

    tblTags->insertRow(counter);

    QTableWidgetItem *b_item0 = new QTableWidgetItem();
    b_item0->setCheckState(Qt::Checked);
    tblTags->setItem(counter, 0, b_item0);

    badTag = b_item0;

    QTableWidgetItem *b_item1 = new QTableWidgetItem();
    b_item1->setText("Bad");
    tblTags->setItem(counter, 1, b_item1);

    QTableWidgetItem *b_item2 = new QTableWidgetItem();
    b_item2->setText("b");
    tblTags->setItem(counter, 2, b_item2);

    QTableWidgetItem *b_item3 = new QTableWidgetItem();
    b_item3->setText("b");
    tblTags->setItem(counter, 3, b_item3);

    QTableWidgetItem *b_item4 = new QTableWidgetItem();
    b_item4->setIcon(QIcon(":/images/bad.png"));
    tblTags->setItem(counter, 4, b_item4);

    QTableWidgetItem *b_item5 = new QTableWidgetItem();
    b_item5->setText("Peak Group is bad.");
    tblTags->setItem(counter, 5, b_item5);

    counter++;

    //Peak group tags
    for (auto &x : DB.peakGroupTags) {

        tblTags->insertRow(counter);

        PeakGroupTag *peakGroupTag = x.second;

        QTableWidgetItem *item0 = new QTableWidgetItem();
        item0->setCheckState(Qt::Checked);
        tblTags->setItem(counter, 0, item0);

        checkBoxTag.insert(make_pair(item0, peakGroupTag));

        QTableWidgetItem *item1 = new QTableWidgetItem();
        item1->setText(peakGroupTag->tagName.c_str());
        tblTags->setItem(counter, 1, item1);

        QTableWidgetItem *item2 = new QTableWidgetItem();
        item2->setText(QString(peakGroupTag->label));
        tblTags->setItem(counter, 2, item2);

        QTableWidgetItem *item3 = new QTableWidgetItem();
        item3->setText(QString(peakGroupTag->hotkey));
        tblTags->setItem(counter, 3, item3);

        QTableWidgetItem *item4 = new QTableWidgetItem();
        item4->setIcon(peakGroupTag->icon);
        tblTags->setItem(counter, 4, item4);

        QTableWidgetItem *item5 = new QTableWidgetItem();
        item5->setText(peakGroupTag->description.c_str());
        tblTags->setItem(counter, 5, item5);

        counter++;
    }

    connect(btnSelectAll, SIGNAL(clicked()), this, SLOT(selectAll()));
    connect(btnDeselectAll, SIGNAL(clicked()), this, SLOT(deselectAll()));
    connect(btnCancel, SIGNAL(clicked()), this, SLOT(hide()));
    connect(btnApplyFilter, SIGNAL(clicked()), this, SLOT(hide()));
}

void FilterTagsDialog::selectAll() {
    qDebug() << "FilterTagsDialog::selectAll()";

    noTags->setCheckState(Qt::Checked);
    goodTag->setCheckState(Qt::Checked);
    badTag->setCheckState(Qt::Checked);

    for (auto it = checkBoxTag.begin(); it != checkBoxTag.end(); ++it) {
        it->first->setCheckState(Qt::Checked);
    }

    tblTags->update();
}

void FilterTagsDialog::deselectAll() {
    qDebug() << "FilterTagsDialog::deselectAll()";

    noTags->setCheckState(Qt::Unchecked);
    goodTag->setCheckState(Qt::Unchecked);
    badTag->setCheckState(Qt::Unchecked);

    for (auto it = checkBoxTag.begin(); it != checkBoxTag.end(); ++it) {
        it->first->setCheckState(Qt::Unchecked);
    }

    tblTags->update();
}

