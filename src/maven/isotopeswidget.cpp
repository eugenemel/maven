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

  formula->setMinimumSize(150, 0);
  adductComboBox->setMinimumSize(150, 0);

  adductComboBox->clear();
  for (Adduct* a : DB.adductsDB) {
    adductComboBox->addItem(a->name.c_str(),QVariant::fromValue(a));
  }

  cmbSampleName->setMinimumSize(150, 0);

  connect(treeWidget, SIGNAL(itemSelectionChanged()), SLOT(showInfo()));
  connect(formula, SIGNAL(textEdited(QString)), this, SLOT(userChangedFormula(QString)));
  connect(adductComboBox, SIGNAL(currentIndexChanged(QString)), this, SLOT(updateAdduct()));
  connect(cmbSampleName, SIGNAL(currentIndexChanged(QString)), this, SLOT(rebuildTableCurrentGroup()));

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

mzSample* IsotopeWidget::getCurrentSample() {
    QVariant v = cmbSampleName->currentData();
    mzSample *sample = v.value<mzSample*>();
    return sample;
}

void IsotopeWidget::updateSampleComboBox() {
    cmbSampleName->blockSignals(true);

    cmbSampleName->clear();
    cmbSampleName->addItem("Select Sample", QVariant::fromValue(nullptr));
    for (mzSample *sample : _mw->getVisibleSamples()) {
        cmbSampleName->addItem(QString(sample->sampleName.c_str()), QVariant::fromValue(sample));
    }

    cmbSampleName->blockSignals(false);
}

void IsotopeWidget::setPeakGroup(PeakGroup* grp) {

    qDebug() << "IsotopeWidget::setPeakGroup():" << grp;

    if (!grp) return;

    if (_group) {
        delete(_group);
        _group = nullptr;
    }

    if (grp->compound) setCompound(grp->compound);

    _group = new PeakGroup(*grp);

    //Issue 402, 406: try to use saved parameters, fall back to current GUI settings
    IsotopeParameters isotopeParameters = _group->isotopeParameters;
    if (isotopeParameters.isotopeParametersType == IsotopeParametersType::INVALID) {
        isotopeParameters = _mw->getIsotopeParameters();
    }
    _group->pullIsotopes(isotopeParameters, _mw->getSamples());

    rebuildTableFromPeakGroup(_group);
}

void IsotopeWidget::rebuildTableCurrentGroup() {
    rebuildTableFromPeakGroup(_group);
}

void IsotopeWidget::rebuildTableFromPeakGroup(PeakGroup* group) {
     qDebug() << "IsotopeWidget::rebuildTableFromPeakGroup2()";

     if (!group) return;

     links.clear();
     links = vector<mzLink>(group->childCount());

     mzSample* sample = getCurrentSample();
     float sampleQuantTotal = 0.0f;

     //monoisotopic abundance, sample quant total
     float mZeroExpectedAbundance = 1.0f;
     for (unsigned int i = 0; i < group->childCount(); i++) {
        PeakGroup* isotope = &(group->children[i]);
        if (isotope->tagString == "C12 PARENT") {
            mZeroExpectedAbundance = isotope->expectedAbundance;
        }
        if (sample) {
            vector<mzSample*> samples{sample};
            float quantVal = isotope->getOrderedIntensityVector(samples, _mw->getUserQuantType()).at(0);
            sampleQuantTotal += quantVal;
        }
     }

     //prevent divide by zero error
     if (sampleQuantTotal <= 0.0f) {
        sampleQuantTotal = 1.0;
     }

     for (unsigned int i = 0; i < group->childCount(); i++) {
        PeakGroup* isotope = &(group->children[i]);
        float quantVal = 0.0f;
        if (sample) {
            vector<mzSample*> samples{sample};
            quantVal = isotope->getOrderedIntensityVector(samples, _mw->getUserQuantType()).at(0);
        }

        mzLink link;

        link.note= group->children[i].tagString;
        link.mz2 = group->children[i].meanMz;
        link.value2 = quantVal;
        link.isotopeFrac = 100.0f * quantVal/sampleQuantTotal;
        link.percentExpected = group->children[i].expectedAbundance * 100.0f;
        link.percentRelative = group->children[i].expectedAbundance / mZeroExpectedAbundance * 100.0f;

        links[i] = link;
     }

     sort(links.begin(),links.end(),mzLink::compMz);
     showTable();
}

