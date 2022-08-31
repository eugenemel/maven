#include "treedockwidget.h"
#include <unordered_set>



TreeDockWidget::TreeDockWidget(MainWindow *mw, QString title, int numColms) {
        _mw = mw;
		treeWidget=new QTreeWidget(this);
		treeWidget->setColumnCount(numColms);
		treeWidget->setObjectName(title);
        connect(treeWidget,SIGNAL(itemSelectionChanged()), SLOT(showInfoAndUpdateMassCalcGUI()));
		treeWidget->setHeaderHidden(true);

        //QShortcut* ctrlA = new QShortcut(QKeySequence(tr("Ctrl+A", "Select All")), this);
        //connect(ctrlA,SIGNAL(activated()),treeWidget,SLOT(selectAll())); 

        //QShortcut* ctrlC = new QShortcut(QKeySequence(tr("Ctrl+C", "Copy Items")), this);
        //connect(ctrlC,SIGNAL(activated()),treeWidget,SLOT(copyToClipbard())); 

		setWidget(treeWidget);
		setWindowTitle(title);
		setObjectName(title);
}

//Issue 259
void TreeDockWidget::addMs3TitleBar() {

    //Issue 226
    treeWidget->setSelectionBehavior(QAbstractItemView::SelectRows);

    QToolBar *toolBar = new QToolBar(this);
    toolBar->setFloatable(false);
    toolBar->setMovable(false);

    QLabel *lblMs1PrecMz = new QLabel("Ms1PreMz:");
    QLabel *lblMs2PrecMz = new QLabel("Ms2PreMz:");

    ms1PrecMzSpn = new QDoubleSpinBox();
    ms1PrecMzSpn->setMinimum(0);
    ms1PrecMzSpn->setMaximum(999999999);
    ms1PrecMzSpn->setSingleStep(1);
    ms1PrecMzSpn->setMinimumWidth(120);
    ms1PrecMzSpn->setDecimals(4);

    ms2PrecMzSpn = new QDoubleSpinBox();
    ms2PrecMzSpn->setMinimum(0);
    ms2PrecMzSpn->setMaximum(999999999);
    ms2PrecMzSpn->setSingleStep(1);
    ms2PrecMzSpn->setMinimumWidth(120);
    ms2PrecMzSpn->setDecimals(4);

    QPushButton *btnSubmit = new QPushButton("Find Scans");

    toolBar->addWidget(lblMs1PrecMz);
    toolBar->addWidget(ms1PrecMzSpn);
    toolBar->addWidget(lblMs2PrecMz);
    toolBar->addWidget(ms2PrecMzSpn);
    toolBar->addWidget(btnSubmit);

    QToolBar *labelToolBar = new QToolBar(this);
    auto dummy1 = new QWidget(this);
    dummy1->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto dummy2 = new QWidget(this);
    dummy2->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QLabel *lblTitle = new QLabel("MS3 List");

    labelToolBar->addWidget(dummy1);
    labelToolBar->addWidget(lblTitle);
    labelToolBar->addWidget(dummy2);

    QMainWindow *titleBarWidget = new QMainWindow();

    titleBarWidget->addToolBar(labelToolBar);
    titleBarWidget->addToolBarBreak();
    titleBarWidget->addToolBar(toolBar);

    setTitleBarWidget(titleBarWidget);

    connect(ms1PrecMzSpn, SIGNAL(valueChanged(double)), this, SLOT(ms3SearchFromSpinBoxes()));
    connect(ms2PrecMzSpn, SIGNAL(valueChanged(double)), this, SLOT(ms3SearchFromSpinBoxes()));

    connect(btnSubmit, SIGNAL(clicked()), this, SLOT(ms3SearchFromSpinBoxes()));
}

void TreeDockWidget::ms3SearchFromSpinBoxes() {

    if (!_mw || !ms1PrecMzSpn || !ms2PrecMzSpn) return;

    _mw->showMs3Scans(static_cast<float>(ms1PrecMzSpn->value()), static_cast<float>(ms2PrecMzSpn->value()));
}

QTreeWidgetItem* TreeDockWidget::addItem(QTreeWidgetItem* parentItem, string key , float value, int type=0) {
	QTreeWidgetItem *item = new QTreeWidgetItem(parentItem,type);
	item->setText(0, QString(key.c_str()));
	item->setText(1, QString::number(value,'f',3));
	return item;
}

QTreeWidgetItem* TreeDockWidget::addItem(QTreeWidgetItem* parentItem, string key , string value, int type=0) {
	QTreeWidgetItem *item = new QTreeWidgetItem(parentItem,type);
	item->setText(0, QString(key.c_str()));
	if (! value.empty() ) { item->setText(1, QString(value.c_str())); }
	return item;
}

