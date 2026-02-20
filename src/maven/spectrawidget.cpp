#include "spectrawidget.h"

SpectraWidget::SpectraWidget(MainWindow* mw, int msLevel) {

    this->mainwindow = mw;
    this->_msLevel = msLevel;

   _currentScan = nullptr;
   _avgScan = nullptr;
   _log10Transform=false;

    initPlot();

    _drawXAxis = true;
    _drawYAxis = true;
    _resetZoomFlag = true;
    _profileMode = false;
    _nearestCoord = QPointF(0,0);
    _focusCoord = QPointF(0,0);
    _showOverlay=false;

    connect(mainwindow->libraryDialog, SIGNAL(unloadLibrarySignal(QString)), this, SLOT(clearOverlayAndReplot()));
}

void SpectraWidget::initPlot() {
    _zoomFactor = 0;
    setScene(new QGraphicsScene(this));
    scene()->setItemIndexMethod(QGraphicsScene::BspTreeIndex);
    scene()->setSceneRect(10,10,this->width()-10, this->height()-10);

    setDragMode(QGraphicsView::RubberBandDrag);
    //setCacheMode(CacheBackground);
    //setRenderHint(QPainter::Antialiasing);
    //setTransformationAnchor(AnchorUnderMouse);
    //setResizeAnchor(AnchorViewCenter);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    _arrow = new QGraphicsLineItem(0,0,0,0);
    scene()->addItem(_arrow);
    QPen redpen(Qt::red, 1,  Qt::DashDotLine, Qt::RoundCap, Qt::RoundJoin);
    QPen bluepen(Qt::blue, 1,  Qt::DashDotLine, Qt::RoundCap, Qt::RoundJoin);
    _arrow->setPen(bluepen);

    _varrow = new QGraphicsLineItem(0,0,0,0);
    scene()->addItem(_varrow);
    _varrow->setPen(redpen);

    QFont font("Helvetica", 10);
    font.setWeight(QFont::Bold);

    _note = new QGraphicsTextItem(0,0);
    scene()->addItem(_note);
    _note->setFont(font);
    _note->setDefaultTextColor(Qt::blue);

    _vnote = new QGraphicsTextItem(0,0);
    scene()->addItem(_vnote);
    _vnote->setFont(font);
    _vnote->setDefaultTextColor(Qt::red);

    _title = new QGraphicsTextItem();
    _title->setVisible(true);
    scene()->addItem(_title);

}

void SpectraWidget::setCurrentScan(Scan* scan) {
    qDebug() << "SpectraWidget::setCurrentScan(scan)";

    //Issue 829: It's possible that the same scan might be referenced when reviewing MS3 data,
    // try leaving this around, despite memory leaks
    if (_currentScan && _currentScan->mslevel <= 2) {
        delete(_currentScan);
    }

    _currentScan = new Scan(nullptr,0,0,0,0,0); //re-initialize to empty scan

    if (scan ) {
        //if (_currentScan and scan->mslevel != _currentScan->mslevel) {
        //  _resetZoomFlag = true;
        //}
        links.clear();
        _currentScan->deepcopy(scan);

        if(_log10Transform) {
            _currentScan->log10Transform();
        }
    }
}

/**
 * @brief SpectraWidget::setCurrentFragment
 * @param fragment
 * @param sample
 * @param mslevel
 *
 * Note that this expects a fragment object, but explicitly uses the consensus spectrum.
 */
void SpectraWidget::setCurrentFragment(Fragment *fragment, int mslevel) {
    qDebug() << "SpectraWidget::setCurrentFragment(fragment)";

    if (fragment) {

        string sampleName;
        if (fragment->scanNumMap.size() > 0) sampleName = fragment->scanNumMap.begin()->first;

        Scan *scan = new Scan(nullptr, fragment->scanNum, mslevel, fragment->rt, fragment->precursorMz, (fragment->precursorCharge > 0 ? 1: -1));
        scan->sampleName = sampleName;

        scan->mz = fragment->mzs;
        scan->intensity = fragment->intensity_array;

        _currentFragment = fragment;
        _sampleScanMap = fragment->scanNumMap;

        setCurrentScan(scan);
        findBounds(true, true);
        drawGraph();
        repaint();

        //Issue 481: keep the scan around to avoid dangling pointer bugs?
        delete(scan);
    }

}

void SpectraWidget::replot() {
    drawGraph();
}


void SpectraWidget::setTitle() {

    if (width()<50) return;

    QString title;

    if (_currentScan){
        QString sampleName = _currentScan->getSampleName().c_str();
        QString polarity;
        _currentScan->getPolarity() > 0 ? polarity = "Pos" : polarity = "Neg";

        if (_currentFragment) {

            title += tr("<b>MS%1 Consensus spectrum</b>").arg(QString::number(_currentScan->mslevel));

            if (_isDisplayFullTitle) {
                for (auto it = _sampleScanMap.begin(); it != _sampleScanMap.end(); ++it){

                    unordered_set<int> scans = it->second;

                    vector<int> scansVector;
                    scansVector.assign(scans.begin(), scans.end());
                    sort(scansVector.begin(), scansVector.end());

                    QStringList scansList;
                    for (auto x : scansVector){
                        scansList.push_back(QString::number(x));
                    }

                    title += tr("<br><b>%1</b>").arg(QString(it->first.c_str()));
                    title += tr(" scans: <b>%1</b>").arg(scansList.join(", "));
                }
            }

        } else {
            //single scan
            title += tr("Sample: <b>%1</b> Scan: <b>%2</b> rt:<b>%3</b> msLevel: <b>%4</b> Ion: <b>%5</b>").arg(
                                QString(sampleName),
                                QString::number(_currentScan->scannum),
                                QString::number(_currentScan->rt,'f',2),
                                QString::number(_currentScan->mslevel),
                                polarity);
        }
    }

    //parameters only apply to a single scan
    if (!_currentFragment) {

        int numDigits = _isDisplayHighPrecisionMz ? 7 : 4;

        QString precString("<br>PreMz: ");
        if (_currentScan && _currentScan->mslevel == 3 && _currentScan->ms1PrecursorForMs3) {
            QString ms1PrecursorForMs3 = "<br>Ms1PreMz: <b>" + QString::number(_currentScan->ms1PrecursorForMs3,'f',numDigits) + "</b>";
            title += ms1PrecursorForMs3;
            precString = QString(" Ms2PreMz: ");
        }

        if (_currentScan && _currentScan->precursorMz) {
            QString precursorMzLink = precString + "<b>" + QString::number(_currentScan->precursorMz,'f',numDigits) + "</b>";
            title += precursorMzLink ;
        }

        if (_currentScan && _currentScan->collisionEnergy) {
            QString precursorMzLink= " CE: <b>" + QString::number(_currentScan->collisionEnergy,'f',1) + "</b>";
            title += precursorMzLink ;
        }

        if (_currentScan && _currentScan->productMz>0) {
            QString precursorMzLink= " ProMz: <b>" + QString::number(_currentScan->productMz,'f',numDigits) + "</b>";
            title += precursorMzLink ;
        }

        if (_currentScan && _currentScan->precursorMz > 0 and _currentScan->sample) {
            double ppm = 10.0;
            double purity = _currentScan->getPrecursorPurity(ppm);
            title += " Purity: <b>" + QString::number(purity*100.0,'f',1) + "%</b>";
            title += " IsolWin: <b>" + QString::number(_currentScan->isolationWindow,'f',1) + "</b>";
        }

        if (_currentScan && _currentScan->injectionTime > 0) {
            title += " Inject. Time: <b>" + QString::number(_currentScan->injectionTime,'f',1) + "</b>";
        }
    }

    int fontSize = 8.0;

    QFont font = QApplication::font();

    //Issue 730: user override font size
    int savedFontSize = mainwindow->getSettings()->value("spnSpectrumTitleTextSize", -1).toInt();
    if (savedFontSize > 0) {
        fontSize = savedFontSize;
        font.setPixelSize(fontSize);
    }

    _title->setFont(font);

    _title->setHtml(title);
    int titleWith = _title->boundingRect().width();
    _title->setPos(scene()->width()/2-titleWith/2, 3);
    //_title->setScale(scene()->width()*0.5);
    _title->update();
}

void SpectraWidget::setScan(Scan* scan) {
    //Issue 754: Replace graph with empty data instead of leaving previous scan/fragment data

    _currentFragment = nullptr;
    _sampleScanMap = {};

    cerr << "SpectraWidget::setScan(scan) " << endl;
    setCurrentScan(scan);
    findBounds(true,true);
    drawGraph();
    repaint();
}

