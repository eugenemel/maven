#include "point.h"
EicPoint::EicPoint(float x, float y, Peak* peak, MainWindow* mw)
{

    setFlag(QGraphicsItem::ItemIsFocusable);
    setFlag(QGraphicsItem::ItemIsSelectable);
    setAcceptHoverEvents(true);

    _x = x;
    _y = y;
    _mw = mw;
    _peak = peak;
    _group = NULL;
    _scan = NULL;
    _cSize = 10;
    _color=QColor(Qt::black);
    _pen=QPen(_color);
    _brush=QBrush(_color);

    setPointShape(CIRCLE);
    forceFillColor(false);

    if (_peak) {
        _cSize += 20*(_peak->quality);
        //mouse press events
         connect(this, SIGNAL(peakSelected(Peak*)), mw, SLOT(showPeakInfo(Peak*)));
         connect(this, SIGNAL(peakSelected(Peak*)), mw->getEicWidget(), SLOT(showPeakArea(Peak*)));

         //mouse hover events
         connect(this, SIGNAL(peakGroupFocus(PeakGroup*)), mw->getEicWidget(), SLOT(setSelectedGroup(PeakGroup*)));
         connect(this, SIGNAL(peakGroupFocus(PeakGroup*)), mw->getEicWidget()->scene(), SLOT(update()));
         connect(this, SIGNAL(peakGroupSelected(PeakGroup*)), mw->massCalcWidget, SLOT(setPeakGroup(PeakGroup*)));
         connect(this, SIGNAL(peakGroupSelected(PeakGroup*)), mw->fragmenationSpectraWidget, SLOT(overlayPeakGroup(PeakGroup*)));
    }

}

QRectF EicPoint::boundingRect() const
{
    return(QRectF(_x-_cSize/2,_y-_cSize/2,_cSize,_cSize));
}

void EicPoint::hoverEnterEvent (QGraphicsSceneHoverEvent*) {
   // qDebug() << "EicPoint:: hoverEnterEvent()";
    //this->setFocus(Qt::MouseFocusReason);

	string sampleName;
    if (_peak && _peak->getSample() ) {
        sampleName = _peak->getSample()->sampleName;

        setToolTip( "<b>  Sample: </b>"   + QString( sampleName.c_str() ) +
                          "<br> <b>intensity: </b>" +   QString::number(_peak->peakIntensity) +
                            "<br> <b>area: </b>" + 		  QString::number(_peak->peakAreaCorrected) +
                            "<br> <b>rt: </b>" +   QString::number(_peak->rt, 'f', 2 ) +
                            "<br> <b>scan#: </b>" +   QString::number(_peak->scan ) + 
                            "<br> <b>m/z: </b>" + QString::number(_peak->peakMz, 'f', 6 )
                            );
	} else if (_scan) { 
		setToolTip( "<b>  Sample: </b>"   + QString( _scan->sample->sampleName.c_str() ) +
					"<br> <b>FilterLine: </b>" + 		  QString(_scan->filterLine.c_str() ) + 
					"<br> <b>Scan#: </b>" +   QString::number(_scan->scannum) +
					"<br> <b>PrecursorMz: </b>" +   QString::number(_scan->precursorMz, 'f', 2 )
		);
	}

    if(_group and not _group->isEmpty()) {
        _group->isFocused = true;
        emit(peakGroupFocus(_group));
    }
}

void EicPoint::hoverLeaveEvent ( QGraphicsSceneHoverEvent*) {
    clearFocus();

    if (_group)  _group->isFocused = false;
    update(); 
}

void EicPoint::mouseDoubleClickEvent(QGraphicsSceneMouseEvent*) {
   bookmark();
}
void EicPoint::mousePressEvent (QGraphicsSceneMouseEvent* event) {
    //if (_group) _group->groupOveralMatrix();

    if (event->button() == Qt::RightButton)  {
        QGraphicsSceneContextMenuEvent e; e.setPos(event->pos());
        this->contextMenuEvent(&e);
        event->ignore();
        return;
    }

    setZValue(10);

    if (_group) emit peakGroupSelected(_group);
    if (_peak)  emit peakSelected(_peak);

    if ( _group && _group->isIsotope() == false ) {
        _mw->isotopeWidget->setPeakGroup(_group);
    }

    if(_scan) {
        _mw->fragmenationSpectraDockWidget->setVisible(true);
        _mw->fragmenationSpectraWidget->setScan(_scan);
        _mw->massCalcWidget->setFragmentationScan(_scan);
       // if(_scan->mslevel >= 2)  _mw->spectralHitsDockWidget->limitPrecursorMz(_scan->precursorMz);
    }

    if (_peak && _mw->covariantsPanel->isVisible()) {
        _mw->getCovariants(_peak);
    }

    //scene()->update();
}

