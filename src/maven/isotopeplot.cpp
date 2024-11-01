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

//Issue 672: Use this constructor when not putting the IsotopePlot widget on the EIC plot.
IsotopePlot::IsotopePlot(QGraphicsItem* parent, QGraphicsScene *scene, MainWindow *mainWindow)
    :QGraphicsItem(parent){

    _mw = mainWindow;
    _width = 200;
    _height= 800;
    _barheight = 20;
    _group = nullptr;
    _parameters = nullptr;

    _isInLegendWidget = true;
}

void IsotopePlot::setMainWindow(MainWindow* mw) { _mw = mw; }

void IsotopePlot::clear() {
    QList<QGraphicsItem *> mychildren = this->childItems();
    if (mychildren.size() > 0 ) {
        foreach (QGraphicsItem *child, mychildren) {
            scene()->removeItem(child);
            delete(child);
            child = nullptr;
        }
    }
}

void IsotopePlot::setPeakGroup(PeakGroup* group) {
    //cerr << "IsotopePlot::setPeakGroup()" << group << endl;

    if (!_mw) return;
    if (!group) return;
    if (group->isIsotope()) return;

    //Ensure that IsotopePlot._group is a copy of a group from some other source.
    if (_group) {
        delete(_group);
        _group = nullptr;
    }

    _group = new PeakGroup(*group);

    //Issue 402: try to use saved parameters, fall back to current GUI settings
    IsotopeParameters isotopeParameters = group->isotopeParameters;

    // childCount() == 0: From EIC widget
    // IsotopeParametersType::INVALID: from settings form
    if (group->childCount() == 0 || isotopeParameters.isotopeParametersType == IsotopeParametersType::INVALID) {

        //from settings form
        if (isotopeParameters.isotopeParametersType == IsotopeParametersType::INVALID) {
            isotopeParameters = _mw->getIsotopeParameters();
        }

        _group->pullIsotopes(isotopeParameters, _mw->getSamples());
        _group->isotopeParameters = isotopeParameters;

        clear(); //Issue 408: Ensure that plot is cleared before returning to prevent dangling pointers

        if (_group->childCount() == 0) return; // Did not detect any isotopes
    }

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
    std::sort(_isotopes.begin(), _isotopes.end(), PeakGroup::compIsotopicIndex);

    clearBars();
    showBars();
}

void IsotopePlot::clearBars() {
    QList<QGraphicsItem *> mychildren = this->childItems();
    if (mychildren.size() > 0 ) {
        foreach (QGraphicsItem *child, mychildren) {
            if (child->type() == IsotopeBar::Type) {
                scene()->removeItem(child);
                delete(child);
                child = nullptr;
            }
        }
    }
}

IsotopePlot::~IsotopePlot() {}

QRectF IsotopePlot::boundingRect() const
{
    return(QRectF(0,0,_width,_height));
}

