#include "tabledockwidget.h"

TableDockWidget::TableDockWidget(MainWindow* mw, QString title, int numColms, QString encodedTableInfo, QString displayTableInfo) {
    this->title = title;
    setAllowedAreas(Qt::AllDockWidgetAreas);
    setFloating(false);
    _mainwindow = mw;

    setObjectName(title);
    setWindowTitle(title);
    setAcceptDrops(true);

    if (isTargetedMs3Table() || isDirectInfusionTable()) {
        directInfusionSearchParams = DirectInfusionSearchParameters::decode(encodedTableInfo.toStdString());
    }

    if (encodedTableInfo.contains("isUseGroupMaxPeakVals=1")) {
        this->isUseGroupMaxPeakVals = true;
    }

    numColms=11;
    viewType = groupView;

    treeWidget=new QTreeWidget(this);
    treeWidget->setSortingEnabled(false);
    treeWidget->setColumnCount(numColms);
    treeWidget->setDragDropMode(QAbstractItemView::DragOnly);
    treeWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    treeWidget->setAcceptDrops(false);    
    treeWidget->setObjectName("PeakGroupTable");
    treeWidget->setSelectionBehavior(QAbstractItemView::SelectRows);

    //Issue 253: Need to use this signal instead of currentItemChanged() or itemClicked()
    //because the current item is not the same as the selected item(s). The current item is
    //determined before selected items, leading to strange GUI behavior
    connect(treeWidget, SIGNAL(itemSelectionChanged()), SLOT(showSelectedGroup()));

    //Issue 271: once compounds have been deleted, remove any references to them in this peaks table
    connect(mw->libraryDialog, SIGNAL(unloadLibrarySignal(QString)), this, SLOT(disconnectCompounds(QString)));
    connect(mw->libraryDialog, SIGNAL(loadLibrarySignal(QString)), this, SLOT(reconnectCompounds(QString)));

    //Issue 285: Update quant values when quant typecombo box changes
    if (_mainwindow->quantType) {
         connect(_mainwindow->quantType, SIGNAL(currentIndexChanged(int)), SLOT(refreshPeakGroupQuant()));
    }

    setupPeakTable();

    traindialog = new TrainDialog(this);
    connect(traindialog->saveButton,SIGNAL(clicked(bool)),SLOT(saveModel()));
    connect(traindialog->trainButton,SIGNAL(clicked(bool)),SLOT(Train()));

    clusterDialog = new ClusterDialog(this);
    clusterDialog->setWindowFlags( clusterDialog->windowFlags() | Qt::WindowStaysOnTopHint);
    connect(clusterDialog->clusterButton,SIGNAL(clicked(bool)),SLOT(clusterGroups()));
    connect(clusterDialog->clearButton,SIGNAL(clicked(bool)),SLOT(clearClusters()));
    connect(clusterDialog->chkPGDisplay, SIGNAL(clicked(bool)), SLOT(changePeakGroupDisplay()));

    this->encodedTableInfo = encodedTableInfo;
    this->displayTableInfo = displayTableInfo;

    searchParamsDialog = new SearchParamsDialog(this);
    searchParamsDialog->setWindowTitle(title);
    searchParamsDialog->setWindowFlags(searchParamsDialog->windowFlags() | Qt::WindowStaysOnTopHint);
    searchParamsDialog->txtSrchParams->setText(this->displayTableInfo);
    if (title == "Bookmarks"){
        searchParamsDialog->txtSrchResults->setText(this->displayTableInfo);
    }

    filterTagsDialog = new FilterTagsDialog(this);
    connect(filterTagsDialog, SIGNAL(updateFilter()), this, SLOT(updateTagFilter()));

    editPeakGroupDialog = new EditPeakGroupDialog(this, _mainwindow);
    connect(editPeakGroupDialog->okButton, SIGNAL(clicked(bool)), SLOT(updateSelectedPeakGroup()));
    connect(editPeakGroupDialog->cancelButton, SIGNAL(clicked(bool)), SLOT(hideEditPeakGroupDialog()));

    isotopeExportSettingsDialog = new IsotopeExportSettingsDialog(this);
    connect(isotopeExportSettingsDialog->boxBtn->button(QDialogButtonBox::Ok), SIGNAL(clicked(bool)), SLOT(exportGroupsAsIsotopes()));
    connect(isotopeExportSettingsDialog->boxBtn->button(QDialogButtonBox::Cancel), SIGNAL(clicked(bool)), SLOT(hideIsotopeExportSettings()));

    QToolBar *toolBar = new QToolBar(this);
    toolBar->setFloatable(false);
    toolBar->setMovable(false);

    QToolButton *btnSwitchView = new QToolButton(toolBar);
    btnSwitchView->setIcon(QIcon(rsrcPath + "/flip.png"));
    btnSwitchView->setToolTip("Switch between Group and Peak Views");
    connect(btnSwitchView,SIGNAL(clicked()),SLOT(switchTableView()));

    QToolButton *btnGroupCSV = new QToolButton(toolBar);

    btnGroupCSV->setIcon(QIcon(rsrcPath + "/exportcsv.png"));
    btnGroupCSV->setToolTip(tr("Export Groups To SpreadSheet (.csv) "));
    btnGroupCSV->setMenu(new QMenu("Export Groups"));
    btnGroupCSV->setPopupMode(QToolButton::InstantPopup);
    QAction* exportSelected = btnGroupCSV->menu()->addAction(tr("Export Selected"));
    QAction* exportAll = btnGroupCSV->menu()->addAction(tr("Export All Groups"));
    QAction* exportIsotopes = btnGroupCSV->menu()->addAction(tr("Isotopes Export"));

    connect(exportSelected, SIGNAL(triggered()), SLOT(exportGroupsToSpreadsheet()));
    connect(exportAll, SIGNAL(triggered()), treeWidget, SLOT(selectAll()));
    connect(exportAll, SIGNAL(triggered()), SLOT(exportGroupsToSpreadsheet()));
    connect(exportIsotopes, SIGNAL(triggered()), SLOT(showIsotopeExportSettings()));

    QToolButton *btnGallery = new QToolButton(toolBar);
    btnGallery->setIcon(QIcon(rsrcPath + "/gallery.png"));
    btnGallery->setToolTip("Show Gallery");
    connect(btnGallery, SIGNAL(clicked()), SLOT(showGallery()));

    QToolButton *btnScatter = new QToolButton(toolBar);
    btnScatter->setIcon(QIcon(rsrcPath + "/scatterplot.png"));
    btnScatter->setToolTip("Show ScatterPlot");
    connect(btnScatter, SIGNAL(clicked()), SLOT(showScatterPlot()));

    QToolButton *btnCluster = new QToolButton(toolBar);
    btnCluster->setIcon(QIcon(rsrcPath + "/cluster.png"));
    btnCluster->setToolTip("Cluster Groups");
    connect(btnCluster, SIGNAL(clicked()),clusterDialog,SLOT(show()));

    QToolButton *btnSettings = new QToolButton(toolBar);
    btnSettings->setIcon(QIcon(rsrcPath + "/settings.png"));
    btnSettings->setToolTip("Show Search Settings");
    connect(btnSettings, SIGNAL(clicked()), searchParamsDialog, SLOT(show()));

    /*
    QToolButton *btnXML = new QToolButton(toolBar);
    btnXML->setIcon(QIcon(rsrcPath + "/exportxml.png"));
    btnXML->setToolTip("Save Peaks");
    connect(btnXML, SIGNAL(clicked()), SLOT(savePeakTable()));

    QToolButton *btnLoad = new QToolButton(toolBar);
    btnLoad->setIcon(QIcon(rsrcPath + "/fileopen.png"));
    btnLoad->setToolTip("Load Peaks");
    connect(btnLoad, SIGNAL(clicked()), SLOT(loadPeakTable()));
    */

    QToolButton *btnGood = new QToolButton(toolBar);
    btnGood->setIcon(QIcon(rsrcPath + "/markgood.png"));
    btnGood->setToolTip("Mark Group as Good");
    connect(btnGood, SIGNAL(clicked()), SLOT(markGroupGood()));

    QToolButton *btnBad = new QToolButton(toolBar);
    btnBad->setIcon(QIcon(rsrcPath + "/markbad.png"));
    btnBad->setToolTip("Mark Group as Bad");
    connect(btnBad, SIGNAL(clicked()), SLOT(markGroupBad()));

    QToolButton *btnHeatmapelete = new QToolButton(toolBar);
    btnHeatmapelete->setIcon(QIcon(rsrcPath + "/delete.png"));
    btnHeatmapelete->setToolTip("Delete Group");
    connect(btnHeatmapelete, SIGNAL(clicked()), this, SLOT(deleteSelected()));

    QToolButton *btnPDF = new QToolButton(toolBar);
    btnPDF->setIcon(QIcon(rsrcPath + "/PDF.png"));
    btnPDF->setToolTip("Generate PDF Report");
    connect(btnPDF, SIGNAL(clicked()), this, SLOT(printPdfReport()));

//    QToolButton *btnRunScript = new QToolButton(toolBar);
//    btnRunScript->setIcon(QIcon(rsrcPath + "/R.png"));
//    btnRunScript->setToolTip("Run Script");
//    connect(btnRunScript, SIGNAL(clicked()), SLOT(runScript()));

    /*
    QToolButton *btnMoveTo = new QToolButton(toolBar);
    btnMoveTo->setMenu(new QMenu("Move To"));
    btnMoveTo->setIcon(QIcon(rsrcPath + "/delete.png"));
    btnMoveTo->setPopupMode(QToolButton::InstantPopup);
    btnMoveTo->menu()->addAction(tr("BookMarks Table"));
    btnMoveTo->menu()->addAction(tr("Table X"));
    btnMoveTo->menu()->addAction(tr("Table Y"));
*/

    btnTagsFilter = new QToolButton(toolBar);
    btnTagsFilter->setIcon(QIcon(rsrcPath + "/icon_filter.png"));
    btnTagsFilter->setToolTip("Filter peak groups based on tags");
    connect(btnTagsFilter, SIGNAL(clicked()), filterTagsDialog, SLOT(show()));

//    QToolButton *btnX = new QToolButton(toolBar);
//    btnX->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));
//    //btnX->setIcon(QIcon(rsrcPath + "/hide.png"));
//    connect(btnX, SIGNAL(clicked()),this,SLOT(deleteAll()));

    filterEditor = new QLineEdit(toolBar);
    filterEditor->setMinimumWidth(15);
    filterEditor->setPlaceholderText("Filter");
    filterEditor->setToolTip("Filter peak groups based on ID string matches");
    connect(filterEditor, SIGNAL(textEdited(QString)), this, SLOT(filterTree(QString)));

    QWidget* spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
    toolBar->addWidget(btnSwitchView);
    toolBar->addWidget(btnGood);
    toolBar->addWidget(btnBad);
    toolBar->addWidget(btnSettings);
    toolBar->addWidget(btnHeatmapelete);
    toolBar->addSeparator();

    //toolBar->addWidget(btnHeatmap);
    toolBar->addWidget(btnScatter);
    toolBar->addWidget(btnGallery);
    toolBar->addWidget(btnCluster);

    toolBar->addSeparator();
    toolBar->addWidget(btnPDF);
    toolBar->addWidget(btnGroupCSV);
    //toolBar->addWidget(btnRunScript);

    toolBar->addSeparator();
    /*
    toolBar->addWidget(btnXML);
    toolBar->addWidget(btnLoad);
    */

   // toolBar->addWidget(btnMoveTo);
  //  toolBar->addWidget(spacer);
    toolBar->addWidget(btnTagsFilter);
    toolBar->addWidget(filterEditor);
    //toolBar->addWidget(btnX);

    ///LAYOUT
    setTitleBarWidget(toolBar);
    QWidget *content = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->setContentsMargins(QMargins(0,0,0,0));
    layout->addWidget(toolBar);
    layout->addWidget(treeWidget);
    content->setLayout(layout);
    setWidget(content);
}

TableDockWidget::~TableDockWidget() { 
    if(traindialog != NULL) delete traindialog;
}

void TableDockWidget::setupPeakTable() {

    QStringList colNames;

    if (viewType == groupView) {

        map<string, int> colNamesMap;

        if (_isShowLipidSummarizationColumns) {
            colNamesMap = groupViewColumnNameToNumberWithLipidSummarization;
        } else {
            colNamesMap = groupViewColumnNameToNumber;
        }

        vector<string> groupViewColNames(colNamesMap.size());
        for (auto it = colNamesMap.begin(); it != colNamesMap.end(); ++it){
            groupViewColNames.at(static_cast<unsigned long>(it->second)) = it->first;
        }

        for (auto colName : groupViewColNames){
            colNames << QString(colName.c_str());
        }

    } else if (viewType == peakView) {

        colNames << "ID";
        colNames << "Compound";
        colNames << "Adduct";
        colNames << "m/z";
        colNames << "RT";

        vector<mzSample*> vsamples = _mainwindow->getVisibleSamples();
        sort(vsamples.begin(), vsamples.end(), mzSample::compSampleOrder);
        for(unsigned int i=0; i<vsamples.size(); i++ ) {
            colNames << QString(vsamples[i]->sampleName.c_str());
        }
    }

    treeWidget->setColumnCount(colNames.size());
    treeWidget->setHeaderLabels(colNames);
    //treeWidget->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    //treeWidget->header()->adjustSize();
    treeWidget->setSortingEnabled(true);

}

void TableDockWidget::updateTable() {
    QTreeWidgetItemIterator it(treeWidget);
    while (*it) {
        updateItem(*it);
        ++it;
    }
    updateIcons();
    updateStatus();
}

