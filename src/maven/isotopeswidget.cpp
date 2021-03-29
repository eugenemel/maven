#include "isotopeswidget.h"
using namespace std;


IsotopeWidget::IsotopeWidget(MainWindow* mw) {
  _mw = mw;

  _scan = nullptr;
  _group = nullptr;
  _compound = nullptr;
  _adduct = nullptr;

  tempCompound = new Compound("Unknown", "Unknown", string(), 0 );	 //temp compound

  setupUi(this);

  adductComboBox->setMinimumSize(250, 0);

  adductComboBox->clear();
  for (Adduct* a : DB.adductsDB) {
    adductComboBox->addItem(a->name.c_str(),QVariant::fromValue(a));
  }

  connect(treeWidget, SIGNAL(itemSelectionChanged()), SLOT(showInfo()));
  connect(formula, SIGNAL(textEdited(QString)), this, SLOT(userChangedFormula(QString)));
  connect(adductComboBox, SIGNAL(currentIndexChanged(QString)), this, SLOT(updateAdduct()));

  workerThread = new BackgroundPeakUpdate(mw);
  workerThread->setRunFunction("pullIsotopes");
  workerThread->setMainWindow(mw);
  workerThread->minGoodPeakCount=1;
  workerThread->minSignalBlankRatio=2;
  workerThread->minSignalBaseLineRatio=2;
  workerThread->minNoNoiseObs=2;
  workerThread->minGroupIntensity=0;
  workerThread->writeCSVFlag=false;
  workerThread->matchRtFlag=false;
  workerThread->showProgressFlag=true;
  workerThread->pullIsotopesFlag=true;
  workerThread->keepFoundGroups=true;

  connect(workerThread, SIGNAL(finished()), this, SLOT(setClipboard()));
  connect(workerThread, SIGNAL(finished()), mw->getEicWidget()->scene(), SLOT(update()));

  QIcon icon = QIcon(rsrcPath + "/exportcsv.png");
  int btnHeight = btnExport->height();
  int btnSize = static_cast<int>(1*btnHeight);

  btnExport->setIcon(icon);
  btnExport->setToolTip(tr("Export currently displayed isotopic envelope"));
  btnExport->setText("");
  btnExport->setIconSize(QSize(btnSize, btnSize));
  btnExport->setFixedSize(btnSize, btnSize);
  btnExport->setFlat(true);

  connect(btnExport, SIGNAL(clicked()), SLOT(exportTableToSpreadsheet()));
  connect(_mw->libraryDialog, SIGNAL(unloadLibrarySignal(QString)), SLOT(unloadCompound(QString)));
}

IsotopeWidget::~IsotopeWidget(){
	links.clear();
}

Adduct* IsotopeWidget::getCurrentAdduct() {

    QVariant v = adductComboBox->currentData();
    Adduct*  adduct =  v.value<Adduct*>();

    if (adduct) {
        return adduct;
    } else if (_mw) {
        return _mw->getUserAdduct();
    }

    return nullptr;
}

void IsotopeWidget::setPeakGroup(PeakGroup* grp) {
    qDebug() << "IsotopeWidget::setPeakGroup():" << grp;
    if (!grp) return;
	_group = grp;
    if (grp && grp->type() != PeakGroup::IsotopeType ) pullIsotopes(grp);
}

void IsotopeWidget::setPeak(Peak* peak) {
    qDebug() << "IsotopeWidget::setPeak():" << peak;
    if (peak == nullptr ) return;

	mzSample* sample = peak->getSample();
    if (sample == nullptr) return;

	Scan* scan = sample->getScan(peak->scan);
    if (scan == nullptr) return;
	_scan = scan;

    if (! _formula.empty() ){
        computeIsotopes(_formula);
    }
}

void IsotopeWidget::setCompound(Compound* cpd ) {

    if (!cpd) return;

    qDebug() << "IsotopeWidget::setCompound():" << cpd->name.c_str();

    //Issue 376: set this first, to ensure that compound name is passed through.
    QString f = QString(cpd->formula.c_str());
    setFormula(f);

    _group = nullptr;
    _compound = cpd;

    setWindowTitle("Isotopes: " + QString(cpd->name.c_str()));
    repaint();
}

