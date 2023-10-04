#include "isotopelegendwidget.h"

IsotopeLegendWidget::IsotopeLegendWidget(MainWindow *mw){

    _scene = new QGraphicsScene(this);
    setScene(_scene);

    setAlignment(Qt::AlignTop|Qt::AlignLeft);

    this->_mw = mw;
    _isotopePlot = new IsotopePlot(nullptr, scene());
    _isotopePlot->setMainWindow(_mw);

    setDragMode(NoDrag);
    _isotopePlot->setIsInLegendWidget(true);

    scene()->addItem(_isotopePlot);
}

IsotopeLegendWidget::~IsotopeLegendWidget(){

    if (_isotopePlot){
        delete(_isotopePlot);
    }

    if (_scene) {
        delete(_scene);
    }
}

void IsotopeLegendWidget::setPeakGroup(PeakGroup *peakGroup){

    qDebug() << "IsotopeLegendWidget::setPeakGroup() group=" << peakGroup;

    if(!peakGroup) return;

    //width is based on the dockwidget width - initialization
    _isotopePlot->setWidth(100);

    _isotopePlot->setPeakGroup(peakGroup);
    _isotopePlot->update();

    //Needed to coerce scroll bars to scroll to correct size
    int pixelBuffer = 20;

    repaint();

    QRectF sceneRect(0, 0, _isotopePlot->boundingRect().width()+pixelBuffer, _isotopePlot->boundingRect().height()+pixelBuffer);

//    qDebug() << "sampleBarPlotWidget::setPeakGroup(): Scene Rect: w=" << sceneRect.width() << " h=" << sceneRect.height() << endl;

    setSceneRect(sceneRect);
    updateSceneRect(sceneRect);
    scene()->update(sceneRect);

    this->verticalScrollBar()->setSliderPosition(0);
    this->horizontalScrollBar()->setSliderPosition(0);

    qDebug() << "IsotopeLegendWidget::setPeakGroup() group=" << peakGroup << "completed";
}