void SpectraWidget::setScan(Scan* scan, float mzmin, float mzmax) {
    if (!scan) return;

    //Issue 481: dangling pointer bugs
    _currentFragment = nullptr;
    _sampleScanMap = {};

    cerr << "SpectraWidget::setScan(scan,min,max) : " << scan->scannum << endl;
    setCurrentScan(scan);
    _minX = mzmin;
    _maxX = mzmax;
    findBounds(false,true);
    //mainwindow->getEicWidget()->setFocusLine(scan->rt);
    drawGraph();
    repaint();
}

void SpectraWidget::setScan(mzSample* sample, int scanNum=-1) {
    if (!sample) return;
    if (sample->scans.size() == 0 ) return;
    if (_currentScan && scanNum < 0 ) scanNum = _currentScan->scannum; 
    if (scanNum > sample->scans.size() ) scanNum = sample->scans.size()-1;

    if ( scanNum >= 0 && scanNum < sample->scans.size() ) { 

        _currentFragment = nullptr;
        _sampleScanMap = {};

        setCurrentScan(sample->scans[ scanNum ]);
        cerr << "SpectraWidget::setScan(scan) " << endl;
        findBounds(false,true);
        drawGraph();
        repaint();
    }
}


void SpectraWidget::setScan(Peak* peak, bool isForceZoom) {
    cerr << "SpectraWidget::setScan(peak) " << endl;
    links.clear();

    if (!peak) return;

    mzSample* sample = peak->getSample();
    if (!sample) return;

    Scan* scan = sample->getScan(peak->scan);
    if (!scan) return;

    _currentFragment = nullptr;
    _sampleScanMap = {};

    setCurrentScan(scan);

    //direct infusion case: no need to update MS1 scan
    if (peak->mzmax - peak->mzmin > 0.5) return;

    _focusCoord = QPointF(peak->peakMz,peak->peakIntensity);

    //Issue 394: Respect settings form when displaying peaks
    //These affect the MS1 plot

    bool isChkSettings = mainwindow->getSettings()->contains("chkAutoMzMin") &&
            mainwindow->getSettings()->value("chkAutoMzMin").toInt();

    if (isChkSettings || isForceZoom) {

        _minX = peak->peakMz-2;
        _maxX = peak->peakMz+6;

    } else {
        _minX = mainwindow->getSettings()->value("spnMzMinVal", 0.0).toFloat();
        _maxX = mainwindow->getSettings()->value("spnMzMaxVal", 1200.0).toFloat();
    }

    _maxY = peak->peakIntensity*1.3f;
    _minY = 0;

    annotateScan();
    drawGraph();
    repaint();
}

void SpectraWidget::overlaySpectralHit(SpectralHit& hit, bool isSetScanToHitScan) {
        _spectralHit = hit;//copy hit

        //determine limits of overlayed spectra
        if (isSetScanToHitScan) {
            setScan(hit.scan, hit.getMinMz()-0.5, hit.getMaxMz()+0.5);
        }

        findBounds(true, true);

        drawGraph();
        repaint();

        if(_currentScan) {
            int pos = _currentScan->findHighestIntensityPos(hit.precursorMz,hit.productPPM);
            if (pos > 0 && pos <= _currentScan->nobs()) {
                _focusCoord.setX(hit.precursorMz);
                _focusCoord.setY(_currentScan->intensity[pos]);
            }
        }
}

void SpectraWidget::clearOverlay() {
    _spectralHit = SpectralHit();
    _showOverlay=false;
}

void SpectraWidget::clearOverlayAndReplot() {
    clearOverlay();
    replot();
    repaint();
}

void SpectraWidget::overlayPeakGroup(PeakGroup* group) {
    if(!group) return;

    Scan* scan = nullptr;

    bool isDisplayConsensusSpectrum = mainwindow->getSettings()->value("chkDisplayConsensusSpectrum", false).toBool();

    if (!group->fragmentationPattern.mzs.empty() && isDisplayConsensusSpectrum) {
        this->setCurrentFragment(&(group->fragmentationPattern), 2);
    } else if (group->peakCount() > 0){

        for (unsigned int i = 0; i < group->peaks.size(); i++) {
            Peak *_peakI = &(group->peaks[i]);
            float fragmentPpm = mainwindow->massCalcWidget->fragmentPPM->value();
            vector<Scan*>ms2s = _peakI->getFragmentationEvents(fragmentPpm);
            if (ms2s.size()){
                scan = ms2s[0];
                break;
            }
        }
        setScan(scan);
    } else {
        setScan(scan); // setScan(nullptr)
    }

    if (group->compound)  {
        if(group->compound->fragment_mzs.size()) overlayCompound(group->compound, false);
        else if (!group->compound->smileString.empty()) overlayTheoreticalSpectra(group->compound);
        else  overlayCompound(group->compound, false);
    }

}

void SpectraWidget::overlayTheoreticalSpectra(Compound* c) {
   if(!c or c->smileString.empty()) return;

   SpectralHit hit;
   hit.compoundId = "THEORY: "; hit.compoundId += c->name.c_str();
   hit.scan = _currentScan;
   hit.precursorMz = c->precursorMz;
   hit.productPPM=  mainwindow->massCalcWidget->fragmentPPM->value();

    BondBreaker bb;
    bb.breakCarbonBonds(true);
    bb.breakDoubleBonds(true);
    bb.setMinMW(70);
    bb.setSmile(c->smileString);
    map<string,double> fragments = bb.getFragments();

    for(auto& pair: fragments) {
        //qDebug() << "Ah:" << pair.second;
        hit.mzList.push_back(pair.second);
        hit.intensityList.push_back(1000);

        hit.mzList.push_back(pair.second-PROTON);
        hit.intensityList.push_back(800);

        hit.mzList.push_back(pair.second+PROTON);
        hit.intensityList.push_back(800);
    }

    _showOverlay = true;
    overlaySpectralHit(hit);
}

void SpectraWidget::showMS2CompoundSpectrum(Compound *c) {
    if (_msLevel != 2) return;
    clearOverlay();

    _showOverlay = true;
    _currentScan = nullptr; //clear out previous scan information
    _spectralHit = generateSpectralHitFromCompound(c);

    findBounds(true, true);
    drawGraph();
}

void SpectraWidget::overlayCompound(Compound* c, bool isSetScanToHitScan) {
   clearOverlay();
   if(!_currentScan or !c) return;

   if(c->fragment_mzs.size() == 0 ) {
       overlayTheoreticalSpectra(c);
       return;
   }

   _spectralHit = generateSpectralHitFromCompound(c);

   _showOverlay = true;
   overlaySpectralHit(_spectralHit, isSetScanToHitScan);

   resetZoom();
}

SpectralHit SpectraWidget::generateSpectralHitFromCompound(Compound *c) {

    SpectralHit hit;

    if (!c) return hit;

    hit.compoundId = c->name.c_str();
    hit.originalCompoundId = c->id.c_str();
    hit.scan = _currentScan;
    hit.precursorMz = c->precursorMz;
    hit.productPPM=  mainwindow->massCalcWidget->fragmentPPM->value();

    qDebug() << "SpectraWidget::overlayCompound() compound=" << c->name.c_str()
         << "# m/z=" << c->fragment_mzs.size();

    if (hit.scan) {
        qDebug() << "#obs=" << hit.scan->nobs()
                 << "#matched=" << hit.matchCount;
    }

    for(unsigned int i=0; i < c->fragment_mzs.size(); i++){
        hit.mzList << static_cast<double>(c->fragment_mzs[i]);
    }

    for(unsigned int i=0; i < c->fragment_intensity.size(); i++){
        hit.intensityList << static_cast<double>(c->fragment_intensity[i]);
    }

    //Issue 159: Careful to avoid array out of bounds violations
    for(unsigned int i=0; i < c->fragment_mzs.size(); i++) {
        if (c->fragment_labels.size() == c->fragment_mzs.size()) {
            hit.fragLabelList << QString(c->fragment_labels[i].c_str());
        } else {
            hit.fragLabelList << QString("");
        }
    }

    //remove previous annotations
    links.clear();

    //add new annotations
    for(auto ion: c->fragment_iontype) {
          float mz= c->fragment_mzs[ion.first];
          string note = ion.second;
          links.push_back(mzLink(mz,mz,note));
    }

    return hit;
}

