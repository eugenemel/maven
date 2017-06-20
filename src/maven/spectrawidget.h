#ifndef SPECTRAWIDGET_H
#define SPECTRAWIDGET_H

#include "spectralhit.h"
#include "stable.h"
#include "mzSample.h"
#include "mainwindow.h"
#include "BondBreaker.h"

class SpectraWidget : public QGraphicsView
{
    Q_OBJECT
public:
    SpectraWidget(MainWindow* mw);
    static vector<mzLink> findLinks(float centerMz, Scan* scan, float ppm, int ionizationMode);


public slots:
    void setScan(Scan* s);
    void setScan(Scan* s, float mzmin, float mzmax );
                    void setScan(Peak* peak);
                    void setScan(mzSample* sample, int scanNum);
                    void setMzFocus(float mz);
                    void setMzFocus(Peak* peak);
                    void incrementScan(int direction, int msLevel);
                    void gotoScan();
                    void showNextScan();
                    void showLastScan();
                    void showNextFullScan();
                    void showLastFullScan();
                    void replot();
                    void spectraToClipboard();
                    void spectraToClipboardTop();
                    void overlaySpectralHit(SpectralHit& hit);
                    void overlayCompound(Compound* c);
                    void overlayPeakGroup(PeakGroup* group);
                    void overlayTheoreticalSpectra(Compound* c);

                    void resetZoom();
                    void zoomIn();
                    void zoomOut();
                    void setProfileMode() { _profileMode=true;  }
                    void setCentroidedMode() { _profileMode=false; }
                    void setLog10Transform(bool flag) { _log10Transform=flag; }
                    void setCurrentScan(Scan* scan);
                    void constructAverageScan(float rtmin, float rtmax);
                    void findSimilarScans();
                    void clearOverlay();
                    Scan* getCurrentScan() { return _currentScan; }

        private:
                    MainWindow* mainwindow;
                    Scan* _currentScan;
                    Scan* _avgScan;


                    vector<mzLink> links;
                    bool  _drawXAxis;
                    bool  _drawYAxis;
                    bool  _resetZoomFlag;
                    bool  _profileMode;
                    bool  _showOverlay;
                    bool  _log10Transform;

                    float _minX;
                    float _maxX;
                    float _minY;
                    float _maxY;
                    float _zoomFactor;

                    SpectralHit  _spectralHit;

                    QPointF _mouseStartPos;
                    QPointF _mouseEndPos;

                    QPointF _focusCoord;
                    QPointF _nearestCoord;

                    QGraphicsTextItem* _title;
                    QGraphicsTextItem* _note;
                    QGraphicsTextItem* _vnote;
                    QGraphicsLineItem* _arrow;
                    QGraphicsLineItem* _varrow;

                    vector<QGraphicsItem*> _items;  //graphic items on the plot
                    vector<QGraphicsItem*> overlayItems;  //graphic items on the plot

                    void initPlot();
                    void addAxes();
                    void findBounds(bool checkX, bool checkY);
                    void drawGraph();
                    void drawSpectalHitLines(SpectralHit& hit);

                    float toX(float x)  { return( (x-_minX)/(_maxX-_minX) * scene()->width()); }
                    float toY(float y, float Norm=1.0, float offset=0)  { float H=scene()->height(); return(H-((y-_minY)/(_maxY-_minY) *H)*Norm)+offset; };
                    float invX(float x) { return(  x/scene()->width())  * (_maxX-_minX) + _minX; }
                    float invY(float y) { return  -1*((y-scene()->height())/scene()->height() * (_maxY-_minY) + _minY); }

                    int findNearestMz(QPointF pos);
                    void drawArrow(float mz1, float int1, float mz2, float ints2);

                    void setDrawXAxis(bool flag) { _drawXAxis = flag; }
                    void setDrawYAxis(bool flag) { _drawYAxis = flag; }
                    void addLabel(QString text, float x, float y);
                    void setTitle();
                    void compareScans(Scan*, Scan*);
                    void annotateScan();

		protected:
                    //void leaveEvent ( QEvent * event );
                    //void enterEvent(QEvent * event);
                    void keyPressEvent(QKeyEvent *event);
                    void resizeEvent ( QResizeEvent * event );
                    void mouseReleaseEvent(QMouseEvent * mouseEvent);
                    void mousePressEvent(QMouseEvent * mouseEvent);
                    void mouseMoveEvent(QMouseEvent * mouseEvent);
                    void mouseDoubleClickEvent ( QMouseEvent * event );
                    void wheelEvent(QWheelEvent *event);
                    void contextMenuEvent(QContextMenuEvent * event);



                };

#endif

