#include "barplot.h"

PeakGroup::QType BarPlot::qtype = PeakGroup::AreaTop;

BarPlot::BarPlot(QGraphicsItem* parent, QGraphicsScene *scene)
    :QGraphicsItem(parent) {
	_mw=NULL;
    _width = 0;
    _height = 0;
    _barwidth=10;
    _showSampleNames=true;
    _showIntensityText=true;
    _showQValueType=true;
    setFlag(ItemIsMovable);
    setFlag(ItemIsSelectable);
    setFlag(ItemIsFocusable);

}

//Issue 94: Use this constructor when not putting the BarPlot widget on the EIC plot.
BarPlot::BarPlot(QGraphicsItem* parent, QGraphicsScene *scene, MainWindow *mainWindow)
    :QGraphicsItem(parent){

    _mw=mainWindow;
    _width = 200;
    _height= 800;
    _barwidth= 10;

    _showSampleNames=true;
    _showIntensityText=true;
    _showQValueType=true;

    isOnEicWidget = false;
}

void BarPlot::switchQValue() {  
	BarPlot::qtype = (PeakGroup::QType) (((int) qtype+1) % 6); 
	PeakGroup* g = NULL;
	if ( _mw != NULL && _mw->getEicWidget() ) g =  _mw->getEicWidget()->getSelectedGroup();
	if ( g != NULL ) {
		setPeakGroup(g);
		scene()->update();
	}
}

BarPlot::~BarPlot() { 
    clear();
}

QRectF BarPlot::boundingRect() const
{
	return(QRectF(0,0,_width,_height));
}

void BarPlot::clear() {
	_yvalues.clear();
	_labels.clear();
	_colors.clear();
}


void BarPlot::setPeakGroup(PeakGroup* group) {
    clear();
    if (group == NULL ) return;
    if (_mw == NULL   ) return;
    qDebug() << "BarPlot::setPeakGroup() group=" << group;

    qtype = _mw->getUserQuantType();
    vector<mzSample*> vsamples = _mw->getVisibleSamples();
    sort(vsamples.begin(), vsamples.end(), mzSample::compSampleOrder);
    vector<float> yvalues = group->getOrderedIntensityVector(vsamples,BarPlot::qtype);

    if (vsamples.size() <=0 ) return;

    if (scene()) {
        if (isOnEicWidget) {
            _width =   scene()->width()*0.20;
            _barwidth = scene()->height()*0.75/vsamples.size();
            if (_barwidth<3)  _barwidth=3;
            if (_barwidth>15) _barwidth=15;
            _height = _yvalues.size()*_barwidth;
        } else {
            //dummy constants that get overwritten later
            _width = 100;
            _barwidth = 20;
            _height = _yvalues.size()*_barwidth;
        }
    }

    for(int i=0; i < vsamples.size(); i++ ) {
        mzSample* sample = vsamples[i];
        QColor color = QColor::fromRgbF(sample->color[0], sample->color[1],sample->color[2],sample->color[3]);
        QString sampleName( sample->sampleName.c_str());
        if (sample->getNormalizationConstant() != 1.0 )
            sampleName = QString::number(sample->getNormalizationConstant(), 'f', 2) + "x " + sampleName;

        _labels.push_back(sampleName);
        _colors.push_back(color);
        _yvalues.push_back(yvalues[i]);
    }
    qDebug() << "BarPlot::setPeakGroup() group=" << group << "completed";
}

void BarPlot::wheelEvent ( QGraphicsSceneWheelEvent * event ) {
    if (isOnEicWidget) {
        qDebug() << "BarPlot::wheelEvent()" ;
        qreal scale = this->scale();
        event->delta() > 0 ? scale *= 1.2 :  scale *= 0.9;
        if (scale < 0.1) scale=0.1;
        if (scale > 2 ) scale=2;
        this->setScale(scale);
    } else {
        QGraphicsItem::wheelEvent(event);
    }
}