void TableDockWidget::updateItem(QTreeWidgetItem* item) {

    //assume that all possible icons that will be combined are squares
    const int SINGLE_ICON_WIDTH = 12;
    const int SINGLE_ICON_HEIGHT = 12;
    const int ICON_SPACE_OFFSET = 1;

    QVariant v = item->data(0,PeakGroupType);
    PeakGroup*  group =  v.value<PeakGroup*>();

    if (!group) return;

    QString adductText;
    if (group->adduct) {
        adductText = QString(group->adduct->name.c_str());
    }

    item->setText(getGroupViewColumnNumber("ID"), groupTagString(group));             // ID
    item->setText(getGroupViewColumnNumber("Adduct"), adductText);                    // Adduct
    heatmapBackground(item);
    duplicateEntryBackground(item);

    //score peak quality
    Classifier* clsf = _mainwindow->getClassifier();
    if (clsf) {
        if (!isTargetedMs3Table() && !isDirectInfusionTable()) {
            clsf->classify(group);
            group->updateQuality();
        }
    }

    // Issue 808: Check cache
    QString iconKey = QString();
    for (char c : group->labels) {
        iconKey += QString(c);
    }

    if (!_iconCache.contains(iconKey)) {

        qDebug() << "iconKey: " << iconKey;

        //Issue 127: gather all icons
        vector<QIcon> icons;
        for (char c : group->labels) {

            if (c == 'g') {
                icons.push_back(goodIcon);
            } else if (c == 'b') {
                icons.push_back(badIcon);
            } else if (c == PeakGroup::ReservedLabel::COMPOUND_MANUALLY_CHANGED) {
                icons.push_back(reassignedIcon);
            } else if (DB.peakGroupTags.find(c) != DB.peakGroupTags.end()) {
                icons.push_back(DB.peakGroupTags[c]->icon);
            }

        }

        if (icons.empty()) {
            _iconCache.insert(iconKey, QIcon());
            //item->setIcon(0, QIcon());
        } else if (icons.size() == 1) {
            _iconCache.insert(iconKey, icons.at(0));
            // item->setIcon(0, icons.at(0));
        } else {

            int N = icons.size();
            int imgWidth = N * SINGLE_ICON_WIDTH + (N-1) * ICON_SPACE_OFFSET;
            int imgHeight = SINGLE_ICON_HEIGHT;

            QPixmap comboPixmap(imgWidth, imgHeight);
            QPainter painter(&comboPixmap);

            // Set a white background color
            // Icons background is a little bit dirty, this helps clean up the appearance
            painter.fillRect(0, 0, imgWidth, imgHeight, Qt::white);

            int leftEdge = 0;
            for (const QIcon &icon : icons) {
                // Get the pixmap at the fixed size. QIcon will scale it if necessary.
                QPixmap pixmap = icon.pixmap(SINGLE_ICON_WIDTH, SINGLE_ICON_HEIGHT);

                // Draw at leftEdge. Vertical alignment is not needed since height is fixed.
                painter.drawPixmap(leftEdge, 0, pixmap);

                leftEdge += (SINGLE_ICON_WIDTH + ICON_SPACE_OFFSET);
            }

            QIcon icon;
            icon.addPixmap(comboPixmap);

            _iconCache.insert(iconKey, icon);
        }
    }

    QIcon icon = _iconCache.value(iconKey);
    item->setIcon(0, icon);
}


void TableDockWidget::heatmapBackground(QTreeWidgetItem* item) {
    if(viewType != peakView) return;

    int firstColumn=5;
    StatisticsVector<float>values;
    for(unsigned int i=firstColumn; i< item->columnCount(); i++) {
          values.push_back(item->text(i).toFloat());
    }

    if (values.size()) {
        //normalize
        float mean = values.mean();
        float sd  = values.stddev();

        for(int i=0; i<values.size();i++) {
            values[i] = (values[i]-mean)/sd; //Z-score
        }

        float maxValue=max(std::fabs(values.maximum()),fabs(values.minimum()));

        float colorramp=0.5;

        for(int i=0; i<values.size();i++) {
            float cellValue=values[i];
            QColor color = Qt::white;


            if (cellValue<0)  {
                float intensity=pow(abs(cellValue/maxValue),colorramp);
                if (intensity > 1 ) intensity=1;
                color.setHsvF(0.6,intensity,intensity,0.5);
            }

            if (cellValue>0 )  {
                float intensity=pow(abs(cellValue/maxValue),colorramp);
                if (intensity > 1 ) intensity=1;
                color.setHsvF(0.1,intensity,intensity,0.5);
            }
            //item->setText(firstColumn+i,QString::number(values[i])) ;
            item->setBackgroundColor(firstColumn+i,color);
       }
    }
}

//Issue 438
void TableDockWidget::duplicateEntryBackground(QTreeWidgetItem *item){

    QVariant v = item->data(0, PeakGroupType);
    PeakGroup *peakGroup = v.value<PeakGroup*>();

    if (peakGroup && peakGroup->compound && compoundToGroup.find(peakGroup->compound) != compoundToGroup.end()) {
        if (compoundToGroup[peakGroup->compound].size() > 1) {
            item->setBackgroundColor(getGroupViewColumnNumber("Compound"), QColor("moccasin"));
        } else {
            item->setBackgroundColor(getGroupViewColumnNumber("Compound"), Qt::white);
        }
    }

    QString groupTagString = item->text(getGroupViewColumnNumber("ID"));
    if (groupIdToGroup.find(groupTagString) != groupIdToGroup.end()) {
        if (groupIdToGroup[groupTagString].size() > 1) {
            item->setBackgroundColor(getGroupViewColumnNumber("ID"), QColor("coral"));
        } else {
            item->setBackgroundColor(getGroupViewColumnNumber("ID"), Qt::white);
        }
    }
}

QString TableDockWidget::groupTagString(PeakGroup* group){
    if (!group) return QString();

    if (!group->displayName.empty()) {
        return QString(group->displayName.c_str());
    }

    QStringList parts;

    if (group->compound) {

        if (!group->importedCompoundName.empty()) {
            parts << group->importedCompoundName.c_str();
        } else {
            parts << group->compound->name.c_str();
        }

        bool isAddedAdductName = false;
        if (group->adduct){
            parts << group->adduct->name.c_str();
            isAddedAdductName = true;
        }

        if (!group->tagString.empty()){
            if (isAddedAdductName && group->adduct->name != group->tagString) {
                parts << group->tagString.c_str(); //If the adduct name was already added, do not add again.
            }
        }

        //direct infusion display for single samples
        if (group->peaks.size() == 1 && group->peaks[0].mzmax - group->peaks[0].mzmin > 0.5) {
            parts << group->peaks[0].sample->sampleName.c_str();
        }

        if (!group->compound->db.empty()) {
            parts << group->compound->db.c_str();
        }

    } else if (!group->tagString.empty()) {
        parts << group->tagString.c_str();
    } else {
        parts << QString::number(group->meanMz,'f',3) + "@" + QString::number(group->meanRt,'f',2);
    }

    if(!group->srmId.empty()) parts << QString(group->srmId.c_str());
    return parts.join("|");
}

void TableDockWidget::addRow(PeakGroup* group, QTreeWidgetItem* root) {
    if(!group) return;

    //Issue 380: Avoid recomputing group data if already computed.
    group->groupStatistics(false);

    //cerr << "addRow" << group->groupId << " "  << group->meanMz << " " << group->meanRt << " " << group->children.size() << " " << group->tagString << endl;

    if (!group) return;

    if (group->compound && group->compound->name == "Arginine-13C15N_IS") {
        qDebug() << group->peakCount() << " " << group->meanMz << "@" << group->meanRt;
    }
    //Issue 768: introduce possibility of browsing saved MS2 data only, missing all actual peak data
    bool isDisplayWithZeroPeaks = group->type() == PeakGroup::IsotopeType || !group->peakGroupScans.empty();
    if ((group->peakCount() == 0 || group->meanMz <= 0) && !isDisplayWithZeroPeaks) return;

    if (group->deletedFlag || group->isGroupLabeled('x')) return; //deleted group

    NumericTreeWidgetItem *item = nullptr;
    if(!root) {
       item = new NumericTreeWidgetItem(treeWidget,PeakGroupType);
    } else {
       item = new NumericTreeWidgetItem(root,PeakGroupType);
    }

    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);

    //Issue 329
    QString compoundText, adductText;
    if (group->compound) {
        compoundText = QString(group->compound->name.c_str());
    }
    if (group->adduct) {
        adductText = QString(group->adduct->name.c_str());
    }

    item->setData(0, PeakGroupType,QVariant::fromValue(group));

    item->setText(getGroupViewColumnNumber("ID"), groupTagString(group));     //ID

    //Issue 506
    if (viewType == groupView && _isShowLipidSummarizationColumns) {

        string acylChainLengthSummarized = LipidSummarizationUtils::getAcylChainLengthSummary(compoundText.toStdString());
        string acylChainCompositionSummarized = LipidSummarizationUtils::getAcylChainCompositionSummary(compoundText.toStdString());
        string lipidClassSummarized = LipidSummarizationUtils::getLipidClassSummary(compoundText.toStdString());

        item->setText(getGroupViewColumnNumber("Lipid Class"), lipidClassSummarized.c_str());
        item->setText(getGroupViewColumnNumber("Sum Composition"), acylChainCompositionSummarized.c_str());
        item->setText(getGroupViewColumnNumber("Acyl Chain Length"), acylChainLengthSummarized.c_str());
    }

    item->setText(getGroupViewColumnNumber("Compound"), compoundText);        //Compound
    item->setText(getGroupViewColumnNumber("Adduct"), adductText);            //Adduct

    if (group->_type == PeakGroup::GroupType::SRMTransitionType) {
        item->setText(getGroupViewColumnNumber("m/z"),
                      QString::number(group->srmPrecursorMz, 'f', 4));              //SRM precursor m/z
    } else if (isUseGroupMaxPeakVals){
        item->setText(getGroupViewColumnNumber("m/z"),
                      QString::number(group->maxPeakMzVal, 'f', 4));
    } else {
        item->setText(getGroupViewColumnNumber("m/z"),
                      QString::number(group->meanMz, 'f', 4));                      //m/z
    }

    float groupRt = group->meanRt;

    if (isUseGroupMaxPeakVals) {
        groupRt = group->maxPeakRtVal;
    }

    item->setText(getGroupViewColumnNumber("RT"),
                  QString::number(groupRt, 'f', 2));

    if (group->compound and group->compound->expectedRt and group->compound->expectedRt > 0) {
        item->setText(getGroupViewColumnNumber("RT Diff"),
                QString::number(groupRt - group->compound->expectedRt, 'f', 2)); //RT Diff
    }

    if (viewType == groupView) {

        if (group->_type == PeakGroup::GroupType::SRMTransitionType) {
            item->setText(getGroupViewColumnNumber("MS2 Score"),
                          QString::number(group->srmProductMz, 'f', 4));           //SRM product m/z
        }  else {
            item->setText(getGroupViewColumnNumber("MS2 Score"),
                    QString::number(group->fragMatchScore.mergedScore));           //MS2 Score
        }

        item->setText(getGroupViewColumnNumber("Rank"),
                      QString::number(group->groupRank,'f',3));                     //Rank
        item->setText(getGroupViewColumnNumber("Charge"),
                      QString::number(group->chargeState));                         //Charge
        item->setText(getGroupViewColumnNumber("Isotope #"),
                      QString::number(group->isotopicIndex));                       //Isotope #
        item->setText(getGroupViewColumnNumber("# Peaks"),
                      QString::number(group->peakCount()));                         //# Peaks
        item->setText(getGroupViewColumnNumber("# MS2s"),
                QString::number(group->ms2EventCount));                             //# MS2s
        item->setText(getGroupViewColumnNumber("Max Width"),
                      QString::number(group->maxNoNoiseObs));                       //Max Width
        item->setText(getGroupViewColumnNumber("Max Intensity"),
                      QString::number(group->maxIntensity,'g',2));                  //Max Intensity
        item->setText(getGroupViewColumnNumber("Max S/N"),
                      QString::number(group->maxSignalBaselineRatio,'f',0));        //Max S/N
        item->setText(getGroupViewColumnNumber("Max Quality"),
                      QString::number(group->maxQuality,'f',2));                    //Max Quality

    } else if ( viewType == peakView) {

        vector<mzSample*> vsamples = _mainwindow->getVisibleSamples();
        sort(vsamples.begin(), vsamples.end(), mzSample::compSampleOrder);
        vector<float>yvalues = group->getOrderedIntensityVector(vsamples,_mainwindow->getUserQuantType());

        for(unsigned int i=0; i<yvalues.size(); i++ ) {
         item->setText(5+i,QString::number(yvalues[i]));
        }

        heatmapBackground(item);
    }


    updateItem(item);

    if ( group->childCount() > 0 ) {

        //cerr << "Add children!" << endl;
        QTreeWidgetItem *childRoot = clusterDialog->chkPGDisplay->isChecked() ? root : item;

        for(unsigned int i=0; i < group->childCount(); i++ ){
            addRow(&(group->children[i]), childRoot);
        }
    }

    groupToItem.insert(make_pair(group, item));
}

bool TableDockWidget::hasPeakGroup(PeakGroup* group) {
    for(unsigned int i=0; i < allgroups.size(); i++ ) {
        if ( allgroups[i] == group ) return true;
        if (static_cast<double>(std::abs(group->meanMz - allgroups[i]->meanMz)) < 1e-5 &&
                static_cast<double>(std::abs(group->meanRt-allgroups[i]->meanRt)) < 1e-5
            ) {
            return true;
        }
    }
    return false;
}

PeakGroup* TableDockWidget::addPeakGroup(PeakGroup *group, bool updateTable) {
    return TableDockWidget::addPeakGroup(group, updateTable, false);
}

void TableDockWidget::selectGroup(PeakGroup* group) {

    //find all items connected to this group
    pair<rowIterator, rowIterator> tableRows = groupToItem.equal_range(group);
    for (rowIterator it = tableRows.first; it != tableRows.second; it++) {
         QTreeWidgetItem *item = it->second;
         if (item && !item->isHidden()) {
             item->setSelected(true);
         }
    }

}

/**
 * This solves the problem of creating a PeakGroup* on a background thread, and ensuring
 * that the pointer is deleted as soon as possible (that is, once the contents are retrieved and relocated).
 *
 * @brief TableDockWidget::addPeakGroup
 * @param group
 * @param updateTable
 * @param isDeletePeakGroupPtr
 * @return PeakGroup* that is guaranteed to persist as long as allgroups[] field persists
 */
