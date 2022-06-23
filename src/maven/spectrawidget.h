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
    SpectraWidget(MainWindow* mw, int msLevel=1);
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

                    void overlaySpectralHit(SpectralHit& hit, bool isSetScanToHitScan=true);
                    void overlayCompound(Compound* c, bool isSetScanToHitScan=true);
                    void overlayPeakGroup(PeakGroup* group);
                    void overlayTheoreticalSpectra(Compound* c);

                    //Issue 442
                    void showMS2CompoundSpectrum(Compound *c);

                    void resetZoom();
                    void zoomIn();
                    void zoomOut();
                    void zoomRegion(float centerMz, float window);
                    void setProfileMode() { _profileMode=true;  }
                    void setCentroidedMode() { _profileMode=false; }
                    void setLog10Transform(bool flag) { _log10Transform=flag; }
                    void setCurrentFragment(Fragment* fragment, int mslevel=2);
                    void constructAverageScan(float rtmin, float rtmax);
                    void findSimilarScans();
                    void clearOverlay();
                    void toggleOverlayLabels();
                    void toggleDisplayFullTitle();
                    void toggleDisplayCompoundId();
                    void toggleDisplayHighPrecisionMz();
                    void clearOverlayAndReplot();

                    inline void setMs3MatchingTolerance(float tolr){_ms3MatchingTolr=tolr;}

                    Scan* getCurrentScan() { return _currentScan; }

                    void selectObservedPeak(int peakIndex);

                    void findBounds(bool checkX, bool checkY);
                    void lockMzRange();
                    void freeMzRange();
        private:
                    int _msLevel;
                    MainWindow* mainwindow;
                    Scan* _currentScan;
                    Scan* _avgScan;

                    float _ms3MatchingTolr = 0.5f;

                    Fragment* _currentFragment = nullptr;
                    map<mzSample*,unordered_set<int>> _sampleScanMap = {};
                    bool _isDisplayFullTitle=false;
                    bool _isDisplayCompoundId=false;
                    bool _isDisplayHighPrecisionMz=false;

                    vector<mzLink> links;
                    bool  _drawXAxis;
                    bool  _drawYAxis;
                    bool  _resetZoomFlag;
                    bool  _profileMode;
                    bool  _showOverlay;
                    bool  _log10Transform;
                    bool _showOverlayLabels = true;

                    float _minX; // units = m/z
                    float _maxX; // units = m/z
                    float _minY; // units = intensity
                    float _maxY; // units = intensity
                    float _zoomFactor;

                    float _maxIntensityScaleFactor = 1.3f;
                    float _showOverlayScale = 0.45f;
                    float _showOverlayOffset = 0.50f;

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
                    void drawGraph();
                    void drawSpectralHitLines(SpectralHit& hit);

                    float toX(float x)  { return( (x-_minX)/(_maxX-_minX) * scene()->width()); }
                    float toY(float y, float Norm=1.0, float offset=0)  { float H=scene()->height(); return(H-((y-_minY)/(_maxY-_minY) *H)*Norm)+offset; };
                    float invX(float x) { return(  x/scene()->width())  * (_maxX-_minX) + _minX; }
                    float invY(float y) { return  -1*((y-scene()->height())/scene()->height() * (_maxY-_minY) + _minY); }
                    double showOverlayOffset(){return -1*_showOverlayOffset*scene()->height();}

                    int findNearestMz(QPointF pos);
                    void drawArrow(float mz1, float int1, float mz2, float ints2);

                    void setDrawXAxis(bool flag) { _drawXAxis = flag; }
                    void setDrawYAxis(bool flag) { _drawYAxis = flag; }
                    void addLabel(QString text, float x, float y);
                    void setTitle();
                    void compareScans(Scan*, Scan*);
                    void annotateScan();

                    //strictly for internal adjustment - use setScan(Scan* scan) for slots.
                    void setCurrentScan(Scan* scan);

                    SpectralHit generateSpectralHitFromCompound(Compound *c);

                    //Issue 481: Use cache to avoid excessive recomputation during resizing
                    bool isUseCachedMatches = false;
                    vector<int> _matches{};

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