//Issue 376: Ensure that removed compounds do not lead to dangling pointers
void IsotopeWidget::unloadCompound(QString db){
    if (_compound && (db == "ALL" || db.toStdString() == _compound->db)) {
        _compound = tempCompound;
        setWindowTitle("Isotopes:" + QString(_compound->name.c_str()));
    }
}

void IsotopeWidget::userChangedFormula(QString f) {
	if (f.toStdString() == _formula ) return;

	_formula = f.toStdString();
    _group = nullptr;

	tempCompound->formula = _formula;
	tempCompound->name =   "Unknown_" + _formula;
	tempCompound->id = "unknown";
	_compound = tempCompound;

	setWindowTitle("Unknown_" + f);
	computeIsotopes(_formula);

}

void IsotopeWidget::setFormula(QString f) {
	formula->setText(f);
	userChangedFormula(f);
}

void IsotopeWidget::updateAdduct() {
    computeIsotopes(_formula);
}

void IsotopeWidget::computeIsotopes(string f) {

    if (f.empty()) return;
    if(links.size() > 0 ) links.clear();

    //use selected adduct to compute theoretical mass.
    double neutralMass = mcalc.computeNeutralMass(f);
    double parentMass = getCurrentAdduct()->computeAdductMass(neutralMass);

    //double parentMass = mcalc.computeMass(f, getCurrentAdduct()->charge);
    float parentPeakIntensity = getIsotopeIntensity(parentMass);

    QSettings* settings = _mw->getSettings();

    double maxNaturalAbundanceErr  = settings->value("maxNaturalAbundanceErr").toDouble();
    bool C13Labeled   =  settings->value("C13Labeled").toBool();
    bool N15Labeled   =  settings->value("N15Labeled").toBool();
    bool S34Labeled   =  settings->value("S34Labeled").toBool();
    bool D2Labeled   =   settings->value("D2Labeled").toBool();

    bool chkIgnoreNaturalAbundance = settings->value("chkIgnoreNaturalAbundance").toBool();
    bool chkExtractNIsotopes = settings->value("chkExtractNIsotopes").toBool();
    int spnMaxIsotopesToExtract = settings->value("spnMaxIsotopesToExtract").toInt();

    //NO ISOTOPES SELECTED..
    if (C13Labeled == false and N15Labeled == false and S34Labeled == false and D2Labeled == false) {
        showTable();
        return;
    }
    qDebug() << "Isotope Calculation: " << C13Labeled << N15Labeled << S34Labeled << D2Labeled;


    int maxNumProtons = INT_MAX;
    if (chkExtractNIsotopes) {
        maxNumProtons = spnMaxIsotopesToExtract;
    }

    vector<Isotope> isotopes = mcalc.computeIsotopes(f, getCurrentAdduct(), maxNumProtons, C13Labeled, N15Labeled, S34Labeled, D2Labeled);

    for (int i=0; i < isotopes.size(); i++ ) {
        Isotope& x = isotopes[i];

        float expectedAbundance    =  x.abundance;

        mzLink link;
        if ( ((x.C13 > 0 && C13Labeled==true) || x.C13 == 0) &&
             ((x.N15 > 0 && N15Labeled==true) || x.N15 == 0) &&
             ((x.S34 > 0 && S34Labeled==true) || x.S34 == 0) &&
             ((x.H2 > 0 && D2Labeled==true ) || x.H2 == 0)) {

            if (chkIgnoreNaturalAbundance) {
                if (expectedAbundance < 1e-8) continue;
                // if (expectedAbundance * parentPeakIntensity < 500) continue;
                float isotopePeakIntensity = getIsotopeIntensity(x.mz);
                float observedAbundance = isotopePeakIntensity/(parentPeakIntensity+isotopePeakIntensity);
                float naturalAbundanceError = abs(observedAbundance-expectedAbundance)/expectedAbundance*100;
                if (naturalAbundanceError > maxNaturalAbundanceErr )  continue;
            }

            link.mz1 = parentMass;
            link.mz2 = x.mz;
            link.note= x.name;
            link.value1 = x.abundance;
            link.value2 = getIsotopeIntensity(x.mz);
            links.push_back(link);
        }
    }
    sort(links.begin(),links.end(),mzLink::compMz);
    showTable();
}

