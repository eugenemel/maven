#include "sampleimage.h"

SampleImageWidget::SampleImageWidget(MainWindow* mw) { 
    this->mainwindow = mw;
   _log10Transform=false;

    initPlot();

    _drawXAxis = false;
    _drawYAxis = false;
    _resetZoomFlag = true;
    _nearestCoord = QPointF(0,0);
    _focusCoord = QPointF(0,0);
    _maxIntensityThreshold=1e8;
    _minIntensityThreshold=1e3;
    VS = mzSlice(0,1,0,1);

}

void SampleImageWidget::initPlot() {
    _zoomFactor = 0;
    setScene(new QGraphicsScene(this));
    scene()->setItemIndexMethod(QGraphicsScene::BspTreeIndex);
    scene()->setSceneRect(0,0,this->width(), this->height());

    setDragMode(QGraphicsView::RubberBandDrag);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

}

void SampleImageWidget::replot() {
    if(not mainwindow) return;
    if(mainwindow->getVisibleSamples().size() == 0) { scene()->clear(); return; }

    //clean up previous plot
    for(unsigned int i=0; i < _items.size(); i++) {
        if(_items[i] != NULL) delete(_items[i]); _items[i]=NULL;
    }
    _items.clear();

    scene()->setSceneRect(0,0,this->width(), this->height());

    cerr << "replot:" << VS.mzmin << "\t" << VS.mzmax << "\t" << VS.rtmin << "\t" << VS.rtmax << endl;
    makeImages(0.01,0.001);
    drawImage();
    addAxes();
    scene()->update();
}

void SampleImageWidget::clearOverlay() {}

void SampleImageWidget::overlayPeakGroup(PeakGroup* group) {
    if(!group) return;
 }

void SampleImageWidget::drawImage() {
   for(auto& img: _sampleImages) { 
       QGraphicsPixmapItem *item = scene()->addPixmap(QPixmap::fromImage(img));
       item->setPos(0,0);
       _items.push_back(item);
    }
}

mzSlice SampleImageWidget::getGlobalBounds() { 

    mzSlice bounds;
    MainWindow* mw = this->mainwindow;
    if(!mw) return bounds;

    vector <mzSample*> samples = mw->getVisibleSamples();
    if(samples.size() == 0) return bounds;
    
    mzSample* s1 = samples[0];
    bounds.mzmin = s1->minMz;
    bounds.mzmax = s1->maxMz;
    bounds.rtmin = s1->minRt;
    bounds.rtmax = s1->maxRt;

    for (mzSample* s : samples) {
        bounds.mzmin = min(bounds.mzmin, s->minMz);
        bounds.mzmax = max(bounds.mzmax, s->maxMz);
        bounds.rtmin = min(bounds.rtmin, s->minRt);
        bounds.rtmax = max(bounds.rtmax, s->maxRt);
    }
    return bounds;
}

void SampleImageWidget::makeImages(float rtAccur, float mzAccur) {


    MainWindow* mw = this->mainwindow;
    if (!mw) return;

    vector <mzSample*> samples = mw->getVisibleSamples();
    if(samples.size()==0) return;

    double rt_range = (VS.rtmax - VS.rtmin);
    double mz_range = (VS.mzmax - VS.mzmin);

    int W  =  scene()->width();
    int H =   scene()->height();

    float logMaxIntensity = log(_maxIntensityThreshold);

    for (mzSample* sample : samples) {
        //draw scan data
        QImage img(W, H, QImage::Format_RGB32);
        img.fill(Qt::black);

        for (Scan* s : sample->scans) {
            if (s->mslevel != 1) continue;
            if (s->rt < VS.rtmin or s->rt > VS.rtmax) continue;
            float xcoord = (s->rt - VS.rtmin) / rt_range * W;

            for (int i = 0; i < s->nobs(); i++) {
                if (s->intensity[i]<_minIntensityThreshold) continue;
                if (s->mz[i] < VS.mzmin or s->mz[i] > VS.mzmax) continue;
                float ycoord = (VS.mzmax - s->mz[i]) / mz_range * H;
                float c = log(s->intensity[i]) / logMaxIntensity * 255; if (c > 255) c = 255;
                img.setPixel(xcoord, ycoord, qRgb(c, 255-c,c));

                //raster
                if (xcoord>1 and xcoord<W-1 and ycoord>1 and ycoord<H-1) {
                    for(int j=-1; j<=1; j++)
                        for(int k=-1; k<=1; k++)
                            img.setPixel(xcoord+j, ycoord+k, qRgb(c, 255-c,c));
                } 
            }
        }
        _sampleImages[sample->getSampleName().c_str()]=img;
    }
}

void SampleImageWidget::checkGlobalBounds() {
    
    mzSlice GB = getGlobalBounds();

	if( VS.mzmin < GB.mzmin) VS.mzmin = GB.mzmin;
	if( VS.mzmax > GB.mzmax) VS.mzmax = GB.mzmax;

	if( VS.rtmin < GB.rtmin) VS.rtmin = GB.rtmin;
	if( VS.rtmax > GB.rtmax) VS.rtmax = GB.rtmax;

    cerr << "checkGlobalBounds:  mz=" << VS.mzmin << "-" << VS.mzmax << " rt=" << VS.rtmin << "-" << VS.rtmax << endl;
}

void SampleImageWidget::keyPressEvent( QKeyEvent *e ) {
    switch( e->key() ) {
    case Qt::Key_0 :
        resetZoom(); return;
    default:
        return;
    }
    e->accept();
}


