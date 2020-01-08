#ifndef SAMPLEIMAGEIDGET_H
#define SAMPLEIMAGEIDGET_H

#include "mzSample.h"
#include "mainwindow.h"

class SampleImageWidget : public QGraphicsView
{
    Q_OBJECT
public:
    SampleImageWidget(MainWindow* mw);

public slots:
        void setMzFocus(float mz, float rt);
        void setMzFocus(Peak* peak);
        void overlayPeakGroup(PeakGroup* group);

        void replot();
        void resetZoom();
        void zoomRegion(float centerMz, float window);
        void zoomRegion(float mzmin, float mzmax, float rtmin, float rtmax);
        void clearOverlay();

        private:
         MainWindow* mainwindow;
                    bool  _drawXAxis;
                    bool  _drawYAxis;
                    bool  _resetZoomFlag;
                    bool  _log10Transform;

                    double _maxIntensityThreshold;
                    double _minIntensityThreshold;

                    mzSlice VS; //visible slice
                    float _zoomFactor;

                    QPointF _mouseStartPos;
                    QPointF _mouseEndPos;

                    QPointF _focusCoord;
                    QPointF _nearestCoord;

                    vector<QGraphicsItem*> _items;  //graphic items on the plot
                    QMap<string,QImage> _sampleImages;

                    void initPlot();
                    void addAxes();
                    void checkGlobalBounds();
                    void drawImage();
                    mzSlice getGlobalBounds();
                    void makeImages(float rtAccur, float mzAccur);

                    float toX(float px);
                    float invX(float x);
                    float toY(float py);
                    float invY(float y);

		protected:
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

