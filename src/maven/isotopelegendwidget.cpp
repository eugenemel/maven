#include "isotopelegendwidget.h"

IsotopeLegendWidget::IsotopeLegendWidget(MainWindow *mw){

    setAlignment(Qt::AlignTop|Qt::AlignLeft);

    this->_mw = mw;
    _isotopePlot = new IsotopePlot(nullptr, scene());

    setDragMode(NoDrag);
    _isotopePlot->setIsInLegendWidget(true);

    scene()->addItem(_isotopePlot);
}

IsotopeLegendWidget::~IsotopeLegendWidget(){

    if (_isotopePlot){
        delete(_isotopePlot);
    }
}
