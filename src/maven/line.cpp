#include "line.h"

EicLine::EicLine(QGraphicsItem* parent, QGraphicsScene *scene, MainWindow* mainwindow): QGraphicsItem(parent)
{
    setHighlighted(false);
    setAcceptHoverEvents(false);

    _endsFixed=false;
    _fillPath=false;
    _closePath=true;
    _eic=nullptr;

    //Issue 374
    _emphasizePoints = false;

    if(scene) scene->addItem(this);

    _mainwindow = mainwindow;
}

QRectF EicLine::boundingRect() const
{

    return(_line.boundingRect());
}
void EicLine::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
   // qDebug() << "EicLine::paint()" << _line.isClosed() << " " << _line.size();

   //creates closed path from end point, to base, and from base up to start point
   if (_closePath == true && _endsFixed == false) fixEnds();
    painter->setPen(_pen);
    painter->setBrush(_brush);

    if (isHighlighed()) {
	QPen pen = _pen;
        pen.setWidth( pen.width() + 2 ) ;
    	painter->setPen(pen);
    }

    //Issue 374: User intuition is to always show polygon
    painter->drawPolygon(_line);

//    if (_fillPath) painter->drawPolygon(_line);
//    //else painter->drawPolyline(_line);
//    else painter->drawLines(_line);

    if (_emphasizePoints) {

        if (_mainwindow) {
            painter->setPen(QPen(_mainwindow->getBackgroundAdjustedBlack(_mainwindow->spectraWidget)));
            painter->setBrush(QBrush(_mainwindow->getBackgroundAdjustedBlack(_mainwindow->spectraWidget)));
        } else {
            painter->setPen(QPen(Qt::black));
            painter->setBrush(QBrush(Qt::black));
        }

        int paintDiameter = 5;

        //blackPen.setWidth(blackPen.width() + 5) ;

        for (auto& point : _line) {
            painter->drawEllipse(static_cast<int>(point.x()-paintDiameter/2),
                                 static_cast<int>(point.y()-paintDiameter/2),
                                 paintDiameter,
                                 paintDiameter);
        }
    }

}

QPainterPath EicLine::shape() const
{
   QPainterPath path;
   path.addPolygon(_line);
   return path;
}

void EicLine::hoverEnterEvent (QGraphicsSceneHoverEvent*event ) {
    qDebug() << "EicLine:: HoverEntered..";
    QGraphicsItem::hoverEnterEvent(event);
    setHighlighted(true);
    setZValue(zValue()+1);
    scene()->update();
}


void EicLine::hoverLeaveEvent ( QGraphicsSceneHoverEvent*) {
    setHighlighted(false);
    setZValue(zValue()-1);
    scene()->update();
}


void EicLine::fixEnds() { 

    if(_line.size() < 1) return;
    if (! scene()) return;
    QPointF p1 = _line[0];
    QPointF p2 = _line[_line.size()-1];
    QPointF a(p2.x(),scene()->height()+_verticalOffset);
    QPointF b(p1.x(),scene()->height()+_verticalOffset);
    _line.append(a);
    _line.append(b);
    _line.append(p1);
    _endsFixed=true;
} 
