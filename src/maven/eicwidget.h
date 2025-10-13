#ifndef PLOT_WIDGET_H
#define PLOT_WIDGET_H

#include "stable.h"
#include "mainwindow.h"
#include "line.h"
#include "barplot.h"
#include "boxplot.h"
#include "isotopeplot.h"
#include "point.h"
#include "plot_axes.h"


class PeakGroup;
class EIC;
class MainWindow;
class BarPlot;
class BoxPlot;
class IsotopePlot;
class Note;
class EicLine;

class EicWidget : public QGraphicsView
{
    Q_OBJECT

public:
    EicWidget(QWidget *p);
    ~EicWidget();

    vector<EIC*> getEICs() { return eics; }
    vector<PeakGroup>& getPeakGroups() { return peakgroups; }
    PeakGroup* getSelectedGroup() 	  {  return &_selectedGroup; }
    mzSlice&   getMzSlice() 		  {	 return _slice; }
    QString eicToTextBuffer();

public slots:
    void unsetAlwaysDisplayGroup() {_alwaysDisplayGroup = nullptr;}
    void setMzSlice(float mz);
    void setPPM(double ppm);

    void resetZoom(bool isReplot=true);
    void zoom(float factor);

    void setMzRtWindow(float mzmin, float mzmax, float rtmin, float rtmax);
    void setMzSlice(const mzSlice& slice, bool isUseSampleBoundsRT=true, bool isPreserveSelectedGroup=false);
    void setSRMTransition(SRMTransition* transition);
    void setRtWindow(float rtmin, float rtmax );
    void setSrmId(string srmId);
    void setPeakGroup(PeakGroup* group);
    void setCompound(Compound* c, Adduct* a, bool isPreservePreviousRtRange=false, bool isSelectGroupNearRt=true, bool isPreserveSelectedGroup=false);
    void setSelectedGroup(PeakGroup* group);

    void addEICLines(bool showSpline);
    void addCubicSpline();
    void addBaseLine();
    void addBaseline(PeakGroup* group);
    void addTicLine();
    void addMergedEIC();
    void setFocusLine(float rt);
    void setFocusLines(vector<float> rts);
    void drawRtRangeLines(PeakGroup* group);
    void drawRtRangeLines(float rtmin, float rtmax);
    void drawRtRangeLinesCurrentGroup();
    void drawFocusLines();
    void drawSelectionLine(float rtmin,float rtmax);
    void addFocusLine(PeakGroup*);
    void addBarPlot(PeakGroup*);
    void addBoxPlot(PeakGroup*);
    void addIsotopicPlot(PeakGroup*);
    void addFitLine(PeakGroup*);
    void addMS2Events(float mzmin, float mzmax);
    void integrateRegion(float rtmin, float rtmax);
    void recompute(bool isUseSampleBoundsRT=true);
    void replot(PeakGroup* group);
    void replot();
    void replotForced();
    void print(QPaintDevice* printer);
    void showPeakArea(Peak*);
    void saveRetentionTime();
    void setGalleryToEics();

    void selectGroupNearRt(float rt);
    void eicToClipboard();

    void showSpline(bool f) { _showSpline=f; }
    void showPeaks(bool f)  { _showPeaks=f; }
    void showTicLine(bool f) { _showTicLine=f; }
    void showBicLine(bool f) { _showBicLine=f; }
    void showBaseLine(bool f) { _showBaseline=f; }
    void showNotes(bool f) { _showNotes=f; }
    void showMergedEIC(bool f) { _showMergedEIC=f; }
    void showEICLines(bool f) { _showEICLines=f; }
    void showEICFill(bool f) { _showEICLines=!f; }
    void automaticPeakGrouping(bool f) { _groupPeaks=f; }
    void showMS2Events(bool f);
    void disconnectCompounds(QString libraryName);
    void emphasizeEICPoints(bool f) { _emphasizeEICPoints=f;}
    void preservePreviousRtRange(bool f) { _isPreservePreviousRtRange=f;}

    bool isPreservePreviousRtRange() {return _isPreservePreviousRtRange;}

    void startAreaIntegration() { toggleAreaIntegration(true); }
    void startSpectralAveraging() { toggleSpectraAveraging(true); }
    void toggleAreaIntegration (bool f) { _areaIntegration=f; f ? setCursor(Qt::SizeHorCursor) : setCursor(Qt::ArrowCursor); }
    void toggleSpectraAveraging(bool f) { _spectraAveraging=f;f ? setCursor(Qt::SizeHorCursor) : setCursor(Qt::ArrowCursor); }


    void showIsotopePlot(bool f) { _showIsotopePlot=f; }
    void showBarPlot(bool f) { _showBarPlot=f; }
    void showBoxPlot(bool f) { _showBoxPlot=f; }