void SpectraWidget::drawSpectralHitLines(SpectralHit& hit) {

    float ppmWindow =    hit.productPPM;
    double maxIntensity= hit.getMaxIntensity();

    if (maxIntensity <= 0.0) return;

    QPen redpen(Qt::red,1);
    QPen bluepen(Qt::blue,2);
    QPen graypen(Qt::gray,1);
    graypen.setStyle(Qt::DashDotLine);

    QPen greenpen(Qt::darkGreen, 2);
    greenpen.setStyle(Qt::DashDotLine);

    QPen sn1FragmentPen(QColor("maroon"), 2);
    sn1FragmentPen.setStyle(Qt::DashDotLine);

    QPen sn2FragmentPen(QColor("darkviolet"), 2);
    sn2FragmentPen.setStyle(Qt::DashDotLine);

    QPen sn3FragmentPen(QColor("plum"), 2);
    sn3FragmentPen.setStyle(Qt::DashDotLine);

    QPen sn4FragmentPen(QColor("powderblue"), 2);
    sn4FragmentPen.setStyle(Qt::DashDotLine);

    //Issue 534: for some reason, this isn't working.
    //float SCALE = (_currentScan || _currentFragment) ? 0.45f : (1.0f/_maxY); // split plot or single plot

    float SCALE = (_currentScan || _currentFragment) ? 0.45f : 0.90f; // split plot or single plot

    //create label
    QGraphicsTextItem* text = new QGraphicsTextItem(hit.compoundId);
    text->setFont(_title->font());
    text->setPos(_title->pos().x(),toY(_maxY*0.95,SCALE));
    scene()->addItem(text);
    _items.push_back(text);

    if (_isDisplayCompoundId) {
        QGraphicsTextItem *compoundIdText = new QGraphicsTextItem(hit.originalCompoundId);
        compoundIdText->setTextWidth(0.90 * (_maxX - _minX));
        compoundIdText->setFont(_title->font());
        compoundIdText->setPos(toX(_minX + 0.05 * (_maxX - _minX)), text->y() + text->boundingRect().height());
        scene()->addItem(compoundIdText);
        _items.push_back(compoundIdText);
    }

    //Issue 481: Prefer on-the-fly fragment matches to avoid obviously bad matches
    vector<int> matches{};

    if (isUseCachedMatches && !_matches.empty()) {
        matches = _matches;
    }

    //Issue 481: debugging
    //qDebug() << "SpectraWidget::drawSpectralHitLines(): _currentFragment=" << _currentFragment <<", _currentScan=" << _currentScan;

    if (matches.empty() && _msLevel == 2 && (_currentFragment || (_currentScan && _currentScan->precursorMz > 0))) {

        float productPpmTolr = static_cast<float>(mainwindow->massCalcWidget->fragmentPPM->value());
        float maxDeltaMz = -1.0f;

        if (_currentFragment) {
            maxDeltaMz = (productPpmTolr * static_cast<float>(_currentFragment->precursorMz))/1000000;
        } else if (_currentScan) {
            maxDeltaMz = (productPpmTolr * static_cast<float>(_currentScan->precursorMz))/1000000;
        }

        if (maxDeltaMz > 0) {
            //hit
            Fragment a;
            a.precursorMz = hit.precursorMz;
            vector<float> mzs(hit.mzList.size());
            for (unsigned int i = 0; i < mzs.size(); i++) {
                mzs[i] = static_cast<float>(hit.mzList[i]);
            }
            a.mzs = mzs;

            vector<float> intensities(hit.intensityList.size());
            for (unsigned int i = 0; i < intensities.size(); i++){
                intensities[i] = static_cast<float>(hit.intensityList[i]);
            }
            a.intensity_array = intensities;

            //_currentFragment or _currentScan
            if (_currentFragment) {
                matches = Fragment::findFragPairsGreedyMz(&a, _currentFragment, maxDeltaMz);
            } else if (_currentScan) {
                Fragment b;
                b.precursorMz = _currentScan->precursorMz;
                b.mzs = _currentScan->mz;
                b.intensity_array = _currentScan->intensity;
                matches = Fragment::findFragPairsGreedyMz(&a, &b, maxDeltaMz);
            }
            _matches = matches;
        }
    }

    for(int i=0; i < hit.mzList.size(); i++) {
        float hitMz=hit.mzList[i];

        if (hitMz <= 0.0f) continue;

        //Issue 531
        if (hitMz < _minX || hitMz > _maxX) continue;

        float hitIntensity= 1.0f;
        if (i < hit.intensityList.size()) hitIntensity= static_cast<float>(hit.intensityList[i]);

        double ppmMz = hitMz;
        if (_currentScan && _currentScan->precursorMz > 0) {
            ppmMz = _currentScan->precursorMz;
        }

        //Issue 226: targeted ms3 search
        int pos = -1;
        if (_currentScan) {
            if (_msLevel == 3) {
                pos = _currentScan->findHighestIntensityPosAMU(hitMz, _ms3MatchingTolr);
            } else {
                if (!matches.empty()) {
                    pos = matches[i];
                } else {
                    pos = _currentScan->findHighestIntensityPos(hitMz, static_cast<float>(ppmMz), ppmWindow);
                }
            }
        }

        int x = static_cast<int>(toX(hitMz));
        int y = static_cast<int>(toY(hitIntensity/static_cast<float>(maxIntensity)*_maxY,SCALE));

        //qDebug() << "SpectraWidget::drawSpectralHitLines():" << "(mz, I) = (" <<hitMz <<","<<hitIntensity <<")";

        QGraphicsLineItem* line = new QGraphicsLineItem(x,y,x,toY(0),0);
        pos >= 0 ?  line->setPen(bluepen) : line->setPen(redpen);
        scene()->addItem(line);
        _items.push_back(line);

        //matched?
        if (pos >= 0 && pos < _currentScan->nobs()) {

            int x = toX(hitMz); //Issue 342: match is drawn from perspective of reference m/z for consistency
            int y = toY(_maxY,SCALE);

            QGraphicsLineItem* line = new QGraphicsLineItem(x,y,x,toY(0),0);

            QString label = hit.fragLabelList[i];
            if (label.startsWith("*")) { //diagnostic fragment
                line->setPen(greenpen);
            } else if (label.startsWith("@")) { // sn1
                line->setPen(sn1FragmentPen);
            } else if (label.startsWith("$")) { // sn2
                line->setPen(sn2FragmentPen);
            } else if (label.startsWith("!")) { // sn3
                line->setPen(sn3FragmentPen);
            } else if (label.startsWith("^")) { // sn4
                line->setPen(sn4FragmentPen);
            } else {
                line->setPen(graypen);
            }

            scene()->addItem(line);
            _items.push_back(line);
        } else {
            //cerr << "!matched:" << hitMz << endl;

        }

        //Issue 159: add fragment labels
        if (this->_showOverlayLabels) {

            int numDigits = _isDisplayHighPrecisionMz ? 7 : 4;

            QString lblString = QString::number(hitMz,'f', numDigits);

            if (!hit.fragLabelList[i].isEmpty()) {
                lblString += QString(" "+ hit.fragLabelList[i]);
            }

            Note* label = new Note(lblString);

            double xFrac = (hitMz -_minX) / (_maxX-_minX);
            double yFrac = (hitIntensity/maxIntensity);

            if (xFrac >= 0.8 && yFrac <= 0.2) {
                label->labelOrientation = Note::Orientation::UpLeft;
            } else if (xFrac >= 0.8 && yFrac >= 0.2) {
                label->labelOrientation = Note::Orientation::DownLeft;
            } else if (xFrac < 0.8 && yFrac <= 0.2) {
                label->labelOrientation = Note::Orientation::UpRight;
            } else if (xFrac < 0.8 && yFrac > 0.2) {
                label->labelOrientation = Note::Orientation::DownRight;
            }

            QSignalMapper *signalMapper = new QSignalMapper(this);
            signalMapper->setMapping(label, pos);

            connect(label, SIGNAL(clicked()), signalMapper, SLOT(map()));
            connect(signalMapper, SIGNAL(mapped(int)), this, SLOT(selectObservedPeak(int)));

            //position label
            label->setPos(x,y-5);
            label->setExpanded(true);

            scene()->addItem(label);
            _items.push_back(label);
        }
    }
}

void SpectraWidget::selectObservedPeak(int peakIndex) {
    if (peakIndex == -1) return; //no matching peak

    _focusCoord.setX(_currentScan->mz[peakIndex]);
    _focusCoord.setY(_currentScan->intensity[peakIndex]);

    drawGraph();
}