PeakGroup* TableDockWidget::addPeakGroup(PeakGroup *group, bool updateTable, bool isDeletePeakGroupPtr) {
    if (!group) return nullptr;

    //debugging
//    cerr << "[TableDockWidget::addPeakGroup] "
//         << group << " Id="
//         << group->groupId
//         << ":(" << group->meanMz
//         << "," << group->meanRt
//         << ") <--> "
//         << group->compound->name << ":"
//         << group->adduct->name << ", score="
//         << group->fragMatchScore.mergedScore << ", ms2EventCount="
//         << group->ms2EventCount
//         << endl;

    //Issue 377, 409: avoid address changes from vector.push_back()
    //these tabledockwidget PeakGroup* objects persist forever, even after the table has been deleted
    //TODO: consider deleting PeakGroup* when not needed any longer
    PeakGroup *permanentGroup = new PeakGroup(*group);

    allgroups.push_back(permanentGroup);

    if (isDeletePeakGroupPtr){
       //qDebug() << "TableDockWidget::addPeakGroup() Deleting group: " << group;
       delete(group);
       group = nullptr;
    }

    //debugging for Issues 277, 235
//    qDebug() << "TableDockWidget::addPeakGroup() group=" << g;
//    qDebug() << "TableDockWidget::addPeakGroup() group data:";
//    qDebug() << "TableDockWidget::addPeakGroup() group->meanMz=" << g->meanMz;
//    qDebug() << "TableDockWidget::addPeakGroup() group->meanRt=" << g->meanRt;
//    if(g->compound) qDebug() << "TableDockWidget::addPeakGroup() group->compound=" << g->compound->name.c_str();
//    if(!g->compound) qDebug() << "TableDockWidget::addPeakGroup() group->compound= nullptr";

    if (updateTable) {
        showAllGroups();
    }

    return permanentGroup;
}

/**
 * Issue 424:
 * This should only be called when adding a PeakGroup that was loaded from a file (the PeakGroup was previously saved).
 * PeakGroups that are created transiently (e.g., from the EicWidget) should not use this method.
 * Similarly, Bookmarking peakgroups should also not use this method (instead, use addPeakGroup()).
 * background_peaks_update should also prefer addPeakGroup(). (Peaks and Compound searches).
 */
void TableDockWidget::addSavedPeakGroup(PeakGroup group) {

    PeakGroup *permanentGroup = new PeakGroup(group);
    permanentGroup->savedGroupId = permanentGroup->groupId;

    //children must also preserve their group ID
    for (auto& group : permanentGroup->children) {
        group.savedGroupId = group.groupId;
    }

    allgroups.push_back(permanentGroup);
}

void TableDockWidget::addDirectInfusionAnnotation(DirectInfusionGroupAnnotation *directInfusionGroupAnnotation, int clusterNum) {

    for (auto directInfusionMatchData : directInfusionGroupAnnotation->compounds){

        Compound* compound = directInfusionMatchData->compound;
        Adduct* adduct = directInfusionMatchData->adduct;

        //use the precursor m/z for direct infusion data. Only if this is missing, try to use the formula.
        float theoMz = compound->precursorMz;
        if (theoMz <= 0) {
            theoMz = adduct->computeAdductMass(MassCalculator::computeNeutralMass(compound->getFormula()));
        }

        //Issue 311: Display sane results for MS2-free matches
        if (directInfusionGroupAnnotation->precMzMax - directInfusionGroupAnnotation->precMzMin > 10) {
            directInfusionGroupAnnotation->precMzMin = theoMz - 0.5f;
            directInfusionGroupAnnotation->precMzMax = theoMz + 0.5f;
        }

        PeakGroup *pg = new PeakGroup();
        pg->metaGroupId = clusterNum;

        unsigned int peakCounter = 1;
        float maxRt = 0;

        for (auto pair : directInfusionGroupAnnotation->annotationBySample) {

            mzSample *sample = pair.first;
            DirectInfusionAnnotation *directInfusionAnnotation = pair.second;

            Peak p;
            if (directInfusionAnnotation->scan) {
                p.scan = directInfusionAnnotation->scan->scannum;
            }
            p.setSample(sample);

            p.peakMz = theoMz;
            p.mzmin = directInfusionGroupAnnotation->precMzMin;
            p.mzmax = directInfusionGroupAnnotation->precMzMax;

            //Used by PeakGroup::getFragmentationEvents() - checks RT bounds with scan RT
            p.rtmin = 0;
            p.rtmax = FLT_MAX;

            //Used by PeakGroup::groupStatistics()
            p.pos = peakCounter;
            p.baseMz = theoMz;
            p.peakIntensity = directInfusionMatchData->observedMs1Intensity;

            //Issue 219: Avoid filling with junk
            p.noNoiseObs = 0;
            p.peakAreaTop = p.peakIntensity;
            p.peakAreaCorrected = p.peakIntensity;
            p.peakArea = p.peakIntensity;

            pg->addPeak(p);

            peakCounter++;

            if (sample->maxRt > maxRt) maxRt = sample->maxRt;

        }

        pg->compound = compound;
        pg->compoundDb = compound->db;
        pg->compoundId = compound->id;

        pg->adduct = adduct;
        pg->minMz = directInfusionGroupAnnotation->precMzMin;
        pg->maxMz = directInfusionGroupAnnotation->precMzMax;
        pg->meanMz = theoMz;
        pg->minRt = 0;
        pg->maxRt = maxRt;

        if (directInfusionGroupAnnotation->fragmentationPattern && directInfusionGroupAnnotation->fragmentationPattern->consensus) {
            pg->fragmentationPattern = directInfusionGroupAnnotation->fragmentationPattern->consensus;
        }

        pg->fragMatchScore = directInfusionMatchData->fragmentationMatchScore;
        pg->fragMatchScore.mergedScore = pg->fragMatchScore.numMatches;

        pg->groupRank = directInfusionMatchData->proportion;

        //avoid writing random junk to table
        pg->maxNoNoiseObs = 0;
        pg->maxQuality = 0;

        allgroups.push_back(pg);
    }

     directInfusionGroupAnnotation->clean();

     //Issue 100: these delete statements are currently failing
//     if (directInfusionGroupAnnotation->fragmentationPattern) delete(directInfusionGroupAnnotation->fragmentationPattern);
//     if (directInfusionGroupAnnotation) delete(directInfusionGroupAnnotation);
}

void TableDockWidget::addMs3Annotation(Ms3Annotation* ms3Annotation, int clusterNum) {

    if (!ms3Annotation) return;

    //the main group
    PeakGroup *pg = new PeakGroup();
    //pg.metaGroupId = clusterNum;

    int maxNumMs3Matches = 0;

    Ms3Compound *ms3Compound = nullptr;

    map<int, PeakGroup> childrenPeakGroupsByPrecMs2Mz{};

    unsigned int counter = 0;
    for (auto it = ms3Annotation->matchesBySample.begin(); it != ms3Annotation->matchesBySample.end(); ++it) {

        Ms3SingleSampleMatch *ms3SingleSampleMatch = it->second;

        if (!pg->compound){

            pg->compound = ms3SingleSampleMatch->ms3Compound->baseCompound;

            ms3Compound = ms3SingleSampleMatch->ms3Compound;

            for (auto it2 = ms3Compound->ms3_fragment_mzs.begin(); it2 != ms3Compound->ms3_fragment_mzs.end(); ++it2) {
                PeakGroup childPeakGroup;
                childPeakGroup.groupRank = 0;
                childrenPeakGroupsByPrecMs2Mz.insert(make_pair(it2->first, childPeakGroup));
            }
        }

        Peak p;
        p.setSample(it->first);
        p.pos = counter;
        p.noNoiseObs = 0;
        p.quality = static_cast<float>(it->second->numMs3MzMatches);

        p.peakIntensity = ms3SingleSampleMatch->sumMs3MzIntensity;
        p.peakAreaTop = p.peakIntensity;
        p.peakAreaCorrected = p.peakIntensity;
        p.peakArea = p.peakIntensity;

        p.rt = 0;
        p.rtmin = 0;
        p.rtmax = FLT_MAX;

        p.peakMz = pg->compound->precursorMz;
        p.baseMz = pg->compound->precursorMz;
        p.mzmin = pg->compound->precursorMz - 0.5f; //TODO
        p.mzmax = pg->compound->precursorMz + 0.5f; //TODO

        if (it->second->numMs3MzMatches > maxNumMs3Matches){
            maxNumMs3Matches = it->second->numMs3MzMatches;
        }

        pg->addPeak(p);

        for (auto it2 = ms3SingleSampleMatch->sumMs3IntensityByMs2Mz.begin(); it2 != ms3SingleSampleMatch->sumMs3IntensityByMs2Mz.end(); ++it2) {

            int ms2PrecursorMzKey = it2->first;

            double ms2PrecursorMz = mzUtils::intKeyToMz(ms2PrecursorMzKey);

            Peak p;
            p.setSample(it->first);
            p.pos = counter;
            p.noNoiseObs = 0;

            p.peakIntensity = it2->second;
            p.peakAreaTop = p.peakIntensity;
            p.peakAreaCorrected = p.peakIntensity;
            p.peakArea = p.peakIntensity;

            p.rt = 0;
            p.rtmin = 0;
            p.rtmax = FLT_MAX;

            p.peakMz = static_cast<float>(ms2PrecursorMz);
            p.baseMz = static_cast<float>(ms2PrecursorMz);
            p.mzmin = static_cast<float>(ms2PrecursorMz) - 0.5f; //TODO
            p.mzmax = static_cast<float>(ms2PrecursorMz) + 0.5f; //TODO

            if (ms3SingleSampleMatch->ms3MatchesByMs2Mz.find(ms2PrecursorMzKey) != ms3SingleSampleMatch->ms3MatchesByMs2Mz.end()) {
                float numMatches = static_cast<float>(ms3SingleSampleMatch->ms3MatchesByMs2Mz[ms2PrecursorMzKey]);
                p.quality = numMatches;
                if (childrenPeakGroupsByPrecMs2Mz[ms2PrecursorMzKey].fragMatchScore.mergedScore < numMatches) {
                    childrenPeakGroupsByPrecMs2Mz[ms2PrecursorMzKey].fragMatchScore.mergedScore = numMatches;
                }
            }

            childrenPeakGroupsByPrecMs2Mz[ms2PrecursorMzKey].addPeak(p);

        }

        counter++;
    }

    //avoid writing random junk to table
    pg->groupRank = 0;
    pg->fragMatchScore.mergedScore = maxNumMs3Matches;

    for (auto it = childrenPeakGroupsByPrecMs2Mz.begin(); it != childrenPeakGroupsByPrecMs2Mz.end(); ++it){

        PeakGroup child = it->second;

        stringstream s;
        s << std::fixed << setprecision(0)
          << "target: ("
          << pg->compound->precursorMz
          << ", "
          << mzUtils::intKeyToMz(it->first)
          << ")";

        child.tagString = s.str();
        pg->addChild(child);
    }

    // qDebug() << "TableDockWidget::addMs3Annotation():" << groupTagString(&pg);

    //delete original Ms3Compound*
    if (ms3Compound) {
        delete(ms3Compound);
        ms3Compound = nullptr;
    }

    allgroups.push_back(pg);
}

QList<PeakGroup*> TableDockWidget::getAllGroups() {
    QList<PeakGroup*> groups;
    for(unsigned int i=0; i < allgroups.size(); i++ ) {
        groups.push_back(allgroups[i]);
    }
    return groups;
}

//Issue 264: Only retrieve visible groups (not filtered out)
//previously, TableDockWidget::getAllGroups()
QList<PeakGroup*> TableDockWidget::getGroups() {

    QList<PeakGroup*> visibleGroups;

    int itemCount = treeWidget->topLevelItemCount();
    for(int i=0; i < itemCount; i++ ) {
         QTreeWidgetItem *item = treeWidget->topLevelItem(i);
            if (!item) continue;
            traverseAndCollectVisibleGroups(item, visibleGroups);
    }

    return visibleGroups;
}

void TableDockWidget::deleteAll() {

    QString msg("Are you sure you want to delete peak groups table \"");
    msg.append(this->windowTitle());
    msg.append("\" and all peak groups therein?");

    if (QMessageBox::No == QMessageBox::question(this, "Delete Confirmation",
          msg, QMessageBox::Yes | QMessageBox::No)) {
        return;
    }

    treeWidget->clear();
    allgroups.clear();
    groupToItem.clear();
    compoundToGroup.clear();
    groupIdToGroup.clear();

    this->hide();

    _mainwindow->getEicWidget()->replotForced();

    if ( _mainwindow->heatmap ) {
        HeatMap* _heatmap = _mainwindow->heatmap;
        _heatmap->setTable(_mainwindow->bookmarkedPeaks);
        _heatmap->replot();
    }
    _mainwindow->deletePeakTable(this);
}