void BarPlot::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{ 
    int visibleSamplesCount = _yvalues.size();
    if (visibleSamplesCount == 0) return;
    if (!scene()) return;

    float maxYvalue=0;
    for(int i=0;i<_yvalues.size();i++) { if (_yvalues[i]>maxYvalue) maxYvalue=_yvalues[i]; }
    if (maxYvalue==0) return;

    float maxBarHeight =   scene()->width()*0.25;
    if ( maxBarHeight < 10 ) maxBarHeight=10;
    if ( maxBarHeight > 200 ) maxBarHeight=200;


    int barSpacer=1;

    QFont font("Helvetica");
    float fontsize = _barwidth*0.8;
    if  (fontsize < 1 ) fontsize=1;
    font.setPointSizeF(fontsize);
    painter->setFont(font);
    QFontMetrics fm( font );
    int legendShift = fm.size(0,"100e+10",0,NULL).width();

    QColor color = QColor::fromRgbF(0.2,0.2,0.2,1.0);
    QBrush brush(color);

    int legendX = 0;
    int legendY = 0;

    QString title;
    switch (qtype ) {
    case PeakGroup::AreaTop: title = "Peak AreaTop"; break;
    case PeakGroup::Area: title = "Peak Area"; break;
    case PeakGroup::Height: title = "Peak Height"; break;
    case PeakGroup::Quality: title = "Peak Quality"; break;
    case PeakGroup::RetentionTime: title = "RetentionTime"; break;
    case PeakGroup::SNRatio: title = "Peak S/N Ratio"; break;
    default: title = "?"; break;
    }

    int legendXPosAdj = 0;
    int peakTitleYPosAdj = 0;

    if (!this->isOnEicWidget) {
        int maxSampleNameWidth = 0;
        for (int i = 0; i < _yvalues.size(); i++) {
            QFontMetrics fm = painter->fontMetrics();
            int textWidth = fm.width(_labels[i]);
            if (textWidth > maxSampleNameWidth) {
                maxSampleNameWidth = textWidth;
            }
        }

        int titleHeight = fm.height();

        maxBarHeight = maxSampleNameWidth;
        legendXPosAdj = legendShift+5;
        peakTitleYPosAdj = titleHeight+5;
    }

    if (_showQValueType) {
        painter->drawText(legendX-legendShift+legendXPosAdj,legendY-1+peakTitleYPosAdj,title);
    }

    for(int i=0; i < _yvalues.size(); i++ ) {
        int posX = legendX;
        int posY = legendY + i*_barwidth + peakTitleYPosAdj;
        int width = _barwidth;
        int height = _yvalues[i] / maxYvalue * maxBarHeight;

        //qDebug() << _yvalues[i] << " " << height << " " << maxYvalue << endl;

        if (_mw && _mw->spectraWidget){
            painter->setPen(_mw->getBackgroundAdjustedBlack(_mw->spectraWidget));
        } else {
            painter->setPen(Qt::black);
        }


        // do not draw outline
        if (_yvalues[i] == 0 ) painter->setPen(Qt::gray);

        brush.setColor(_colors[i]);
        brush.setStyle(Qt::SolidPattern);

        painter->setBrush(brush);
        painter->drawRect(posX+3+legendXPosAdj,posY,height,width);

        if (_showSampleNames) {
            painter->drawText(posX+6+legendXPosAdj,posY+_barwidth-2,_labels[i]);
        }

        char numType='g';
        int  numPrec=2;
        if (maxYvalue < 10000 ) { numType='f'; numPrec=0;}
        if (maxYvalue < 1000 ) { numType='f';  numPrec=1;}
        if (maxYvalue < 100 ) { numType='f';   numPrec=2;}
        if (maxYvalue < 1 ) { numType='f';     numPrec=3;}

        //Issue 324: handle normalized intensities using scientific notation
        if (maxYvalue < 0.01) {numType ='g'; numPrec=2;}

        if (_yvalues[i] > 0 && _showIntensityText) {
            QString value = QString::number(_yvalues[i],numType,numPrec);
            painter->drawText(posX-legendShift+legendXPosAdj,posY+_barwidth-2,value);
        }

        if ( posY+_barwidth > _height) _height = posY+_barwidth+barSpacer;
    }

    painter->setPen(Qt::black);        // do not draw outline
    painter->setBrush(Qt::NoBrush);
    painter->drawLine(legendX+legendXPosAdj, legendY+peakTitleYPosAdj, legendX+legendXPosAdj, legendY+_height);

    _width = legendShift+maxBarHeight;

    _latestWidth = _width;
    _latestHeight = _height;
}