    void setStatusText(QString text);
    void autoZoom(bool f ) { _autoZoom=f; }

    void markGroupGood();
    void markGroupBad();
    void copyToClipboard();
    void selectionChangedAction();
    void freezeView(bool freeze);
    void groupPeaks();
    void clearEICLines();
    void clearFocusLine();
    void clearFocusLines();
    void clearRtRangeLines();
    void clearPeakAreas();

protected:
    void moved(QMouseEvent *event);
    void selected(const QRect&);
    void wheelEvent(QWheelEvent *event);
    //void scaleView(qreal scaleFactor);
    void mouseReleaseEvent(QMouseEvent * mouseEvent);
    void mousePressEvent(QMouseEvent * mouseEvent);
    void mouseMoveEvent(QMouseEvent * mouseEvent);
    void mouseDoubleClickEvent ( QMouseEvent * event );
    void resizeEvent( QResizeEvent * ) { replot(nullptr); }
    void contextMenuEvent(QContextMenuEvent * event);
    void keyPressEvent( QKeyEvent *e );
    void timerEvent ( QTimerEvent * event );

    void setupColors();
    void setTitle();
    void setScan(Scan*);
    void addAxes();
    void showAllPeaks();
    void addPeakPositions(PeakGroup* group);
    void createNotesTable();

signals:
    void viewSet(float,float,float,float);
    void scanChanged(Scan*);

private:

    //Issue 802: offset traces/axis to avoid MS2 triangles colliding with axes
    //When MS2s are displayed, offset some of the plot elements for cleaner visualization
    static const int EIC_LINE_AND_POINT_VERTICAL_OFFSET = -15;
    static const int MS2_VERTICAL_OFFSET = -13;

    int _eicLineAndPointVerticalOffset = 0;
    int _ms2VerticalOffset = 0;

    mzSlice _slice;						// current slice
    float _focusLineRt;					// 0

    vector <EIC*> eics;					// vectors mass slices one from each sample
    deque  <EIC*> tics;					// vectors total chromatogram intensities;

    float _minX;						//plot bounds
    float _minY;
    float _maxX;
    float _maxY;
    float _zoomFactor;					//scaling of zoom in

    QPointF _lastClickPos;
    QPointF _mouseStartPos;
    QPointF _mouseEndPos;

    BarPlot* _barplot;
    BoxPlot* _boxplot;
    IsotopePlot* _isotopeplot;
    Note* _statusText;

    bool _showSpline;
    bool _showBaseline;
    bool _showTicLine;
    bool _showBicLine;
    bool _showMergedEIC;
    bool _showNotes;
    bool _showPeaks;
    bool _showEICLines;
    bool _autoZoom;
    bool _groupPeaks;
    bool _showMS2Events;

    bool _areaIntegration;
    bool _spectraAveraging;

    bool _showIsotopePlot;
    bool _showBarPlot;
    bool _showBoxPlot;

    //Issue 426
    bool _isPreservePreviousRtRange;

    //Issue 374
    bool _emphasizeEICPoints;

    bool _frozen;
    int _freezeTime;
    int _timerId;
    float _intensityZoomVal = -1.0f;      //expect to be already converted with invY() --> peak intensity value

    int _xAxisPlotMargin = 20;

    vector<PeakGroup> peakgroups;	    //peaks grouped across samples

    PeakGroup  _selectedGroup;			//currently selected peak group

    PeakGroup*  _integratedGroup;		 //manually integrated peak group
    PeakGroup*  _alwaysDisplayGroup;     //always display this group, whether or not grouping is enabled

    //gui related
    QWidget *parent;
    QGraphicsLineItem* _focusLine;
    QGraphicsLineItem* _selectionLine;

    vector<float> _focusLinesRts{};
    QVector<QGraphicsLineItem*> _focusLines;

    //Issue 550
    QGraphicsLineItem* _rtMinLine;
    QGraphicsLineItem* _rtMaxLine;

    vector<EicLine*> _peakAreas{};

    void showPeak(float freq, float amplitude);
    void computeEICs(bool isUseSampleBoundsRT=true);
    void cleanup();		//deallocate eics, fragments, peaks, peakgroups
    void clearPlot();	//removes non permenent graphics objects

    void findPlotBounds(); //find _minX, _maxX...etc
    void findRtBounds();

    mzSlice visibleSamplesBounds(); //find absoulte max and min for visible samples
    mzSlice visibleEICBounds(); //find absolute min and max for extracted ion chromatograms

    float toX(float x);
    float toY(float y);
    float invX(float x);
    float invY(float y);


    MainWindow* getMainWindow();		//return parent 
    void zoomPeakGroup( PeakGroup* group );
    void blockEicPointSignals(bool isBlockSignals);
};


#endif
