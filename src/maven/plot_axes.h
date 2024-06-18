#ifndef PLOT_AXES_H
#define PLOT_AXES_H

#include "stable.h"
#include "mainwindow.h"

class Axes : public QGraphicsItem
{
public:
    Axes(QGraphicsItem* parent):QGraphicsItem(parent){}
    Axes( int type, float min, float max, int nticks, MainWindow *mainwindow);
    QRectF boundingRect() const;
	void setRange(double a, double b) { min=a; max=b; } 
	double getMin() {return min;}
	double getMax() {return max;}
	void setLabel(QString t) { label = t; }
	void setTitle(QString  t) { title =  t; }
	void setNumTicks(int x)  { nticks = x; }
	void preferedNumTicks(int x) {nticksPref = x; }
	void forceNumTicks(bool t) { force_nticks = t; }
	bool numTicksForced() { return force_nticks; }
	void useIntLabels(bool t) {intLabels = t;}
	void setMargin(int m) { margin=m; }
	void setOffset(int o ) { offset=o;  }
	void showTicLines(bool f) { tickLinesFlag=f; }

    void setRenderScale(float f){ renderScale = f;}
    void setRenderOffset(float f){ renderOffset = f;}
    void setVerticalOffset(int i){ verticalOffset = i;}
	
protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    
private:
   MainWindow *mainwindow = nullptr;
   int type;
   float min;
   float max;
   int nticks;
   int nticksPref;
   int margin;
   int offset;
   bool force_nticks;
   bool intLabels;
   bool tickLinesFlag;
   QString label;
   QString title;

   float renderScale = 1;
   float renderOffset= 0;
   int verticalOffset = 0;
};

#endif