float IsotopeWidget::getIsotopeIntensity(float mz)  {
    float highestIntensity=0;
	double ppm = _mw->getUserPPM();

    if (_scan == nullptr ) return 0;
	mzSample* sample = _scan->getSample();
    if ( sample == nullptr) return 0;

	for(int i=_scan->scannum-2; i < _scan->scannum+2; i++ ) {
		Scan* s = sample->getScan(i);
		vector<int> matches = s->findMatchingMzs(mz-mz/1e6*ppm, mz+mz/1e6*ppm);
		for(int i=0; i < matches.size(); i++ ) {
			int pos = matches[i];
			if ( s->intensity[pos] > highestIntensity ) highestIntensity = s->intensity[pos];
		}
	}
    return highestIntensity;
}

Peak* IsotopeWidget::getSamplePeak(PeakGroup* group, mzSample* sample) {
		for (int i=0; i< group->peaks.size(); i++ ) {
				if ( group->peaks[i].getSample() == sample ) {
					return &(group->peaks[i]);
				}
		}
        return nullptr;
}
void IsotopeWidget::pullIsotopes(PeakGroup* group) {
		if(!group) return;

		//clear clipboard
		QClipboard *clipboard = QApplication::clipboard();
		clipboard->clear(QClipboard::Clipboard);

        setClipboard(group);

        //check if isotope search is on
        QSettings* settings = _mw->getSettings();
        bool C13Labeled   =  settings->value("C13Labeled").toBool();
        bool N15Labeled   =  settings->value("N15Labeled").toBool();
        bool S34Labeled   =  settings->value("S34Labeled").toBool();
        bool D2Labeled   =   settings->value("D2Labeled").toBool();
        if (C13Labeled == false and N15Labeled == false and S34Labeled == false and D2Labeled == false) return;

        if (group->compound == nullptr) {
			_mw->setStatusText(tr("Unknown compound. Clipboard set to %1").arg(group->tagString.c_str()));
			return;
		}

		int isotopeCount=0;
		for (int i=0; i < group->children.size(); i++ ) {
			if ( group->children[i].isIsotope() ) isotopeCount++;
		}

        vector <mzSample*> vsamples = _mw->getVisibleSamples();
		workerThread->stop();
		workerThread->setPeakGroup(group);
		workerThread->setSamples(vsamples);
		workerThread->compoundPPMWindow = _mw->getUserPPM();
        workerThread->start();
        _mw->setStatusText("IsotopeWidget:: pullIsotopes(() started");
}

void IsotopeWidget::setClipboard() {
	if ( _group ) {

		//update clipboard
		setClipboard(_group);

		//update eic widget
  		_mw->getEicWidget()->setSelectedGroup(_group);

        /*
		//update gallery widget
		if (_mw->galleryDockWidget->isVisible() ) {
			vector<PeakGroup*>isotopes;
			for (int i=0; i < _group->children.size(); i++ ) {
				if ( _group->children[i].isIsotope() ) isotopes.push_back(&_group->children[i]);
			}
			_mw->galleryWidget->clear();
			_mw->galleryWidget->addEicPlots(isotopes);
		}
        */
	}
}

void IsotopeWidget::setClipboard(QList<PeakGroup*>& groups) {
    QString clipboardText;

    unsigned int groupCount=0;
    bool includeSampleHeader=true;
    foreach(PeakGroup* group, groups) {
        if(!group) continue;
        if (groupCount>0) includeSampleHeader=false;
         QString infoText = groupIsotopeMatrixExport(group,includeSampleHeader);
         clipboardText += infoText;
         groupCount++;
    }
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText( clipboardText );
}

void IsotopeWidget::setClipboard(PeakGroup* group) {
    bool includeSampleHeader=true;
    QString infoText = groupIsotopeMatrixExport(group,includeSampleHeader);
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText( infoText );
}

