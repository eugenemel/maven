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
        inline IsotopePlot* getIsotopePlot(){return _isotopePlot;}
        void setPeakGroup(PeakGroup *peakGroup);

    private:
        QGraphicsScene *_scene;
        MainWindow *_mw;
        IsotopePlot *_isotopePlot;

};


#endif // ISOTOPELEGENDWIDGET_H
