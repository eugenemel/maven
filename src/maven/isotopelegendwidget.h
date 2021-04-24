#ifndef ISOTOPELEGENDWIDGET_H
#define ISOTOPELEGENDWIDGET_H

#include "stable.h"
#include "mainwindow.h"
#include "isotopeplot.h"

class IsotopeLegendWidget : public QGraphicsView {

    Q_OBJECT

    public:
        IsotopeLegendWidget(MainWindow *mw);
        ~IsotopeLegendWidget();

    private:
        MainWindow *_mw;
        IsotopePlot *_isotopePlot;

};


#endif // ISOTOPELEGENDWIDGET_H
