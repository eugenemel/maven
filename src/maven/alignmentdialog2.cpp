#include "alignmentdialog2.h"

AlignmentDialog2::AlignmentDialog2(QWidget *parent) : QDialog(parent) {
        setupUi(this);
        setModal(false);

        connect(this->btnSelectFile, SIGNAL(clicked(bool)), SLOT(loadRtFile()));
}

void AlignmentDialog2::loadRtFile() {

        const QString name = QFileDialog::getOpenFileName(
            this, "Select .rt File", ".",tr("Model File (*.rt)"));

        if (QFile::exists(name)) {
            this->txtSelectFile->setText(name);
        }
 }

void AlignmentDialog2::bringIntoView(){
        if (isVisible()){
            show();
            raise();
            activateWindow();
        } else {
            show();
        }
}
