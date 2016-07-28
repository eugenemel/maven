#include "calibratedialog.h"

CalibrateDialog::CalibrateDialog(QWidget *parent) : QDialog(parent) { 
	setupUi(this); 
        setModal(true);
}

void CalibrateDialog::show() {

    /*if (mainwindow != NULL ) {
        QSettings* settings = mainwindow->getSettings();
        if ( settings ) {
            eic_smoothingWindow->setValue(settings->value("eic_smoothingWindow").toDouble());
            grouping_maxRtDiff->setValue(settings->value("grouping_maxRtWindow").toDouble());
        }
    }*/

    QStringList dbnames = DB.getDatabaseNames();
    compoundDatabase->clear();
    for(QString db: dbnames ) {
        compoundDatabase->addItem(db);
    }

    /*

    QString selectedDB = mainwindow->ligandWidget->getDatabaseName();
    compoundDatabase->setCurrentIndex(compoundDatabase->findText(selectedDB));
    compoundPPMWindow->setValue( mainwindow->getUserPPM() );  //total ppm window, not half sized.
    */

    QDialog::show();
}