void TreeDockWidget::clearTree() { 
		treeWidget->clear();
}


void TreeDockWidget::setInfo(vector<mzSlice*>& slices) { 
    QStringList header; header << "Slices";
	treeWidget->clear();
    treeWidget->setHeaderLabels( header );
    treeWidget->setSortingEnabled(true);

	for(int i=0; i < slices.size(); i++ ) { 
			mzSlice* slice = slices[i];
			Compound* x = slice->compound;
			QString tag = QString(slice->srmId.c_str());

			QTreeWidgetItem *parent = new QTreeWidgetItem(treeWidget,mzSliceType);
			parent->setText(0,tag);
  	    	parent->setData(0,Qt::UserRole,QVariant::fromValue(*slice));

			if (x) { 
				addCompound(x, parent);
				parent->setText(0, tr("%1 %2").arg(QString(x->name.c_str()), tag ));
			}
	}
}

void TreeDockWidget::setInfo(vector<SRMTransition*>& srmTransitions) {
    treeWidget->clear();
    treeWidget->setSortingEnabled(true);

    for (unsigned int i = 0; i < srmTransitions.size(); i++) {
        SRMTransition *srmTransition = srmTransitions[i];

        QString compoundName;
        QString adductName;
        QString retentionTime;

        //Issue 563: SRM Transitions have no RT themselves, it is only the compounds that provide RTs
        if (srmTransition->compound) {
            compoundName = QString(srmTransition->compound->name.c_str());
            if (srmTransition->compound->expectedRt > 0) {
                retentionTime = QString::number(static_cast<double>(srmTransition->compound->expectedRt), 'f', 2);
            }
        }
        if (srmTransition->adduct) adductName = QString(srmTransition->adduct->name.c_str());

        QTreeWidgetItem *item = new QTreeWidgetItem(treeWidget, SRMTransitionType);
        item->setData(0, Qt::UserRole, QVariant::fromValue(*srmTransition));
        item->setText(0, QString::number(srmTransition->precursorMz, 'f', 2));
        item->setText(1, QString::number(srmTransition->productMz, 'f', 2));
        item->setText(2, retentionTime);
        item->setText(3, compoundName);
        item->setText(4, adductName);
        item->setText(5, QString::number(srmTransition->srmIds.size())); //# srmIds in all samples
        item->setText(6, QString::number(srmTransition->srmIdBySample.size())); // # samples
    }
}


void TreeDockWidget::setInfo(Peak* peak) {
	treeWidget->clear();
    addPeak(peak, nullptr);
}

void TreeDockWidget::selectMs2Scans(Peak *peak) {

    if (!peak) return;
    if (!peak->sample) return;

    treeWidget->blockSignals(true);
    treeWidget->clearSelection();
    for (int i = 0; i < treeWidget->topLevelItemCount(); i++) {

        QTreeWidgetItem *item = treeWidget->topLevelItem(i);

        QVariant v =   item->data(0,Qt::UserRole);
        Scan*  scan =  v.value<Scan*>();

        if (scan && scan->mslevel == 2 && scan->sample == peak->sample &&
                scan->rt >= peak->rtmin && scan->rt <= peak->rtmax) {
            item->setSelected(true);
        }
    }
    showInfo();
    treeWidget->blockSignals(false);
}

void TreeDockWidget::selectMs2Scans(PeakGroup* group) {

    if (!group) return;

    treeWidget->blockSignals(true);
    treeWidget->clearSelection();
    for (int i = 0; i < treeWidget->topLevelItemCount(); i++) {

        QTreeWidgetItem *item = treeWidget->topLevelItem(i);

        QVariant v =   item->data(0,Qt::UserRole);
        Scan*  scan =  v.value<Scan*>();

        if (scan && scan->mslevel == 2 && scan->sample) {

            for (unsigned int i = 0; i < group->peaks.size(); i++) {
                mzSample *peakSample = group->peaks[i].getSample();

                if (peakSample && scan->sample == peakSample &&
                        scan->rt >= group->peaks[i].rtmin && scan->rt <= group->peaks[i].rtmax) {
                       item->setSelected(true);
                       break;
                }
            }

        }
    }
    showInfo();
    treeWidget->blockSignals(false);
}