void SpectraWidget::drawGraph() {

    //clean up previous plot
    if ( _arrow ) {
        _arrow->setVisible(false);
    }

    delete_all(_items);

    scene()->setSceneRect(10,10,this->width()-10, this->height()-10);
    Scan* scan = _currentScan;

    // Issue 442: support showing library spectrum alone
    bool isDrawCompound = _showOverlay && !_spectralHit.compoundId.isEmpty();
    bool isDrawGraph = (scan || isDrawCompound);
    bool isDrawCompoundOnly = !scan && isDrawCompound;
    bool isMissingMs2s = !_currentFragment && scan && !scan->sample && scan->scannum == 0 && scan->mz.empty() && this->_msLevel == 2;

    if (!isDrawGraph) return;

    //draw title text
    setTitle();

    QColor sampleColor = mainwindow->getBackgroundAdjustedBlack(this);
    string sampleName =  ".";
    QString polarity;

    QPen blackpen(mainwindow->getBackgroundAdjustedBlack(this), 2);


    float fontSize = scene()->height()*0.05;
    if ( fontSize < 1) fontSize=1;
    if ( fontSize > 16) fontSize=16;

    //Issue 730: user override font size
    float savedFontSize = mainwindow->getSettings()->value("spnMzTextSize", -1).toFloat();
    if (savedFontSize > 0) {
        fontSize = savedFontSize;
    }

    QFont font("Helvetica", fontSize);
    float _focusedMz = _focusCoord.x();

    QPen slineColor(sampleColor, 2);
    EicLine* sline = new EicLine(nullptr, scene(), mainwindow);
    sline->setColor(sampleColor);
    sline->setPen(slineColor);

    _items.push_back(sline);

   if( _profileMode ) {
        QBrush slineFill(sampleColor);
        sline->setFillPath(true);
        sline->setBrush(slineFill);
    }

    QMap<float,int>shownPositions; //items sorted by key

    float SCALE=1.0f;
    float OFFSET=0.0f;
    if (_showOverlay) {
        SCALE= _showOverlayScale;
        OFFSET= showOverlayOffset();
    }

    int yzero = toY(0,SCALE,OFFSET);

    if (!isDrawCompoundOnly) {
        sline->addPoint(toX(_maxX),yzero);
        sline->addPoint(toX(_minX),yzero);
    } else {
        SCALE = 0.9f;
    }

    if (scan) {
        for(int j=0; j<scan->nobs(); j++ ) {
            if ( scan->mz[j] < _minX  || scan->mz[j] > _maxX ) continue;

            if ( scan->intensity[j] / _maxY > 0.001 ) { // do want this?
                shownPositions[ scan->intensity[j] ] = j;
            }

            int x = toX(scan->mz[j]);
            int y = toY(scan->intensity[j],SCALE,OFFSET);

            if( _profileMode ) {
                   sline->addPoint(x,y);
            } else {

                QGraphicsLineItem* line = new QGraphicsLineItem(x,y,x,yzero,0);
                scene()->addItem(line);
                line->setPen(blackpen);
                _items.push_back(line);

    //            cerr << "(axisCoord, intensity) = (" << yzero << ", 0)" << endl;
    //            cerr << "(axisCoord, intensity) = (" << y << ", " << scan->intensity[j] << ")" << endl;

            }

            float tol = 0.005f;
            if (_msLevel == 3) {
                tol = _ms3MatchingTolr;
            }

            if( abs(scan->mz[j]-_focusedMz) < tol) {
                QPen redpen(Qt::red, 3);
                QGraphicsLineItem* line = new QGraphicsLineItem(x,y,x,toY(0,SCALE,OFFSET),0);
                scene()->addItem(line);
                line->setPen(redpen);
                _items.push_back(line);
            }
        }
    }

    //show labels
    QMap<float,int>::iterator itr; int labelCount=0;
    QVector<QGraphicsItem*> mzlabels;

    for(itr = shownPositions.end(); itr != shownPositions.begin(); itr--) {
        int pos = itr.value();
        if(pos < 0 || pos >= scan->mz.size()) continue;

        int prec=2;

        //testing
        if (_isDisplayHighPrecisionMz) {
            prec = 7;
        } else {
            if(abs(_maxX-_minX)>300) prec=1;
            else if(abs(_maxX-_minX)>100) prec=3;
            else if(abs(_maxX-_minX)>50) prec=4;
            else if(abs(_maxX-_minX)>10) prec=6;
        }

        //position label
        int x = toX(scan->mz[pos]);
        int y = toY(scan->intensity[pos],SCALE,OFFSET);

        //create new mzlabel
        QGraphicsTextItem* mz_label = new QGraphicsTextItem(QString::number(scan->mz[pos],'f',prec),0);
        mz_label->setFont(font);
        scene()->addItem(mz_label);
        mz_label->setPos(x,y-30);

        //check for overallping labels
         QList<QGraphicsItem *> overlapItems = mz_label->collidingItems();
         bool doNotShowFlag=false;
         foreach (QGraphicsItem *item, overlapItems) {
             if (item->type() == mz_label->type()) doNotShowFlag=true;
         }
         if(doNotShowFlag) { delete(mz_label); continue; }

        //keep track
        _items.push_back(mz_label);
        mzlabels.push_back(mz_label);

        //add a nice line to point from label to the peak
        float labelwidth=mz_label->boundingRect().width()/2;
        float labelheight=mz_label->boundingRect().height()/2;
        QGraphicsLineItem* mz_label_pointline = new QGraphicsLineItem(x,y, mz_label->x(), mz_label->y()+labelheight);
        QPen graydashed(Qt::gray,1,Qt::DotLine);
        mz_label_pointline->setPen(graydashed);
        _items.push_back(mz_label_pointline);
        scene()->addItem(mz_label_pointline);

        labelCount++;
        if(labelCount >= 200) break;
    }

    //show annotations
    for(int i=0; i < links.size(); i++ ) {
        if ( links[i].mz2 < _minX || links[i].mz2 > _maxX ) continue;

        //create label
       // Note* text = new Note(QString(links[i].note.c_str()) + " " + QString::number(links[i].mz2,'f',3));
        Note* label = new Note(QString(links[i].note.c_str()));
        scene()->addItem(label);
        _items.push_back(label);

        //position label
        int x = toX(links[i].mz2);
        int y = toY(links[i].value2,OFFSET,SCALE);
        label->setPos(x-2,y-20);
        label->setExpanded(true);
    }

    if(isDrawCompound) {
        drawSpectralHitLines(_spectralHit);
    }

    addAxes();

    if (isMissingMs2s) {

        QFont font = QApplication::font();

        int pxSize = scene()->height()*0.03;
        if ( pxSize < 14 ) pxSize = 14;
        if ( pxSize > 20 ) pxSize = 20;

        //Issue 730: user override font size
        int savedFontSize = mainwindow->getSettings()->value("spnEICTitleTextSize", -1).toInt();
        if (savedFontSize > 0) {
             pxSize = savedFontSize;
        }

        font.setPixelSize(pxSize);

        QGraphicsTextItem* text = scene()->addText("NO MS2 SCANS", font);
        int textWith = text->boundingRect().width();
        text->setPos(scene()->width()/2-textWith/2, 0.1*scene()->height());

        _items.push_back(text);
    }
}

