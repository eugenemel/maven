#include "spectrawidget.h"

SpectraWidget::SpectraWidget(MainWindow* mw) { 
    this->mainwindow = mw;
   _currentScan = NULL;
   _avgScan = NULL;
   _log10Transform=false;

    initPlot();

    _drawXAxis = true;
    _drawYAxis = true;
    _resetZoomFlag = true;
    _profileMode = false;
    _nearestCoord = QPointF(0,0);
    _focusCoord = QPointF(0,0);
    _showOverlay=false;
}

void SpectraWidget::initPlot() {
    _title = NULL;
    _zoomFactor = 0;
    setScene(new QGraphicsScene(this));
    scene()->setItemIndexMethod(QGraphicsScene::BspTreeIndex);
    scene()->setSceneRect(10,10,this->width()-10, this->height()-10);

    setDragMode(QGraphicsView::RubberBandDrag);
    //setCacheMode(CacheBackground);
    //setRenderHint(QPainter::Antialiasing);
    //setTransformationAnchor(AnchorUnderMouse);
    //setResizeAnchor(AnchorViewCenter);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    _arrow = new QGraphicsLineItem(0,0,0,0);
    scene()->addItem(_arrow);
    QPen redpen(Qt::red, 1,  Qt::DashDotLine, Qt::RoundCap, Qt::RoundJoin);
    QPen bluepen(Qt::blue, 1,  Qt::DashDotLine, Qt::RoundCap, Qt::RoundJoin);
    _arrow->setPen(bluepen);

    _varrow = new QGraphicsLineItem(0,0,0,0);
    scene()->addItem(_varrow);
    _varrow->setPen(redpen);

    QFont font("Helvetica", 10);
    font.setWeight(QFont::Bold);

    _note = new QGraphicsTextItem(0,0);
    scene()->addItem(_note);
    _note->setFont(font);
    _note->setDefaultTextColor(Qt::blue);

    _vnote = new QGraphicsTextItem(0,0);
    scene()->addItem(_vnote);
    _vnote->setFont(font);
    _vnote->setDefaultTextColor(Qt::red);

    _title = new QGraphicsTextItem("",0);
    scene()->addItem(_title);

}

void SpectraWidget::setCurrentScan(Scan* scan) {

    if (!_currentScan) {
        _currentScan = new Scan(0,0,0,0,0,0); //empty scan;
    }

    if (scan ) {
        //if (_currentScan and scan->mslevel != _currentScan->mslevel) {
        //  _resetZoomFlag = true;
        //}
        links.clear();
        _currentScan->deepcopy(scan);
        if(_log10Transform) {
            _currentScan->log10Transform();
        }
    }
}

void SpectraWidget::replot() {
    drawGraph();
}


void SpectraWidget::setTitle() {

    if (width()<50) return;

    QString title;
    if (_currentScan->sample) {
         QString sampleName = _currentScan->sample->sampleName.c_str();
         QString polarity;
         _currentScan->getPolarity() > 0 ? polarity = "Pos" : polarity = "Neg";

         //QColor sampleColor = QColor::fromRgbF( sample->color[0], sample->color[1], sample->color[2], 1 );

         title += tr("Sample:<b>%1</b>  Scan:<b>%2</b> rt:<b>%3</b>   msLevel:<b>%4</b>  Ion:<b>%5</b>").arg(
                             QString(sampleName),
                             QString::number(_currentScan->scannum),
                             QString::number(_currentScan->rt,'f',2),
                             QString::number(_currentScan->mslevel),
                             polarity
                             );
    }

    if (_currentScan->precursorMz) {
        title += "<br>";
        QString precursorMzLink= " PreMz: " + QString::number(_currentScan->precursorMz,'f',4);
        title += precursorMzLink ;
    }

    if (_currentScan->collisionEnergy) {
        QString precursorMzLink= " CE: " + QString::number(_currentScan->collisionEnergy,'f',1);
        title += precursorMzLink ;
    }

    if (_currentScan->productMz>0) {
        QString precursorMzLink= " ProMz: " + QString::number(_currentScan->productMz,'f',4);
        title += precursorMzLink ;
    }

    QFont font = QApplication::font();
    font.setPixelSize(8.0);
    _title->setHtml(title);
    int titleWith = _title->boundingRect().width();
    _title->setPos(scene()->width()/2-titleWith/2, 3);
    //_title->setScale(scene()->width()*0.5);
    _title->update();
}

void SpectraWidget::setScan(Scan* scan) {
    if ( scan == NULL ) return;
    setCurrentScan(scan);
    cerr << "SpectraWidget::setScan(scan) " << endl;
    findBounds(true,true);
    drawGraph();
    repaint();
}