void EicPoint::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    QPen pen = _pen;
    QBrush brush = _brush;

    float scale = min(scene()->height(),scene()->width())/500;
    float paintDiameter = _cSize*scale;  
    if (paintDiameter<5) paintDiameter=5; if (paintDiameter>30) paintDiameter=30;

    PeakGroup* selGroup = _mw->getEicWidget()->getSelectedGroup();

    if (_group != NULL && selGroup->minMz == _group->minMz && selGroup->minRt == _group->minRt ) {
        brush.setStyle(Qt::SolidPattern);
        pen.setColor(_color.darker());
        pen.setWidth(_pen.width()+1);
    } else {
        brush.setStyle(Qt::NoBrush);
    }

    if (_forceFill) {
        brush.setStyle(Qt::SolidPattern);
    }

    painter->setPen(pen);
    painter->setBrush(brush);

    if(pointShape == CIRCLE ) {
        painter->drawEllipse(_x-paintDiameter/2, _y-paintDiameter/2, paintDiameter,paintDiameter);
    } else if (pointShape == SQUARE) {
        painter->drawRect(_x-paintDiameter/2, _y-paintDiameter/2, paintDiameter,paintDiameter);
    } else if (pointShape == TRIANGLE_DOWN ) {
        painter->drawPie(_x-paintDiameter/2,_y-paintDiameter/2,paintDiameter,paintDiameter,30*16,120*16);
    } else if (pointShape == TRIANGLE_UP ) {
        painter->drawPie(_x-paintDiameter/2,_y-paintDiameter/2,paintDiameter,paintDiameter,260*16,20*16);
    }
}

void EicPoint::setClipboardToGroup() { if(_group) _mw->setClipboardToGroup(_group); }

void EicPoint::bookmark() { if(_group) _mw->bookmarkPeakGroup(_group); }

void EicPoint::setClipboardToIsotopes() {
    if (_group &&_group->compound != NULL && ! _group->compound->formula.empty() )  {
        _mw->isotopeWidget->setPeakGroup(_group);
    }
}

void EicPoint::linkCompound() {
    if (_group &&_group->compound != NULL )  {
            //link group to compound
            _group->compound->setPeakGroup(*_group);

            //update compound retention time
            if (_peak) _group->compound->expectedRt=_peak->rt;

            //log information about retention time change
           // _mw->getEicWidget()->addNote(_peak->peakMz,_peak->peakIntensity, "Compound Link");
            _mw->getEicWidget()->saveRetentionTime();

            //upadte ligand widget
            QString dbname(_group->compound->db.c_str());
            _mw->ligandWidget->setDatabaseAltered(dbname,true);
            //_mw->ligandWidget->updateTable();
            _mw->ligandWidget->updateCurrentItemData();
	}
}

void EicPoint::contextMenuEvent (QGraphicsSceneContextMenuEvent* event) {
    QMenu menu;

    QAction* c1 = menu.addAction("Copy Details to Clipboard");
    c1->setIcon(QIcon(rsrcPath + "/copyCSV.png"));
    connect(c1, SIGNAL(triggered()), SLOT(setClipboardToGroup()));

    if (_group && _group->compound ) {
       if ( _group->isIsotope() == false && !_group->compound->formula.empty() ) {
            QAction* z = menu.addAction("Copy Isotope Information to Clipboard");
            z->setIcon(QIcon(rsrcPath + "/copyCSV.png"));
            connect(z, SIGNAL(triggered()), SLOT(setClipboardToIsotopes()));
        }

        QAction* e = menu.addAction("Link to Compound");
        e->setIcon(QIcon(rsrcPath + "/link.png"));
        connect(e, SIGNAL(triggered()), SLOT(linkCompound()));
    }

    QAction* c2 = menu.addAction("Mark Good");
    c2->setIcon(QIcon(rsrcPath + "/markgood.png"));
    connect(c2, SIGNAL(triggered()), _mw->getEicWidget(), SLOT(markGroupGood()));

    QAction* c3 = menu.addAction("Mark Bad");
    c3->setIcon(QIcon(rsrcPath + "/markbad.png"));
    connect(c3, SIGNAL(triggered()), _mw->getEicWidget(), SLOT(markGroupBad()));

     QPointF p = event->pos();
     QPoint pos = scene()->views().first()->mapToGlobal(p.toPoint());
     menu.exec(pos);
     event->ignore();
}

void EicPoint::keyPressEvent( QKeyEvent *e ) {
	if (!_group) return;
	update();
	e->accept();
}