void SpectraWidget::findBounds(bool checkX, bool checkY) {

    //if (_currentScan || _currentScan->nobs() == 0) return;

    float minMZ = 0.0f;
    float maxMZ = 1200.0f;

    if (_msLevel == 2) {

        bool isAutoMzMin = mainwindow->getSettings()->value("chkMs2AutoMzMin", false).toBool();
        bool isAutoMzMax = mainwindow->getSettings()->value("chkMs2AutoMzMax", true).toBool();

        double mzMinOffset = mainwindow->getSettings()->value("spnMs2MzMinOffset", 10.0).toDouble();
        double mzMaxOffset = mainwindow->getSettings()->value("spnMs2MzMaxOffset", 10.0).toDouble();

        double mzMinVal = mainwindow->getSettings()->value("spnMs2MzMin", 0.0).toDouble();
        double mzMaxVal = mainwindow->getSettings()->value("spnMs2MzMax", 1200.0).toDouble();

        if (_currentScan && _currentScan->mz.size() > 0) {
            minMZ = isAutoMzMin ? _currentScan->mz[0] - mzMinOffset : mzMinVal;
            maxMZ = isAutoMzMax ? _currentScan->mz[_currentScan->mz.size()-1] + mzMaxOffset : mzMaxVal;
        } else if (!_spectralHit.compoundId.isEmpty()){
            minMZ = isAutoMzMin ? _spectralHit.getMinMz() - mzMinOffset : mzMinVal;
            maxMZ = isAutoMzMax ? _spectralHit.getMaxMz() + mzMaxOffset : mzMaxVal;
        }

    } else if (_msLevel == 1){

        bool isAutoMzMin = mainwindow->getSettings()->value("chkAutoMzMin", false).toBool();
        bool isAutoMzMax = mainwindow->getSettings()->value("chkAutoMzMax", true).toBool();

        double mzMinOffset = mainwindow->getSettings()->value("spnMzMinOffset", 10.0).toDouble();
        double mzMaxOffset = mainwindow->getSettings()->value("spnMzMaxOffset", 10.0).toDouble();

        double mzMinVal = mainwindow->getSettings()->value("spnMzMinVal", 0.0).toDouble();
        double mzMaxVal = mainwindow->getSettings()->value("spnMzMaxVal", 1200.0).toDouble();

        if (_currentScan && _currentScan->mz.size() > 0) {
            minMZ = isAutoMzMin ? _currentScan->mz[0] - mzMinOffset : mzMinVal;
            maxMZ = isAutoMzMax ? _currentScan->mz[_currentScan->mz.size()-1] + mzMaxOffset : mzMaxVal;
        } else if (!_spectralHit.compoundId.isEmpty()) {
            minMZ = isAutoMzMin ? _spectralHit.getMinMz() - mzMinOffset : mzMinVal;
            maxMZ = isAutoMzMax ? _spectralHit.getMaxMz() + mzMaxOffset : mzMaxVal;
        }

    } else if (_msLevel == 3) {

        bool isAutoMzMin = mainwindow->getSettings()->value("chkMs3AutoMzMin", false).toBool();
        bool isAutoMzMax = mainwindow->getSettings()->value("chkMs3AutoMzMax", true).toBool();

        double mzMinOffset = mainwindow->getSettings()->value("spnMs3MzMinOffset", 10.0).toDouble();
        double mzMaxOffset = mainwindow->getSettings()->value("spnMs3MzMaxOffset", 10.0).toDouble();

        double mzMinVal = mainwindow->getSettings()->value("spnMs3MzMin", 0.0).toDouble();
        double mzMaxVal = mainwindow->getSettings()->value("spnMs3MzMax", 1200.0).toDouble();

        if (_currentScan && _currentScan->mz.size()> 0){
            minMZ = isAutoMzMin ? _currentScan->mz[0] - mzMinOffset : mzMinVal;
            maxMZ = isAutoMzMax ? _currentScan->mz[_currentScan->mz.size()-1] + mzMaxOffset : mzMaxVal;
        } else if (!_spectralHit.compoundId.isEmpty()) {
            minMZ = isAutoMzMin ? _spectralHit.getMinMz() - mzMinOffset : mzMinVal;
            maxMZ = isAutoMzMax ? _spectralHit.getMaxMz() + mzMaxOffset : mzMaxVal;
        }

    }

    //No enforcement is done in the GUI options panel - prevent negative range
    if (maxMZ < minMZ) {
        minMZ = 0;
        maxMZ = 1200;
    }

    //cerr << _currentScan->filterLine << " " << _currentScan->nobs() << endl;
    //cerr << "findBounds():  RANGE=" << minMZ << "-" << maxMZ << endl;
	if( _minX < minMZ) _minX = minMZ;
	if( _maxX > maxMZ) _maxX = maxMZ;

    if ( checkX ) { _minX = minMZ; _maxX = maxMZ; }

    if ( abs(_minX -_maxX)<1e-6 ) { _minX-=0.5; _maxX+=0.5; }

    _minY = 0;
    _maxY = 1;

    if (checkY && _currentScan)  {
        for(unsigned int j=0; j<_currentScan->nobs(); j++ ) {
			if (_currentScan->mz[j] >= _minX && _currentScan->mz[j] <= _maxX) {
				if (_currentScan->intensity[j] > _maxY) _maxY = _currentScan->intensity[j];
            }
        }
    }
    _maxY *= _maxIntensityScaleFactor;

    //qDebug() << "SpectraWidget::findBounds():  mz=" << _minX << "-" << _maxX << " ints=" << _minY << "-" << _maxY;
}

void SpectraWidget::keyPressEvent( QKeyEvent *e ) {
    switch( e->key() ) {
    case Qt::Key_Left:
        showLastScan(); return;
    case Qt::Key_Right :
        showNextScan(); return;
    case Qt::Key_0 :
        resetZoom(); return;
    case Qt::Key_Plus :
        zoomIn(); return;
    case Qt::Key_Minus:
        zoomOut(); return;
    default:
        return;
    }
    e->accept();
}


void SpectraWidget::showNextScan() { incrementScan(+1, 0); }
void SpectraWidget::showLastScan() { incrementScan(-1, 0); }
void SpectraWidget::showNextFullScan() { incrementScan(+1, 1); }
void SpectraWidget::showLastFullScan() { incrementScan(-1, 1); }


void SpectraWidget::incrementScan(int increment, int msLevel=0 ) {
	if (_currentScan == NULL || _currentScan->sample == NULL) return;

	mzSample* sample = _currentScan->getSample();
    if (sample == NULL) return;

	Scan* newScan=sample->getScan(_currentScan->scannum+increment);
    if (msLevel != 0 && newScan && newScan->mslevel != msLevel ) {
        for(unsigned int i=newScan->scannum; i< sample->scans.size(); i+=increment ) {
            newScan =sample->getScan(i);
            if ( newScan && newScan->mslevel==msLevel) break;
        }
    }
    if(newScan==NULL) return;

    //do not reset soom when moving between full scans
	if(newScan->mslevel == 1 && _currentScan->mslevel == 1) {
		setScan(newScan,_minX,_maxX);
    }  else {
        setScan(newScan);
    }
    //if ( _resetZoomFlag == true ) { resetZoom(); _resetZoomFlag=false; }
}


void SpectraWidget::resizeEvent (QResizeEvent * event) {
    isUseCachedMatches=true;
    replot();
    isUseCachedMatches=false;
}

/*
void SpectraWidget::enterEvent (QEvent *) {
    grabKeyboard();
}

void SpectraWidget::leaveEvent (QEvent*) {
    releaseKeyboard();
}
*/

void SpectraWidget::addAxes() {

    if (_drawXAxis ) {
        Axes* x = new Axes(0,_minX, _maxX, 10, mainwindow);
		scene()->addItem(x);
		x->setZValue(998);
		_items.push_back(x);
	}

    if (_drawYAxis ) {

        //Issue 442: Only draw two y axes when both theoretical compound and data are displayed
        bool isCompoundAndScan = _currentScan && !_spectralHit.compoundId.isEmpty();

        if (isCompoundAndScan) {

            //scan y-axis
            Axes* y = new Axes(1,_minY, _maxY, 5, mainwindow);

            y->setRenderScale((_showOverlayScale));
            y->setRenderOffset(_showOverlayOffset);

            y->setZValue(999);
            y->showTicLines(false);
            y->setOffset(5);

            scene()->addItem(y);
            _items.push_back(y);

            //overlay y-axis
            Axes* yOverlay = new Axes(1, 0, 1.0, 5, mainwindow);

            yOverlay->setRenderScale(_showOverlayScale);


            yOverlay->setZValue(999);
            yOverlay->showTicLines(false);
            yOverlay->setOffset(5);
            yOverlay->setVerticalOffset(_showOverlayOffset*scene()->height());

            scene()->addItem(yOverlay);
            _items.push_back(yOverlay);

        } else {

            Axes* y = new Axes(1,_minY, _maxY, 5, mainwindow);
            y->setZValue(999);
            y->showTicLines(false);
            y->setOffset(5);

            scene()->addItem(y);
            _items.push_back(y);
        }

    }

}

void SpectraWidget::mousePressEvent(QMouseEvent *event) {
    QGraphicsView::mousePressEvent(event);
    _mouseStartPos = event->pos();

}

void SpectraWidget::mouseReleaseEvent(QMouseEvent *event) {

    bool isDrawCompound = _showOverlay && !_spectralHit.compoundId.isEmpty();
    bool isDrawGraph = (_currentScan || isDrawCompound);

    if(! isDrawGraph) return;

    QGraphicsView::mouseReleaseEvent(event);
    _mouseEndPos	= event->pos();

    int deltaX =  _mouseEndPos.x() - _mouseStartPos.x();
    float deltaXfrac = (float) deltaX / (width()+1);


    if ( deltaXfrac > 0.01 ) {
        float xmin = invX( std::min(_mouseStartPos.x(), _mouseEndPos.x()) );
        float xmax = invX( std::max(_mouseStartPos.x(), _mouseEndPos.x()) );
        _minX = xmin;
        _maxX = xmax;
        //qDebug() << "SpectraWidget::mouseReleaseEvent(): xmin=" << xmin << ", xmax=" << xmax;
    } else if ( deltaXfrac < -0.01 ) {

        float minmz = -1;
        float maxmz = -1;
        if (_currentScan && _currentScan->mz.size() > 0 ) {
            minmz = _currentScan->mz[0];
            maxmz = _currentScan->mz[_currentScan->nobs()-1];
        } else if (isDrawCompound) {
            minmz = _spectralHit.getMinMz();
            maxmz = _spectralHit.getMaxMz();
        }

        if (minmz >= 0 && maxmz >= 0) {

            _minX *= 0.9f;
            _maxX *= 1.1f;

            if (_minX < minmz ) _minX=minmz;
            if (_maxX > maxmz ) _maxX=maxmz;
        }

    } else if (_nearestCoord.x() > 0 ) {
        setMzFocus(_nearestCoord.x());
    }
    findBounds(false, true);
    replot();
}