void TreeDockWidget::selectMs2Scans(Scan* ms2Scan) {

    if (!ms2Scan) return;
    if (!ms2Scan->sample) return;
    if (ms2Scan->mslevel != 2) return;

    treeWidget->blockSignals(true);
    treeWidget->clearSelection();

    for (int i = 0; i < treeWidget->topLevelItemCount(); i++) {

        QTreeWidgetItem *item = treeWidget->topLevelItem(i);

        QVariant v =   item->data(0,Qt::UserRole);
        Scan*  scan =  v.value<Scan*>();

        if (scan && scan->mslevel == 2 && scan->sample && scan == ms2Scan) {

            if (scan == ms2Scan) {
                item->setSelected(true);
            }

        }
    }
    showInfo();
    treeWidget->blockSignals(false);
}

/**
 * @brief TreeDockWidget::showInfoAndUpdateMassCalcGUI
 * Path to showInfo - only use this for user clicks
 */
void TreeDockWidget::showInfoAndUpdateMassCalcGUI() {
    showInfo(true);
}

/**
 * @brief TreeDockWidget::showInfo
 * @param isUpdateMassCalcGUI
 * If true, Also update the mass calc gui with computed values
 */
void TreeDockWidget::showInfo(bool isUpdateMassCalcGUI) {

    MainWindow* mainwindow = static_cast<MainWindow*>(parentWidget());
        if ( ! mainwindow ) return;

        int mslevel = -1;
        unordered_set<float> rts{};

        //retrieve settings
        //scan filter
        float minFracIntensity = mainwindow->getSettings()->value("spnScanFilterMinIntensityFraction", 0).toFloat();
        float minSNRatio = mainwindow->getSettings()->value("spnScanFilterMinSN", 0).toFloat();
        int maxNumberOfFragments = mainwindow->getSettings()->value("spnScanFilterRetainTopX", -1).toInt();
        int baseLinePercentile = static_cast<int>(mainwindow->getSettings()->value("spnScanFilterBaseline", 5).toDouble());
        bool isRetainFragmentsAbovePrecursorMz = mainwindow->getSettings()->value("chkScanFilterRetainHighMzFragments", true).toBool();
        float precursorPurityPpm = -1;
        float minIntensity = mainwindow->getSettings()->value("spnScanFilterMinIntensity", 0).toFloat();

        //consensus
        float productPpmTolr = mainwindow->getSettings()->value("spnConsensusPeakMatchTolerance", 10).toFloat();
        bool isIntensityAvgByObserved = mainwindow->getSettings()->value("chkConsensusAvgOnlyObserved", true).toBool();
        bool isNormalizeIntensityArray = mainwindow->getSettings()->value("chkConsensusNormalizeTo10K", false).toBool();
        int minNumScansForConsensus = mainwindow->getSettings()->value("spnConsensusMinPeakPresence", 0).toInt();
        float minFractionScansForConsensus = mainwindow->getSettings()->value("spnConsensusMinPeakPresenceFraction", 0).toFloat();

        //Issue 217
        Fragment::ConsensusIntensityAgglomerationType consensusIntensityAgglomerationType = Fragment::ConsensusIntensityAgglomerationType::Mean;
        QString consensusIntensityAgglomerationTypeStr = mainwindow->getSettings()->value("cmbConsensusAgglomerationType").toString();

        if (consensusIntensityAgglomerationTypeStr == "Mean") {
            consensusIntensityAgglomerationType = Fragment::ConsensusIntensityAgglomerationType::Mean;
        } else if (consensusIntensityAgglomerationTypeStr == "Median") {
            consensusIntensityAgglomerationType = Fragment::ConsensusIntensityAgglomerationType::Median;
        }

        if (this->exclusiveItemType == ScanType) {
            if (treeWidget->selectedItems().size() == 1) {

                QTreeWidgetItem *item = treeWidget->selectedItems().at(0);
                QVariant v =   item->data(0,Qt::UserRole);
                Scan*  scan =  v.value<Scan*>();

                if (scan->mslevel == 1){
                    mainwindow->getSpectraWidget()->setScan(scan);
                } else if (scan->mslevel == 2){
                    mainwindow->fragmentationSpectraWidget->setScan(scan);

                    //Issue 550: option to update compound widget
                    if (isUpdateMassCalcGUI) mainwindow->massCalcWidget->setFragmentationScan(scan);

                } else if (scan->mslevel == 3) {
                    mainwindow->ms3SpectraWidget->setScan(scan);
                }

                mainwindow->getEicWidget()->clearEICLines();
                mainwindow->getEicWidget()->setFocusLine(scan->rt);

            } else if (treeWidget->selectedItems().size() > 1) {

                Fragment *f = nullptr;
                for (QTreeWidgetItem *item : treeWidget->selectedItems()){

                    QVariant v =   item->data(0,Qt::UserRole);
                    Scan*  scan =  v.value<Scan*>();

                    rts.insert(scan->rt);

                    if (f) {
                        Fragment *brother = new Fragment(scan,
                                                         minFracIntensity,
                                                         minSNRatio,
                                                         maxNumberOfFragments,
                                                         baseLinePercentile,
                                                         isRetainFragmentsAbovePrecursorMz,
                                                         precursorPurityPpm,
                                                         minIntensity);
                        f->addFragment(brother);
                    } else {
                        f = new Fragment(scan,
                                         minFracIntensity,
                                         minSNRatio,
                                         maxNumberOfFragments,
                                         baseLinePercentile,
                                         isRetainFragmentsAbovePrecursorMz,
                                         precursorPurityPpm,
                                         minIntensity);
                        mslevel = scan->mslevel;
                    }

                }

                if (f) {
                    f->buildConsensus(productPpmTolr,
                                      consensusIntensityAgglomerationType,
                                      isIntensityAvgByObserved,
                                      isNormalizeIntensityArray,
                                      minNumScansForConsensus,
                                      minFractionScansForConsensus);

                    //Issue 550: SpectraWidget::setCurrentFragment() expects Fragment* sorted by mz
                    f->consensus->sortByMz();

                    if (mslevel == 1){
                        mainwindow->getSpectraWidget()->setCurrentFragment(f->consensus, mslevel);
                    } else if (mslevel == 2){
                        mainwindow->fragmentationSpectraWidget->setCurrentFragment(f->consensus, mslevel);

                        //Issue 550: option to update compound widget
                        if (isUpdateMassCalcGUI) mainwindow->massCalcWidget->setFragment(f->consensus);

                    } else if (mslevel == 3) {
                        mainwindow->ms3SpectraWidget->setCurrentFragment(f->consensus, mslevel);
                    }

                    vector<float> rtsVector;
                    rtsVector.assign(rts.begin(), rts.end());
                    mainwindow->getEicWidget()->clearEICLines();
                    mainwindow->getEicWidget()->setFocusLines(rtsVector);

                    //Issue 501: Introduce memory leak to avoid dangling pointer bugs
                    //delete(f);
                }

            }

        } else if (this->exclusiveItemType == ScanVectorType) {

            vector<Scan*> allScans{};
            for (QTreeWidgetItem *item : treeWidget->selectedItems()){

                QVariant v =   item->data(0,Qt::UserRole);
                vector<Scan*> scanVector = v.value<vector<Scan*>>();

                allScans.insert(allScans.end(), scanVector.begin(), scanVector.end());
            }

            Fragment *f = nullptr;
            for (auto scan : allScans){

                rts.insert(scan->rt);

                if (f) {
                    Fragment *brother = new Fragment(scan,
                                                     minFracIntensity,
                                                     minSNRatio,
                                                     maxNumberOfFragments,
                                                     baseLinePercentile,
                                                     isRetainFragmentsAbovePrecursorMz,
                                                     precursorPurityPpm,
                                                     minIntensity);
                    f->addFragment(brother);
                } else {
                    f = new Fragment(scan,
                                     minFracIntensity,
                                     minSNRatio,
                                     maxNumberOfFragments,
                                     baseLinePercentile,
                                     isRetainFragmentsAbovePrecursorMz,
                                     precursorPurityPpm,
                                     minIntensity);
                    mslevel = scan->mslevel;
                }

            }

            if (f) {
                f->buildConsensus(productPpmTolr,
                                  consensusIntensityAgglomerationType,
                                  isIntensityAvgByObserved,
                                  isNormalizeIntensityArray,
                                  minNumScansForConsensus,
                                  minFractionScansForConsensus);

                //Issue 550: SpectraWidget::setCurrentFragment() expects Fragment* sorted by mz
                f->consensus->sortByMz();

                if (mslevel == 2){
                    mainwindow->fragmentationSpectraWidget->setCurrentFragment(f->consensus, mslevel);

                    //Issue 550: option to update compound widget
                    if (isUpdateMassCalcGUI) mainwindow->massCalcWidget->setFragment(f->consensus);
                }

                vector<float> rtsVector;
                rtsVector.assign(rts.begin(), rts.end());
                mainwindow->getEicWidget()->clearEICLines();
                mainwindow->getEicWidget()->setFocusLines(rtsVector);

                //Issue 501: Introduce memory leak to avoid dangling pointer bugs
                //delete(f);
            }

        } else {
            foreach(QTreeWidgetItem* item, treeWidget->selectedItems() ) {
                            QString text = item->text(0);
                            QVariant v =   item->data(0,Qt::UserRole);
                            int itemType = item->type();

                            if ( itemType == CompoundType ) {
                                    Compound* c =  v.value<Compound*>();
                                    if (c) mainwindow->setCompoundFocus(c);
                            } else if ( itemType == PeakGroupType ) {
                                    PeakGroup* group =  v.value<PeakGroup*>();
                                    if (group) mainwindow->setPeakGroup(group);
                            } else if ( itemType == ScanType ) {
                                    Scan*  scan =  v.value<Scan*>();
                                    if (scan->mslevel == 2)  {
                                        mainwindow->fragmentationSpectraWidget->setScan(scan);
                                        mainwindow->massCalcWidget->setFragmentationScan(scan);

                                        Scan* lastms1 = scan->getLastFullScan(50);
                                        if(lastms1) {
                                            mainwindow->getSpectraWidget()->setScan(lastms1);
                                            mainwindow->getSpectraWidget()->zoomRegion(scan->precursorMz,2);
                                        }
                                        mainwindow->getEicWidget()->setFocusLine(scan->rt);
                                    } else if (scan->mslevel == 1) {
                                        mainwindow->getSpectraWidget()->setScan(scan);
                                        mainwindow->getEicWidget()->setFocusLine(scan->rt);
                                    }
                            } else if (itemType == EICType ) {
                                    mainwindow->getEicWidget()->setSrmId(text.toStdString());
                            } else if (itemType == mzSliceType ) {
                                     mzSlice slice =  v.value<mzSlice>();
                                     mainwindow->getEicWidget()->setMzSlice(slice, false);
                                     mainwindow->getEicWidget()->resetZoom();
                                     qDebug() << "TreeDockWidget::showInfo() mzSlice: " << slice.srmId.c_str();
                            } else if (itemType == mzLinkType ) {
                                     if (text.toDouble()) { mainwindow->getEicWidget()->setMzSlice(text.toDouble());  }
                            } else if (this->exclusiveItemType == SRMTransitionType) { //Issue 347
                                     SRMTransition srmTransition = v.value<SRMTransition>();

                                     QString compoundName, adductName;

                                     if (srmTransition.compound) compoundName = QString(srmTransition.compound->name.c_str());
                                     if (srmTransition.adduct) adductName = QString(srmTransition.adduct->name.c_str());

                                     qDebug() << "TreeDockWidget::showInfo() srmTransition: "
                                              << "(" << srmTransition.precursorMz << ", " << srmTransition.productMz << ") "
                                              << compoundName << " " << adductName;

                                    mainwindow->getEicWidget()->setSRMTransition(srmTransition);
                                    mainwindow->getEicWidget()->resetZoom(true);

                            } else {
                                    cerr << "UNKNOWN TYPE=" << v.type() << endl;
                            }
            }
        }
}