void SpectraWidget::setScan(Scan* scan, float mzmin, float mzmax) {
    if ( scan == NULL ) return;
    cerr << "SpectraWidget::setScan(scan,min,max) : " << scan->scannum << endl;
    setCurrentScan(scan);
    _minX = mzmin;
    _maxX = mzmax;
    findBounds(false,true);
    mainwindow->getEicWidget()->setFocusLine(scan->rt);
    drawGraph();
    repaint();
}

void SpectraWidget::setScan(mzSample* sample, int scanNum=-1) {
    if (!sample) return;
    if (sample->scans.size() == 0 ) return;
    if (_currentScan && scanNum < 0 ) scanNum = _currentScan->scannum; 
    if (scanNum > sample->scans.size() ) scanNum = sample->scans.size()-1;

    if ( scanNum >= 0 && scanNum < sample->scans.size() ) { 
        setCurrentScan(sample->scans[ scanNum ]);
        cerr << "SpectraWidget::setScan(scan) " << endl;
        findBounds(false,true);
        drawGraph();
        repaint();
    }
}


void SpectraWidget::setScan(Peak* peak) {
    cerr << "SpectraWidget::setScan(peak) " << endl;
    links.clear();

    if (peak == NULL ) return;

    mzSample* sample = peak->getSample();
    if ( sample == NULL ) return;

    Scan* scan = sample->getScan(peak->scan);
    if ( scan == NULL ) return;

    setCurrentScan(scan);

    _focusCoord = QPointF(peak->peakMz,peak->peakIntensity);
    _minX = peak->peakMz-2;
    _maxX = peak->peakMz+6;
    _maxY = peak->peakIntensity*1.3;
    _minY = 0;

    annotateScan();
    drawGraph();
    repaint();
}

void SpectraWidget::overlaySpectralHit(SpectralHit& hit) {
        _spectralHit = hit;//copy hit

        //determine limits of overlayed spectra
        setScan(hit.scan, hit.getMinMz()-0.5, hit.getMaxMz()+0.5);
        findBounds(false,true);

        drawGraph();
        repaint();

        if(_currentScan) {
            int pos = _currentScan->findHighestIntensityPos(hit.precursorMz,hit.productPPM);
            if (pos > 0 && pos <= _currentScan->nobs()) {
                _focusCoord.setX(hit.precursorMz);
                _focusCoord.setY(_currentScan->intensity[pos]);
            }
        }
}

void SpectraWidget::clearOverlay() {
    _spectralHit = SpectralHit();
    _showOverlay=false;
}

void SpectraWidget::overlayPeakGroup(PeakGroup* group) {
    if(!group) return;
    Scan* avgScan = group->getAverageFragmenationScan(20);
    setScan(avgScan);
    if (group->compound)  {
        if(group->compound->fragment_mzs.size()) overlayCompound(group->compound);
        else if (!group->compound->smileString.empty()) overlayTheoreticalSpectra(group->compound);
        else  overlayCompound(group->compound);
    }
    delete(avgScan);
}

void SpectraWidget::overlayTheoreticalSpectra(Compound* c) {
   if(!c or c->smileString.empty()) return;

   SpectralHit hit;
   hit.compoundId = "THEORY: "; hit.compoundId += c->name.c_str();
   hit.scan = _currentScan;
   hit.precursorMz = c->precursorMz;
   hit.productPPM=  mainwindow->massCalcWidget->fragmentPPM->value();

    BondBreaker bb;
    bb.breakCarbonBonds(true);
    bb.breakDoubleBonds(true);
    bb.setMinMW(70);
    bb.setSmile(c->smileString);
    map<string,double> fragments = bb.getFragments();

    for(auto& pair: fragments) {
        //qDebug() << "Ah:" << pair.second;
        hit.mzList.push_back(pair.second);
        hit.intensityList.push_back(1000);

        hit.mzList.push_back(pair.second-PROTON);
        hit.intensityList.push_back(800);

        hit.mzList.push_back(pair.second+PROTON);
        hit.intensityList.push_back(800);
    }

    _showOverlay = true;
    overlaySpectralHit(hit);
}

void SpectraWidget::overlayCompound(Compound* c) {
   clearOverlay();
   if(!_currentScan or !c) return;

   if(c->fragment_mzs.size() == 0 ) {
       overlayTheoreticalSpectra(c);
       return;
   }

   SpectralHit hit;
   hit.compoundId = c->name.c_str();
   hit.scan = _currentScan;
   hit.precursorMz = c->precursorMz;
   hit.productPPM=  mainwindow->massCalcWidget->fragmentPPM->value();
   cerr << "overlayCompound() " << c->name << " #fragments=" << c->fragment_mzs.size() << endl;

   for(int i=0; i < c->fragment_mzs.size(); i++) 	   hit.mzList << c->fragment_mzs[i];
   for(int i=0; i < c->fragment_intensity.size(); i++) hit.intensityList << c->fragment_intensity[i];

   //remove previous annotations
   links.clear();;

   //add new annotations
   for(auto ion: c->fragment_iontype) {
         float mz= c->fragment_mzs[ion.first];
         string note = ion.second;
         links.push_back(mzLink(mz,mz,note));
   }

   //for(int i=0; i < c->fragment_mzs.size(); i++) { cerr << "overlay:" << c->fragment_mzs[i] << endl; }

   _showOverlay = true;
   overlaySpectralHit(hit);
   resetZoom();
}


