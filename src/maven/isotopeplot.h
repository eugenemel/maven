#ifndef ISOTOPEPLOT_H
#define ISOTOPEPLOT_H

#include "stable.h"
#include "mzSample.h"
#include "mainwindow.h"

class MainWindow;
class PeakGroup;
class QGraphicsItem;
class QGraphicsScene;

class IsotopeBar : public QObject, public QGraphicsRectItem
{
    Q_OBJECT
#if QT_VERSION >= 0x040600
    Q_INTERFACES( QGraphicsItem )
#endif

	public:
    IsotopeBar(QGraphicsItem *parent, QGraphicsScene *scene):QGraphicsRectItem(parent){
			setFlag(ItemIsSelectable);
			setFlag(ItemIsFocusable);
            setAcceptHoverEvents(true);
	}

    QRectF boundingRect() const {
        return QGraphicsRectItem::boundingRect();
    }

	signals:
		void groupSelected(PeakGroup* g);
		void groupUpdated(PeakGroup*  g);
		void showInfo(QString,int xpos=0, int ypos=0);
        void showMiniEICPlot(PeakGroup*g);
        void peakSelected(Peak*);

	protected:        
        void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
            QGraphicsRectItem::paint(painter,option,widget);
        }

	void hoverEnterEvent (QGraphicsSceneHoverEvent*event);
//	void mouseDoubleClickEvent (QGraphicsSceneMouseEvent*event);
    void mousePressEvent (QGraphicsSceneMouseEvent*event);
	void keyPressEvent(QKeyEvent *e);
};


class IsotopePlot : public QObject, public QGraphicsItem
{
    Q_OBJECT

#if QT_VERSION >= 0x040600
    Q_INTERFACES( QGraphicsItem )
#endif

public:
    IsotopePlot(QGraphicsItem *parent, QGraphicsScene *scene);
    ~IsotopePlot();

    void setPeakGroup(PeakGroup* group);
    void setMainWindow(MainWindow* mw);
    QRectF boundingRect() const;
    void clear();
    void showBars();
    inline void setIsInLegendWidget(bool isInLegendWidget){_isInLegendWidget = isInLegendWidget;}
    inline void setWidth(float width){_width = width;}

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event);

private:

    float _width;
    float _height;
    float _barheight;
    bool _isInLegendWidget = false;

    QGraphicsTextItem* _parameters;

    vector<mzSample*> _samples;
    MainWindow* _mw;

    PeakGroup* _group;
    vector<PeakGroup*> _isotopes;

    void computeParameters();
};

#endif