void SampleImageWidget::resizeEvent (QResizeEvent * event) {
    replot();
}

void SampleImageWidget::addAxes() { 

	if (_drawXAxis) {
		Axes* x = new Axes(0,VS.rtmin, VS.rtmax,10);
		scene()->addItem(x);
		x->setZValue(998);
		_items.push_back(x);
	}

    if (_drawYAxis) {
         Axes* y = new Axes(1,VS.mzmin, VS.mzmax, 10);
         y->setZValue(999);
         scene()->addItem(y);
         _items.push_back(y);
    }
}

void SampleImageWidget::mousePressEvent(QMouseEvent *event) {
    QGraphicsView::mousePressEvent(event);

    if (event->button() == Qt::LeftButton) {
		_mouseStartPos = event->pos();
	} 

}

float SampleImageWidget::toX(float x)  { float W=scene()->width();    return((x-VS.rtmin)/(VS.rtmax-VS.rtmin)* W); }
float SampleImageWidget::invX(float px) {  float W=scene()->width();  return(px/W * (VS.rtmax-VS.rtmin) + VS.rtmin); }

float SampleImageWidget::toY(float y)   {  float H=scene()->height(); return((VS.mzmax-y)/(VS.mzmax-VS.mzmin)* H); }
float SampleImageWidget::invY(float py) {  float H=scene()->height(); return( (H-py)/H * (VS.mzmax-VS.mzmin) + VS.mzmin); }

void SampleImageWidget::mouseReleaseEvent(QMouseEvent *event) {
    QGraphicsView::mouseReleaseEvent(event);
    _mouseEndPos	= event->pos();

    float mzmin = invY(_mouseStartPos.y());
    float mzmax = invY(_mouseEndPos.y());

    qDebug() << _mouseStartPos <<  " mz=" << mzmin;
    qDebug() << _mouseEndPos   <<  " mz=" << mzmax;

    int deltaX =  _mouseEndPos.x() - _mouseStartPos.x();
    int deltaY =  _mouseEndPos.y() - _mouseStartPos.y();

    if ( abs(deltaX) > 5 and abs(deltaY) > 5 ) {
        VS.rtmin = invX(_mouseStartPos.x());
        VS.rtmax = invX(_mouseEndPos.x());
        VS.mzmin = min(mzmin, mzmax);
        VS.mzmax = max(mzmin, mzmax);
        zoomRegion(VS.mzmin, VS.mzmax, VS.rtmin, VS.rtmax);
    } else {
        setMzFocus(invY(_mouseEndPos.y()), invX(_mouseEndPos.x()));
    }

}

void SampleImageWidget::setMzFocus(Peak* peak) { 
    setMzFocus(peak->peakMz, peak->rt);
}

void SampleImageWidget::setMzFocus(float mz, float rt) {

        if(!mainwindow) return;
        float maxIntensity=0;
		float bestMz = mz;

        float rtResol = (VS.rtmax - VS.rtmin)/scene()->width()*2;
        float mzResol = (VS.mzmax - VS.mzmin)/scene()->height()*2;
        if (mzResol < 0.01) mzResol=0.1;
        if (rtResol < 0.1)  rtResol=0.1;

        vector <mzSample*> samples = mainwindow->getVisibleSamples();
        for (mzSample* sample : samples) {
          for (Scan* s : sample->scans) {
            if (s->mslevel != 1) continue;
            if (s->rt < rt-rtResol or s->rt > rt+rtResol) continue;
                for (int i = 0; i < s->nobs(); i++) {
                    if (s->mz[i] < mz-mzResol) continue;
                    if (s->mz[i] > mz+mzResol) break;
                    if (s->intensity[i]>maxIntensity) { maxIntensity=s->intensity[i]; bestMz=s->mz[i]; }
                }
            }
        } 

        mainwindow->setMzValue(bestMz);
        mainwindow->massCalcWidget->setMass(bestMz);
}

void SampleImageWidget::mouseDoubleClickEvent(QMouseEvent* event){
    QGraphicsView::mouseDoubleClickEvent(event);
    _focusCoord = _nearestCoord;
    drawImage();

}

void SampleImageWidget::mouseMoveEvent(QMouseEvent* event){ return; }

void SampleImageWidget::wheelEvent(QWheelEvent *event) {
    if ( event->delta() > 0 ) {
        _maxIntensityThreshold *= 0.95;
        _minIntensityThreshold *= 0.5;
    } else {
        _maxIntensityThreshold *= 1.05;
        _minIntensityThreshold *= 1.5;
    }
    replot();
}


void SampleImageWidget::resetZoom() {
    VS = getGlobalBounds();
    checkGlobalBounds();
    replot();
}


void SampleImageWidget::zoomRegion(float mzmin, float mzmax, float rtmin, float rtmax) {
    VS.mzmin = mzmin;
    VS.mzmax = mzmax;
    VS.rtmin = rtmin;
    VS.rtmax = rtmax;
    checkGlobalBounds();
    replot();
}


void SampleImageWidget::zoomRegion(float centerMz, float window) {
    if (centerMz <= 0) return;
    VS.mzmin = centerMz - window;
    VS.mzmax = centerMz + window;
    checkGlobalBounds();
    replot();
}


void SampleImageWidget::contextMenuEvent(QContextMenuEvent * event) {
    event->ignore();
    QMenu menu;

    QAction* a0 = menu.addAction("Reset Zoom");
    connect(a0, SIGNAL(triggered()), SLOT(resetZoom()));

   menu.exec(event->globalPos());
}