void SpectraWidget::drawSpectalHitLines(SpectralHit& hit) {

    float ppmWindow =    hit.productPPM;
    double maxIntensity= hit.getMaxIntensity();

    QPen redpen(Qt::red,1);
    QPen bluepen(Qt::blue,2);
    QPen graypen(Qt::gray,1);
    graypen.setStyle(Qt::DashDotLine);

    float SCALE=0.45;

    //create label
    QGraphicsTextItem* text = new QGraphicsTextItem(hit.compoundId);
    text->setFont(_title->font());
    text->setPos(_title->pos().x(),toY(_maxY*0.95,SCALE));
    scene()->addItem(text);
    _items.push_back(text);


    for(int i=0; i < hit.mzList.size(); i++) {
        float hitMz=hit.mzList[i];
        int hitIntensity= 1;
        if (i < hit.intensityList.size()) hitIntensity=hit.intensityList[i];
        int pos = _currentScan->findHighestIntensityPos(hitMz,ppmWindow);

        int x = toX(hitMz);
        int y = toY(hitIntensity/maxIntensity*_maxY,SCALE);

        QGraphicsLineItem* line = new QGraphicsLineItem(x,y,x,toY(0),0);
        pos > 0 ?  line->setPen(bluepen) : line->setPen(redpen);
        scene()->addItem(line);
        _items.push_back(line);

        //matched?
        if (pos > 0 && pos < _currentScan->nobs()) {
            //cerr << "matched:" << hitMz << " " <<  _currentScan->mz[pos] << " "  << hitIntensity << " " << _currentScan->intensity[pos] << endl;
            int x = toX(_currentScan->mz[pos]);
            int y = toY(_maxY,SCALE);
            QGraphicsLineItem* line = new QGraphicsLineItem(x,y,x,toY(0),0);
            line->setPen(graypen);
            scene()->addItem(line);
            _items.push_back(line);
        } else {
            //cerr << "!matched:" << hitMz << endl;

        }
    }
}