QTreeWidgetItem* TreeDockWidget::addPeak(Peak* peak, QTreeWidgetItem* parent) {
    if (peak == NULL ) return NULL;
	QTreeWidgetItem *item=NULL;
    if ( parent == NULL ) {
	    item = new QTreeWidgetItem(treeWidget,PeakType);
    } else {
	    item = new QTreeWidgetItem(parent,PeakType);
    }
    return item;
}

void TreeDockWidget::keyPressEvent(QKeyEvent *e ) {
	QTreeWidgetItem *item = treeWidget->currentItem();
	if (e->key() == Qt::Key_Delete ) { 
    	if ( item->type() == PeakGroupType ) {
			unlinkGroup(); 
		} else if ( item->type() == CompoundType  ) {
			unlinkGroup();
		}
	}
}
	
void TreeDockWidget::unlinkGroup() {
	QTreeWidgetItem *item = treeWidget->currentItem();
	if ( item == NULL ) return;
	PeakGroup* group=NULL;
	Compound*  cpd=NULL;

    if ( item->type() == PeakGroupType ) {
		QVariant v = item->data(0,Qt::UserRole);
   	 	group =  v.value<PeakGroup*>();
	}  else if (item->type() == CompoundType ) {
		QVariant v = item->data(0,Qt::UserRole);
   		cpd =  v.value<Compound*>();
	} else {
		return;
	}

    MainWindow* mainwindow = (MainWindow*)parentWidget();

	if (cpd) { 
		cpd->unlinkGroup(); 
    	mainwindow->setCompoundFocus(cpd);
	} else if (group && group->parent) {
		PeakGroup* parentGroup = group->parent;
		if ( parentGroup->deleteChild(group) ) {
			QTreeWidgetItem* parentItem = item->parent();
			if ( parentItem ) { parentItem->removeChild(item); delete(item); }
			mainwindow->getEicWidget()->setPeakGroup(parentGroup);
		}
	} else if (group) {
		for(int i=0; i < treeWidget->topLevelItemCount(); i++ ) {
			if ( treeWidget->topLevelItem(i) == item ) {
					item->setHidden(true);
					treeWidget->removeItemWidget(item,0); delete(item);
					group->deletePeaks();
					group->deleteChildren();
					if (group->hasCompoundLink() && group->getCompound()->getPeakGroup() == group ) {
							group->getCompound()->unlinkGroup();
							cerr << "Unlinking compound" << group << endl;
					}
					break;

			}
		}
	}

	treeWidget->update();
	return;
}



