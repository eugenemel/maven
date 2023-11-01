#ifndef ISOTOPEEXPORTSETTINGSDIALOG_H
#define ISOTOPEEXPORTSETTINGSDIALOG_H

#include "stable.h"
#include "ui_isotopesexportsettings.h"

class IsotopeExportSettingsDialog : public QDialog, public Ui_Dialog {

    Q_OBJECT

    public:
        IsotopeExportSettingsDialog(QWidget *parent);
};

#endif // ISOTOPEEXPORTSETTINGSDIALOG_H