void SpectraWidget::drawGraph() {


    //clean up previous plot
    if ( _arrow ) {
        _arrow->setVisible(false);
    }

    for(unsigned int i=0; i < _items.size(); i++) {
        if(_items[i] != NULL) delete(_items[i]); _items[i]=NULL;
    }
    _items.clear();

    scene()->setSceneRect(10,10,this->width()-10, this->height()-10);
    Scan* scan = _currentScan;
    if (scan == NULL ) return;

    //draw title text
    setTitle();

    QColor sampleColor = QColor::fromRgb(0,0,0,255);
    string sampleName =  ".";
    QString polarity;

    QPen pen(Qt::black, 2);
    float fontSize = scene()->height()*0.05;
    if ( fontSize < 1) fontSize=1;
    if ( fontSize > 10) fontSize=10;
    QFont font("Helvetica", fontSize);
    float _focusedMz = _focusCoord.x();

    QPen slineColor(sampleColor, 2);
    EicLine* sline = new EicLine(NULL,scene());
    sline->setColor(sampleColor);
    sline->setPen(slineColor);
   _items.push_back(sline);


   if( _profileMode ) {
        QBrush slineFill(sampleColor);
        sline->setFillPath(true);
        sline->setBrush(slineFill);
    }

    QMap<float,int>shownPositions; //items sorted by key

    float SCALE=1.0;
    float OFFSET=0;
    if (_showOverlay) { SCALE=0.45; OFFSET=-scene()->height()/2.0; }


    int yzero = toY(0,SCALE,OFFSET);
    sline->addPoint(toX(_maxX),yzero);
    sline->addPoint(toX(_minX),yzero);

    for(int j=0; j<scan->nobs(); j++ ) {
        if ( scan->mz[j] < _minX  || scan->mz[j] > _maxX ) continue;

        if ( scan->intensity[j] / _maxY > 0.005 ) {
            shownPositions[ scan->intensity[j] ] = j;
        }

        int x = toX(scan->mz[j]);
        int y = toY(scan->intensity[j],SCALE,OFFSET);

        if( _profileMode ) {
               sline->addPoint(x,y);
        } else {
                sline->addPoint(x,yzero);
                sline->addPoint(x,y);
                sline->addPoint(x,yzero);
        }

        if( abs(scan->mz[j]-_focusedMz)<0.005 ) {
            QPen redpen(Qt::red, 3);
            QGraphicsLineItem* line = new QGraphicsLineItem(x,y,x,toY(0,SCALE,OFFSET),0);
            scene()->addItem(line);
            line->setPen(redpen);
            _items.push_back(line);
        }
    }

    //show labels
    QMap<float,int>::iterator itr; int labelCount=0;
    for(itr = shownPositions.end(); itr != shownPositions.begin(); itr--) {
        int pos = itr.value();
        if(pos < 0 || pos >= scan->mz.size()) continue;

        int prec=2;
        if(abs(_maxX-_minX)>100) prec=3;
        else if(abs(_maxX-_minX)>50) prec=4;
        else if(abs(_maxX-_minX)>10) prec=6;

        //create label
        QGraphicsTextItem* text = new QGraphicsTextItem(QString::number(scan->mz[pos],'f',prec),0);
        text->setFont(font);
        scene()->addItem(text);
        _items.push_back(text);

        //position label
        int x = toX(scan->mz[pos]);
        int y = toY(scan->intensity[pos],SCALE,OFFSET);
        text->setPos(x-2,y-20);

        labelCount++;
        if(labelCount >= 20 ) break;
    }

    //show annotations
    for(int i=0; i < links.size(); i++ ) {
        if ( links[i].mz2 < _minX || links[i].mz2 > _maxX ) continue;

        //create label
       // Note* text = new Note(QString(links[i].note.c_str()) + " " + QString::number(links[i].mz2,'f',3));
        Note* label = new Note(QString(links[i].note.c_str()));
        scene()->addItem(label);
        _items.push_back(label);

        //position label
        int x = toX(links[i].mz2);
        int y = toY(links[i].value2,OFFSET,SCALE);
        label->setPos(x-2,y-20);
        label->setExpanded(true);
    }

    if(_showOverlay and _spectralHit.mzList.size()>0) {
        drawSpectalHitLines(_spectralHit);
    }

    addAxes();
}



void SpectraWidget::findBounds(bool checkX, bool checkY) {
    //bounds
	if (_currentScan == NULL || _currentScan->mz.size() == 0) return;

	float minMZ; float maxMZ;
	if ( _currentScan->mz.size() == 1 ) {
		minMZ = _currentScan->mz[0]-0.5;
		maxMZ = _currentScan->mz[0]+0.5;
	} else { 
		minMZ = _currentScan->mz[0]-0.01;
		maxMZ = _currentScan->mz[_currentScan->mz.size()-1]+0.01;
	}

    //cerr << _currentScan->filterLine << " " << _currentScan->nobs() << endl;
    //cerr << "findBounds():  RANGE=" << minMZ << "-" << maxMZ << endl;
	if( _minX < minMZ) _minX = minMZ;
	if( _maxX > maxMZ) _maxX = maxMZ;

    if ( checkX ) { _minX = minMZ; _maxX = maxMZ; }
    if ( _minX == _maxX ) { _minX-=0.5; _maxX+=0.5; }

    if (checkY)  {
    	_minY = 0; 
		_maxY = 1;
		for(int j=0; j<_currentScan->nobs(); j++ ) {
			if (_currentScan->mz[j] >= _minX && _currentScan->mz[j] <= _maxX) {
				if (_currentScan->intensity[j] > _maxY) _maxY = _currentScan->intensity[j];
            }
        }
    }

    _minY=0; _maxY *= 1.3;
   // cerr << "findBounds():  mz=" << _minX << "-" << _maxX << " ints=" << _minY << "-" << _maxY << endl;
}

void SpectraWidget::keyPressEvent( QKeyEvent *e ) {
    switch( e->key() ) {
    case Qt::Key_Left:
        showLastScan(); return;
    case Qt::Key_Right :
        showNextScan(); return;
    case Qt::Key_0 :
        resetZoom(); return;
    case Qt::Key_Plus :
        zoomIn(); return;
    case Qt::Key_Minus:
        zoomOut(); return;
    default:
        return;
    }
    e->accept();
}


void SpectraWidget::showNextScan() { incrementScan(+1, 0); }
void SpectraWidget::showLastScan() { incrementScan(-1, 0); }
void SpectraWidget::showNextFullScan() { incrementScan(+1, 1); }
void SpectraWidget::showLastFullScan() { incrementScan(-1, 1); }