void SpectraWidget::setMzFocus(Peak* peak) { 
    setMzFocus(peak->peakMz);
}

void SpectraWidget::setMzFocus(float mz) {
    if (!_currentScan or _currentScan->mslevel>1 or _msLevel != 1) return;
    int bestMatch=-1; 
    float bestMatchDiff=FLT_MAX;

    mzSlice eicSlice = mainwindow->getEicWidget()->getMzSlice();

    /*
	if (!_currentScan->filterLine.empty() && _currentScan->mslevel==2) {
        mzSlice slice(mzmin,mzmax,eicSlice.rtmin,eicSlice.rtmax); 
		slice.srmId=_currentScan->filterLine;
        mainwindow->getEicWidget()->setMzSlice(slice);
		mainwindow->getEicWidget()->setFocusLine(_currentScan->rt);
        mainwindow->getEicWidget()->replotForced();
        return;
    }*/

	for (int i=0; i < _currentScan->nobs(); i++ ) {
		float diff = abs(_currentScan->mz[i] - mz);
        if ( diff < bestMatchDiff ) { bestMatchDiff = diff; bestMatch=i; }
    }

    if ( bestMatchDiff < 1 ) {
		float bestMz = _currentScan->mz[bestMatch];
        mainwindow->setMzValue(bestMz);
        mainwindow->massCalcWidget->setCharge(_currentScan->getPolarity());
        mainwindow->massCalcWidget->setMass(bestMz);
        //mainwindow->massCalc->compute();
    }
}

void SpectraWidget::mouseDoubleClickEvent(QMouseEvent* event){
    QGraphicsView::mouseDoubleClickEvent(event);
    _focusCoord = _nearestCoord;
    drawGraph();

}

void SpectraWidget::addLabel(QString text,float x, float y) {
    QFont font = QApplication::font(); font.setPointSizeF(font.pointSize()*0.8);

    QGraphicsTextItem* _label = scene()->addText(text, font);
    _label->setPos(toX(x), toY(y));
}

void SpectraWidget::mouseMoveEvent(QMouseEvent* event){
    if (_currentScan == nullptr && _spectralHit.compoundId.isEmpty()) return;

    QGraphicsView::mouseMoveEvent(event);
    QPointF pos = event->pos();

    if (pos.y() < 5 || pos.y() > height()-5 || pos.x() < 5 || pos.y() > width()-5 ) {
        _vnote->hide();
        _note->hide();
        _varrow->hide();
        _arrow->hide();
        return;
    }

    if (!_currentScan) return;

    int nearestPos = findNearestMz(pos);
    if (nearestPos >= 0) {
		_nearestCoord = QPointF(_currentScan->mz[nearestPos], _currentScan->intensity[nearestPos]);
        drawArrow(_currentScan->mz[nearestPos], _currentScan->intensity[nearestPos], invX(pos.x()), invY(pos.y()));

        //search is too slow
        //if (mainwindow->massCalcWidget->isVisible())
        //	  mainwindow->massCalcWidget->setMass(_currentScan->mz[nearestPos]);
    } else {
        _vnote->hide();
        _note->hide();
        _varrow->hide();
        _arrow->hide();
    }


}

int SpectraWidget::findNearestMz(QPointF pos) { 

    float mz = invX(pos.x());
    float mzmin = invX(pos.x()-50);
    float mzmax = invX(pos.x()+50);
    float ycoord  =invY(pos.y());
    int best=-1;

    if (_msLevel == 3) {

        best = _currentScan->findHighestIntensityPosAMU(invX(pos.x()), _ms3MatchingTolr);

    } else {
        vector<int> matches = _currentScan->findMatchingMzs(mzmin,mzmax);
        if (matches.size() > 0) {
            float dist=FLT_MAX;
            for(int i=0; i < matches.size(); i++ ) {
                int p = matches[i];
                float d = sqrt(POW2(_currentScan->intensity[p]-ycoord)+POW2(_currentScan->mz[p]-mz));
                if ( d < dist ){ best=p; dist=d; }
            }
        }
    }


    return best;
}

void SpectraWidget::drawArrow(float mz1, float intensity1, float mz2, float intensity2) {

    float SCALE=1.0;
    float OFFSET=0;
    if (_showOverlay) { SCALE=0.45; OFFSET=-scene()->height()/2.0; }

    int x1=  toX(mz1);
    int y1=  toY(intensity1,SCALE,OFFSET);

    int x2 = toX(mz2);
    int y2 = toY(intensity2,SCALE,OFFSET);

    int x3 = toX(_focusCoord.x());
    int y3 = toY(_focusCoord.y(),SCALE,OFFSET);

    if ( ppmDist(mz1,mz2) < 0.1 ) return;


    if (_arrow != NULL ) {
        _arrow->setVisible(true);
        _arrow->setLine(x1,y1,x2,y2);
    }

    float distance = mz1-_focusCoord.x();
    //float totalInt = _focusCoord.y()+intensity1;
    float totalInt = _focusCoord.y();
    float diff=0;
    if (totalInt)  diff = intensity1/totalInt;

    if (_varrow != NULL && abs(distance) > 0.1 ) {
        _varrow->setLine(x1,y1,x3,y3);
        _vnote->setPos(x1+(x3-x1)/2.0, y1+(y3-y1)/2.0);
        _vnote->setPlainText(QString::number(diff*100,'f',2) + "%" );
        _varrow->show(); _vnote->show();
    } else {
        _varrow->hide(); _vnote->hide();
    }


    int numDigits = _isDisplayHighPrecisionMz ? 7 : 3;

    QString note = tr("m/z: %1 intensity: %2 &Delta;%3").arg( QString::number(mz1,'f',numDigits),QString::number(intensity1,'f',0),QString::number(distance,'f',3));

    if (_note  != NULL ) {
        _note->setPos(x2+1,y2-30);
        _note->setHtml(note);
    }

    _note->show();
    _arrow->show();
}


void SpectraWidget::wheelEvent(QWheelEvent *event) {
    if ( event->delta() > 0 ) {
        zoomOut();
    } else {
        zoomIn();
    }
    replot();
}

/*
void SpectraWidget::annotateScan() {
    return;

    float mz1 = _focusCoord.x();
	if (mz1==0 || _currentScan == NULL) return;

	links = findLinks(mz1,_currentScan, 20, _currentScan->getPolarity());

    for(int i=0; i < links.size(); i++ ) {
		vector<int> matches = _currentScan->findMatchingMzs(links[i].mz2-0.01, links[i].mz2+0.01);
        //qDebug() << "annotateScan() " << links[i].note.c_str() << " " << links[i].mz2 << " " << matches.size();
        links[i].value2=0;

        if (matches.size() > 0) {
			links[i].value2 = _currentScan->intensity[matches[0]];

            for(int i=1; i < matches.size(); i++ ) {
				if(_currentScan->intensity[ matches[i] ] > links[i].value2 ) {
					links[i].value2==_currentScan->intensity[ matches[i] ];
                }
            }
        }
    }
}
*/

void SpectraWidget::annotateScan() {  }



void SpectraWidget::resetZoom() {
    findBounds(true,true);
    replot();
}



void SpectraWidget::zoomIn() {
    float D = (_maxX-_minX)/2;
    if (D < 0.5 ) return;

    float _centerX = _minX+D;

    if (_focusCoord.x() != 0 && _focusCoord.x() > _minX && _focusCoord.x() < _maxX ) _centerX = _focusCoord.x();
	//cerr << "zoomIn center=" << _centerX << " D=" << D <<  " focus=" << _focusCoord.x() << endl;

    _minX = _centerX-D/2;
    _maxX = _centerX+D/2;
    //cerr << _centerX << " " << _minX << " " << _maxX << " " << _minZ << " " << _maxZ << endl;
    findBounds(false, true);
    replot();

}

void SpectraWidget::zoomRegion(float centerMz, float window) {
    if (centerMz <= 0) return;

     int pos = _currentScan->findHighestIntensityPos(centerMz,10.00);
     if (pos > 0 && pos <= _currentScan->nobs()) {
         _focusCoord.setX(_currentScan->mz[pos]);
         _focusCoord.setY(_currentScan->intensity[pos]);
     }

	float _centerX = _focusCoord.x();
	_minX =  _centerX - window;
	_maxX =  _centerX + window;
    findBounds(false, true);
    replot();
}

void SpectraWidget::zoomOut() {
    _minX = _minX * 0.9;
    _maxX = _maxX * 1.1;
    findBounds(false,true);
    replot();
}


void SpectraWidget::compareScans(Scan* s1, Scan* s2) { 

}

