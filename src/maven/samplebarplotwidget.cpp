#include "samplebarplotwidget.h"

SampleBarPlotWidget::SampleBarPlotWidget(MainWindow *mw){

    _scene = new QGraphicsScene(this);
    setScene(_scene);

    setAlignment(Qt::AlignTop|Qt::AlignLeft);

    this->_mw = mw;
    _barPlot = new BarPlot(nullptr, scene(), _mw);

    setDragMode(NoDrag);
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

    if(!peakGroup) return;

    _barPlot->setPeakGroup(peakGroup);

    //Needed to coerce scroll bars to scroll to correct size
    int pixelBuffer = 20;
    setSceneRect(0, 0, _barPlot->_latestWidth+pixelBuffer, _barPlot->_latestHeight+pixelBuffer);

    scene()->update();
    repaint();

    this->verticalScrollBar()->setSliderPosition(0);
}