void SpectraWidget::incrementScan(int increment, int msLevel=0 ) {
	if (_currentScan == NULL || _currentScan->sample == NULL) return;

	mzSample* sample = _currentScan->getSample();
    if (sample == NULL) return;

	Scan* newScan=sample->getScan(_currentScan->scannum+increment);
    if (msLevel != 0 && newScan && newScan->mslevel != msLevel ) {
        for(unsigned int i=newScan->scannum; i< sample->scans.size(); i+=increment ) {
            newScan =sample->getScan(i);
            if ( newScan && newScan->mslevel==msLevel) break;
        }
    }
    if(newScan==NULL) return;

    //do not reset soom when moving between full scans
	if(newScan->mslevel == 1 && _currentScan->mslevel == 1) {
		setScan(newScan,_minX,_maxX);
    }  else {
        setScan(newScan);
    }
    //if ( _resetZoomFlag == true ) { resetZoom(); _resetZoomFlag=false; }
}


void SpectraWidget::resizeEvent (QResizeEvent * event) {
    replot();
}

/*
void SpectraWidget::enterEvent (QEvent *) {
    grabKeyboard();
}

void SpectraWidget::leaveEvent (QEvent*) {
    releaseKeyboard();
}
*/

void SpectraWidget::addAxes() { 

	if (_drawXAxis ) {
		Axes* x = new Axes(0,_minX, _maxX,10);
		scene()->addItem(x);
		x->setZValue(998);
		_items.push_back(x);
	}

    if (_drawYAxis ) {
    	Axes* y = new Axes(1,_minY, _maxY,10);
    	scene()->addItem(y);
		y->setZValue(999);
        y->showTicLines(false);
		y->setOffset(5);
    	_items.push_back(y);
    }

}

void SpectraWidget::mousePressEvent(QMouseEvent *event) {
    QGraphicsView::mousePressEvent(event);
    _mouseStartPos = event->pos();

}

void SpectraWidget::mouseReleaseEvent(QMouseEvent *event) {
    if(! _currentScan) return;

    QGraphicsView::mouseReleaseEvent(event);
    _mouseEndPos	= event->pos();

    int deltaX =  _mouseEndPos.x() - _mouseStartPos.x();
    float deltaXfrac = (float) deltaX / (width()+1);


    if ( deltaXfrac > 0.01 ) {
        float xmin = invX( std::min(_mouseStartPos.x(), _mouseEndPos.x()) );
        float xmax = invX( std::max(_mouseStartPos.x(), _mouseEndPos.x()) );
        _minX = xmin;
        _maxX = xmax;
    } else if ( deltaXfrac < -0.01 ) {
		if ( _currentScan->mz.size() > 0 ) {
			float minmz = _currentScan->mz[0];
			float maxmz = _currentScan->mz[_currentScan->nobs()-1];
            _minX *= 0.9;
            _maxX *= 1.1;
            if (_minX < minmz ) _minX=minmz;
            if (_maxX > maxmz ) _maxX=maxmz;
        }
    } else if (_nearestCoord.x() > 0 ) {
        setMzFocus(_nearestCoord.x());
    }
    findBounds(false,true);
    replot();
}

void SpectraWidget::setMzFocus(Peak* peak) { 
    setMzFocus(peak->peakMz);
}

void SpectraWidget::setMzFocus(float mz) {
    if (_currentScan == NULL or _currentScan->mslevel>1) return;
    int bestMatch=-1; 
    float bestMatchDiff=FLT_MAX;

    mzSlice eicSlice = mainwindow->getEicWidget()->getMzSlice();

    /*
	if (!_currentScan->filterLine.empty() && _currentScan->mslevel==2) {
        mzSlice slice(mzmin,mzmax,eicSlice.rtmin,eicSlice.rtmax); 
		slice.srmId=_currentScan->filterLine;
        mainwindow->getEicWidget()->setMzSlice(slice);
		mainwindow->getEicWidget()->setFocusLine(_currentScan->rt);
        mainwindow->getEicWidget()->replotForced();
        return;
    }*/

	for (int i=0; i < _currentScan->nobs(); i++ ) {
		float diff = abs(_currentScan->mz[i] - mz);
        if ( diff < bestMatchDiff ) { bestMatchDiff = diff; bestMatch=i; }
    }

    if ( bestMatchDiff < 1 ) {
		float bestMz = _currentScan->mz[bestMatch];
        mainwindow->setMzValue(bestMz);
        mainwindow->massCalcWidget->setCharge(_currentScan->getPolarity());
        mainwindow->massCalcWidget->setMass(bestMz);
        //mainwindow->massCalc->compute();
    }
}

void SpectraWidget::mouseDoubleClickEvent(QMouseEvent* event){
    QGraphicsView::mouseDoubleClickEvent(event);
    _focusCoord = _nearestCoord;
    drawGraph();

}

