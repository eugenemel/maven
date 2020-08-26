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
    qDebug() << "SampleBarPlotWidget::setPeakGroup() group=" << peakGroup;
    if(!peakGroup) return;

    _barPlot->setPeakGroup(peakGroup);
    _barPlot->update();

    //Needed to coerce scroll bars to scroll to correct size
    int pixelBuffer = 20;

    repaint();

    QRectF sceneRect(0, 0, _barPlot->_latestWidth+pixelBuffer, _barPlot->_latestHeight+pixelBuffer);

//    qDebug() << "sampleBarPlotWidget::setPeakGroup(): Scene Rect: w=" << sceneRect.width() << " h=" << sceneRect.height() << endl;

    setSceneRect(sceneRect);
    updateSceneRect(sceneRect);
    scene()->update(sceneRect);

    this->verticalScrollBar()->setSliderPosition(0);
    this->horizontalScrollBar()->setSliderPosition(0);

    refresh();
}

void SampleBarPlotWidget::refresh() {
    repaint();
    scene()->update();
}