void IsotopeWidget::setCompound(Compound* cpd ) {

    if (!cpd) return;

    qDebug() << "IsotopeWidget::setCompound():" << cpd->name.c_str();

    //Issue 376: set this first, to ensure that compound name is passed through.
    QString f = QString(cpd->formula.c_str());
    setFormula(f);

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

    if (!_compound) {
        tempCompound->formula = _formula;
        tempCompound->name =   "Unknown_" + _formula;
        tempCompound->id = "unknown";
        _compound = tempCompound;
    }

    computeIsotopes(_formula);
}

void IsotopeWidget::computeIsotopes(string f) {

    if (f.empty()) return;

    if(links.size() > 0 ) links.clear();

    IsotopeParameters isotopeParameters= _mw->getIsotopeParameters();

    int maxNumProtons = INT_MAX;
    if (isotopeParameters.isExtractNIsotopes) {
        maxNumProtons = isotopeParameters.maxIsotopesToExtract;
    }

    vector<Isotope> massList = MassCalculator::computeIsotopes2(
        f,
        getCurrentAdduct(),
        isotopeParameters.getLabeledIsotopes(),
        isotopeParameters.labeledIsotopeRetentionPolicy,
        NaturalAbundanceData::defaultNaturalAbundanceData,
        isotopeParameters.isNatAbundance,
        maxNumProtons,
        isotopeParameters.natAbundanceThreshold,
        false
        );

    links.clear();
    links = vector<mzLink>(massList.size());

    double maxAbundance = 0.0;
    for (auto isotope : massList){
        if (isotope.abundance > maxAbundance) maxAbundance = isotope.abundance;
    }

    for (unsigned int i = 0; i < links.size(); i++) {
        links[i].note = massList[i].name;
        links[i].mz2 = static_cast<float>(massList[i].mz);
        links[i].value2 = 0.0f;
        links[i].isotopeFrac = 0.0f;
        links[i].percentExpected = 100.0f * static_cast<float>(massList[i].abundance);
        links[i].percentRelative =  maxAbundance > 0 ? 100.0f * static_cast<float>(massList[i].abundance/maxAbundance) : 0.0f;
    }

    sort(links.begin(),links.end(),mzLink::compMz);
    showTable();
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
        std::sort(isotopes.begin(), isotopes.end(), PeakGroup::compIsotopicIndex);

		if (isotopes.size() > 0 ) {
            IsotopeMatrix isotopeMatrix = _mw->getIsotopicMatrix(group);
            MatrixXf MM = isotopeMatrix.isotopesData;
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
    p->setColumnCount(6);
    p->setHeaderLabels(QStringList() << "Isotope Name" << "m/z" << "Intensity" << "% Observed" << "% Theoretical" << "% Theoretical Max");
	p->setUpdatesEnabled(true);
	p->setSortingEnabled(true);

	for(unsigned int i=0;  i < links.size(); i++ ) {

		NumericTreeWidgetItem *item = new NumericTreeWidgetItem(treeWidget,mzSliceType);
		QString item1 = QString( links[i].note.c_str() );
        QString item2 = QString::number(static_cast<double>(links[i].mz2), 'f', 5 );
        QString item3 = QString::number(static_cast<double>(links[i].value2), 'f', 4 );
        QString item4 = QString::number(static_cast<double>(links[i].isotopeFrac), 'f',4 );
        QString item5 = QString::number(static_cast<double>(links[i].percentExpected), 'f', 4 );
        QString item6 = QString::number(static_cast<double>(links[i].percentRelative), 'f', 4 );

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

    repaint();
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
       slice.mzmin = mz - mz/1e6f*static_cast<float>(ppm);
       slice.mzmax = mz + mz/1e6f*static_cast<float>(ppm);
	   if(_compound ) slice.compound = _compound;
	   if(!_compound) slice.compound = tempCompound;

       //Issue 536
       _mw->getEicWidget()->unsetAlwaysDisplayGroup();
       _mw->getEicWidget()->setMzSlice(slice, false, false); //Use slice RT values
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
