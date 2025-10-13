
#ifndef LINE_H
#define LINE_H

#include "stable.h"
#include "mzSample.h"
#include "mainwindow.h"

class EIC;
class MainWindow;

class EicLine : public QGraphicsItem
{
public:
    EicLine(QGraphicsItem* parent, QGraphicsScene *scene, MainWindow* mainwindow=nullptr);
    void addPoint(float x, float y)  { _line << QPointF(x,y+_verticalOffset); }
    void addPoint(QPointF p)  { _line << QPointF(p.x(), p.y()+_verticalOffset); }
    void setColor(QColor &c)  { _color = c; }
    void setPen(QPen &p)  { _pen = p; }
    void setBrush(QBrush &b)  { _brush = b; }
    void fixEnds();
    void setEIC(EIC* e) { _eic = e; }
    EIC* getEIC() { return _eic; }
    bool isHighlighed() { return _highlighted; }
    void setHighlighted(bool value) { _highlighted=value; }
    void setFillPath(bool value) { _fillPath=value; }
    QPainterPath shape() const;
    void setClosePath(bool value ) {_closePath=value;}
    void setMainWindow(MainWindow* mainwindow){_mainwindow=mainwindow;}
    void setEmphasizePoints(bool value) {_emphasizePoints = value;}
    bool isEmphasizePoints(){return _emphasizePoints;}
    void setVerticalOffset(int verticalOffset){_verticalOffset=verticalOffset;}

protected:
    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    void hoverEnterEvent (QGraphicsSceneHoverEvent*event );
    void hoverLeaveEvent (QGraphicsSceneHoverEvent*event );
    
    
private:
    bool _highlighted;
    EIC* _eic;
    QPolygonF _line;
    QColor _color;
    QPen _pen;
    QBrush _brush;       
    bool _endsFixed;
    bool _closePath;
    bool _fillPath;
    bool _emphasizePoints;
    MainWindow* _mainwindow;
    int _verticalOffset = 0;

};

#endif