void TableDockWidget::showAllGroups() {

    //Issue 277: treeWidget->clear() will emit a signal that the selection has changed
    //that is ultimately received by MainWindow::setPeakGroup().
    //the PeakGroup that is sent may be junk.
    treeWidget->blockSignals(true);

    treeWidget->clear();
    groupToItem.clear();    //addRow() calls will refill the map
    compoundToGroup.clear();
    groupIdToGroup.clear();

    if (allgroups.size() == 0 ) return;
    treeWidget->setSortingEnabled(false);

    //Issue 539: organized based on compounds
    //Either organize groups by compounds, or color them based on duplicate compounds (not both)
    vector<vector<PeakGroup*>> allGroups;
    if (_isGroupCompoundsTogether) {
        allGroups = clusterByCompounds();
    } else {
        allGroups = vector<vector<PeakGroup*>>(allgroups.size());
        for (unsigned int i = 0; i < allgroups.size(); i++) {

            PeakGroup* group = allgroups[i];
            allGroups[i] = vector<PeakGroup*>{group};

            //Issue 438: only apply to top level (do not apply to children)
            //used for highlighting colors
            if (group->compound) {
                if (compoundToGroup.find(group->compound) == compoundToGroup.end()) {
                    compoundToGroup.insert(make_pair(group->compound, vector<PeakGroup*>{}));
                }
                compoundToGroup[group->compound].push_back(group);
            }
            QString groupIdString = groupTagString(group);
            if (groupIdToGroup.find(groupIdString) == groupIdToGroup.end()) {
                groupIdToGroup.insert(make_pair(groupIdString, vector<PeakGroup*>{}));
            }
            groupIdToGroup[groupIdString].push_back(group);
        }
    }

    QMap<int,QTreeWidgetItem*> parents;
    for (unsigned int i = 0; i < allGroups.size(); i++) {
        vector<PeakGroup*> groupI = allGroups[i];

        PeakGroup *pg = groupI[0];

        //First PeakGroup in a vector<PeakGroup> may also be involved in a cluster.
        int metaGroupId  = pg->metaGroupId;
        if (metaGroupId && pg->meanMz > 0 && pg->peakCount()>0) {

            QTreeWidgetItem* parent = nullptr;
            if (!parents.contains(metaGroupId)) {
                if (windowTitle() != "Bookmarks") {
                    parents[metaGroupId]= new QTreeWidgetItem(treeWidget);

                    //Issue 311: improve UI
                    QString clusterString;
                    if (metaGroupId == DirectInfusionSearchSet::getNoMs2ScansMapKey() && isDirectInfusionTable()){
                        clusterString = QString("Compounds with no MS2 matches");
                    } else {
                        clusterString = QString("Cluster ").append(QString::number(metaGroupId));
                    }
                    parents[metaGroupId]->setText(0,clusterString);

                    parents[metaGroupId]->setText(3,QString::number(static_cast<double>(pg->meanRt),'f',2));
                    parents[metaGroupId]->setExpanded(true);
                    parent = parents[metaGroupId];
                }
            } else {
                parent = parents[metaGroupId];
            }

            addRow(pg, parent);
        } else {
            addRow(pg, nullptr);
        }

        //only need to retrieve the parent peak group if there are more peak groups to organize.
        if (groupI.size() == 1) continue;

        QTreeWidgetItem* pgParent = nullptr;
        pair<rowIterator, rowIterator> tableRows = groupToItem.equal_range(pg);
        for (rowIterator it = tableRows.first; it != tableRows.second; ++it) {
             pgParent = it->second;
             break;
        }

        //All subsequent PeakGroups in a vector<PeakGroup> must be subordinate to the first PeakGroup.
        for (unsigned int j = 1; j < groupI.size(); j++) {
            addRow(groupI[j], pgParent);
        }
    }

    QScrollBar* vScroll = treeWidget->verticalScrollBar();
    if ( vScroll ) {
        vScroll->setSliderPosition(vScroll->maximum());
    }
    treeWidget->setSortingEnabled(true);

    treeWidget->expandAll();

    updateStatus();
    updateIcons();

    if (windowTitle() != "Bookmarks") {
        searchParamsDialog->txtSrchResults->setText("Total # groups: " + QString::number(allgroups.size()));
    }

    treeWidget->blockSignals(false);
}

void TableDockWidget::showAllGroupsThenSort() {
    showAllGroups();
    if (viewType == groupView) {
        if (isTargetedMs3Table()) {

            treeWidget->sortByColumn(getGroupViewColumnNumber("Max Intensity"),
                                     Qt::DescendingOrder); //decreasing by max intensity

        } else if (isDirectInfusionTable()) {
            treeWidget->sortByColumn(getGroupViewColumnNumber("m/z"),
                                     Qt::AscendingOrder); // increasing by m/z
        } else {
            treeWidget->sortByColumn(getGroupViewColumnNumber("MS2 Score"),
                                     Qt::DescendingOrder); //decreasing by score

            QTreeWidgetItemIterator it(treeWidget);
            while (*it) {
                QTreeWidgetItem* item = *it;

                if (item->childCount() > 0) {
                    item->sortChildren(getGroupViewColumnNumber("m/z"), Qt::AscendingOrder);
                }
                ++it;
            }
        }
    }
}

void TableDockWidget::exportGroupsToSpreadsheet() {

    qDebug() << "TableDockWidget::exportGroupsToSpreadsheet()";

    if (allgroups.size() == 0 ) {
        QString msg = "Peaks Table is Empty";
        QMessageBox::warning(this, tr("Error"), msg);
        return;
    }

    QString dir = ".";
    QSettings* settings = _mainwindow->getSettings();

    if ( settings->contains("lastDir") ) dir = settings->value("lastDir").value<QString>();

    QString groupsCSV = "Groups Summary Matrix Format Comma Delimited (*.csv)";
    QString peaksCSV =  "Peaks Detailed Format Comma Delimited (*.csv)";
    QString groupsTAB = "Groups Summary Matrix Format (*.tab)";
    QString peaksTAB =  "Peaks Detailed Format (*.tab)";

    QString sFilterSel;
    QString fileName = QFileDialog::getSaveFileName(this, 
            tr("Export Groups"), dir, 
            groupsCSV + ";;" + peaksCSV + ";;" + groupsTAB + ";;" + peaksTAB,
            &sFilterSel);

    if(fileName.isEmpty()) return;

    if ( sFilterSel == groupsCSV || sFilterSel == peaksCSV) {
        if(!fileName.endsWith(".csv",Qt::CaseInsensitive)) fileName = fileName + ".csv";
    } else  if ( sFilterSel == groupsTAB || sFilterSel == peaksTAB) {
        if(!fileName.endsWith(".tab",Qt::CaseInsensitive)) fileName = fileName + ".tab";
    }


    vector<mzSample*> samples = _mainwindow->getSamples();
    if ( samples.size() == 0) return;

    CSVReports* csvreports = new CSVReports(samples);
    csvreports->setUserQuantType( _mainwindow->getUserQuantType() );

    if (sFilterSel == groupsCSV) {
        csvreports->setCommaDelimited();
        csvreports->openGroupReport(fileName.toStdString());
    } else if (sFilterSel == groupsTAB )  {
        csvreports->setTabDelimited();
        csvreports->openGroupReport(fileName.toStdString());
    } else if (sFilterSel == peaksCSV )  {
        csvreports->setCommaDelimited();
        csvreports->openPeakReport(fileName.toStdString());
    } else if (sFilterSel == peaksTAB )  {
        csvreports->setTabDelimited();
        csvreports->openPeakReport(fileName.toStdString());
    } else {
        qDebug() << "exportGroupsToSpreadsheet: Unknown file type. " << sFilterSel;
    }


    qDebug() << "Writing report to " << fileName << "...";

    //Issue 332
    for (auto item : treeWidget->selectedItems()) {

        QVariant v = item->data(0, PeakGroupType);
        PeakGroup *peakGroup = v.value<PeakGroup*>();

        //Issue 615, 760: Check if item is expanded. Only add children if it isn't.
        //This avoids a bug where children peak groups were added twice when tree widget items
        //are already expanded and 'select all' is chosen.
        if (peakGroup) {
            csvreports->addGroup(peakGroup, !item->isExpanded());
        }
    }

    csvreports->closeFiles();

    if (csvreports) delete(csvreports);

    qDebug() << "Finished Writing report to " << fileName << ".";
}

void TableDockWidget::showIsotopeExportSettings() {
    qDebug() << "TableDockWidget::showIsotopeExportSettings()";

    isotopeExportSettingsDialog->show();
}

void TableDockWidget::hideIsotopeExportSettings() {
    qDebug() << "TableDockWidget::hideIsotopeExportSettings()";

    isotopeExportSettingsDialog->hide();
}

void TableDockWidget::exportGroupsAsIsotopes() {
    qDebug() << "TableDockWidget::exportGroupsAsIsotopes()";

    QString dir = ".";
    QSettings* settings = _mainwindow->getSettings();

    if ( settings->contains("lastIsotopesExportDir")) {
        dir = settings->value("lastIsotopesExportDir").value<QString>();
    }

    QString fileName = QFileDialog::getSaveFileName(
        this,
        tr("Isotopes Export"),
        dir,
        QString(".csv"));

    QFileInfo fi(fileName);
    settings->setValue("lastIsotopesExportDir", fi.baseName());

    if(fileName.isEmpty()) return;

    if (!fileName.endsWith(".csv")) {
        fileName.append(".csv");
    }
    string sep = ",";

    ofstream isotopesReport;
    isotopesReport.open(fileName.toStdString().c_str());

    QStringList header;
    header << "compound" << "adduct" << "label" << "RT" << "isotope";

    vector<mzSample*> vsamples = _mainwindow->getVisibleSamples();
    sort(vsamples.begin(), vsamples.end(), mzSample::compSampleOrder);

    for (auto mzSample : vsamples) {
        header << mzSample->sampleName.c_str();
    }

    foreach(QString h, header) isotopesReport << h.toStdString() << sep.c_str();
    isotopesReport << endl;

    //Issue 739: respect filters for this export
    QList<PeakGroup*> visibleGroups = getGroups();
    for (unsigned int i = 0; i < visibleGroups.size(); i++) {
        PeakGroup *group = visibleGroups[i];

        //Issue 685
        if (group->deletedFlag) continue;

        if (group->compound && group->adduct) {

            IsotopeMatrix matrix = this->_mainwindow->getIsotopicMatrix(
                group,
                isotopeExportSettingsDialog->chkCorrectIsotopes->isChecked(),
                isotopeExportSettingsDialog->radFractional->isChecked());

            //Issue 788: Record which isotopic peak groups have already been flagged for deletion
            set<string> deletedPeakGroups{};
            for (PeakGroup &p : group->children) {
                if (p.deletedFlag) {
                    deletedPeakGroups.insert(p.tagString);
                }
            }

            for (unsigned int j = 0; j < matrix.isotopesData.cols(); j++) { // isotope

                string isotopeName = matrix.isotopeNames.at(j);

                //Issue 788: Skip over any peaks where the specific isotope has been deleted
                if (deletedPeakGroups.find(isotopeName) != deletedPeakGroups.end()) {
                    continue;
                }

                //Issue 750: add labels
                string labelsStr(group->labels.begin(), group->labels.end());

                //Issue 710: Add double-quotes around strings, if necessary
                isotopesReport << mzUtils::doubleQuoteString(group->compound->name) << sep.c_str()
                               << mzUtils::doubleQuoteString(group->adduct->name) << sep.c_str()
                               << mzUtils::doubleQuoteString(labelsStr) << sep.c_str()
                               << group->medianRt() << sep.c_str()
                               << mzUtils::doubleQuoteString(isotopeName);

                for (unsigned int i = 0; i < matrix.isotopesData.rows(); i++) { // sample

                    isotopesReport << sep.c_str() << matrix.isotopesData(i, j);
                }

                isotopesReport << "\n";
            }
        }
    }

    isotopesReport.close();
}

void TableDockWidget::showSelectedGroup() {
    QTreeWidgetItem *item = treeWidget->currentItem();
    if (!item) return;
    if (item->type() != PeakGroupType) return;

    QVariant v = item->data(0,PeakGroupType);
    PeakGroup*  group =  v.value<PeakGroup*>();

    //Issue 226: Handle ms3 searches differently
    if (isTargetedMs3Table()) {

        PeakGroup *parentGroup = nullptr;
        PeakGroup *childGroup = nullptr;

        //case: leaf group
        if (group->childCount() == 0) {
            parentGroup = group->parent;
            childGroup = group;
        } else {
            parentGroup = group;

            //choose any child that actually contains peaks, valid m/z
            for (auto& child : parentGroup->getChildren()) {
                if (child.meanMz > 0 && child.peakCount() > 0) {
                    childGroup = &(child);
                    break;
                }
            }

            if (!childGroup) return; // should never happen
        }

        if (directInfusionSearchParams) {
            _mainwindow->ms3SpectraWidget->setMs3MatchingTolerance(directInfusionSearchParams->ms3MatchTolrInDa);
        }
          _mainwindow->setMs3PeakGroup(parentGroup, childGroup);
    } else {
         _mainwindow->setPeakGroup(group);
    }
}

QList<PeakGroup*> TableDockWidget::getSelectedGroups() {
    QList<PeakGroup*> selectedGroups;
    foreach(QTreeWidgetItem* item, treeWidget->selectedItems() ) {
        if (item) {
            QVariant v = item->data(0,PeakGroupType);
            PeakGroup*  group =  v.value<PeakGroup*>();
            if (group) {
                selectedGroups.append(group);
            }
        }
    }
    return selectedGroups;
}

PeakGroup* TableDockWidget::getSelectedGroup() { 
    QTreeWidgetItem *item = treeWidget->currentItem();
    if (!item) return nullptr;

    QVariant v = item->data(0,PeakGroupType);
    PeakGroup*  group =  v.value<PeakGroup*>();

    if (group){
        return group;
    }

    return nullptr;
}

void TableDockWidget::setGroupLabel(char label) {
    foreach(QTreeWidgetItem* item, treeWidget->selectedItems() ) {
        if (item) {
            QVariant v = item->data(0,PeakGroupType);
            PeakGroup*  group =  v.value<PeakGroup*>();
            if (group) {
                group->toggleLabel(label);
            }
            updateItem(item);
        }
    }

    //The labeled group might be filtered out.
    updateTagFilter();
    updateIcons();
    updateStatus();
}

//mark groups to be deleted as 'x'
void TableDockWidget::traverseAndDeleteGroups(QTreeWidgetItem *item) {

    QVariant v = item->data(0,PeakGroupType);
    PeakGroup*  group =  v.value<PeakGroup*>();

    if (group) {
        group->deletedFlag = true;
        group->addLabel('x');
        for (int i = 0; i < item->childCount(); ++i){
            traverseAndDeleteGroups(item->child(i));
        }
    }

}

void TableDockWidget::traverseAndCollectVisibleGroups(QTreeWidgetItem *item, QList<PeakGroup*>& groups) {

    if (item && !item->isHidden()) {

        QVariant v = item->data(0,PeakGroupType);
        PeakGroup* group =  v.value<PeakGroup*>();

        if (group) {
            groups << group;
        }

        for (int i = 0; i < item->childCount(); ++i){
            traverseAndCollectVisibleGroups(item->child(i), groups);
        }
    }
}

void TableDockWidget::deleteSelected() {

    qDebug() << "deleteSelected()";
    QList<PeakGroup*> selectedGroups = getSelectedGroups();
    QTreeWidgetItem* nextItem = nullptr;

    foreach(QTreeWidgetItem* item, treeWidget->selectedItems() ) {
        if (item) {

            traverseAndDeleteGroups(item);

            //which items will displayed next
            nextItem = treeWidget->itemBelow(item); //get next item
            if(!nextItem) nextItem = treeWidget->itemAbove(item);

            //remove item from the tree
            if (item->parent()) {
                 item->parent()->removeChild(item);
            } else {
                treeWidget->takeTopLevelItem(treeWidget->indexOfTopLevelItem(item));
            }
        }
    }

    //deleting items from tree causes crash.. not clear yet where bug is
    /*
    if (deleteCount) {
        for(int i=0; i < allgroups.size(); i++) {
            if(allgroups[i].deletedFlag)  {
                qDebug() << "Delete=" << allgroups[i].groupId;
                allgroups.erase(allgroups.begin()+i);
                i--;
            }
        }
    }
    */

    if (nextItem)  {
        treeWidget->setCurrentItem(nextItem);
        //_mainwindow->getEicWidget()->replotForced();
    }

    updateIcons();
    updateStatus();
}

