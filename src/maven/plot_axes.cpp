#include "plot_axes.h"

Axes::Axes( int type, float min, float max, int nticks) {
    this->type = type;
    this->min = min;
    this->max = max;
    this->nticks = nticks;
	this->offset=0;
	this->margin=0;
	this->tickLinesFlag=false;
}

QRectF Axes::boundingRect() const
{   

    int textmargin=50;
	if (!scene())return QRectF(0,0,0,0);

	if(type == 0 ) {
    	return(QRectF(0,scene()->height()-textmargin,scene()->width(),scene()->height()-textmargin));
	} else {
        return(QRectF(0,0,+textmargin,scene()->height()+textmargin));
	}
}

void Axes::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    QPen pen(Qt::darkGray, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter->setPen(pen);

    double fontsize=12;
    QFont font("Helvetica",fontsize);
    font.setBold(true);
    painter->setFont(font);

    if (nticks == 0 ) nticks = 2;
    int x0 = margin;
    int x1 = scene()->width()-margin;

    int Y0=scene()->height()-offset;
	int X0=margin+offset;

    float range = (max-min);
    float expbase = pow(10,floor( log(range/nticks)/log(10) ) );
    float b;
    for(int i=0; i<10; i+=1) { b=i*expbase; if (b>range/nticks) break; }

    float ticks = range/b;
    float ix = (x1-x0)/ticks;

    int lastTickTextHeight = 20;
    int y0 = rint((scene()->height()-margin) * (1.0f - renderOffset));
    int y1 = margin+lastTickTextHeight+offset+verticalOffset;

    float yRange = y1-y0;
    float iy = yRange/nticks; //independent variable, use slope to find corresponding intensities

    float intensityCoordMin = (min/renderScale) * (1 - (y0/(scene()->height())) - renderOffset);
    float intensityCoordMax = (max/renderScale) * (1 - (y1/(scene()->height())) - renderOffset);

//    //(x1, y1) = (y0, intensityCoordMin) and (x2, y2) = (y1, intensityCoordMax)
    float m = (intensityCoordMax-intensityCoordMin)/(y1-y0);
    float y_intercept = min - m * y0;

    ticks = nticks;

    if ( b <= 0 ) return;

    if ( type == 0) { 	//X axes
        painter->drawLine(x0,Y0,x1,Y0);
        for (int i=0; i <= ticks; i++ ){

            float xCoord = x0+ix*i;

            if (xCoord > x1) continue;

            painter->drawLine(static_cast<int>(xCoord),Y0-5,static_cast<int>(xCoord),Y0+5);
            painter->drawText(static_cast<int>(xCoord),Y0+12,QString::number(min+b*i,'f',2));
        }
    } else if ( type == 1 ) { //Y axes

        painter->drawLine(X0,y0,X0,y1);

		for (int i=0; i <= ticks; i++ ) {
			painter->drawLine(X0-5,y0+iy*i,X0+5,y0+iy*i);
		}

        for (int i=0; i <= ticks; i++ ) {

            int axisCoord = y0+iy*i;

           float val = m*axisCoord + y_intercept;

            QString value;

            if ( max < 10000 ) {
                value = QString::number(val,'f',1);
            } else {
                value = QString::number(val,'g',2);
            }

            painter->drawText(X0+2,axisCoord,value);

//            qDebug() << "TEXT=" << value << " scene()->height()=" << scene()->height() << " m=" << m << " b=" << y_intercept << " intensityCoordMax=" << intensityCoordMax
//                     << " @ y0=" << y0 <<", Y0=" << Y0 << "y1= " << y1 << "y()" << y() << ", margin=" << margin << ", i=" << i << ", iy=" << iy
//                     << ", iy*i=" << iy*i << ", y0+iy*i=" << y0+iy*i;
        }

		if(tickLinesFlag) {
			//horizontal tick lines
			QPen pen(Qt::gray, 0.1,Qt::DotLine);
			painter->setPen(pen);
			for (int i=0; i <= ticks; i++ ) painter->drawLine(X0-5,y0+iy*i,x1,y0+iy*i);
		}
    }
	//painter->drawText(posX-10,posY-3,QString( eics[i]->sampleName.c_str() ));
}