void IsotopePlot::showBars() {
    if (_isotopes.size() == 0 ) return;
    if (_samples.size() == 0 ) return;

    int visibleSamplesCount = _samples.size();
    sort(_samples.begin(), _samples.end(), mzSample::compSampleOrder);

    //PeakGroup::QType qtype = PeakGroup::AreaTop;
    //if ( _mw ) qtype = _mw->getUserQuantType();

    IsotopeMatrix isotopeMatrix= _mw->getIsotopicMatrix(
        _group,
        _mw->isDisplayNaturalAbundanceCorrectedValues(),
        true); // isFractionOfSampleTotal

    MatrixXf MM = isotopeMatrix.isotopesData;

    int parametersOffset = 0;

    if (scene()) {

        if (_isInLegendWidget) {
            computeParameters();
            parametersOffset = _parameters->boundingRect().height();

            _barheight = 30; // TODO: configurable?
            _height = visibleSamplesCount*_barheight + parametersOffset;
            _width = 1.2*_parameters->boundingRect().width();

        } else {
            _width =   scene()->width()*0.20;
            _barheight = scene()->height()*0.75/visibleSamplesCount;
            if (_barheight<3)  _barheight=3;
            if (_barheight>15) _barheight=15;
            _height = visibleSamplesCount*_barheight;
        }

    }

    for(int i=0; i<MM.rows(); i++ ) { //samples

        double ycoord = _barheight*i + parametersOffset;
        double xcoord = 0;

        for(int j=0; j < MM.cols(); j++ ) {	//isotopes
            double length  = MM(i,j) * _width;
            int h = j % 20;
            if(length<0 ) length=0;
            //qDebug() << length << " " << xcoord << " " << ycoord;
            QBrush brush(QColor::fromHsvF(h/20.0,1.0,1.0,1.0));
            IsotopeBar* rect = new IsotopeBar(this);
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

void IsotopePlot::computeParameters() {

    if (!_group) return;

    QString parameters("Isotope Parameters: ");

    if (_group->isotopeParameters.isotopeParametersType == IsotopeParametersType::SAVED) {
      parameters.append("SAVED<br>");
    } else if (_group->isotopeParameters.isotopeParametersType == IsotopeParametersType::FROM_GUI) {
      parameters.append("FROM GUI<br>");
    }

    //Issue 652
    parameters.append("Algorithm: ");
    string algName = IsotopeParameters::getAlgorithmName(_group->isotopeParameters.isotopicExtractionAlgorithm);
    parameters.append(QString(algName.c_str()));
    parameters.append("<br>");

    parameters.append("Labels:");
    bool isHasIsotopes = false;
    if (_group->isotopeParameters.isC13Labeled){
        if (isHasIsotopes) parameters.append(",");
        parameters.append(" 13C");
        isHasIsotopes = true;
    }
    if (_group->isotopeParameters.isD2Labeled) {
        if (isHasIsotopes) parameters.append(",");
        parameters.append(" D");
        isHasIsotopes = true;
    }
    if (_group->isotopeParameters.isN15Labeled) {
        if (isHasIsotopes) parameters.append(",");
        parameters.append(" 15N");
        isHasIsotopes = true;
    }
    if (_group->isotopeParameters.isS34Labeled) {
        if (isHasIsotopes) parameters.append(",");
        parameters.append(" 34S");
        isHasIsotopes = true;
    }

    if (_group->isotopeParameters.isO18Labeled) {
        if (isHasIsotopes) parameters.append(",");
        parameters.append(" 18O");
    }

    if (_group->isotopeParameters.isNatAbundance) {
        parameters.append("<br>Natural Abundance Isotopes >= ");
        parameters.append(QString::number(100.0f * _group->isotopeParameters.natAbundanceThreshold, 'f', 6));
        parameters.append("% of [M+0]");
    } else {
        parameters.append("<br>No Natural Abundance Isotopes");
    }

    parameters.append("<br>m/z tol: ");
    parameters.append(QString::number(static_cast<double>(_group->isotopeParameters.ppm), 'f', 0));
    parameters.append(" ppm");

    if (_group->isotopeParameters.isApplyMZeroMzOffset) {
        parameters.append(" (after [M+0] m/z offset)");
    }

    if (_group->isotopeParameters.isotopicExtractionAlgorithm == IsotopicExtractionAlgorithm::MAVEN_GUI_VERSION_ONE) {
        parameters.append(", RT tol: ");

        parameters.append(QString::number(static_cast<int>(_group->isotopeParameters.maxIsotopeScanDiff)));
        parameters.append(" scans");

        parameters.append("<br>Min corr: ");
        parameters.append(QString::number(_group->isotopeParameters.minIsotopicCorrelation, 'f', 2));
        parameters.append(", ");
    } else {
        parameters.append("<br>");
    }

    parameters.append("Extract");
    if (_group->isotopeParameters.isExtractNIsotopes) {
        parameters.append(" up to ");
        parameters.append(QString::number(_group->isotopeParameters.maxIsotopesToExtract));
    } else {
        parameters.append(" unlimited");
    }
    parameters.append(" isotopes");

    if (_group->isotopeParameters.isotopicExtractionAlgorithm == IsotopicExtractionAlgorithm::MAVEN_GUI_VERSION_ONE) {
        if (_group->isotopeParameters.isIgnoreNaturalAbundance) {
            parameters.append("<br>Ignore if >= ");
            parameters.append(QString::number(_group->isotopeParameters.maxNaturalAbundanceErr, 'f', 1));
            parameters.append("% nat. abund. error");
        }
    }

    if(_mw->isDisplayNaturalAbundanceCorrectedValues()) {
        parameters.append("<br>Displaying natural abundance-corrected values.");
    }

    //Issue 671: Some GUI issues around overwriting existing text
    scene()->removeItem(_parameters);
    if (_parameters) {
        delete(_parameters);
        _parameters = nullptr;
    }

    _parameters = new QGraphicsTextItem();
    _parameters->setHtml(parameters);

    scene()->addItem(_parameters);
}