void TableDockWidget::setClipboard() { 
    QList<PeakGroup*>groups = getSelectedGroups();
    if (groups.size() >0) {
        _mainwindow->isotopeWidget->setClipboard(groups);
    }
}

void TableDockWidget::tagGroup(const QString &tagLabel){
    char c = tagLabel.toLatin1().data()[0];
    setGroupLabel(c);
}

void TableDockWidget::markGroupGood() { 
    setGroupLabel('g');
}

void TableDockWidget::markGroupBad() { 
    setGroupLabel('b');
}

void TableDockWidget::unmarkSelectedGroups() {
    setGroupLabel('\0');
}

void TableDockWidget::showLastGroup() {
    QTreeWidgetItem *item= treeWidget->currentItem();
    if (item)  {
        treeWidget->setCurrentItem(treeWidget->itemAbove(item));
    }
}

PeakGroup* TableDockWidget::getLastBookmarkedGroup() {
    if (allgroups.size() > 0) {
        return allgroups.back();
    } else {
        return nullptr;
    }
}

void TableDockWidget::showNextGroup() {

    QTreeWidgetItem *item= treeWidget->currentItem();
    if ( !item ) return;

    QTreeWidgetItem* nextitem = treeWidget->itemBelow(item); //get next item
    if ( nextitem  ){
        treeWidget->setCurrentItem(nextitem);

        //Issue 125: When working through a list, only keep one item selected at a time.
        item->setSelected(false);

    }

}

void TableDockWidget::Train() { 
    Classifier* clsf = _mainwindow->getClassifier();


    if (allgroups.size() == 0 ) return;
    if (clsf == NULL ) return;

    vector<PeakGroup*> train_groups;
    vector<PeakGroup*> test_groups;
    vector<PeakGroup*> good_groups;
    vector<PeakGroup*> bad_groups;
    MTRand mtrand;

    for(int i=0; i <allgroups.size(); i++ ) {
        PeakGroup* grp = allgroups[i];
        if (grp->isGroupGood()) good_groups.push_back(grp);
        if (grp->isGroupBad()) bad_groups.push_back(grp);
    }

    mzUtils::shuffle(good_groups);
    for(int i=0; i <good_groups.size(); i++ ) {
        PeakGroup* grp = good_groups[i];
        i%2 == 0 ? train_groups.push_back(grp): test_groups.push_back(grp);
    }

    mzUtils::shuffle(bad_groups);
    for(int i=0; i <bad_groups.size(); i++ ) {
        PeakGroup* grp =bad_groups[i];
        i%2 == 0 ? train_groups.push_back(grp): test_groups.push_back(grp);
    }

    //cerr << "Groups Total=" <<allgroups.size() << endl;
    //cerr << "Good Groups=" << good_groups.size() << " Bad Groups=" << bad_groups.size() << endl;
    //cerr << "Splitting: Train=" << train_groups.size() << " Test=" << test_groups.size() << endl;

    clsf->train(train_groups);
    clsf->classify(test_groups);
    showAccuracy(test_groups);
    updateTable();
}

void TableDockWidget::keyPressEvent(QKeyEvent *e ) {

     qDebug() << "KEY=" << e->key();

    if (e->key() == Qt::Key_Enter ) {
        showSelectedGroup();
    } else if (e->key() == Qt::Key_Delete ) {
        deleteSelected();
    } else if ( e->key() == Qt::Key_T ) {
        Train();
    } else if ( e->key() == Qt::Key_G ) {
        markGroupGood();
    } else if ( e->key() == Qt::Key_B ) {
        markGroupBad();
    } else if ( e->key() == Qt::Key_U ) {
        unmarkSelectedGroups();
    }

    //TODO: respond to event appropriately
    for (auto &x : DB.peakGroupTags) {

        PeakGroupTag *tag = x.second;

        QString keyString = QKeySequence(e->key()).toString().toLower();

        if (tag->hotkey == keyString.toLatin1().data()[0]){

            setGroupLabel(tag->label);

            break; // found the key, no need to continue searching through tag list
        }

    }

    QDockWidget::keyPressEvent(e);
    updateStatus();
    updateIcons();
}

void TableDockWidget::updateStatus() {

    int totalCount=0;
    int goodCount=0;
    int badCount=0;
    for(auto g : allgroups) {
        if (g->isGroupGood()){
            goodCount++;
        } else if (g->isGroupBad()) {
            badCount++;
        }
        totalCount++;
    }
    QString title = tr("Group Validation Status: Good=%2 Bad=%3 Total=%1").arg(
            QString::number(totalCount),
            QString::number(goodCount),
            QString::number(badCount));
    _mainwindow->setStatusText(title);
}

void TableDockWidget::updateIcons() {
    qDebug() << "TableDockWidget::updateIcons(): Resizing icons.";

    QSize maxIconSize = treeWidget->iconSize();

    for (const QIcon& icon : _iconCache) {

        if (icon.availableSizes().isEmpty()) continue;

        QSize availableSize = icon.availableSizes().first();

        //enlarge sizes as needed
        if (availableSize.width() > maxIconSize.width()) {
            maxIconSize.setWidth(availableSize.width());
        }
        if (availableSize.height() > maxIconSize.height()) {
            maxIconSize.setHeight(availableSize.height());
        }
    }

    if (maxIconSize != treeWidget->iconSize()) {
        treeWidget->setIconSize(maxIconSize);
    }
}

float TableDockWidget::showAccuracy(vector<PeakGroup*>&groups) {
    //check accuracy
    if ( groups.size() == 0 ) return 0;

    int fp=0; int fn=0; int tp=0; int tn=0;  int total=0; float accuracy=0;
    int gc=0;
    int bc=0;
    for(int i=0; i < groups.size(); i++ ) {
        if (groups[i]->isGroupGood() || groups[i]->isGroupBad()) {
            for(int j=0; j < groups[i]->peaks.size(); j++ ) {
                float q = groups[i]->peaks[j].quality;
                char  l = groups[i]->peaks[j].label;
                if (l == 'g' ) gc++;
                if (l == 'g' && q > 0.5 ) tp++;
                if (l == 'g' && q < 0.5 ) fn++;

                if (l == 'b' ) bc++;
                if (l == 'b' && q < 0.5 ) tn++;
                if (l == 'b' && q > 0.5 ) fp++;
                total++;
            }
        }
    }
    if ( total > 0 ) accuracy = 1.00-((float)(fp+fn)/total);
    cerr << "TOTAL=" << total << endl;
    if ( total == 0 ) return 0;

    cerr << "GC=" << gc << " BC=" << bc << endl;
    cerr << "TP=" << tp << " FN=" << fn << endl;
    cerr << "TN=" << tn << " FP=" << fp << endl;
    cerr << "Accuracy=" << accuracy << endl;

    traindialog->FN->setText(QString::number(fn));
    traindialog->FP->setText(QString::number(fp));
    traindialog->TN->setText(QString::number(tn));
    traindialog->TP->setText(QString::number(tp));
    traindialog->accuracy->setText(QString::number(accuracy*100,'f',2));
    traindialog->show();
    _mainwindow->setStatusText(tr("Good Groups=%1 Bad Groups=%2 Accuracy=%3").arg(
            QString::number(gc),
            QString::number(bc),
            QString::number(accuracy*100)
            ));

    return accuracy;
}

void TableDockWidget::showScatterPlot() { 
    if (groupCount() == 0 ) return;
    _mainwindow->scatterDockWidget->setVisible(true);
    ((ScatterPlot*) _mainwindow->scatterDockWidget)->setTable(this);
    ((ScatterPlot*) _mainwindow->scatterDockWidget)->replot();
    ((ScatterPlot*) _mainwindow->scatterDockWidget)->contrastGroups();
}


void TableDockWidget::printPdfReport() {

    QString dir = ".";
    QSettings* settings = _mainwindow->getSettings();
    if ( settings->contains("lastDir") ) dir = settings->value("lastDir").value<QString>();

    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Group Report a PDF File"),dir,tr("*.pdf"));
    if (fileName.isEmpty()) return;
    if(!fileName.endsWith(".pdf",Qt::CaseInsensitive)) fileName = fileName + ".pdf";

    QPrinter printer;
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOrientation(QPrinter::Landscape);
    //printer.setResolution(QPrinter::HighResolution);
    printer.setCreator("MAVEN Metabolics Analyzer");
    printer.setOutputFileName(fileName);

    QPainter painter;

    if (! painter.begin(&printer)) { // failed to open file
        qWarning("failed to open file, is it writable?");
        return;
    }

    if(printer.printerState() != QPrinter::Active) {
        qDebug() << "PrinterState:" << printer.printerState();
    }

    //PDF report only for selected groups
    QList<PeakGroup*>selected = getSelectedGroups();

    for(int i=0; i <selected.size(); i++ ) {
        PeakGroup* grp = selected[i];
        _mainwindow->getEicWidget()->setPeakGroup(grp);
        _mainwindow->getEicWidget()->render(&painter);

        if (! printer.newPage()) {
            qWarning("failed in flushing page to disk, disk full?");
            return;
        }
    }
    painter.end();
 }

void TableDockWidget::showHeatMap() { 

    _mainwindow->heatMapDockWidget->setVisible(true);
    HeatMap* _heatmap = _mainwindow->heatmap;
    if ( _heatmap ) {
        _heatmap->setTable(this);
        _heatmap->replot();
    }
}

void TableDockWidget::showGallery() { 

    if ( _mainwindow->galleryWidget ) {
        _mainwindow->galleryDockWidget->setVisible(true);
        QList<PeakGroup*>selected = getSelectedGroups();
        vector<PeakGroup*>groups(selected.size());
        for(int i=0; i<selected.size(); i++) { groups[i]=selected[i]; }
        _mainwindow->galleryWidget->clear();
        _mainwindow->galleryWidget->addEicPlots(groups);
    }
}


void TableDockWidget::showTreeMap() { 

    /*
	_mainwindow->treeMapDockWidget->setVisible(true);
	TreeMap* _treemap = _mainwindow->treemap;
	if ( _treemap ) {
			_treemap->setTable(this);
			_treemap->replot();
	}
	*/
}

void TableDockWidget::contextMenuEvent ( QContextMenuEvent * event ) 
{
    QMenu menu;

    if (windowTitle() == "Bookmarks" || windowTitle() == "rumsDB" || windowTitle() == "clamDB" || isDetectedFeaturesTable()) {
        QAction* z8 = menu.addAction("Edit Selected Peak Group ID");
        connect(z8, SIGNAL(triggered()), SLOT(showEditPeakGroupDialog()));

        QAction *z9 = menu.addAction("Export RT Alignment from Selected");
        connect(z9, SIGNAL(triggered()), SLOT(exportAlignmentFile()));

    }

    //Issue 641: Make compound reassignment available from peakdetector
    QAction *z10 = menu.addAction("Set Compound Search Selected Compound");
    connect(z10, SIGNAL(triggered()), SLOT(setCompoundSearchSelectedCompound()));

    QAction* actionShowLipidSummarizationColumn = menu.addAction("View Lipid Summarization Columns");
    actionShowLipidSummarizationColumn->setCheckable(true);
    actionShowLipidSummarizationColumn->setChecked(_isShowLipidSummarizationColumns);

    connect(actionShowLipidSummarizationColumn, SIGNAL(toggled(bool)), SLOT(showLipidSummarizationColumns(bool)));
    connect(actionShowLipidSummarizationColumn, SIGNAL(toggled(bool)), SLOT(setupPeakTable()));
    connect(actionShowLipidSummarizationColumn, SIGNAL(toggled(bool)), SLOT(showAllGroups()));

    menu.addSeparator();

    vector<PeakGroupTag*> supplementalTags = DB.getSupplementalPeakGroupTags();

    if (!supplementalTags.empty()) {
        for (auto peakGroupTag : supplementalTags) {

            QString actionMsg("Tag Group(s): ");
            actionMsg.append(peakGroupTag->tagName.c_str());
            QAction *actionTag = menu.addAction(peakGroupTag->icon, actionMsg);

            QString labelAsQString(peakGroupTag->label);

            QSignalMapper *signalMapper = new QSignalMapper(this);
            signalMapper -> setMapping(actionTag, labelAsQString);

            connect(actionTag, SIGNAL(triggered()), signalMapper, SLOT(map()));
            connect(signalMapper, SIGNAL(mapped(const QString &)), this, SLOT(tagGroup(const QString &)));
        }

        menu.addSeparator();
    }

    QAction *markGood = menu.addAction(QIcon(rsrcPath + "/good.png"), "Mark Selected Group(s) as Good");
    connect(markGood, SIGNAL(triggered()), this, SLOT(markGroupGood()));

    QAction *markBad = menu.addAction(QIcon(rsrcPath + "/bad.png"), "Mark Selected Group(s) as Bad");
    connect(markBad, SIGNAL(triggered()), this, SLOT(markGroupBad()));

    QAction *unmark = menu.addAction("Unmark Selected Group(s)");
    connect(unmark, SIGNAL(triggered()), this, SLOT(unmarkSelectedGroups()));

    menu.addSeparator();

    QAction* z5 = menu.addAction("Delete This Peak Table");
    connect(z5, SIGNAL(triggered()), SLOT(deleteAll()));

    QAction *zTrain = menu.addAction(QIcon(rsrcPath + "/train.png"), "Train Model");
    connect(zTrain, SIGNAL(triggered()), traindialog, SLOT(show()));

    menu.addSeparator();

    QAction* z0 = menu.addAction("Copy to Clipboard");
    connect(z0, SIGNAL(triggered()), this ,SLOT(setClipboard()));

    QAction* z3 = menu.addAction("Align Groups");
    connect(z3, SIGNAL(triggered()), SLOT(align()));

    QAction* z4 = menu.addAction("Find Matching Compound");
    connect(z4, SIGNAL(triggered()), SLOT(findMatchingCompounds()));

    QAction* z6 = menu.addAction("Show Hidden Groups");
    connect(z6, SIGNAL(triggered()), SLOT(unhideFocusedGroups()));

    QAction* z7 = menu.addAction("Rescore fragmentation");
    connect(z7, SIGNAL(triggered()), SLOT(rescoreFragmentation()));

    QMenu analysis("Cluster Analysis");
    QAction* zz0 = analysis.addAction("Cluster Groups by Retention Time");
    connect(zz0, SIGNAL(triggered()), this ,SLOT(clusterGroups()));

    //Issue 539: Alternative approach
    QAction* zz3 = analysis.addAction("Cluster Groups by Compound");
    zz3->setCheckable(true);
    zz3->setChecked(_isGroupCompoundsTogether);
    connect(zz3, SIGNAL(toggled(bool)), SLOT(groupCompoundsTogether(bool)));
    connect(zz3, SIGNAL(toggled(bool)), SLOT(showAllGroups()));

    QAction* zz1 = analysis.addAction("Collapse All");
    connect(zz1, SIGNAL(triggered()), treeWidget,SLOT(collapseAll()));
    QAction* zz2 = analysis.addAction("Expand All");
    connect(zz2, SIGNAL(triggered()), treeWidget,SLOT(expandAll()));


    menu.addMenu(&analysis);

    menu.exec(event->globalPos());
}