void SpectraWidget::addLabel(QString text,float x, float y) {
    QFont font = QApplication::font(); font.setPointSizeF(font.pointSize()*0.8);

    QGraphicsTextItem* _label = scene()->addText(text, font);
    _label->setPos(toX(x), toY(y));
}

void SpectraWidget::mouseMoveEvent(QMouseEvent* event){
	if (_currentScan == NULL ) return;

    QGraphicsView::mouseMoveEvent(event);
    QPointF pos = event->pos();

    if (pos.y() < 5 || pos.y() > height()-5 || pos.x() < 5 || pos.y() > width()-5 ) {
        _vnote->hide(); _note->hide(); _varrow->hide(); _arrow->hide();
        return;
    }


    int nearestPos = findNearestMz(pos);
    if (nearestPos >= 0) {
		_nearestCoord = QPointF(_currentScan->mz[nearestPos], _currentScan->intensity[nearestPos]);
        drawArrow(_currentScan->mz[nearestPos], _currentScan->intensity[nearestPos], invX(pos.x()), invY(pos.y()));

        //search is too slow
        //if (mainwindow->massCalcWidget->isVisible())
        //	  mainwindow->massCalcWidget->setMass(_currentScan->mz[nearestPos]);
    } else {
        _vnote->hide(); _note->hide(); _varrow->hide(); _arrow->hide();
    }


}

int SpectraWidget::findNearestMz(QPointF pos) { 

    float mz = invX(pos.x());
    float mzmin = invX(pos.x()-50);
    float mzmax = invX(pos.x()+50);
    float ycoord  =invY(pos.y());
    int best=-1;

	vector<int> matches = _currentScan->findMatchingMzs(mzmin,mzmax);
    if (matches.size() > 0) {
        float dist=FLT_MAX;
        for(int i=0; i < matches.size(); i++ ) {
            int p = matches[i];
			float d = sqrt(POW2(_currentScan->intensity[p]-ycoord)+POW2(_currentScan->mz[p]-mz));
            if ( d < dist ){ best=p; dist=d; }
        }


    }
    return best;
}

void SpectraWidget::drawArrow(float mz1, float intensity1, float mz2, float intensity2) {

    float SCALE=1.0;
    float OFFSET=0;
    if (_showOverlay) { SCALE=0.45; OFFSET=-scene()->height()/2.0; }

    int x1=  toX(mz1);
    int y1=  toY(intensity1,SCALE,OFFSET);

    int x2 = toX(mz2);
    int y2 = toY(intensity2,SCALE,OFFSET);

    int x3 = toX(_focusCoord.x());
    int y3 = toY(_focusCoord.y(),SCALE,OFFSET);

    if ( ppmDist(mz1,mz2) < 0.1 ) return;


    if (_arrow != NULL ) {
        _arrow->setVisible(true);
        _arrow->setLine(x1,y1,x2,y2);
    }

    float distance = mz1-_focusCoord.x();
    //float totalInt = _focusCoord.y()+intensity1;
    float totalInt = _focusCoord.y();
    float diff=0;
    if (totalInt)  diff = intensity1/totalInt;

    if (_varrow != NULL && abs(distance) > 0.1 ) {
        _varrow->setLine(x1,y1,x3,y3);
        _vnote->setPos(x1+(x3-x1)/2.0, y1+(y3-y1)/2.0);
        _vnote->setPlainText(QString::number(diff*100,'f',2) + "%" );
        _varrow->show(); _vnote->show();
    } else {
        _varrow->hide(); _vnote->hide();
    }


    QString note = tr("m/z: %1 &Delta;%2").arg( QString::number(mz1,'f',6),QString::number(distance,'f',3));

    if (_note  != NULL ) {
        _note->setPos(x2+1,y2-30);
        _note->setHtml(note);
    }

    _note->show();
    _arrow->show();
}


void SpectraWidget::wheelEvent(QWheelEvent *event) {
    if ( event->delta() > 0 ) {
        zoomOut();
    } else {
        zoomIn();
    }
    replot();
}

/*
void SpectraWidget::annotateScan() {
    return;

    float mz1 = _focusCoord.x();
	if (mz1==0 || _currentScan == NULL) return;

	links = findLinks(mz1,_currentScan, 20, _currentScan->getPolarity());

    for(int i=0; i < links.size(); i++ ) {
		vector<int> matches = _currentScan->findMatchingMzs(links[i].mz2-0.01, links[i].mz2+0.01);
        //qDebug() << "annotateScan() " << links[i].note.c_str() << " " << links[i].mz2 << " " << matches.size();
        links[i].value2=0;

        if (matches.size() > 0) {
			links[i].value2 = _currentScan->intensity[matches[0]];

            for(int i=1; i < matches.size(); i++ ) {
				if(_currentScan->intensity[ matches[i] ] > links[i].value2 ) {
					links[i].value2==_currentScan->intensity[ matches[i] ];
                }
            }
        }
    }
}
*/