QTreeWidgetItem* TreeDockWidget::addSlice(mzSlice* s, QTreeWidgetItem* parent) {
        QTreeWidgetItem *item = new QTreeWidgetItem(treeWidget,mzSliceType);
        item->setText(0,QString::number(s->mzmin,'f',4));
        item->setText(1,QString::number(s->mzmax,'f',4));
        item->setText(2,QString::number(s->rtmin,'f',4));
        item->setText(3,QString::number(s->rtmax,'f',4));
        item->setText(4,QString::number(s->ionCount,'f',4));
        return item;
}

void TreeDockWidget::setInfo(vector<mzLink>& links) {
    treeWidget->clear();
    if (links.size() == 0 ) return;

    QStringList header; header << "m/z" <<  "correlation" << "note";
    treeWidget->clear();
    treeWidget->setHeaderLabels( header );
    treeWidget->setSortingEnabled(false);

    QTreeWidgetItem *item0 = new QTreeWidgetItem(treeWidget,mzLinkType);
    item0->setText(0,QString::number(links[0].mz1,'f',4));
    item0->setExpanded(true);
    for(int i=0; i < links.size(); i++) addLink(&links[i],item0);

    treeWidget->setSortingEnabled(true);
    treeWidget->sortByColumn(1,Qt::DescendingOrder);
}

