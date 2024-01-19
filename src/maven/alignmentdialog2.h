#ifndef ALIGNDIALOG2_H
#define ALIGNDIALOG2_H

#include "stable.h"
#include "ui_alignmentdialog2.h"

class AlignmentDialog2 : public QDialog, public Ui_AlignmentDialog2 {
		Q_OBJECT

		public:
             AlignmentDialog2(QWidget *parent);
};

#endif