void SpectraWidget::annotateScan() {  }



void SpectraWidget::resetZoom() {
    findBounds(true,true);
    replot();
}



void SpectraWidget::zoomIn() {
    float D = (_maxX-_minX)/2;
    if (D < 0.5 ) return;

    float _centerX = _minX+D;

    if (_focusCoord.x() != 0 && _focusCoord.x() > _minX && _focusCoord.x() < _maxX ) _centerX = _focusCoord.x();
	//cerr << "zoomIn center=" << _centerX << " D=" << D <<  " focus=" << _focusCoord.x() << endl;

    _minX = _centerX-D/2;
    _maxX = _centerX+D/2;
    //cerr << _centerX << " " << _minX << " " << _maxX << " " << _minZ << " " << _maxZ << endl;
    findBounds(false,true);
    replot();

}

void SpectraWidget::zoomOut() {
	cerr << "zoomOut" << endl;
    _minX = _minX * 0.9;
    _maxX = _maxX * 1.1;
    findBounds(false,true);
    replot();
}


void SpectraWidget::compareScans(Scan* s1, Scan* s2) { 

}

void SpectraWidget::contextMenuEvent(QContextMenuEvent * event) {
    event->ignore();
    QMenu menu;

    QAction* a0 = menu.addAction("Reset Zoom");
    connect(a0, SIGNAL(triggered()), SLOT(resetZoom()));

    QAction* a1 = menu.addAction("Go To Scan");
    connect(a1, SIGNAL(triggered()), SLOT(gotoScan()));

    QAction* a6 = menu.addAction("Log10 Transform");
    a6->setCheckable(true);
    a6->setChecked(_log10Transform);
    connect(a6, SIGNAL(toggled(bool)), SLOT(setLog10Transform(bool)));
    connect(a6, SIGNAL(toggled(bool)),  SLOT(resetZoom()));

    QAction* a3b = menu.addAction("Find Similar Scans");
    connect(a3b, SIGNAL(triggered()), SLOT(findSimilarScans()));

    QAction* a3a = menu.addAction("Copy Top Peaks to Clipboard");
    connect(a3a, SIGNAL(triggered()), SLOT(spectraToClipboardTop()));

    QAction* a3 = menu.addAction("Copy Spectra to Clipboard");
    connect(a3, SIGNAL(triggered()), SLOT(spectraToClipboard()));

    QAction* a4 = menu.addAction("Profile Mode");
    connect(a4, SIGNAL(triggered()), SLOT(setProfileMode()));
    //connect(a4, SIGNAL(triggered()), SLOT(spectraToClipboard()));
    
    QAction* a5 = menu.addAction("Centroided Mode");
    connect(a5, SIGNAL(triggered()), SLOT(setCentroidedMode()));
        
    
   menu.exec(event->globalPos());
}

void SpectraWidget::spectraToClipboard() {
	if(!_currentScan) return;

    QStringList clipboardText;
	for(int i=0; i < _currentScan->nobs(); i++ ) {
        clipboardText  << tr("%1\t%2")
				.arg(QString::number(_currentScan->mz[i],'f', 6))
				.arg(QString::number(_currentScan->intensity[i],'f',6));
    }
    QApplication::clipboard()->setText(clipboardText.join("\n"));

}

void SpectraWidget::spectraToClipboardTop() {
    if(!_currentScan) return;
    vector< pair<float,float> > mzarray= _currentScan->getTopPeaks(0.05,1,5);

    QStringList clipboardText;
    for(int i=0; i < mzarray.size(); i++ ) {
    pair<float,float> p = mzarray[i];
            clipboardText  << tr("%1\t%2")
                .arg(QString::number(p.second,'f', 5))
                .arg(QString::number(p.first, 'f', 2));
    }
    QApplication::clipboard()->setText(clipboardText.join("\n"));
}


void SpectraWidget::gotoScan() { 
		if (_currentScan == NULL) return;
		int curScanNum = _currentScan->scannum;
		int maxScanNum = _currentScan->sample->scans.size()-1;
		bool ok=false;

		int scanNumber = QInputDialog::getInt (this, 
						"Go To Scan Number", "Enter Scan Number", curScanNum, 
						0, maxScanNum, 1, &ok, 0);
		if (ok && scanNumber > 0 && scanNumber < maxScanNum) {
			Scan* newscan = _currentScan->sample->scans[scanNumber];
			if (newscan) setScan(newscan);
		}
}