QTreeWidgetItem* TreeDockWidget::addLink(mzLink* s,QTreeWidgetItem* parent)  {

        if (!s) return NULL;

        QTreeWidgetItem *item=NULL;
    if( parent == NULL ){
            item = new QTreeWidgetItem(treeWidget,mzLinkType);
        } else {
            item = new QTreeWidgetItem(parent,mzLinkType);
        }

        if ( item ) {
                item->setText(0,QString::number(s->mz2,'f',4));
                item->setText(1,QString::number(s->correlation,'f',2));
                item->setText(2,QString(s->note.c_str()));
        }
        return item;
}


void TreeDockWidget::filterTree(QString needle) {
        int itemCount = treeWidget->topLevelItemCount();
        for(int i=0; i < itemCount; i++ ) {
                QTreeWidgetItem *item = treeWidget->topLevelItem(i);
                if (!item) continue;
                if ( needle.isEmpty() || item->text(0).contains(needle,Qt::CaseInsensitive) ) {
                        item->setHidden(false);
                } else {
                        item->setHidden(true);
                }
        }
}

void TreeDockWidget::setupScanListHeader() {
    QStringList colNames;
    colNames << "pre m/z" << "rt" << "purity" << "CE" << "TIC" << "#peaks" << "scannum" << "sample";
    treeWidget->setColumnCount(colNames.size());
    treeWidget->setHeaderLabels(colNames);
    treeWidget->setSortingEnabled(true);
    treeWidget->setHeaderHidden(false);
    treeWidget->sortByColumn(1, Qt::AscendingOrder);

}

void TreeDockWidget::setupConsensusScanListHeader() {
    QStringList colNames;
    colNames << "sample" << "pre m/z" << "# scans";
    treeWidget->setColumnCount(colNames.size());
    treeWidget->setHeaderLabels(colNames);
    treeWidget->setSortingEnabled(true);
    treeWidget->setHeaderHidden(false);
}

void TreeDockWidget::setupSRMTransitionListHeader() {
    QStringList colNames;
    colNames << "Precursor m/z" << "Product m/z" << "RT" << "Compound" << "Adduct" << "# scans" << "# samples";
    treeWidget->setColumnCount(colNames.size());
    treeWidget->setHeaderLabels(colNames);
    treeWidget->setSortingEnabled(true);
    treeWidget->setHeaderHidden(false);
}