void SpectraWidget::contextMenuEvent(QContextMenuEvent * event) {
    event->ignore();
    QMenu menu;

    QAction* a0 = menu.addAction("Reset Zoom");
    connect(a0, SIGNAL(triggered()), SLOT(resetZoom()));

    menu.addSeparator();

    QAction *aFree = menu.addAction(QIcon(rsrcPath +"/free_range.png"), "Span m/z range");
    connect(aFree, SIGNAL(triggered()), SLOT(freeMzRange()));
    aFree->setToolTip("m/z range spans current spectral data");

    QAction *aLock = menu.addAction(QIcon(rsrcPath + "/padlock.png"), " Lock m/z range");
    connect(aLock, SIGNAL(triggered()), SLOT(lockMzRange()));
    aLock->setToolTip("Lock m/z range to current displayed range");

    menu.addSeparator();

    QAction* a3 = menu.addAction("Copy Spectrum to Clipboard");
    connect(a3, SIGNAL(triggered()), SLOT(spectrumToClipboard()));

    QAction* aMspEntry = menu.addAction("Copy MSP Entry to Clipboard");
    connect(aMspEntry, SIGNAL(triggered()), SLOT(mspEntryToClipboard()));

    QAction* aCopyNormSpectrum = menu.addAction("Copy Normalized Spectrum to Clipboard");
    connect(aCopyNormSpectrum, SIGNAL(triggered()), SLOT(normalizedSpectrumToClipboard()));

    QAction* aCopyEncoded = menu.addAction("Copy Encoded Spectrum to Clipboard");
    connect(aCopyEncoded, SIGNAL(triggered()), SLOT(encodedSpectrumToClipboard()));

    menu.addSeparator();

    QAction* a1 = menu.addAction("Go To Scan");
    connect(a1, SIGNAL(triggered()), SLOT(gotoScan()));

    QAction* a3b = menu.addAction("Find Similar Scans");
    connect(a3b, SIGNAL(triggered()), SLOT(findSimilarScans()));

    menu.addSeparator();

    QAction *aHighPrecision = menu.addAction("Display High Precision m/z");
    connect(aHighPrecision, SIGNAL(triggered()), SLOT(toggleDisplayHighPrecisionMz()));
    aHighPrecision->setCheckable(true);
    aHighPrecision->setChecked(_isDisplayHighPrecisionMz);

    QAction* a7 = menu.addAction("Display Fragment Labels");
    connect (a7, SIGNAL(triggered()), SLOT(toggleOverlayLabels()));
    a7->setCheckable(true);
    a7->setChecked(_showOverlayLabels);

    QAction *a8 = menu.addAction("Display Full Title");
    connect(a8, SIGNAL(triggered()), SLOT(toggleDisplayFullTitle()));
    a8->setCheckable(true);
    a8->setChecked(_isDisplayFullTitle);

    QAction *a9 = menu.addAction("Display Compound ID");
    connect(a9, SIGNAL(triggered()), SLOT(toggleDisplayCompoundId()));
    a9->setCheckable(true);
    a9->setChecked(_isDisplayCompoundId);

    menu.exec(event->globalPos());
}

string SpectraWidget::getSpectrumString(string type, bool isNormalizeToMaxIntensity){
    string spectrum = "";
    if (_currentFragment) {
        spectrum = _currentFragment->encodeSpectrum(5, type, isNormalizeToMaxIntensity);
    } else if (_currentScan) {
        Fragment *f = new Fragment();
        f->mzs = _currentScan->mz;
        f->intensity_array = _currentScan->intensity;
        spectrum = f->encodeSpectrum(5, type, isNormalizeToMaxIntensity);
        delete(f);
    }
    return spectrum;
}

void SpectraWidget::spectrumToClipboard() {
    QApplication::clipboard()->setText(getSpectrumString("\t", false).c_str());
}

void SpectraWidget::mspEntryToClipboard() {

    stringstream mspEntry;
    mspEntry << std::fixed << setprecision(5);
    int numPeaks = 0;
    string spectrum = "";
    float precursorMz = 0.0f;

    if (_currentFragment) {
        numPeaks = _currentFragment->nobs();
        spectrum = _currentFragment->encodeSpectrum(5, "\t", false);
        precursorMz = _currentFragment->precursorMz;
    } else if (_currentScan) {
        numPeaks = _currentScan->nobs();
        precursorMz = _currentScan->precursorMz;
        Fragment *f = new Fragment();
        f->mzs = _currentScan->mz;
        f->intensity_array = _currentScan->intensity;
        spectrum = f->encodeSpectrum(5, "\t", false);
        delete(f);
    }

    if (_spectralHit.compoundId != "") {
        mspEntry
            << "Name: " << _spectralHit.compoundId.toStdString() << "\n"
            << "ID: " << _spectralHit.originalCompoundId.toStdString() << "\n";
    } else {
        mspEntry
            << "Name: " << "???" << "\n";
    }

    mspEntry
         << "PrecursorMz: " << precursorMz << "\n"
         << "NumPeaks: " << numPeaks << "\n"
         << spectrum << "\n\n";

    QApplication::clipboard()->setText(mspEntry.str().c_str());
}

void SpectraWidget::normalizedSpectrumToClipboard(){
    QApplication::clipboard()->setText(getSpectrumString("\t", true).c_str());
}

void SpectraWidget::encodedSpectrumToClipboard(){
    QApplication::clipboard()->setText(getSpectrumString("encoded", true).c_str());
}


void SpectraWidget::gotoScan() { 
        if (!_currentScan || !_currentScan->sample || _currentScan->sample->scans.empty()) return;
        int curScanNum = _currentScan->scannum;
		int maxScanNum = _currentScan->sample->scans.size()-1;
		bool ok=false;

		int scanNumber = QInputDialog::getInt (this, 
						"Go To Scan Number", "Enter Scan Number", curScanNum, 
						0, maxScanNum, 1, &ok, 0);
		if (ok && scanNumber > 0 && scanNumber < maxScanNum) {
			Scan* newscan = _currentScan->sample->scans[scanNumber];
			if (newscan) setScan(newscan);
		}
}

vector<mzLink> SpectraWidget::findLinks(float centerMz, Scan* scan, float ppm, int ionizationMode) {

    vector<mzLink> links;
    //check for possible C13s
    for(int i=1; i<20; i++ ) {
        if(i==0) continue;
        float mz=centerMz+(i*1.0034);
        float mzz=centerMz+(i*1.0034)/2;
        float mzzz=centerMz+(i*1.0034)/3;

        if( scan->hasMz(mz,ppm) ) {
            QString noteText = tr("C13-[%1]").arg(i);
            links.push_back(mzLink(centerMz,mz,noteText.toStdString()));
        }

        if( i % 2 !=0 && mzz!=mz && scan->hasMz(mzz,ppm) ) {
            QString noteText = tr("C13-[%1] z=2").arg(i);
            links.push_back(mzLink(centerMz,mzz,noteText.toStdString()));
        }

        if( i % 3 !=0 && scan->hasMz(mzzz,ppm) ) {
            QString noteText = tr("C13-[%1] z=3").arg(i);
            links.push_back(mzLink(centerMz,mzzz,noteText.toStdString()));
        }
    }

    for(int i=0; i < DB.fragmentsDB.size(); i++ ) {
        Adduct* frag  = DB.fragmentsDB[i];
    	if(frag->charge != 0 && SIGN(frag->charge) != SIGN(ionizationMode) ) continue;
        float mzMinus=centerMz-frag->mass;
        float mzPlus =centerMz+frag->mass;
        if( scan->hasMz(mzPlus,ppm)) {
            QString noteText = tr("%1 Fragment").arg(QString(DB.fragmentsDB[i]->name.c_str()));
            links.push_back(mzLink(centerMz,mzPlus,noteText.toStdString()));
        }

        if( scan->hasMz(mzMinus,ppm)) {
            QString noteText = tr("%1 Fragment").arg(QString(DB.fragmentsDB[i]->name.c_str()));
            links.push_back(mzLink(centerMz,mzMinus,noteText.toStdString()));
        }
    }

    //parent check
    for(int i=0; i < DB.adductsDB.size(); i++ ) {
    	if ( SIGN(DB.adductsDB[i]->charge) != SIGN(ionizationMode) ) continue;
        float parentMass=DB.adductsDB[i]->computeParentMass(centerMz);
        parentMass += ionizationMode*HMASS;   //adjusted mass

        if( abs(parentMass-centerMz)>0.1 && scan->hasMz(parentMass,ppm)) {
            cerr << DB.adductsDB[i]->name << " " << DB.adductsDB[i]->charge << " " << parentMass << endl;
            QString noteText = tr("Possible Parent %1").arg(QString(DB.adductsDB[i]->name.c_str()));
            links.push_back(mzLink(centerMz,parentMass,noteText.toStdString()));
        }
    }

    //adduct check
    for(int i=0; i < DB.adductsDB.size(); i++ ) {
    	if ( SIGN(DB.adductsDB[i]->charge) != SIGN(ionizationMode) ) continue;
        float parentMass = centerMz-ionizationMode*HMASS;   //adjusted mass
        float adductMass=DB.adductsDB[i]->computeAdductMass(parentMass);
        if( abs(adductMass-centerMz)>0.1 && scan->hasMz(adductMass,ppm)) {
            QString noteText = tr("Adduct %1").arg(QString(DB.adductsDB[i]->name.c_str()));
            links.push_back(mzLink(centerMz,adductMass,noteText.toStdString()));
        }
    }


    return links;
}