void TableDockWidget::saveModel() { 
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Classification Model to a File"));
    if (fileName.isEmpty()) return;

    if(!fileName.endsWith(".model",Qt::CaseInsensitive)) fileName = fileName + ".model";

    Classifier* clsf = _mainwindow->getClassifier();
    if (clsf != NULL ) {
        clsf->saveModel(fileName.toStdString());
    }

    if (clsf) {
        vector<PeakGroup*>groups;
        for(unsigned int i=0; i < allgroups.size(); i++ )
            if(allgroups[i]->isGroupGood()|| allgroups[i]->isGroupBad())
                groups.push_back(allgroups[i]);
        clsf->saveFeatures(groups,fileName.toStdString() + ".csv");
    }
}


void TableDockWidget::findMatchingCompounds() { 
    //matching compounds
    float ppm = _mainwindow->getUserPPM();
    float ionizationMode = _mainwindow->getIonizationMode();
    for(unsigned int i=0; i < allgroups.size(); i++ ) {
        PeakGroup *g = allgroups[i];
        vector<MassCalculator::Match> matches = DB.findMatchingCompounds(g->meanMz,ppm,ionizationMode);
        foreach(auto& m, matches) {
            g->compound = m.compoundLink;
            g->adduct   = m.adductLink;
        }
    }
    updateTable();
}

void TableDockWidget::writeGroupXML(QXmlStreamWriter& stream, PeakGroup* g) { 
    if (!g)return;

    stream.writeStartElement("PeakGroup");
    stream.writeAttribute("groupId",  QString::number(g->groupId) );
    stream.writeAttribute("tagString",  QString(g->tagString.c_str()) );
    stream.writeAttribute("metaGroupId",  QString::number(g->metaGroupId) );
    stream.writeAttribute("expectedRtDiff",  QString::number(g->expectedRtDiff,'f',4) );
    stream.writeAttribute("groupRank",  QString::number(g->groupRank,'f',4) );
    stream.writeAttribute("label",  QString(g->getPeakGroupLabel().c_str()));
    stream.writeAttribute("type",  QString::number((int)g->type()));
    if(g->srmId.length())	stream.writeAttribute("srmId",  QString(g->srmId.c_str()));

    //for sample contrasts  ratio and pvalue
    if ( g->hasCompoundLink() ) {
        Compound* c = g->compound;
		stream.writeAttribute("compoundId",  QString(c->id.c_str()));
        stream.writeAttribute("compoundDB",  QString(c->db.c_str()) );
		stream.writeAttribute("compoundName",  QString(c->name.c_str()));
    }

    for(int j=0; j < g->peaks.size(); j++ ) {
        Peak& p = g->peaks[j];
        stream.writeStartElement("Peak");
        stream.writeAttribute("pos",  QString::number(p.pos));
        stream.writeAttribute("minpos",  QString::number(p.minpos));
        stream.writeAttribute("maxpos",  QString::number(p.maxpos));
        stream.writeAttribute("rt",  QString::number(p.rt,'f',4));
        stream.writeAttribute("rtmin",  QString::number(p.rtmin,'f',4));
        stream.writeAttribute("rtmax",  QString::number(p.rtmax,'f',4));
        stream.writeAttribute("mzmin",  QString::number(p.mzmin,'f',4));
        stream.writeAttribute("mzmax",  QString::number(p.mzmax,'f',4));
        stream.writeAttribute("scan",   QString::number(p.scan));
        stream.writeAttribute("minscan",   QString::number(p.minscan));
        stream.writeAttribute("maxscan",   QString::number(p.maxscan));
        stream.writeAttribute("peakArea",  QString::number(p.peakArea,'f',4));
        stream.writeAttribute("peakAreaCorrected",  QString::number(p.peakAreaCorrected,'f',4));
        stream.writeAttribute("peakAreaTop",  QString::number(p.peakAreaTop,'f',4));
        stream.writeAttribute("peakAreaFractional",  QString::number(p.peakAreaFractional,'f',4));
        stream.writeAttribute("peakRank",  QString::number(p.peakRank,'f',4));
        stream.writeAttribute("peakIntensity",  QString::number(p.peakIntensity,'f',4));

        stream.writeAttribute("peakBaseLineLevel",  QString::number(p.peakBaseLineLevel,'f',4));
        stream.writeAttribute("peakMz",  QString::number(p.peakMz,'f',4));
        stream.writeAttribute("medianMz",  QString::number(p.medianMz,'f',4));
        stream.writeAttribute("baseMz",  QString::number(p.baseMz,'f',4));
        stream.writeAttribute("quality",  QString::number(p.quality,'f',4));
        stream.writeAttribute("width",  QString::number(p.width));
        stream.writeAttribute("gaussFitSigma",  QString::number(p.gaussFitSigma,'f',4));
        stream.writeAttribute("gaussFitR2",  QString::number(p.gaussFitR2,'f',4));
        stream.writeAttribute("groupNum",  QString::number(p.groupNum));
        stream.writeAttribute("noNoiseObs",  QString::number(p.noNoiseObs));
        stream.writeAttribute("noNoiseFraction",  QString::number(p.noNoiseFraction,'f',4));
        stream.writeAttribute("symmetry",  QString::number(p.symmetry,'f',4));
        stream.writeAttribute("signalBaselineRatio",  QString::number(p.signalBaselineRatio, 'f', 4));
        stream.writeAttribute("groupOverlap",  QString::number(p.groupOverlap,'f',4));
        stream.writeAttribute("groupOverlapFrac",  QString::number(p.groupOverlapFrac,'f',4));
        stream.writeAttribute("localMaxFlag",  QString::number(p.localMaxFlag));
        stream.writeAttribute("fromBlankSample",  QString::number(p.fromBlankSample));
        stream.writeAttribute("label",  QString::number(p.label));
        stream.writeAttribute("sample",  QString(p.getSample()->sampleName.c_str()));
        stream.writeEndElement();
    }

    if ( g->childCount() ) {
        stream.writeStartElement("children");
        for(int i=0; i < g->children.size(); i++ ) {
            PeakGroup* child = &(g->children[i]);
            writeGroupXML(stream,child);
        }
        stream.writeEndElement();
    }
    stream.writeEndElement();
}

void TableDockWidget::writePeakTableXML(QXmlStreamWriter& stream) {

    if (allgroups.size() ) {
        stream.writeStartElement("PeakGroups");
        for(unsigned int i=0; i < allgroups.size(); i++ ) writeGroupXML(stream, allgroups[i]);
        stream.writeEndElement();
    }
}

void TableDockWidget::align() { 
    if ( allgroups.size() > 0 ) {
        vector<PeakGroup*> groups;
        for(unsigned int i=0; i <allgroups.size(); i++ ) groups.push_back(allgroups[i]);
        Aligner aligner;
        aligner.setMaxItterations(_mainwindow->alignmentDialog->maxItterations->value());
        aligner.setPolymialDegree(_mainwindow->alignmentDialog->polynomialDegree->value());
        aligner.doAlignment(groups);
        _mainwindow->getEicWidget()->replotForced();
        showSelectedGroup();
    }
}

PeakGroup* TableDockWidget::readGroupXML(QXmlStreamReader& xml,PeakGroup* parent) {
    PeakGroup g;
    PeakGroup* gp=nullptr;

    g.groupId = xml.attributes().value("groupId").toString().toInt();
    g.tagString = xml.attributes().value("tagString").toString().toStdString();
    g.metaGroupId = xml.attributes().value("metaGroupId").toString().toInt();
    g.expectedRtDiff = xml.attributes().value("expectedRtDiff").toString().toDouble();
    g.groupRank = xml.attributes().value("groupRank").toString().toInt();

    QString label = xml.attributes().value("label").toString();
    QByteArray bytes = label.toLatin1().data();
    for (char c : bytes) {
        g.addLabel(c);
    }

    g.setType( (PeakGroup::GroupType) xml.attributes().value("type").toString().toInt());

    string compoundId = xml.attributes().value("compoundId").toString().toStdString();
    string compoundDB = xml.attributes().value("compoundDB").toString().toStdString();
	string compoundName = xml.attributes().value("compoundName").toString().toStdString();

    string srmId = xml.attributes().value("srmId").toString().toStdString();
    if (!srmId.empty()) g.setSrmId(srmId);

	if (!compoundId.empty()){
        Compound* c = DB.findSpeciesById(compoundId, compoundDB, _mainwindow->isAttemptToLoadDB);
		if (c) g.compound = c;
	} else if (!compoundName.empty() && !compoundDB.empty()) {
        vector<Compound*>matches = DB.findSpeciesByName(compoundName, compoundDB, _mainwindow->isAttemptToLoadDB);
		if (matches.size()>0) g.compound = matches[0];
	}


    if (parent) {
        parent->addChild(g);
        if (parent->children.size() > 0 ) {
            gp = &(parent->children[ parent->children.size()-1]);
            //cerr << parent << "\t addChild() " << gp << endl;
        }
    } else {
        gp = addPeakGroup(&g,true);
        //cerr << "addParent() " << gp << endl;
    }

    return gp;
}

void TableDockWidget::readPeakXML(QXmlStreamReader& xml,PeakGroup* parent) {

    Peak p;
    p.pos = xml.attributes().value("pos").toString().toInt();
    p.minpos = xml.attributes().value("minpos").toString().toInt();
    p.maxpos = xml.attributes().value("maxpos").toString().toInt();
    p.rt = xml.attributes().value("rt").toString().toDouble();
    p.rtmin = xml.attributes().value("rtmin").toString().toDouble();
    p.rtmax = xml.attributes().value("rtmax").toString().toDouble();
    p.mzmin = xml.attributes().value("mzmin").toString().toDouble();
    p.mzmax = xml.attributes().value("mzmax").toString().toDouble();
    p.scan = xml.attributes().value("scan").toString().toInt();
    p.minscan = xml.attributes().value("minscan").toString().toInt();
    p.maxscan = xml.attributes().value("maxscan").toString().toInt();
    p.peakArea = xml.attributes().value("peakArea").toString().toDouble();
    p.peakAreaCorrected = xml.attributes().value("peakAreaCorrected").toString().toDouble();
    p.peakAreaTop = xml.attributes().value("peakAreaTop").toString().toDouble();
    p.peakAreaFractional = xml.attributes().value("peakAreaFractional").toString().toDouble();
    p.peakRank = xml.attributes().value("peakRank").toString().toDouble();
    p.peakIntensity = xml.attributes().value("peakIntensity").toString().toDouble();
    p.peakBaseLineLevel = xml.attributes().value("peakBaseLineLevel").toString().toDouble();
    p.peakMz = xml.attributes().value("peakMz").toString().toDouble();
    p.medianMz = xml.attributes().value("medianMz").toString().toDouble();
    p.baseMz = xml.attributes().value("baseMz").toString().toDouble();
    p.quality = xml.attributes().value("quality").toString().toDouble();
    p.width = xml.attributes().value("width").toString().toInt();
    p.gaussFitSigma = xml.attributes().value("gaussFitSigma").toString().toDouble();
    p.gaussFitR2 = xml.attributes().value("gaussFitR2").toString().toDouble();
    p.groupNum = xml.attributes().value("groupNum").toString().toInt();
    p.noNoiseObs = xml.attributes().value("noNoiseObs").toString().toInt();
    p.noNoiseFraction = xml.attributes().value("noNoiseFraction").toString().toDouble();
    p.symmetry = xml.attributes().value("symmetry").toString().toDouble();
    p.signalBaselineRatio = xml.attributes().value("signalBaselineRatio").toString().toDouble();
    p.groupOverlap = xml.attributes().value("groupOverlap").toString().toDouble();
    p.groupOverlapFrac = xml.attributes().value("groupOverlapFrac").toString().toDouble();
    p.localMaxFlag = xml.attributes().value("localMaxFlag").toString().toInt();
    p.fromBlankSample = xml.attributes().value("fromBlankSample").toString().toInt();
    p.label = xml.attributes().value("label").toString().toInt();
    string sampleName = xml.attributes().value("sample").toString().toStdString();
    vector<mzSample*> samples = _mainwindow->getSamples();
    for(int i=0; i< samples.size(); i++ ) {
        if ( samples[i]->sampleName == sampleName ) { p.setSample(samples[i]); break;}
    }

    //cerr << "\t\t\t" << p.getSample() << " " << p.rt << endl;
    parent->addPeak(p);
}

void TableDockWidget::savePeakTable() {

    if (allgroups.size() == 0 ) { 
        QString msg = "Peaks Table is Empty";
        QMessageBox::warning(this, tr("Error"), msg);
        return;
    }

    QString dir = ".";
    QSettings* settings = _mainwindow->getSettings();
    if ( settings->contains("lastDir") ) dir = settings->value("lastDir").value<QString>();

    QString fileName = QFileDialog::getSaveFileName(this, tr("Save to Project File"),dir,
            "Maven Project File(*.mzrollDB)");
    if (fileName.isEmpty()) return;
    if(!fileName.endsWith(".mzrollDB",Qt::CaseInsensitive)) fileName = fileName + ".mzrollDB";

    _mainwindow->getProjectWidget()->saveProjectSQLITE(fileName);

    //savePeakTable(fileName);
}