QString IsotopeWidget::groupIsotopeMatrixExport(PeakGroup* group, bool includeSampleHeader) {
    if (group == nullptr ) return "";
		//header line
		QString tag(group->tagString.c_str());
        if ( tag.isEmpty() && group->compound != nullptr ) tag = QString(group->compound->name.c_str());
        if ( tag.isEmpty() && group->srmId.length()   ) tag = QString(group->srmId.c_str());
        if ( tag.isEmpty() && group->meanMz > 0 )       tag = QString::number(group->meanMz,'f',6) + "@" +  QString::number(group->meanRt,'f',2);
        if ( tag.isEmpty() )  tag = QString::number(group->groupId);
        QString isotopeInfo;

       vector <mzSample*> vsamples = _mw->getVisibleSamples();
       sort(vsamples.begin(), vsamples.end(), mzSample::compSampleOrder);

        //include header
       if (includeSampleHeader) {
           for(int i=0; i < vsamples.size(); i++ ) {
               isotopeInfo += "\t" + QString(vsamples[i]->sampleName.c_str());
           }
           isotopeInfo += "\n";


           bool includeSetNamesLine=true;
           if (includeSetNamesLine){
               for(int i=0; i < vsamples.size(); i++ ) {
                   isotopeInfo += "\t" + QString(vsamples[i]->getSetName().c_str());
               }
               isotopeInfo += "\n";
           }
       }

       //get isotopic groups
		vector<PeakGroup*>isotopes;
		for(int i=0; i < group->childCount(); i++ ) {
			if (group->children[i].isIsotope() ) {
                PeakGroup* isotope = &(group->children[i]);
                isotopes.push_back(isotope);
			}
		}
		std::sort(isotopes.begin(), isotopes.end(), PeakGroup::compC13);

		if (isotopes.size() > 0 ) {
				MatrixXf MM = _mw->getIsotopicMatrix(group);
				///qDebug() << "MM row=" << MM.rows() << " " << MM.cols() << " " << isotopes.size() <<  " " << vsamples.size() << endl;
				for (int i=0; i < isotopes.size(); i++ ) {
						QStringList groupInfo;
                        groupInfo << tag + " " + QString(isotopes[i]->tagString.c_str());
						for( unsigned int j=0; j < vsamples.size(); j++) {
								//qDebug() << i << " " << j << " " << MM(j,i);
								groupInfo << QString::number(MM(j,i), 'f', 2 );
						}
						isotopeInfo += groupInfo.join("\t") + "\n";
				}
				_mw->setStatusText("Clipboard set to isotope summary");
		} else {
                isotopeInfo += tag + groupTextEport(group) + "\n";
				_mw->setStatusText("Clipboard to group summary");
		}
        return isotopeInfo;

}

void IsotopeWidget::showTable() {
	QTreeWidget *p = treeWidget;
	p->clear();
    p->setColumnCount( 6);
    p->setHeaderLabels(  QStringList() << "Isotope Name" << "m/z" << "Intensity" << "%Labeling" << "%Expected" << "%Relative");
	p->setUpdatesEnabled(true);
	p->setSortingEnabled(true);

    double isotopeIntensitySum=0;
    double maxIntensity=0;
    for(unsigned int i=0;  i < links.size(); i++ )  {
        isotopeIntensitySum += links[i].value2;
        if( links[i].value1 > maxIntensity) maxIntensity=links[i].value1;
    }

	for(unsigned int i=0;  i < links.size(); i++ ) {
		float frac=0;
        if ( isotopeIntensitySum>0) links[i].isotopeFrac = links[i].value2/isotopeIntensitySum*100;

        links[i].percentExpected = links[i].value1*100;
        links[i].percentRelative = links[i].value1/maxIntensity*100;

		NumericTreeWidgetItem *item = new NumericTreeWidgetItem(treeWidget,mzSliceType);
		QString item1 = QString( links[i].note.c_str() );
		QString item2 = QString::number( links[i].mz2, 'f', 5 );
        QString item3 = QString::number( links[i].value2, 'f', 4 );
        QString item4 = QString::number(links[i].isotopeFrac, 'f',4 );
        QString item5 = QString::number(links[i].percentExpected, 'f', 4 );
        QString item6 = QString::number(links[i].percentRelative, 'f', 4 );

		item->setText(0,item1);
		item->setText(1,item2);
		item->setText(2,item3);
		item->setText(3,item4);
		item->setText(4,item5);
        item->setText(5,item6);
	}

	p->resizeColumnToContents(0);
	p->setSortingEnabled(true);
    p->sortByColumn(1,Qt::AscendingOrder);
	p->setUpdatesEnabled(true);
}

