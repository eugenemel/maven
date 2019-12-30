#include "samplebarplotwidget.h"

SampleBarPlotWidget::SampleBarPlotWidget(MainWindow *mw){

    _scene = new QGraphicsScene(this);
    setScene(_scene);

    this->_mw = mw;
    _barPlot = new BarPlot(nullptr, scene());
    _barPlot->setMainWindow(_mw);

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

    int bwidth = _barPlot->boundingRect().width();
    int xpos = scene()->width()*0.05-bwidth;
    int ypos = scene()->height()*0.10;

    _barPlot->setPos(xpos, ypos);
    _barPlot->setZValue(1000);

    scene()->update();
}