vector<mzLink> SpectraWidget::findLinks(float centerMz, Scan* scan, float ppm, int ionizationMode) {

    vector<mzLink> links;
    //check for possible C13s
    for(int i=1; i<20; i++ ) {
        if(i==0) continue;
        float mz=centerMz+(i*1.0034);
        float mzz=centerMz+(i*1.0034)/2;
        float mzzz=centerMz+(i*1.0034)/3;

        if( scan->hasMz(mz,ppm) ) {
            QString noteText = tr("C13-[%1]").arg(i);
            links.push_back(mzLink(centerMz,mz,noteText.toStdString()));
        }

        if( i % 2 !=0 && mzz!=mz && scan->hasMz(mzz,ppm) ) {
            QString noteText = tr("C13-[%1] z=2").arg(i);
            links.push_back(mzLink(centerMz,mzz,noteText.toStdString()));
        }

        if( i % 3 !=0 && scan->hasMz(mzzz,ppm) ) {
            QString noteText = tr("C13-[%1] z=3").arg(i);
            links.push_back(mzLink(centerMz,mzzz,noteText.toStdString()));
        }
    }

    for(int i=0; i < DB.fragmentsDB.size(); i++ ) {
        Adduct* frag  = DB.fragmentsDB[i];
    	if(frag->charge != 0 && SIGN(frag->charge) != SIGN(ionizationMode) ) continue;
        float mzMinus=centerMz-frag->mass;
        float mzPlus =centerMz+frag->mass;
        if( scan->hasMz(mzPlus,ppm)) {
            QString noteText = tr("%1 Fragment").arg(QString(DB.fragmentsDB[i]->name.c_str()));
            links.push_back(mzLink(centerMz,mzPlus,noteText.toStdString()));
        }

        if( scan->hasMz(mzMinus,ppm)) {
            QString noteText = tr("%1 Fragment").arg(QString(DB.fragmentsDB[i]->name.c_str()));
            links.push_back(mzLink(centerMz,mzMinus,noteText.toStdString()));
        }
    }

    //parent check
    for(int i=0; i < DB.adductsDB.size(); i++ ) {
    	if ( SIGN(DB.adductsDB[i]->charge) != SIGN(ionizationMode) ) continue;
        float parentMass=DB.adductsDB[i]->computeParentMass(centerMz);
        parentMass += ionizationMode*HMASS;   //adjusted mass

        if( abs(parentMass-centerMz)>0.1 && scan->hasMz(parentMass,ppm)) {
            cerr << DB.adductsDB[i]->name << " " << DB.adductsDB[i]->charge << " " << parentMass << endl;
            QString noteText = tr("Possible Parent %1").arg(QString(DB.adductsDB[i]->name.c_str()));
            links.push_back(mzLink(centerMz,parentMass,noteText.toStdString()));
        }
    }

    //adduct check
    for(int i=0; i < DB.adductsDB.size(); i++ ) {
    	if ( SIGN(DB.adductsDB[i]->charge) != SIGN(ionizationMode) ) continue;
        float parentMass = centerMz-ionizationMode*HMASS;   //adjusted mass
        float adductMass=DB.adductsDB[i]->computeAdductMass(parentMass);
        if( abs(adductMass-centerMz)>0.1 && scan->hasMz(adductMass,ppm)) {
            QString noteText = tr("Adduct %1").arg(QString(DB.adductsDB[i]->name.c_str()));
            links.push_back(mzLink(centerMz,adductMass,noteText.toStdString()));
        }
    }


    return links;
}


void SpectraWidget::constructAverageScan(float rtmin, float rtmax) {
    if (_avgScan != NULL) delete(_avgScan);
    _avgScan=NULL;

    if (_currentScan && _currentScan->getSample()) {
        Scan* avgScan = _currentScan->getSample()->getAverageScan(rtmin,rtmax,_currentScan->mslevel,_currentScan->getPolarity(),(float)100.0);
        qDebug() << "constructAverageScan() " << rtmin << " " << rtmax << " mslevel=" << _currentScan->mslevel << endl;
        avgScan->simpleCentroid();
        if(avgScan) setScan(avgScan);
    }
}


void SpectraWidget::findSimilarScans() {
    if(!_currentScan) return;
    vector< pair<float,float> > mzarray= _currentScan->getTopPeaks(0.05,1,5);

    QStringList clipboardText;
    for(int i=0; i < mzarray.size(); i++ ) {
    pair<float,float> p = mzarray[i];
            clipboardText  << tr("%1\t%2")
                .arg(QString::number(p.second,'f', 5))
                .arg(QString::number(p.first, 'f', 2));
    }

    mainwindow->spectraMatchingForm->fragmentsText->setPlainText(clipboardText.join("\n"));
    mainwindow->spectraMatchingForm->precursorMz->setText(QString::number(_currentScan->precursorMz,'f',6));
    mainwindow->spectraMatchingForm->show();

}