void TableDockWidget::savePeakTable(QString fileName) {
    QFile file(fileName);
    if ( !file.open(QFile::WriteOnly) ) {
        QErrorMessage errDialog(this);
        errDialog.showMessage("File open " + fileName + " failed");
        return; //error
    }

    QXmlStreamWriter stream(&file);
    stream.setAutoFormatting(true);
    writePeakTableXML(stream);
    file.close();


}

void TableDockWidget::loadPeakTable() {
    QString dir = ".";
    QSettings* settings = _mainwindow->getSettings();
    if ( settings->contains("lastDir") ) dir = settings->value("lastDir").value<QString>();
    QString selFilter;
    QStringList filters;
    filters << "Maven Project File(*.mzrollDB)"
            << "Maven XML Project File(*.mzroll)"
            << "XCMS peakTable Tab Delimited(*.tab *.csv *.txt *.tsv)";

    QString fileName = QFileDialog::getOpenFileName(this, "Load Saved Peaks", dir, filters.join(";;"), &selFilter);
    if (fileName.isEmpty()) return;

    if (selFilter == filters[0]) {
        _mainwindow->projectDockWidget->loadProjectSQLITE(fileName);
    } else if (selFilter == filters[1]) {
        loadPeakTableXML(fileName);
    } else if (selFilter == filters[2]) {
        loadCSVFile(fileName,"\t");
    }
}

void TableDockWidget::runScript() {
    QString dir = ".";
    QSettings* settings = _mainwindow->getSettings();

    treeWidget->selectAll();
    _mainwindow->getRconsoleWidget()->linkTable(this);
    _mainwindow->getRconsoleWidget()->updateStatus();
    _mainwindow->getRconsoleWidget()->show();
    _mainwindow->getRconsoleWidget()->raise();

    //find R executable
    QString Rprogram = "R.exe";
    if (settings->contains("Rprogram") ) Rprogram = settings->value("Rprogram").value<QString>();
    if (!QFile::exists( Rprogram)) { QErrorMessage dialog(this); dialog.showMessage("Can't find R executable"); return; }


}


void TableDockWidget::loadPeakTableXML(QString fileName) {

    QFile data(fileName);
    if ( !data.open(QFile::ReadOnly) ) {
        QErrorMessage errDialog(this);
        errDialog.showMessage("File open: " + fileName + " failed");
        return;
    }
    QXmlStreamReader xml(&data);

    PeakGroup* group=NULL;
    PeakGroup* parent=NULL;
    QStack<PeakGroup*>stack;

    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement()) {
            if (xml.name() == "PeakGroup") { group=readGroupXML(xml,parent); }
            if (xml.name() == "Peak" && group ) { readPeakXML(xml,group); }
            if (xml.name() == "children" && group) { stack.push(group); parent=stack.top(); }
        }

        if (xml.isEndElement()) {
            if (xml.name()=="children")  {
                if(stack.size() > 0) parent = stack.pop();
                if(parent && parent->childCount()) {
                    for(int i=0; i < parent->children.size(); i++ ) parent->children[i].groupStatistics();
                }
                if (stack.size()==0) parent=NULL;  }
            if (xml.name()=="PeakGroup") { if(group) group->groupStatistics(); group  = NULL; }
        }
    }
    for(unsigned int i=0; i < allgroups.size(); i++ ) allgroups[i]->groupStatistics();
    showAllGroups();
}

void TableDockWidget::clearClusters() {
    for(unsigned int i=0; i<allgroups.size(); i++) allgroups[i]->metaGroupId=0;
    showAllGroups();
}

void TableDockWidget::changePeakGroupDisplay() {
    cerr << "TableDockWidget::changePeakGroupDisplay(): flatten peak groups? " << clusterDialog->chkPGDisplay->isChecked() << endl;
    showAllGroups();
}

void TableDockWidget::clusterGroups() {

    double maxRtDiff =  clusterDialog->maxRtDiff_2->value();
    double minSampleCorrelation =  clusterDialog->minSampleCorr->value();
    double minRtCorrelation = clusterDialog->minRt->value();
    double ppm	= _mainwindow->getUserPPM();

    //clear cluster information
    for(unsigned int i=0; i<allgroups.size(); i++) allgroups[i]->metaGroupId=0;

    vector<mzSample*> samples = _mainwindow->getSamples();

    //run clustering
    PeakGroup::clusterGroups(allgroups,samples,maxRtDiff,minSampleCorrelation,minRtCorrelation,ppm);

    _mainwindow->setProgressBar("Clustering done!", allgroups.size(), allgroups.size());
    showAllGroups();
}

vector<vector<PeakGroup*>> TableDockWidget::clusterByCompounds() {

    qDebug() << "TableDockWidget::clusterByCompounds()";

    map<Compound*, vector<PeakGroup*>> groupsByCompound{};

    auto allCompounds = vector<vector<PeakGroup*>>();

    //organize peakgroups by common compound
    for (auto pg : allgroups) {
        if (pg->compound) {
            Compound *compound = pg->compound;
            vector<PeakGroup*> compoundGroups{};
            if (groupsByCompound.find(compound) == groupsByCompound.end()) {
                groupsByCompound.insert(make_pair(compound, compoundGroups));
            }
            groupsByCompound.at(compound).push_back(pg);
        } else {
            vector<PeakGroup*> singleton{pg};
            allCompounds.push_back(singleton);
        }
    }

    //sort compounds appropriately
    for (auto it = groupsByCompound.begin(); it != groupsByCompound.end(); ++it){
        vector<PeakGroup*> groups = it->second;
        if (groups.size() > 1) {
            sort(groups.begin(), groups.end(), [&](PeakGroup* lhs, PeakGroup* rhs){
                return (lhs->fragMatchScore.mergedScore > rhs->fragMatchScore.mergedScore);
            });
        }

        allCompounds.push_back(groups);
    }

    return allCompounds;

}

void TableDockWidget::unchildrenizeGroups() {
    qDebug() << "TableDockWidget::unchildrenizeGroups()";

     vector<PeakGroup*> updated_all_groups{};

    for (auto pg : allgroups) {
        updated_all_groups.push_back(pg);

        for (auto child : pg->children) {
            updated_all_groups.push_back(&child);
        }

        pg->children = {};
        pg->setParent(nullptr);
    }

    allgroups = updated_all_groups;

    showAllGroups();
}

void TableDockWidget::filterPeakTable() {
    updateTable();
}


void TableDockWidget::showFocusedGroups() {
    int N=treeWidget->topLevelItemCount();
    for(int i=0; i < N; i++ ) {
        QTreeWidgetItem* item = treeWidget->topLevelItem(i);
        QVariant v = item->data(0,PeakGroupType);
        PeakGroup*  group =  v.value<PeakGroup*>();
        if (group && group->isFocused) item->setHidden(false); else item->setHidden(true);
       
        if ( item->text(0).startsWith("Cluster") ) {
            bool showParentFlag=false;
            for(int j=0; j<item->childCount();j++) {
                QVariant v = (item->child(j))->data(0,PeakGroupType);
                PeakGroup*  group =  v.value<PeakGroup*>();
                if (group && group->isFocused) { item->setHidden(false); showParentFlag=true; } else item->setHidden(true);
            }
            if (showParentFlag) item->setHidden(false);
        }
    }
}

void TableDockWidget::clearFocusedGroups() {
    for(unsigned int i=0; i< allgroups.size();i++) {
        allgroups[i]->isFocused=false;
    }
}

void TableDockWidget::unhideFocusedGroups() {
    clearFocusedGroups();
    QTreeWidgetItemIterator it(treeWidget);
    while (*it) {
        (*it)->setHidden(false);
        ++it;
    }
}

void TableDockWidget::dragEnterEvent(QDragEnterEvent *event)
{
    foreach (QUrl url, event->mimeData()->urls() ) {
        std::cerr << "dragEnterEvent:" << url.toString().toStdString() << endl;
        if (url.toString() == "ok") {
            event->acceptProposedAction();
            return;
        } else {
            return;
        }
    }
}

void TableDockWidget::dropEvent(QDropEvent *event)
 {
    foreach (QUrl url, event->mimeData()->urls() ) {
         std::cerr << "dropEvent:" << url.toString().toStdString() << endl;
    }
 }

void TableDockWidget::loadSpreadsheet(QString fileName){
     qDebug() << "Loading SpreadSheet   : " << fileName;

     if( fileName.endsWith(".txt",Qt::CaseInsensitive)) {
       loadCSVFile(fileName,"\t");
    } else if( fileName.endsWith(".csv",Qt::CaseInsensitive)) {
        loadCSVFile(fileName,",");
    } else if( fileName.endsWith(".tsv",Qt::CaseInsensitive)) {
        loadCSVFile(fileName,"\t");
    } else if( fileName.endsWith(".tab",Qt::CaseInsensitive)) {
        loadCSVFile(fileName,"\t");
    }
}

int TableDockWidget::loadCSVFile(QString filename, QString sep="\t"){

    if(filename.isEmpty()) return 0;

    QFile myfile(filename);
    if(!myfile.open(QIODevice::ReadOnly | QIODevice::Text)) return 0;

    QTextStream stream(&myfile);
    if (stream.atEnd()) return 0;

    QString line;
    int lineCount=0;
    QMap<QString, int>headerMap;
    QStringList header;

    do {
         line = stream.readLine();
         if (line.isEmpty() || line[0] == '#') continue;
         QStringList fields = line.split(sep);
        lineCount++;
         if (lineCount==1) { //header line
             for(int i=0; i < fields.size(); i++ ) {
                 fields[i] = fields[i].toLower();
                 fields[i].replace("\"","");
                 headerMap[ fields[i] ] = i;
                 header << fields[i];
             }
             qDebug() << header  << endl;
         } else {
            PeakGroup* g = new PeakGroup();
            if (headerMap.contains("name")) g->tagString= fields[ headerMap["name"]].toStdString();
            if (headerMap.contains("mz"))   g->meanMz= fields[ headerMap["mz"]].toFloat();
            if (headerMap.contains("mzmed")) g->meanMz= fields[ headerMap["mzmed"]].toFloat();
            if (headerMap.contains("mzmin")) g->minMz= fields[ headerMap["mzmin"]].toFloat();
            if (headerMap.contains("mzmax")) g->maxMz= fields[ headerMap["mzmax"]].toFloat();

            if (headerMap.contains("rt")) g->meanRt= fields[ headerMap["rt"]].toFloat()/60;
            if (headerMap.contains("rtmed")) g->meanRt= fields[ headerMap["rtmed"]].toFloat()/60;
            if (headerMap.contains("rtmin")) g->minRt= fields[ headerMap["rtmin"]].toFloat()/60;
            if (headerMap.contains("rtmax")) g->maxRt= fields[ headerMap["rtmax"]].toFloat()/60;

             if (headerMap.contains("fold")) g->changeFoldRatio= fields[ headerMap["fold"]].toFloat();
             if (headerMap.contains("pvalue")) g->changePValue= fields[ headerMap["pvalue"]].toFloat();

            qDebug() << headerMap["mz"] << " " << g->meanRt;


            for(unsigned int i=14; i<header.size();i++) {
                Peak p;
                p.peakIntensity = fields[i].toInt();
                p.rt = g->meanRt; p.rtmin = g->minRt; p.rtmax=g->maxRt;
                p.peakMz = g->meanMz; p.mzmin = g->minMz; p.mzmax=g->maxMz;

                g->addPeak(p);
            }


            if (g->meanMz > 0) {
                addPeakGroup(g,false);
            }
            delete(g);
         }
     } while (!line.isNull());

    showAllGroups();
    return lineCount;
}

//Issue 285: performs a complete rebuild
void TableDockWidget::refreshPeakGroupQuant() {
    if (viewType == peakView) {
        showAllGroups();
        updateTable();
        filterTree();
    }
}

void TableDockWidget::switchTableView() {
    viewType == groupView ? viewType=peakView: viewType=groupView;
    setupPeakTable();
    showAllGroups();
    updateTable();
    filterTree(); //Issue 284
}

void TableDockWidget::filterTree() {
    filterTree(filterEditor->text());
}

void TableDockWidget::filterTree(QString needle) {

        int itemCount = treeWidget->topLevelItemCount();
        for(int i=0; i < itemCount; i++ ) {
             QTreeWidgetItem *item = treeWidget->topLevelItem(i);
                if (!item) continue;
                traverseNode(item, needle);
        }
}

bool TableDockWidget::traverseNode(QTreeWidgetItem *item, QString needle) {

    bool isThisNodePassesFilters = false;
    bool isHasPassingChild = false;

    //first check for passing children
    for (int i = 0; i < item->childCount(); ++i){
        if (traverseNode(item->child(i), needle)){
            isHasPassingChild = true;
        }
    }

    //check the node itself
    //must pass both the string and tag filters.
    if (isPassesTextFilter(item, needle)){

       QVariant v = item->data(0,PeakGroupType);
       PeakGroup* group =  v.value<PeakGroup*>();

       isThisNodePassesFilters = tagFilterState.isPeakGroupPasses(group);
    }

    //Issue 313: toggle state of this node and all children
    if (isThisNodePassesFilters) {
        traverseAndSetVisibleState(item, true);
    } else if (isHasPassingChild) {
        item->setHidden(false);
    } else {
        item->setHidden(true);
    }

    //If either this node is visible,
    //or this node has a child that is visible,
    //this node will be visible.
    return !(item->isHidden());
}

bool TableDockWidget::isPassesTextFilter(QTreeWidgetItem *item, QString needle) {
    if (!item) return false;

    // Issue 810: all valid items pass on an empty string.
    if (item && needle.isEmpty()) return true;

    bool isPassesID = item->text(getGroupViewColumnNumber("ID")).contains(needle, Qt::CaseInsensitive);
    bool isPassesAdduct = item->text(getGroupViewColumnNumber("Adduct")).contains(needle, Qt::CaseInsensitive);

    if (isPassesID || isPassesAdduct) return true;

    //Issue 510
    bool isPassesMz = item->text(getGroupViewColumnNumber("m/z")).contains(needle, Qt::CaseInsensitive);
    bool isPassesRt = item->text(getGroupViewColumnNumber("RT")).contains(needle, Qt::CaseInsensitive);

    if (isPassesMz | isPassesRt) return true;

    if (_isShowLipidSummarizationColumns) {
        bool isPassesLipidClass = item->text(getGroupViewColumnNumber("Lipid Class")).contains(needle, Qt::CaseInsensitive);
        bool isPassesSummedComposition = item->text(getGroupViewColumnNumber("Sum Composition")).contains(needle, Qt::CaseInsensitive);
        bool isPassesAcylChainLengths = item->text(getGroupViewColumnNumber("Acyl Chain Length")).contains(needle, Qt::CaseInsensitive);

        if (isPassesLipidClass || isPassesSummedComposition || isPassesAcylChainLengths) return true;
    }

    return false;
}

