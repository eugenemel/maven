#ifndef EICPOINT_H
#define EICPOINT_H

#include "stable.h"
#include "globals.h"
#include "mainwindow.h"
#include "note.h"

class Peak;
class PeakGroup;
class MainWindow;
class SpectraWidget;

class EicPoint : public QObject, public QGraphicsItem {
    Q_OBJECT
    Q_INTERFACES( QGraphicsItem )

public:
    enum POINTSHAPE { CIRCLE, SQUARE, TRIANGLE_UP, TRIANGLE_DOWN };
    EicPoint(QObject *parent = nullptr);
    EicPoint(float x, float y, Peak* peak, MainWindow* mw);
    void setColor(QColor &c)  { _color = c; _pen.setColor(c); _brush.setColor(c); }
    void setPen(QPen &p)  { _pen = p;  }
    void setBrush(QBrush &b)  { _brush = b; }
    void setPeakGroup(PeakGroup* g) { _group = g; }
    void setPeak(Peak* p) { _peak=p; }
    void setScan(Scan* x) { _scan=x; }
    Peak* getPeak() { return _peak; }
    PeakGroup* getPeakGroup() { return _group; }
    void setPointShape(POINTSHAPE shape) { pointShape=shape; }
    void forceFillColor(bool flag) { _forceFill = flag; }
    void setSize(float size) { _cSize=size; }

    enum {Type = UserType + 1};

    int type() const override {return Type;}

protected:
    QRectF boundingRect() const;
 	void hoverEnterEvent ( QGraphicsSceneHoverEvent * event );
	void hoverLeaveEvent ( QGraphicsSceneHoverEvent * event );
    void mousePressEvent( QGraphicsSceneMouseEvent * event);
	void mouseDoubleClickEvent (QGraphicsSceneMouseEvent* event);
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    void contextMenuEvent ( QGraphicsSceneContextMenuEvent* event );
	void keyPressEvent(QKeyEvent *event);
    
private:
    float _x;
    float _y;
    Scan* _scan;
    Peak* _peak;
    PeakGroup* _group;
    MainWindow* _mw;
    QColor _color;
    QPen _pen;
    QBrush _brush;       
    POINTSHAPE pointShape;
    bool _forceFill;
    float _cSize;
    QRectF _boundingRect;

private slots:
	void bookmark();
	void linkCompound();
	void setClipboardToGroup();
    void setClipboardToIsotopes();

signals:
    void peakSelected(Peak*);
    void peakSelectedNoShiftModifier(Peak*);
    void peakGroupSelected(PeakGroup*);
    void spectaFocused(Peak*);
    void peakGroupFocus(PeakGroup*);
    void scanSelected(Scan*);


};

#endif
