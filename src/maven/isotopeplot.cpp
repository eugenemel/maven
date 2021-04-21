#include "isotopeplot.h"

// import most common Eigen types
//USING_PART_OF_NAMESPACE_EIGEN
using namespace Eigen;


IsotopePlot::IsotopePlot(QGraphicsItem* parent, QGraphicsScene *scene)
    :QGraphicsItem(parent) {
    _barheight=10;
    _mw=nullptr;
    _group=nullptr;
    if (scene) { _width = scene->width()*0.25; }
	_height = 10;
    _parameters = nullptr;
}

void IsotopePlot::setMainWindow(MainWindow* mw) { _mw = mw; }

void IsotopePlot::clear() { 
    QList<QGraphicsItem *> mychildren = QGraphicsItem::childItems();
    if (mychildren.size() > 0 ) {
        foreach (QGraphicsItem *child, mychildren) {
            scene()->removeItem(child);
            delete(child);
        }
    }
}

void IsotopePlot::setPeakGroup(PeakGroup* group) {
    //cerr << "IsotopePlot::setPeakGroup()" << group << endl;

    if (!_mw) return;
    if (!group) return;
    if (group->isIsotope()) return;

    if (_group) {
        delete(_group);
        _group = nullptr;
    }

    _group = new PeakGroup(*group);

    //Issue 402: try to use saved parameters, fall back to current GUI settings
    IsotopeParameters isotopeParameters = group->isotopeParameters;
    if (isotopeParameters.isotopeParametersType == IsotopeParametersType::INVALID) {
        isotopeParameters = _mw->getIsotopeParameters();
    }
    _group->pullIsotopes(isotopeParameters);
    _group->isotopeParameters = isotopeParameters;

    if (_group->childCount() == 0) return; // Did not detect any isotopes

	_samples.clear();
	_samples = _mw->getVisibleSamples();
    if (_samples.empty()) return; // No visible samples

     sort(_samples.begin(), _samples.end(), mzSample::compSampleOrder);

    _isotopes.clear();
    for(unsigned int i=0; i < _group->childCount(); i++ ) {
        if (_group->children[i].isIsotope() ) {
            PeakGroup* isotope = &(_group->children[i]);
            _isotopes.push_back(isotope);
        }
    }
    std::sort(_isotopes.begin(), _isotopes.end(), PeakGroup::compC13);

    showBars();
}


IsotopePlot::~IsotopePlot() {}

QRectF IsotopePlot::boundingRect() const
{
    return(QRectF(0,0,_width,_height));
}

void IsotopePlot::showBars() {
    clear();
    if (_isotopes.size() == 0 ) return;
    if (_samples.size() == 0 ) return;

    int visibleSamplesCount = _samples.size();
    sort(_samples.begin(), _samples.end(), mzSample::compSampleOrder);

    //PeakGroup::QType qtype = PeakGroup::AreaTop;
    //if ( _mw ) qtype = _mw->getUserQuantType();

    MatrixXf MM = _mw->getIsotopicMatrix(_group);

    if (scene()) {
        QString parameters;

        parameters.append("TODO: isotope parameters");

        //if previous value of _parameter is not null, clear() call above will delete and free memory
        _parameters = new QGraphicsTextItem();

        _parameters->setHtml(parameters);
        scene()->addItem(_parameters);

        _width =   scene()->width()*0.20;
        _barheight = scene()->height()*0.75/visibleSamplesCount;
        if (_barheight<3)  _barheight=3;
        if (_barheight>15) _barheight=15;
        _height = visibleSamplesCount*_barheight;

    }

    _parameters->setPos(static_cast<double>(_width)-_parameters->boundingRect().width()/2.0,
                        static_cast<double>(_height)+_parameters->boundingRect().height()/2.0-1);

    //qDebug() << "showBars: " << _width << " " << _height;


    for(int i=0; i<MM.rows(); i++ ) {		//samples
        float sum= MM.row(i).sum();
        if (sum == 0) continue;
        MM.row(i) /= sum;

        double ycoord = _barheight*i + _parameters->boundingRect().height();
        double xcoord = 0;

        for(int j=0; j < MM.cols(); j++ ) {	//isotopes
            double length  = MM(i,j) * _width;
            int h = j % 20;
            if(length<0 ) length=0;
            //qDebug() << length << " " << xcoord << " " << ycoord;
            QBrush brush(QColor::fromHsvF(h/20.0,1.0,1.0,1.0));
            IsotopeBar* rect = new IsotopeBar(this,scene());
            rect->setBrush(brush);
            rect->setRect(xcoord,ycoord,length,_barheight);

            QString name = tr("%1 <br> %2 <b>%3\%</b>").arg(_samples[i]->sampleName.c_str(),
                                                            _isotopes[j]->tagString.c_str(),
                                                            QString::number(MM(i,j)*100));

            rect->setData(0,QVariant::fromValue(name));
            rect->setData(1,QVariant::fromValue(_isotopes[j]));
            rect->setData(2,QVariant::fromValue(_samples[i]));

            if(_mw) {
//                connect(rect,SIGNAL(groupSelected(PeakGroup*)),_mw, SLOT(setPeakGroup(PeakGroup*)));
                connect(rect, SIGNAL(peakSelected(Peak*)), _mw, SLOT(showPeakInfo(Peak*)));
            }
            xcoord += length;
        }
    }
}


void IsotopePlot::contextMenuEvent(QGraphicsSceneContextMenuEvent* event) {
    QMenu menu;

    SettingsForm* settingsForm = _mw->settingsForm;

    QAction* d = menu.addAction("Isotope Detection Options");
    connect(d, SIGNAL(triggered()), settingsForm, SLOT(showIsotopeDetectionTab()));
    connect(d, SIGNAL(triggered()), settingsForm, SLOT(show()));

    QPoint pos = QCursor::pos();
    menu.exec(pos);

}

void IsotopePlot::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) { return; }

/*
void IsotopeBar::mouseDoubleClickEvent (QGraphicsSceneMouseEvent*event) {
	QVariant v = data(1);
   	PeakGroup*  x = v.value<PeakGroup*>();
}

void IsotopeBar::mousePressEvent (QGraphicsSceneMouseEvent*event) {}
*/


void IsotopeBar::hoverEnterEvent (QGraphicsSceneHoverEvent *event) {
    QVariant v = data(0);
    QString note = v.value<QString>();
    if (note.length() == 0 ) return;

    QString htmlNote = note;
    setToolTip(note);
    QPointF posG = mapToScene(event->pos());
    emit(showInfo(htmlNote, posG.x(), posG.y()+5));
}

void IsotopeBar::keyPressEvent(QKeyEvent *e) {
    if (e->key() == Qt::Key_Delete || e->key() == Qt::Key_Backspace ) {
        QVariant v = data(1);
    	PeakGroup*  g = v.value<PeakGroup*>();
        if (g && g->parent && g->parent != g) { g->parent->deleteChild(g); emit(groupUpdated(g->parent)); }
        IsotopePlot* parent = (IsotopePlot*) parentItem();
        if (parent) parent->showBars();
    }
}

void IsotopeBar::mousePressEvent(QGraphicsSceneMouseEvent *event){

    QVariant v = this->data(1);
    PeakGroup *peakGroup = v.value<PeakGroup*>();

    QVariant v2 = this->data(2);
    mzSample *sample = v2.value<mzSample*>();

    if (peakGroup && sample) {
        Peak *peak = peakGroup->getPeak(sample);
        if (peak) emit(peakSelected(peak));
    }
}