void TreeDockWidget::addScanItem(Scan* scan) {
        if (!scan) return;

        MainWindow* mainwindow = (MainWindow*)parentWidget();
        QIcon icon = mainwindow->projectDockWidget->getSampleIcon(scan->sample);

        NumericTreeWidgetItem *item = new NumericTreeWidgetItem(treeWidget,ScanType);
        item->setData(0,Qt::UserRole,QVariant::fromValue(scan));
        item->setIcon(0,icon);

        item->setText(0,QString::number(static_cast<double>(scan->precursorMz),'f',4));
        item->setText(1,QString::number(static_cast<double>(scan->rt)));
        item->setText(2,QString::number(scan->getPrecursorPurity(20.00),'g',3));
        item->setText(3,QString::number(static_cast<double>(scan->collisionEnergy),'f',0));
        item->setText(4,QString::number(scan->totalIntensity(),'g',3));
        item->setText(5,QString::number(scan->nobs()));
        item->setText(6,QString::number(scan->scannum));
        item->setText(7,QString(scan->sample->sampleName.c_str()));
        item->setText(8,QString(scan->filterLine.c_str()));
}

void TreeDockWidget::addMs2ScanVectorItem(vector<Scan*> scanVector) {

    if (scanVector.empty()) return;

    Scan *scan = scanVector[0];

    MainWindow* mainwindow = (MainWindow*)parentWidget();
    QIcon icon = mainwindow->projectDockWidget->getSampleIcon(scan->sample);

    NumericTreeWidgetItem *item = new NumericTreeWidgetItem(treeWidget, ScanVectorType);
    item->setData(0,Qt::UserRole,QVariant::fromValue(scanVector));
    item->setIcon(0,icon);

    item->setText(0,QString(scan->sample->sampleName.c_str()));
    item->setText(1,QString::number(scan->precursorMz,'f',4));
    item->setText(2,QString::number(scanVector.size()));
}

void TreeDockWidget::setupMs1ScanHeader() {
    QStringList colNames;
    colNames << "sample" << "filter string" << "scannum" <<"mzmin" << "mzmax" << "rt" << "#peaks";
    treeWidget->setColumnCount(colNames.size());
    treeWidget->setHeaderLabels(colNames);
    treeWidget->setSortingEnabled(true);
    treeWidget->setHeaderHidden(false);
}

void TreeDockWidget::addMs1ScanItem(Scan* scan) {
    if (!scan) return;

    MainWindow* mainwindow = (MainWindow*)parentWidget();
    QIcon icon = mainwindow->projectDockWidget->getSampleIcon(scan->sample);

    NumericTreeWidgetItem *item = new NumericTreeWidgetItem(treeWidget,ScanType);
    item->setData(0,Qt::UserRole,QVariant::fromValue(scan));
    item->setIcon(0,icon);

    item->setText(0,QString(scan->sample->sampleName.c_str()));
    item->setText(1,QString(scan->filterString.c_str()));
    item->setText(2,QString::number(scan->scannum));
    item->setText(3,QString::number(scan->minMz()));
    item->setText(4,QString::number(scan->maxMz()));
    item->setText(5,QString::number(scan->rt));
    item->setText(6,QString::number(scan->nobs()));
}

void TreeDockWidget::setupMs3ScanHeader() {
    QStringList colNames;
    colNames << "sample" << "Ms1PreMz" << "Ms2PreMz" << "scannum" << "rt" << "z" << "TIC" << "#peaks";
    treeWidget->setColumnCount(colNames.size());
    treeWidget->setHeaderLabels(colNames);
    treeWidget->setSortingEnabled(true);
    treeWidget->setHeaderHidden(false);
}

void TreeDockWidget::addMs3ScanItem(Scan *scan) {
    if (!scan) return;

    MainWindow* mainwindow = (MainWindow*)parentWidget();
    QIcon icon = mainwindow->projectDockWidget->getSampleIcon(scan->sample);

    NumericTreeWidgetItem *item = new NumericTreeWidgetItem(treeWidget,ScanType);
    item->setData(0,Qt::UserRole,QVariant::fromValue(scan));
    item->setIcon(0,icon);

    item->setText(0,QString(scan->sample->sampleName.c_str()));
    item->setText(1, QString::number(scan->ms1PrecursorForMs3,'f',4));
    item->setText(2, QString::number(scan->precursorMz, 'f', 4));
    item->setText(3, QString::number(scan->scannum));
    item->setText(4, QString::number(scan->rt, 'f', 2));
    item->setText(5, QString::number(scan->precursorCharge));
    item->setText(6, QString::number(scan->totalIntensity(), 'f', 2));
    item->setText(7, QString::number(scan->nobs()));
}

