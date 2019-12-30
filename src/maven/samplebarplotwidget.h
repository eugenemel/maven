#ifndef SAMPLEBARPLOTWIDGET_H
#define SAMPLEBARPLOTWIDGET_H

#include "stable.h"
#include "mainwindow.h"
#include "barplot.h"

class SampleBarPlotWidget : public QGraphicsView {

    Q_OBJECT

    public:
        SampleBarPlotWidget(MainWindow *mw);
        ~SampleBarPlotWidget();

    public slots:
        void clear() {scene() ->clear();}
        void setPeakGroup(PeakGroup *peakGroup);

    private:
        QGraphicsScene *_scene;
        MainWindow *_mw;
        BarPlot *_barPlot;

};

#endif // SAMPLEBARPLOTWIDGET_H