void IsotopeWidget::showInfo() {
	QList<QTreeWidgetItem *> selectedItems = 	treeWidget->selectedItems ();
	if(selectedItems.size() < 1) return;
	QTreeWidgetItem* item = selectedItems[0];
	if(!item) return;

   QString __mz = item->text(1);
   float mz = __mz.toDouble();

   if (mz > 0 ) {
	   double ppm = _mw->getUserPPM();
	   mzSlice slice  = _mw->getEicWidget()->getMzSlice();
	   slice.mzmin = mz - mz/1e6*ppm;
	   slice.mzmax = mz + mz/1e6*ppm;
	   if(_compound ) slice.compound = _compound;
	   if(!_compound) slice.compound = tempCompound;
	   _mw->getEicWidget()->setMzSlice(slice);
   }
}

QString IsotopeWidget::groupTextEport(PeakGroup* group) {

        if (group == nullptr ) return "";
		QStringList info;

		PeakGroup::QType qtype = _mw->getUserQuantType();
		vector<mzSample*> vsamples = _mw->getVisibleSamples();
		sort(vsamples.begin(), vsamples.end(), mzSample::compSampleOrder);
		vector<float> yvalues = group->getOrderedIntensityVector(vsamples,qtype);

		info << QString(group->tagString.c_str());
		for( unsigned int j=0; j < vsamples.size(); j++) {
			info << QString::number(yvalues[j], 'f', 2 );
		}
		return info.join("\t");
}

void IsotopeWidget::exportTableToSpreadsheet() {

    qDebug() << "IsotopeWidget::exportTableToSpreadsheet()";

    if (links.size() == 0 ) {
        QString msg = "No isotopes information currently available";
        QMessageBox::warning(this, tr("Error"), msg);
        return;
    }

    QString dir = ".";
    QSettings* settings = _mw->getSettings();

    if ( settings->contains("lastDir") ) dir = settings->value("lastDir").value<QString>();

    QString formatCSV = "Comma Separted Values (*.csv)";
    QString formatMatrix =  "Summary Matrix Format (*.tab)";

    QString sFilterSel;
    QString fileName = QFileDialog::getSaveFileName(this,
            tr("Export Isotopes Information"), dir,
            formatCSV + ";;" + formatMatrix,
            &sFilterSel);

    if(fileName.isEmpty()) return;

    if (sFilterSel == formatCSV) {
        if(!fileName.endsWith(".csv",Qt::CaseInsensitive)) fileName = fileName + ".csv";
    } else if ( sFilterSel == formatMatrix) {
        if(!fileName.endsWith(".tab",Qt::CaseInsensitive)) fileName = fileName + ".tab";
    }


    vector<mzSample*> samples = _mw->getSamples();
    if ( samples.size() == 0) return;

    CSVReports* csvreports = new CSVReports(samples);
    csvreports->setUserQuantType( _mw->getUserQuantType() );

    if (sFilterSel == formatCSV) {
        csvreports->setCommaDelimited();
        csvreports->openMzLinkReport(fileName.toStdString());
    } else if (sFilterSel == formatMatrix )  {
        csvreports->setTabDelimited();
        csvreports->openMzLinkReport(fileName.toStdString());
    } else {
        qDebug() << "exportGroupsToSpreadsheet: Unknown file type. " << sFilterSel;
    }


    qDebug() << "Writing report to " << fileName << "...";

    //Issue 332
    for (auto item : links) {
        csvreports->writeIsotopeTableMzLink(&(item));
    }

    csvreports->closeFiles();
    if(csvreports) delete(csvreports);

    qDebug() << "Finished Writing IsotopesWidget mzLink report to " << fileName << ".";
}
