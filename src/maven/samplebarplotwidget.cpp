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


    int bwidth = _barPlot->boundingRect().width();
    int bheight = _barPlot->boundingRect().height();

    int xpos = scene()->width()*0.10-bwidth;
    int ypos = scene()->height()*0.95;

    //debugging
    cout
            << "width: " << scene()->width()
            << ", height: " << scene()->height()
            << ", bwidth: " << bwidth
            << ", bheight: " << bheight
            << endl;

    _barPlot->setPos(xpos, ypos);

    _barPlot->setZValue(1000);

    scene()->update();
}