//Issue 313
void TableDockWidget::traverseAndSetVisibleState(QTreeWidgetItem *item, bool isVisible){

    if (!item) return;

    item->setHidden(!isVisible);
    for (int i = 0; i < item->childCount(); ++i) {
        traverseAndSetVisibleState(item->child(i), isVisible);
    }
}

void TableDockWidget::rescoreFragmentation() {
    float ppm=20.0;
    for(auto grp: allgroups) {
        if(grp->compound) {
            grp->computeFragPattern(ppm);
            if(grp->fragmentationPattern.nobs() == 0) continue;
            Compound* cpd = grp->compound;
            grp->fragMatchScore = cpd->scoreCompoundHit(&(grp->fragmentationPattern),ppm,false);
            grp->fragMatchScore.mergedScore = grp->fragMatchScore.hypergeomScore;
        }
    }
    updateTable();
}

void TableDockWidget::updateSelectedPeakGroup() {

    PeakGroup *selectedPeakGroup = getSelectedGroup();
    if (!selectedPeakGroup) return; //Probably a cluster

    editPeakGroupDialog->hide();
    string updatedVal = editPeakGroupDialog->txtUpdateID->toPlainText().toStdString();

    bool isUpdateItem = false;
    if (!updatedVal.empty()) {

        selectedPeakGroup->displayName = updatedVal;

        isUpdateItem = true;
    }

    //Issue 330: Check for updated adduct
    QString adductText = editPeakGroupDialog->cmbSelectAdduct->currentText();

    bool isChangeAdduct = (selectedPeakGroup->adduct && selectedPeakGroup->adduct->name != adductText.toStdString()) ||
            (!selectedPeakGroup->adduct && !adductText.isEmpty());

    if (isChangeAdduct) {
        if (adductText.isEmpty()) {
            selectedPeakGroup->adduct = nullptr;
        } else {
            QVariant v = editPeakGroupDialog->cmbSelectAdduct->currentData();
            Adduct* adduct =  v.value<Adduct*>();
            if (adduct) {
                selectedPeakGroup->adduct = adduct;
            }
        }
        isUpdateItem = true;
    }

    if (isUpdateItem) {

        QString oldGroupTagString = treeWidget->currentItem()->text(getGroupViewColumnNumber("ID"));
        QString updatedGroupTagString = groupTagString(selectedPeakGroup);

        if (groupIdToGroup.find(oldGroupTagString) != groupIdToGroup.end()) {

            vector<PeakGroup*>& groups = groupIdToGroup[oldGroupTagString];
            groups.erase(remove(groups.begin(), groups.end(), selectedPeakGroup), groups.end());

            if (groups.empty()){
                groupIdToGroup.erase(oldGroupTagString);
            }
        }

        if (groupIdToGroup.find(updatedGroupTagString) == groupIdToGroup.end()){
            groupIdToGroup.insert(make_pair(updatedGroupTagString, vector<PeakGroup*>()));
        }
        groupIdToGroup[updatedGroupTagString].push_back(selectedPeakGroup);

        updateHighlightedItems(oldGroupTagString, updatedGroupTagString, nullptr, nullptr);
    }

}

void TableDockWidget::showEditPeakGroupDialog() {
    editPeakGroupDialog->setPeakGroup(getSelectedGroup());
    editPeakGroupDialog->show();
}

void TableDockWidget::hideEditPeakGroupDialog() {
    editPeakGroupDialog->hide();
}

void TableDockWidget::updateTagFilter() {

    qDebug() << "TableDockWidget::updateTagFilter()";

    tagFilterState = filterTagsDialog->getFilterState();
    if (tagFilterState.isAllPass) {
        this->btnTagsFilter->setIcon(QIcon(rsrcPath + "/icon_filter.png"));
    } else {
        this->btnTagsFilter->setIcon(QIcon(rsrcPath + "/icon_filter_selected.png"));
    }

    filterTree(filterEditor->text());
}

void TableDockWidget::exportAlignmentFile() {
    qDebug() << "TableDockWidget::exportAlignmentFile()";

    QList<PeakGroup*> selectedPeakGroups = getSelectedGroups();

    if (selectedPeakGroups.size() < 3) {
        QMessageBox::information(
                    this->_mainwindow,
                    QString("Insufficient number of peak groups selected"),
                    QString("Please select at least 3 peak groups.")
                    );
        return;
    }

    QString dir = ".";
    QSettings* settings = _mainwindow->getSettings();
    if ( settings->contains("lastDir") ) dir = settings->value("lastDir").value<QString>();

    QString fileName = QFileDialog::getSaveFileName(this,
            tr("Create Alignment File from Selected Groups"), dir,
            "RT Alignment File (*.rt)");

    if (fileName.isEmpty()) return;

    vector<PeakGroup*> selectedPeakGroupVector(selectedPeakGroups.size());

    for (unsigned int i = 0; i < selectedPeakGroupVector.size(); i++) {
        selectedPeakGroupVector[i] = selectedPeakGroups[i];
    }

    Aligner rtAligner;
    vector<AnchorPointSet> anchorPoints = rtAligner
            .groupsToAnchorPoints(
                _mainwindow->samples,
                selectedPeakGroupVector,
                5,      //TODO retrieve from parameters
                0.0f    //Takes everything selected - may want to add a filter here
                );

    rtAligner.exportAlignmentFile(anchorPoints, _mainwindow->samples[0], fileName.toStdString());
}

//Issue 271: avoid nullptr issues
void TableDockWidget::disconnectCompounds(QString dbName) {
    qDebug() << "TableDockWidget::disconnectCompounds():" << dbName;
    for (auto pg : allgroups) {
        if (pg->compound && (dbName == "ALL" || pg->compound->db == dbName.toStdString())) {
            pg->compound = nullptr;
        }
    }
}


void TableDockWidget::reconnectCompounds(QString dbName) {
    qDebug() << "TableDockWidget::reconnectCompounds():" << dbName;
    for (auto pg : allgroups) {
        if (!pg->compound && !pg->compoundDb.empty() && !pg->compoundId.empty()) {
            Compound *compound = DB.findSpeciesById(pg->compoundId, pg->compoundDb, false);
            if (compound) pg->compound = compound;
        }
    }
}

//Issue 429
void TableDockWidget::setCompoundSearchSelectedCompound(){
    qDebug() << "TableDockWidget::setCompoundSearchSelectedCompound()";

    if (this->getSelectedGroups().size() != 1) return; // only works on a single group

    PeakGroup *currentGroup = this->getSelectedGroup();

    if (!currentGroup) return;

    if (_mainwindow->massCalcWidget->isHidden()) return;

    QList<QTreeWidgetItem*> selectedCompoundSearchMatches = _mainwindow->massCalcWidget->treeWidget->selectedItems();
    if (selectedCompoundSearchMatches.size() != 1) return;

    QTreeWidgetItem *currentSelectedRow = selectedCompoundSearchMatches.at(0);

    QVariant v = currentSelectedRow->data(0, CompoundType);
    Compound *compound =  v.value<Compound*>();

    QVariant v2= currentSelectedRow->data(0, AdductType);
    Adduct *adduct = v2.value<Adduct*>();

    if (!compound) return;

    if (currentGroup->compound == compound) return;

    Compound *oldCompound = currentGroup->compound;

    //Issue 438: Update compound maps
    if (oldCompound && compoundToGroup.find(oldCompound) != compoundToGroup.end()) {

        vector<PeakGroup*>& groups = compoundToGroup[oldCompound];
        groups.erase(remove(groups.begin(), groups.end(), currentGroup), groups.end());

        if (groups.empty()){
            compoundToGroup.erase(oldCompound);
        }
    }

    if (compoundToGroup.find(compound) == compoundToGroup.end()) {
        compoundToGroup.insert(make_pair(compound, vector<PeakGroup*>()));
    }
    compoundToGroup[compound].push_back(currentGroup);

    currentGroup->compound = compound;
    currentGroup->importedCompoundName = ""; // set this to empty string to allow groupTagString() to work
    currentGroup->fragMatchScore.mergedScore = -1;

    bool isChangeAdduct = adduct && currentGroup->adduct && adduct->name != currentGroup->adduct->name;
    if (isChangeAdduct) {
        currentGroup->adduct = adduct;
    }

    //update UI
    QTreeWidgetItem *item = treeWidget->selectedItems().at(0);

    QString oldGroupTagString = item->text(getGroupViewColumnNumber("ID"));

    //Issue 438: Update group ID maps
    if (groupIdToGroup.find(oldGroupTagString) != groupIdToGroup.end()) {

        vector<PeakGroup*>& groups = groupIdToGroup[oldGroupTagString];
        groups.erase(remove(groups.begin(), groups.end(), currentGroup), groups.end());

        if (groups.empty()){
            groupIdToGroup.erase(oldGroupTagString);
        }
    }

    QString updatedGroupTagString = groupTagString(currentGroup);

    if (groupIdToGroup.find(updatedGroupTagString) == groupIdToGroup.end()){
        groupIdToGroup.insert(make_pair(updatedGroupTagString, vector<PeakGroup*>()));
    }
    groupIdToGroup[updatedGroupTagString].push_back(currentGroup);

    item->setText(getGroupViewColumnNumber("ID"), updatedGroupTagString);
    item->setText(getGroupViewColumnNumber("Compound"), QString(compound->name.c_str()));

    if (compound->expectedRt) {
        item->setText(getGroupViewColumnNumber("RT Diff"),
                      QString::number(currentGroup->meanRt - compound->expectedRt, 'f', 2));//RT Diff
    }

    if (isChangeAdduct) {
        item->setText(getGroupViewColumnNumber("Adduct"), QString(adduct->name.c_str()));
    }

    if (viewType == groupView) {
        item->setText(getGroupViewColumnNumber("MS2 Score"), QString::number(-1)); //invalidate score
    }

    if (!currentGroup->isGroupLabeled(PeakGroup::ReservedLabel::COMPOUND_MANUALLY_CHANGED)) {
        tagGroup(QString(PeakGroup::ReservedLabel::COMPOUND_MANUALLY_CHANGED));
        //calls updateItem()
    } else {
        updateItem(item);
    }

    updateHighlightedItems(oldGroupTagString, updatedGroupTagString, oldCompound, compound);

}

void TableDockWidget::updateHighlightedItems(QString oldGroupTagString,  QString updatedGroupTagString, Compound *oldCompound, Compound *compound) {

    set<QTreeWidgetItem*> itemsToUpdate{};
    set<PeakGroup*> groupsToUpdate{};

    if (groupIdToGroup.find(updatedGroupTagString) != groupIdToGroup.end()) {
        vector<PeakGroup*> groupsSameTagString = groupIdToGroup[updatedGroupTagString];
        copy(groupsSameTagString.begin(), groupsSameTagString.end(), inserter(groupsToUpdate, groupsToUpdate.end()));
    }

    if (compound && compoundToGroup.find(compound) != compoundToGroup.end()) {
        vector<PeakGroup*> groupsSameCompound = compoundToGroup[compound];
        copy(groupsSameCompound.begin(), groupsSameCompound.end(), inserter(groupsToUpdate, groupsToUpdate.end()));
    }

    if (groupIdToGroup.find(oldGroupTagString) != groupIdToGroup.end()) {
        vector<PeakGroup*> groupsOldTagString = groupIdToGroup[oldGroupTagString];
        copy(groupsOldTagString.begin(), groupsOldTagString.end(), inserter(groupsToUpdate, groupsToUpdate.end()));
    }

    if (oldCompound && compoundToGroup.find(oldCompound) != compoundToGroup.end()) {
        vector<PeakGroup*> groupsOldCompound = compoundToGroup[oldCompound];
        copy(groupsOldCompound.begin(), groupsOldCompound.end(), inserter(groupsToUpdate, groupsToUpdate.end()));
    }

    for (PeakGroup *group : groupsToUpdate) {
        pair<rowIterator, rowIterator> tableRows = groupToItem.equal_range(group);
        for (rowIterator it = tableRows.first; it != tableRows.second; it++) {
             itemsToUpdate.insert(it->second);
        }
    }

    for (QTreeWidgetItem *item : itemsToUpdate) {
        updateItem(item);
    }

    updateIcons();
}

map<string, int> TableDockWidget::groupViewColumnNameToNumber{
    {"ID", 0},
    {"Compound", 1},
    {"Adduct", 2},
    {"m/z", 3},
    {"RT", 4},
    {"RT Diff", 5},
    {"MS2 Score", 6},
    {"Rank", 7},
    {"Charge", 8},
    {"Isotope #", 9},
    {"# Peaks", 10},
    {"# MS2s", 11},
    {"Max Width", 12},
    {"Max Intensity", 13},
    {"Max S/N", 14},
    {"Max Quality", 15}
};

map<string, int> TableDockWidget::groupViewColumnNameToNumberWithLipidSummarization{
    {"ID", 0},

    //Issue 506: additional columns
    {"Lipid Class", 1},
    {"Sum Composition", 2},
    {"Acyl Chain Length", 3},

    {"Compound", 4},
    {"Adduct", 5},

    {"m/z", 6},
    {"RT", 7},
    {"RT Diff", 8},
    {"MS2 Score", 9},
    {"Rank", 10},
    {"Charge", 11},
    {"Isotope #", 12},
    {"# Peaks", 13},
    {"# MS2s", 14},
    {"Max Width", 15},
    {"Max Intensity", 16},
    {"Max S/N", 17},
    {"Max Quality", 18}
};

//Issue 506: Support different columns designs
int TableDockWidget::getGroupViewColumnNumber(string key){
    if( _isShowLipidSummarizationColumns) {
        if (groupViewColumnNameToNumberWithLipidSummarization.find(key) != groupViewColumnNameToNumberWithLipidSummarization.end()) {
            return groupViewColumnNameToNumberWithLipidSummarization[key];
        }
    } else {
        if (groupViewColumnNameToNumber.find(key) != groupViewColumnNameToNumber.end()) {
            return groupViewColumnNameToNumber[key];
        }
    }

    //default to ID column to avoid exceptions
    return 0;
}