void TreeDockWidget::setInfo(vector<Compound*>&compounds) {
    clearTree();
        for(int i=0; i < compounds.size(); i++ ) addCompound(compounds[i],NULL);
}

void TreeDockWidget::setInfo(double ms1PrecMz, double ms2PrecMz){

    ms1PrecMzSpn->setValue(ms1PrecMz);
    ms2PrecMzSpn->setValue(ms2PrecMz);

    if (treeWidget->topLevelItemCount() > 0) {
       treeWidget->topLevelItem(0)->setSelected(true);
    }
}

void TreeDockWidget::setInfo(Compound* x)  {
        addCompound(x,NULL);
}

void TreeDockWidget::setInfo(PeakGroup* group) {
        if (group == NULL) return;
    if (hasPeakGroup(group)) return;
    clearTree();
    addPeakGroup(group,NULL);
}


bool TreeDockWidget::hasPeakGroup(PeakGroup* group) {
    if (treeWidget == nullptr) return true;
    for(int i=0; i < treeWidget->topLevelItemCount();i++ ) {

        QTreeWidgetItem* item = treeWidget->topLevelItem(i);
        if ( item->type() != PeakGroupType ) continue;
        QVariant v = item->data(0,Qt::UserRole);
        PeakGroup*  g = v.value<PeakGroup*>();

        if (g && group && *g == *group ) return true;

        for(int j=0; j < item->childCount();j++ ) {
            QTreeWidgetItem* item2 = item->child(j);
            if ( item2->type() != PeakGroupType ) continue;
            QVariant v = item2->data(0,Qt::UserRole);
            PeakGroup*  g = v.value<PeakGroup*>();
            if (g && group && *g == *group ) return true;
        }
    }
    return false;
}

QTreeWidgetItem* TreeDockWidget::addCompound(Compound* c, QTreeWidgetItem* parent) {
    if (c == NULL) return NULL;
    QTreeWidgetItem* item = NULL;

    if ( parent == NULL ) {
            item = new QTreeWidgetItem(treeWidget, CompoundType);
    } else {
            item = new QTreeWidgetItem(parent, CompoundType);
    }

        if (!item) return NULL;

        QString id = QString(c->name.c_str());
        item->setText(0,id);
    item->setData(0, Qt::UserRole,QVariant::fromValue(c));

        if ( c->hasGroup() ){
                        PeakGroup* group = c->getPeakGroup();
                        if (group != NULL) addPeakGroup(group,item);
        }
    return item;
}


QTreeWidgetItem* TreeDockWidget::addPeakGroup(PeakGroup* group, QTreeWidgetItem* parent) {
    if (group == NULL) return NULL;
    QTreeWidgetItem* item = NULL;

    if ( parent == NULL ) {
            item = new QTreeWidgetItem(treeWidget, PeakGroupType);
    } else {
            item = new QTreeWidgetItem(parent, PeakGroupType);
    }

        if (!item) return NULL;

    QString id(group->getName().c_str());
    item->setText(0,id);
    item->setExpanded(true);
    item->setData(0, Qt::UserRole,QVariant::fromValue(group));
    //for(int i=0; i < group->peaks.size(); i++ ) addPeak(&(group->peaks[i]), item);
    for(int i=0; i < group->childCount(); i++ ) addPeakGroup(&(group->children[i]), item);
    return item;
}

void TreeDockWidget::contextMenuEvent ( QContextMenuEvent * event ) 
{
    QMenu menu;

    QAction* z0 = menu.addAction("Copy to Clipboard");
    connect(z0, SIGNAL(triggered()), this ,SLOT(copyToClipbard()));

    menu.exec(event->globalPos());
}


void TreeDockWidget::copyToClipbard() { 
        qDebug() << "copyToClipbard";
        int itemCount = treeWidget->topLevelItemCount();

        QString clipboardtext;
        for(int i=0; i < itemCount; i++ ) {
                QTreeWidgetItem *item = treeWidget->topLevelItem(i);
                if ( item == NULL) continue;
                itemToClipboard(item,clipboardtext);
        }

		QClipboard *clipboard = QApplication::clipboard();
		clipboard->setText( clipboardtext );
}

void TreeDockWidget::itemToClipboard(QTreeWidgetItem* item, QString& clipboardtext) { 
        for(int j=0; j< item->columnCount();j++ ) {
		    clipboardtext += item->text(j) + "\t";
        }
        clipboardtext += "\n";

        for(int j=0; j< item->childCount(); j++ ) {
            itemToClipboard(item->child(j), clipboardtext);
        }
}           


