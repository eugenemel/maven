#pragma once

#include "ui_directinfusiondialog.h"

class DirectInfusionDialog : public QDialog, public Ui_directInfusionDialog{

    Q_OBJECT

    public:
        DirectInfusionDialog(QWidget* parent);
        ~DirectInfusionDialog();

public slots:
    void setProgressBar(QString text, int progress, int totalSteps);

};