void SpectraWidget::constructAverageScan(float rtmin, float rtmax) {
    if (_avgScan != NULL) delete(_avgScan);
    _avgScan=NULL;

    if (_currentScan && _currentScan->getSample()) {
        Scan* avgScan = _currentScan->getSample()->getAverageScan(rtmin,rtmax,_currentScan->mslevel,_currentScan->getPolarity(),(float)100.0);
        qDebug() << "constructAverageScan() " << rtmin << " " << rtmax << " mslevel=" << _currentScan->mslevel << endl;
        avgScan->simpleCentroid();
        if(avgScan) setScan(avgScan);
    }
}


void SpectraWidget::findSimilarScans() {
    if(!_currentScan) return;
    vector< pair<float,float> > mzarray= _currentScan->getTopPeaks(0.05,1,5);

    QStringList clipboardText;
    for(int i=0; i < mzarray.size(); i++ ) {
    pair<float,float> p = mzarray[i];
            clipboardText  << tr("%1\t%2")
                .arg(QString::number(p.second,'f', 5))
                .arg(QString::number(p.first, 'f', 2));
    }

    mainwindow->spectraMatchingForm->fragmentsText->setPlainText(clipboardText.join("\n"));
    mainwindow->spectraMatchingForm->precursorMz->setText(QString::number(_currentScan->precursorMz,'f',6));
    mainwindow->spectraMatchingForm->show();

}

void SpectraWidget::toggleOverlayLabels() {
    qDebug() << "SpectraWidget::toggleOverlayLabels()";
    _showOverlayLabels = !_showOverlayLabels;
    drawGraph();
    repaint();
}

void SpectraWidget::toggleDisplayFullTitle() {
    qDebug() << "SpectraWidget::toggleDisplayFullTitle()";
    _isDisplayFullTitle= !_isDisplayFullTitle;
    drawGraph();
    repaint();
}

void SpectraWidget::toggleDisplayCompoundId() {
    qDebug() << "SpectraWidget::toggleDisplayCompoundId()";
    _isDisplayCompoundId= !_isDisplayCompoundId;
    drawGraph();
    repaint();
}

void SpectraWidget::toggleDisplayHighPrecisionMz() {
    qDebug() << "SpectraWidget::toggleDisplayHighPrecision()";
    _isDisplayHighPrecisionMz= !_isDisplayHighPrecisionMz;
    drawGraph();
    repaint();
}

void SpectraWidget::lockMzRange(){
    qDebug() << "SpectraWidget::lockMzRange():";
    qDebug() << "ms level" << _msLevel << ": [" << _minX << "-" << _maxX << "]";

    if (_msLevel == 1) {

        mainwindow->getSettings()->setValue("chkAutoMzMin", Qt::CheckState::Unchecked);
        mainwindow->getSettings()->setValue("chkAutoMzMax", Qt::CheckState::Unchecked);
        mainwindow->getSettings()->setValue("spnMzMinVal", _minX);
        mainwindow->getSettings()->setValue("spnMzMaxVal", _maxX);

        mainwindow->settingsForm->chkMs1Autoscale->blockSignals(true);
        mainwindow->settingsForm->chkMs1Autoscale->setCheckState(Qt::CheckState::Unchecked);
        mainwindow->settingsForm->chkMs1Autoscale->blockSignals(false);

        mainwindow->settingsForm->spnMs1MzMinVal->blockSignals(true);
        mainwindow->settingsForm->spnMs1MzMinVal->setValue(static_cast<double>(_minX));
        mainwindow->settingsForm->spnMs1MzMaxVal->blockSignals(false);

        mainwindow->settingsForm->spnMs1MzMaxVal->blockSignals(true);
        mainwindow->settingsForm->spnMs1MzMaxVal->setValue(static_cast<double>(_maxX));
        mainwindow->settingsForm->spnMs1MzMaxVal->blockSignals(false);

    } else if (_msLevel == 2) {

        mainwindow->getSettings()->setValue("chkMs2AutoMzMin", Qt::CheckState::Unchecked);
        mainwindow->getSettings()->setValue("chkMs2AutoMzMax", Qt::CheckState::Unchecked);
        mainwindow->getSettings()->setValue("spnMs2MzMin", _minX);
        mainwindow->getSettings()->setValue("spnMs2MzMax", _maxX);

        mainwindow->settingsForm->chkMs2Autoscale->blockSignals(true);
        mainwindow->settingsForm->chkMs2Autoscale->setCheckState(Qt::CheckState::Unchecked);
        mainwindow->settingsForm->chkMs2Autoscale->blockSignals(false);

        mainwindow->settingsForm->spnMs2MzMin->blockSignals(true);
        mainwindow->settingsForm->spnMs2MzMin->setValue(static_cast<double>(_minX));
        mainwindow->settingsForm->spnMs2MzMin->blockSignals(false);

        mainwindow->settingsForm->spnMs2MzMax->blockSignals(true);
        mainwindow->settingsForm->spnMs2MzMax->setValue(static_cast<double>(_maxX));
        mainwindow->settingsForm->spnMs2MzMax->blockSignals(false);

    } else if (_msLevel == 3) {

        mainwindow->getSettings()->setValue("chkMs3AutoMzMin", Qt::CheckState::Unchecked);
        mainwindow->getSettings()->setValue("chkMs3AutoMzMax", Qt::CheckState::Unchecked);
        mainwindow->getSettings()->setValue("spnMs3MzMin", _minX);
        mainwindow->getSettings()->setValue("spnMs3MzMax", _maxX);

        mainwindow->settingsForm->chkMs3Autoscale->blockSignals(true);
        mainwindow->settingsForm->chkMs3Autoscale->setCheckState(Qt::CheckState::Unchecked);
        mainwindow->settingsForm->chkMs3Autoscale->blockSignals(false);

        mainwindow->settingsForm->spnMs3MzMin->blockSignals(true);
        mainwindow->settingsForm->spnMs3MzMin->setValue(static_cast<double>(_minX));
        mainwindow->settingsForm->spnMs3MzMin->blockSignals(false);

        mainwindow->settingsForm->spnMs3MzMax->blockSignals(true);
        mainwindow->settingsForm->spnMs3MzMax->setValue(static_cast<double>(_maxX));
        mainwindow->settingsForm->spnMs3MzMax->blockSignals(false);

    }

    findBounds(true, true);
    replot();
    repaint();
}

void SpectraWidget::freeMzRange(){

    qDebug() << "SpectraWidget::freeMzRange()";

    if (_msLevel == 1) {

        mainwindow->getSettings()->setValue("chkAutoMzMin", Qt::CheckState::Checked);
        mainwindow->getSettings()->setValue("chkAutoMzMax", Qt::CheckState::Checked);

        mainwindow->settingsForm->chkMs1Autoscale->blockSignals(true);
        mainwindow->settingsForm->chkMs1Autoscale->setCheckState(Qt::CheckState::Checked);
        mainwindow->settingsForm->chkMs1Autoscale->blockSignals(false);

    } else if (_msLevel == 2) {

        mainwindow->getSettings()->setValue("chkMs2AutoMzMin", Qt::CheckState::Checked);
        mainwindow->getSettings()->setValue("chkMs2AutoMzMax", Qt::CheckState::Checked);

        mainwindow->settingsForm->chkMs2Autoscale->blockSignals(true);
        mainwindow->settingsForm->chkMs2Autoscale->setCheckState(Qt::CheckState::Checked);
        mainwindow->settingsForm->chkMs2Autoscale->blockSignals(false);

    } else if (_msLevel == 3) {

        mainwindow->getSettings()->setValue("chkMs3AutoMzMin", Qt::CheckState::Checked);
        mainwindow->getSettings()->setValue("chkMs3AutoMzMax", Qt::CheckState::Checked);

        mainwindow->settingsForm->chkMs3Autoscale->blockSignals(true);
        mainwindow->settingsForm->chkMs3Autoscale->setCheckState(Qt::CheckState::Checked);
        mainwindow->settingsForm->chkMs3Autoscale->blockSignals(false);
    }

    findBounds(true, true);
    replot();
    repaint();
}
