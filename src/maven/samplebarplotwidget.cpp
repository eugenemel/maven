#include "samplebarplotwidget.h"

SampleBarPlotWidget::SampleBarPlotWidget(MainWindow *mw){

    _scene = new QGraphicsScene(this);
    setScene(_scene);

    setAlignment(Qt::AlignTop|Qt::AlignLeft);

    this->_mw = mw;
    _barPlot = new BarPlot(nullptr, scene());
    _barPlot->setMainWindow(_mw);
    _barPlot->isOnEicWidget = false;

    scene()->addItem(_barPlot);
}

SampleBarPlotWidget::~SampleBarPlotWidget(){

    if (_barPlot){
        delete(_barPlot);
    }

    if (_scene) {
        delete(_scene);
    }
}

void SampleBarPlotWidget::setPeakGroup(PeakGroup *peakGroup){
    _barPlot->setPeakGroup(peakGroup);
    scene()->update();

    //Needed for scroll bars
    int pixelBuffer = 20;
    this->setSceneRect(0, 0, _barPlot->_latestWidth+pixelBuffer, _barPlot->_latestHeight+pixelBuffer);
}
