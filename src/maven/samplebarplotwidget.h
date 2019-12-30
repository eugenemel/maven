#ifndef SAMPLEBARPLOTWIDGET_H
#define SAMPLEBARPLOTWIDGET_H

#include "stable.h"
#include "mainwindow.h"

class SampleBarPlotWidget : public QGraphicsView {

    Q_OBJECT

    public:
        SampleBarPlotWidget(MainWindow *mw);
        ~SampleBarPlotWidget();

    public slots:
        void clear() {scene() ->clear();}
        void setPeakGroup(PeakGroup *peakGroup);

    private:
        MainWindow *_mw;

};

#endif // SAMPLEBARPLOTWIDGET_H
