#include "searchparamsdialog.h"

SearchParamsDialog::SearchParamsDialog(QWidget *parent) :
    QDialog(parent) {
        setupUi(this);
        setModal(false);
}
