#include "tabledockwidget.h"

TableDockWidget::TableDockWidget(MainWindow* mw, QString title, int numColms) {
    setAllowedAreas(Qt::AllDockWidgetAreas);
    setFloating(false);
    _mainwindow = mw;

    setObjectName(title);
    setWindowTitle(title);
    setAcceptDrops(true);

    numColms=11;
    viewType = groupView;

    treeWidget=new QTreeWidget(this);
    treeWidget->setSortingEnabled(false);
    treeWidget->setColumnCount(numColms);
    treeWidget->setDragDropMode(QAbstractItemView::DragOnly);
    treeWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    treeWidget->setAcceptDrops(false);    
    treeWidget->setObjectName("PeakGroupTable");

    connect(treeWidget, SIGNAL(itemPressed(QTreeWidgetItem*,int)),SLOT(showSelectedGroup()));
    connect(treeWidget, SIGNAL(itemClicked(QTreeWidgetItem*,int)),SLOT(showSelectedGroup()));
    connect(treeWidget, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),SLOT(showSelectedGroup()));

    setupPeakTable();

    traindialog = new TrainDialog(this);
    connect(traindialog->saveButton,SIGNAL(clicked(bool)),SLOT(saveModel()));
    connect(traindialog->trainButton,SIGNAL(clicked(bool)),SLOT(Train()));

    clusterDialog = new ClusterDialog(this);
    clusterDialog->setWindowFlags( clusterDialog->windowFlags() | Qt::WindowStaysOnTopHint);
    connect(clusterDialog->clusterButton,SIGNAL(clicked(bool)),SLOT(clusterGroups()));
    connect(clusterDialog->clearButton,SIGNAL(clicked(bool)),SLOT(clearClusters()));
    connect(clusterDialog->chkPGDisplay, SIGNAL(clicked(bool)), SLOT(changePeakGroupDisplay()));

    editPeakGroupDialog = new EditPeakGroupDialog(this);
    connect(editPeakGroupDialog->okButton, SIGNAL(clicked(bool)), SLOT(updateSelectedPeakGroup()));
    connect(editPeakGroupDialog->cancelButton, SIGNAL(clicked(bool)), SLOT(hideEditPeakGroupDialog()));

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
    connect(exportSelected, SIGNAL(triggered()), SLOT(exportGroupsToSpreadsheet()));
    connect(exportAll, SIGNAL(triggered()), treeWidget, SLOT(selectAll()));
    connect(exportAll, SIGNAL(triggered()), SLOT(exportGroupsToSpreadsheet()));
    //connect(btnGroupCSV, SIGNAL(clicked()), SLOT(exportGroupsToSpreadsheet()));

    //QToolButton *btnHeatmap = new QToolButton(toolBar);
    //btnHeatmap->setIcon(QIcon(rsrcPath + "/heatmap.png"));
    //btnHeatmap->setToolTip("Show HeatMap");
    //connect(btnHeatmap, SIGNAL(clicked()), SLOT(showHeatMap()));

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

    QToolButton *btnTrain = new QToolButton(toolBar);
    btnTrain->setIcon(QIcon(rsrcPath + "/train.png"));
    btnTrain->setToolTip("Train Neural Net");
    connect(btnTrain,SIGNAL(clicked()),traindialog,SLOT(show()));

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

    QToolButton *btnRunScript = new QToolButton(toolBar);
    btnRunScript->setIcon(QIcon(rsrcPath + "/R.png"));
    btnRunScript->setToolTip("Run Script");
    connect(btnRunScript, SIGNAL(clicked()), SLOT(runScript()));

    /*
    QToolButton *btnMoveTo = new QToolButton(toolBar);
    btnMoveTo->setMenu(new QMenu("Move To"));
    btnMoveTo->setIcon(QIcon(rsrcPath + "/delete.png"));
    btnMoveTo->setPopupMode(QToolButton::InstantPopup);
    btnMoveTo->menu()->addAction(tr("BookMarks Table"));
    btnMoveTo->menu()->addAction(tr("Table X"));
    btnMoveTo->menu()->addAction(tr("Table Y"));
*/
    QToolButton *btnX = new QToolButton(toolBar);
    btnX->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));
    //btnX->setIcon(QIcon(rsrcPath + "/hide.png"));
    connect(btnX, SIGNAL(clicked()),this,SLOT(deleteAll()));

    QLineEdit*  filterEditor = new QLineEdit(toolBar);
    filterEditor->setMinimumWidth(15);
    filterEditor->setPlaceholderText("Filter");
    connect(filterEditor, SIGNAL(textEdited(QString)), this, SLOT(filterTree(QString)));

    QWidget* spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
    toolBar->addWidget(btnSwitchView);
    toolBar->addWidget(btnGood);
    toolBar->addWidget(btnBad);
    toolBar->addWidget(btnTrain);
    toolBar->addWidget(btnHeatmapelete);
    toolBar->addSeparator();

    //toolBar->addWidget(btnHeatmap);
    toolBar->addWidget(btnScatter);
    toolBar->addWidget(btnGallery);
    toolBar->addWidget(btnCluster);

    toolBar->addSeparator();
    toolBar->addWidget(btnPDF);
    toolBar->addWidget(btnGroupCSV);
    toolBar->addWidget(btnRunScript);

    toolBar->addSeparator();
    /*
    toolBar->addWidget(btnXML);
    toolBar->addWidget(btnLoad);
    */

   // toolBar->addWidget(btnMoveTo);
    toolBar->addWidget(spacer);
    toolBar->addWidget(filterEditor);
    toolBar->addWidget(btnX);

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

void TableDockWidget::sortBy(int col) { 
    treeWidget->sortByColumn(col,Qt::AscendingOrder);
}

void TableDockWidget::setupPeakTable() {

    QStringList colNames;
    colNames << "ID";
    colNames << "m/z";
    colNames << "rt";

    if (viewType == groupView) {
        colNames << "rtDiff";
        colNames << "MS2 Score";
        colNames << "Rank";
        colNames << "Charge";
        colNames << "Istope#";
        colNames << "#Peaks";
        colNames << "#MS2s";
        colNames << "Max Width";
        colNames << "Max Intensity";
        colNames << "Max S/N";
        colNames << "Max Quality";

    } else if (viewType == peakView) {
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
    updateStatus();
}

void TableDockWidget::updateItem(QTreeWidgetItem* item) {
    QVariant v = item->data(0,PeakGroupType);
    PeakGroup*  group =  v.value<PeakGroup*>();
    if (!group) return;
    heatmapBackground(item);

    //score peak quality
    Classifier* clsf = _mainwindow->getClassifier();
    if (clsf != NULL) {
        clsf->classify(group);
        group->updateQuality();
        item->setText(0,groupTagString(group));
    }

    int good=0; int bad=0;
    int total=group->peakCount();
    for(int i=0; i < group->peakCount(); i++ ) {
        group->peaks[i].quality > 0.5 ? good++ : bad++;
    }

    //TODO: Fix this block when code gets removed

    QBrush brush=Qt::NoBrush;
    if (good>0 && group->label == 'b' ) {
        float incorrectFraction= ((float) good)/total;
        brush  = QBrush(QColor::fromRgbF(0.8,0,0,incorrectFraction));
    } else if(bad>0 && group->label == 'g') {
        float incorrectFraction= ((float) bad)/total;
        brush  = QBrush(QColor::fromRgbF(0.8,0,0,incorrectFraction));
    }
    item->setBackground(0,brush);

    if (group->label == 'g' ) {
        item->setIcon(0,QIcon(":/images/good.png"));
    } else if (group->label == 'b' ) {
        item->setIcon(0,QIcon(":/images/bad.png"));
    } else {
        item->setIcon(0, QIcon()); //empty icon
    }

    //Issue 127: gather all icons
    vector<QIcon> icons;
    for (char c : group->labels) {

        if (c == 'g') {
            icons.push_back(QIcon(":/images/good.png"));
        } else if (c == 'b') {
            icons.push_back(QIcon(":/images/bad.png"));
        } else if (DB.peakGroupTags.find(c) != DB.peakGroupTags.end()) {
            icons.push_back(DB.peakGroupTags[c]->icon);
        }

    }

    if (icons.empty()) {
        item->setIcon(0, QIcon());
    } else if (icons.size() == 1) {
        item->setIcon(0, icons.at(0));
    } else {

        QRect rect = treeWidget->visualItemRect(item);

        int imgWidth = 0;
        int imgHeight = rect.height();

        //Issue 127: fall back to height of images if height could not be determined from rectangle
        if (imgHeight == 0) {
            for (auto &x : icons) {
                if (x.availableSizes().first().height() > imgHeight){
                    imgHeight = x.availableSizes().first().height();
                }
            }
        }

        vector<int> heightOffsets(icons.size());

        for (unsigned int i = 0; i < icons.size(); i++) {

            QIcon x = icons[i];

            int width = x.availableSizes().first().width();
            imgWidth += width;

            int height = x.availableSizes().first().height();
            int heightOffset = (imgHeight - height)/2;

            if (heightOffset > 0) {
                heightOffsets[i] = heightOffset;
            }  else {
                heightOffsets[i] = 0;
            }
        }

        QPixmap comboPixmap(imgWidth, imgHeight);
        QPainter painter(&comboPixmap);

        //always set a white background color, otherwise the images are hard to see
        painter.fillRect(0, 0, imgWidth, imgHeight, Qt::white);

        int leftEdge = 0;
        for (unsigned int i = 0; i < icons.size(); i++) {

            int heightCoord = heightOffsets[i];
            QIcon x = icons[i];

            painter.drawPixmap(leftEdge, heightCoord, x.pixmap(x.availableSizes().first()));

            leftEdge += x.availableSizes().first().width();
        }

        QIcon icon;
        icon.addPixmap(comboPixmap);

        item->setIcon(0, icon);

        QSize iconSize = treeWidget->iconSize();

        //enlarge sizes as needed
        if (!icon.availableSizes().isEmpty() && icon.availableSizes().first().width() > iconSize.width()){
            treeWidget->setIconSize(icon.availableSizes().first());
        }
    }

}

void TableDockWidget::heatmapBackground(QTreeWidgetItem* item) {
    if(viewType != peakView) return;

    int firstColumn=3;
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

QString TableDockWidget::groupTagString(PeakGroup* group){
    if (!group) return QString();

    if (!group->displayName.empty()) {
        return QString(group->displayName.c_str());
    }

    QStringList parts;

    if (group->compound) {
        parts << group->compound->name.c_str();

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
/*
QString TableDockWidget::groupTagString(PeakGroup* group){ 
    if (!group) return QString();
    QString tag(group->tagString.c_str());
    if (group->compound) tag = QString(group->compound->name.c_str());
    if (! group->tagString.empty()) tag += " | " + QString(group->tagString.c_str());
    if (! group->srmId.empty()) tag +=  " | " + QString(group->srmId.c_str());
    if ( tag.isEmpty() ) tag = QString::number(group->groupId);
    return tag;
}
*/

void TableDockWidget::addRow(PeakGroup* group, QTreeWidgetItem* root) {
    if(!group) return;

    group->groupStatistics();
    //cerr << "addRow" << group->groupId << " "  << group->meanMz << " " << group->meanRt << " " << group->children.size() << " " << group->tagString << endl;

    if (!group) return;
    if (group->peakCount() == 0 ) return;
    if (group->meanMz <= 0 ) return;
    if (group->deletedFlag || group->label == 'x') return; //deleted group

    NumericTreeWidgetItem *item = nullptr;
    if(!root) {
       item = new NumericTreeWidgetItem(treeWidget,PeakGroupType);
    } else {
       item = new NumericTreeWidgetItem(root,PeakGroupType);
    }

    item->setFlags(Qt::ItemIsSelectable |  Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);

    item->setData(0,PeakGroupType,QVariant::fromValue(group));
    item->setText(0,groupTagString(group));
    item->setText(1,QString::number(group->meanMz, 'f', 4));
    item->setText(2,QString::number(group->meanRt, 'f', 2));

    if (group->compound and group->compound->expectedRt) {
        item->setText(3,QString::number(group->meanRt - group->compound->expectedRt, 'f', 2));
    }

    if (group->label == 'g' ) item->setIcon(0,QIcon(":/images/good.png"));
    if (group->label == 'b' ) item->setIcon(0,QIcon(":/images/bad.png"));

    if (viewType == groupView) {
        item->setText(4,QString::number(group->fragMatchScore.mergedScore));
        item->setText(5,QString::number(group->groupRank,'f',3));
        item->setText(6,QString::number(group->chargeState));
        item->setText(7,QString::number(group->isotopicIndex));
        item->setText(8,QString::number(group->peakCount()));
        item->setText(9,QString::number(group->ms2EventCount));
        item->setText(10,QString::number(group->maxNoNoiseObs));
        item->setText(11,QString::number(group->maxIntensity,'g',2));
        item->setText(12,QString::number(group->maxSignalBaselineRatio,'f',0));
        item->setText(13,QString::number(group->maxQuality,'f',2));
    } else if ( viewType == peakView) {
        vector<mzSample*> vsamples = _mainwindow->getVisibleSamples();
        sort(vsamples.begin(), vsamples.end(), mzSample::compSampleOrder);
        vector<float>yvalues = group->getOrderedIntensityVector(vsamples,_mainwindow->getUserQuantType());
        for(unsigned int i=0; i<yvalues.size(); i++ ) {
         item->setText(3+i,QString::number(yvalues[i]));
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
    for(int i=0; i < allgroups.size(); i++ ) {
        if ( &allgroups[i] == group ) return true;
        if ((double) std::abs(group->meanMz - allgroups[i].meanMz) < 1e-5 && (double)
            std::abs(group->meanRt-allgroups[i].meanRt) < 1e-5) {
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
         item->setSelected(true);
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

    allgroups.push_back(*group);

    if (isDeletePeakGroupPtr){
       delete(group);
    }

    PeakGroup* g = &allgroups[allgroups.size()-1];

    if (updateTable)  showAllGroups();
    return g;
}

void TableDockWidget::addDirectInfusionAnnotation(DirectInfusionGroupAnnotation *directInfusionGroupAnnotation, int clusterNum) {

    for (auto directInfusionMatchData : directInfusionGroupAnnotation->compounds){

        Compound* compound = directInfusionMatchData->compound;
        Adduct* adduct = directInfusionMatchData->adduct;

        float theoMz = adduct->computeAdductMass(MassCalculator::computeNeutralMass(compound->getFormula()));

        PeakGroup pg;
        pg.metaGroupId = clusterNum;

        unsigned int peakCounter = 1;
        float maxRt = 0;

        for (auto pair : directInfusionGroupAnnotation->annotationBySample) {

            mzSample *sample = pair.first;
            DirectInfusionAnnotation *directInfusionAnnotation = pair.second;

            Peak p;
            p.scan = directInfusionAnnotation->scan->scannum;
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
            p.peakIntensity = 0; //TODO

            pg.addPeak(p);

            peakCounter++;

            if (sample->maxRt > maxRt) maxRt = sample->maxRt;

        }

        pg.compound = compound;
        pg.adduct = adduct;
        pg.minMz = directInfusionGroupAnnotation->precMzMin;
        pg.maxMz = directInfusionGroupAnnotation->precMzMax;
        pg.meanMz = theoMz;
        pg.minRt = 0;
        pg.maxRt = maxRt;

        pg.fragmentationPattern = directInfusionGroupAnnotation->fragmentationPattern->consensus;

        pg.fragMatchScore = directInfusionMatchData->fragmentationMatchScore;
        pg.fragMatchScore.mergedScore = pg.fragMatchScore.numMatches;

        pg.groupRank = directInfusionMatchData->proportion;

        allgroups.push_back(pg);
    }

     directInfusionGroupAnnotation->clean();
     if (directInfusionGroupAnnotation->fragmentationPattern) delete(directInfusionGroupAnnotation->fragmentationPattern);
     if (directInfusionGroupAnnotation) delete(directInfusionGroupAnnotation);
}

QList<PeakGroup*> TableDockWidget::getGroups() {
    QList<PeakGroup*> groups;
    for(int i=0; i < allgroups.size(); i++ ) {
        groups.push_back(&allgroups[i]);
    }
    return groups;
}

void TableDockWidget::deleteAll() {

    if (QMessageBox::No == QMessageBox::question(this, "Delete Confirmation",
          "Do you want to delete all peakgroups from this table?", QMessageBox::Yes | QMessageBox::No)) {
        return;
    }

    treeWidget->clear();
    allgroups.clear();
    groupToItem.clear();
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

    treeWidget->clear();
    groupToItem.clear();    //addRow() calls will refill the map

    if (allgroups.size() == 0 ) return;

    treeWidget->setSortingEnabled(false);

    QMap<int,QTreeWidgetItem*> parents;
    for(int i=0; i < allgroups.size(); i++ ) { 
        int metaGroupId  = allgroups[i].metaGroupId;
        if (metaGroupId && allgroups[i].meanMz > 0 && allgroups[i].peakCount()>0) {

            QTreeWidgetItem* parent = nullptr;
            if (!parents.contains(metaGroupId)) {
                if (windowTitle() != "Bookmarks") {
                    parents[metaGroupId]= new QTreeWidgetItem(treeWidget);
                    parents[metaGroupId]->setText(0,QString("Cluster ") + QString::number(metaGroupId));
                    parents[metaGroupId]->setText(3,QString::number(allgroups[i].meanRt,'f',2));
                    parents[metaGroupId]->setExpanded(true);
                    parent = parents[metaGroupId];
                }
            } else {
                parent = parents[metaGroupId];
            }

            addRow(&allgroups[i], parent); 
        } else {
            addRow(&allgroups[i], nullptr);
        }
    }

    QScrollBar* vScroll = treeWidget->verticalScrollBar();
    if ( vScroll ) {
        vScroll->setSliderPosition(vScroll->maximum());
    }
    treeWidget->setSortingEnabled(true);

    treeWidget->expandAll();

    if (this->windowTitle() == "rumsDB") {
        treeWidget->sortByColumn(4,Qt::DescendingOrder); //decreasing by score
    }

    updateStatus();

    /*
	if (allgroups.size() > 0 ) { //select last item
		treeWidget->setCurrentItem(treeWidget->topLevelItem(allgroups.size()-1));
	}
	(*/
}

void TableDockWidget::exportGroupsToSpreadsheet() {

    if (allgroups.size() == 0 ) {
        QString msg = "Peaks Table is Empty";
        QMessageBox::warning(this, tr("Error"), msg);
        return;
    }

    QString dir = ".";
    QSettings* settings = _mainwindow->getSettings();

    if ( settings->contains("lastDir") ) dir = settings->value("lastDir").value<QString>();

    QString groupsTAB = "Groups Summary Matrix Format (*.tab)";
    QString groupsCSV = "Groups Summary Matrix Format Comma Delimited (*.csv)";
    QString peaksTAB =  "Peaks Detailed Format (*.tab)";
    QString peaksCSV =  "Peaks Detailed Format Comma Delimited  (*.csv)";

    QString sFilterSel;
    QString fileName = QFileDialog::getSaveFileName(this, 
            tr("Export Groups"), dir, 
            groupsTAB + ";;" + peaksTAB + ";;" + groupsCSV + ";;" + peaksCSV,
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


    qDebug() << "Writing report to " << fileName;
    for(int i=0; i<allgroups.size(); i++ ) {
        PeakGroup& group = allgroups[i];

        //check that group is currently selected
        bool isSelected = false;
        PeakGroup *peakGroupPtr = &(group);
        pair<rowIterator, rowIterator> tableRows = groupToItem.equal_range(peakGroupPtr);
        for (rowIterator it = tableRows.first; it != tableRows.second; it++) {
            if (it->second && it->second->isSelected()) {
                isSelected = true;
                break;
            }
        }

        if (isSelected && !group.deletedFlag) { //don't add deleted groups
            csvreports->addGroup(&group);
        }

    }
    csvreports->closeFiles();
}


void TableDockWidget::showSelectedGroup() {
    //sortBy(treeWidget->header()->sortIndicatorSection());

    QTreeWidgetItem *item = treeWidget->currentItem();
    if (!item) return;
    if (item->type() != PeakGroupType) return;

    QVariant v = item->data(0,PeakGroupType);
    PeakGroup*  group =  v.value<PeakGroup*>();

    if ( group != NULL  and lastSelectedGroup != group) {
        lastSelectedGroup = group;
        cerr << "showSelectedGroup: group" << group->groupId << " " << group->peaks.size() << " " << group->children.size() << endl;
        _mainwindow->setPeakGroup(group);
        _mainwindow->rconsoleDockWidget->updateStatus();
    }

    /*
    if ( item->childCount() > 0 ) {
        vector<PeakGroup*>children;
        for(int i=0; i < item->childCount(); i++ ) {
            QTreeWidgetItem* child = item->child(i);
            QVariant data = child->data(0,Qt::UserRole);
            PeakGroup*  group =  data.value<PeakGroup*>();
            if(group) children.push_back(group);
        }

        //if (children.size() > 0) {
         //   if (_mainwindow->galleryWidget->isVisible() ) {
          //      _mainwindow->galleryWidget->clear();
           //     _mainwindow->galleryWidget->addEicPlots(children);
        //    }
        }
    }
    */
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

                if (label == '\0') {
                    group->labels.clear();
                } else {
                    //check vector of all labels
                    bool isLabelInVector = false;
                    for (auto &x : group->labels) {
                        if (x == label) {
                            isLabelInVector = true;
                            break;
                        }
                    }

                    if (isLabelInVector) {
                        //erase-remove idiom
                        group->labels.erase(remove(group->labels.begin(), group->labels.end(), label), group->labels.end());
                    } else {
                        group->labels.push_back(label);
                    }

                    //peak groups can only be labled as good or bad, not both
                    if (label == 'b') {
                        group->labels.erase(remove(group->labels.begin(), group->labels.end(), 'g'), group->labels.end());
                    } else if (label == 'g') {
                        group->labels.erase(remove(group->labels.begin(), group->labels.end(), 'b'), group->labels.end());
                    }
                }

                if (group->label != label) {
                    group->setLabel(label);
                } else {
                    group->setLabel(0); //Issue 125: if group already labeled with this label, return to unlabeled state
                }
            }
            updateItem(item);
        }
    }
    updateStatus();
}

//mark groups to be deleted as 'x'
void TableDockWidget::traverseAndDeleteGroups(QTreeWidgetItem *item) {

    QVariant v = item->data(0,PeakGroupType);
    PeakGroup*  group =  v.value<PeakGroup*>();

    if (group) {
        group->deletedFlag = true;
        group->setLabel('x');
        for (int i = 0; i < item->childCount(); ++i){
            traverseAndDeleteGroups(item->child(i));
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
    if (treeWidget->selectedItems().size() == 1) showNextGroup();
}

void TableDockWidget::markGroupGood() { 
    setGroupLabel('g');
    if (treeWidget->selectedItems().size() == 1) showNextGroup();
}

void TableDockWidget::markGroupBad() { 
    setGroupLabel('b');
    if (treeWidget->selectedItems().size() == 1) showNextGroup();
}

void TableDockWidget::unmarkSelectedGroups() {
    setGroupLabel('\0');
    if (treeWidget->selectedItems().size() == 1) showNextGroup();
}

void TableDockWidget::showLastGroup() {
    QTreeWidgetItem *item= treeWidget->currentItem();
    if ( item != NULL )  {
        treeWidget->setCurrentItem(treeWidget->itemAbove(item));
    }
}

PeakGroup* TableDockWidget::getLastBookmarkedGroup() {
    if (allgroups.size() > 0) {
        return &(allgroups.back());
    } else {
        return 0;
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
        PeakGroup* grp = &allgroups[i];
        if (grp->label == 'g') good_groups.push_back(grp);
        if (grp->label == 'b') bad_groups.push_back(grp);
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
            if (treeWidget->selectedItems().size() == 1) showNextGroup();

            break; // found the key, no need to continue searching through tag list
        }

    }

    QDockWidget::keyPressEvent(e);
    updateStatus();
}

void TableDockWidget::updateStatus() {

    int totalCount=0;
    int goodCount=0;
    int badCount=0;
    for(int i=0; i < allgroups.size(); i++ ) {
        char groupLabel = allgroups[i].label;
        if (groupLabel == 'g' ) {
            goodCount++;
        } else if ( groupLabel == 'b' ) {
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

float TableDockWidget::showAccuracy(vector<PeakGroup*>&groups) {
    //check accuracy
    if ( groups.size() == 0 ) return 0;

    int fp=0; int fn=0; int tp=0; int tn=0;  int total=0; float accuracy=0;
    int gc=0;
    int bc=0;
    for(int i=0; i < groups.size(); i++ ) {
        if (groups[i]->label == 'g' || groups[i]->label == 'b' ) {
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

    if (windowTitle() == "Bookmarks") {
        QAction* z8 = menu.addAction("Edit Selected Peak Group ID");
        connect(z8, SIGNAL(triggered()), SLOT(showEditPeakGroupDialog()));
        menu.addSeparator();
    }

    if (!DB.peakGroupTags.empty()) {

        for (auto &x : DB.peakGroupTags) {
            PeakGroupTag *peakGroupTag = x.second;

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

    QAction *markGood = menu.addAction(QIcon(":/images/good.png"), "Mark Selected Group(s) as Good");
    connect(markGood, SIGNAL(triggered()), this, SLOT(markGroupGood()));

    QAction *markBad = menu.addAction(QIcon(":/images/bad.png"), "Mark Selected Group(s) as Bad");
    connect(markBad, SIGNAL(triggered()), this, SLOT(markGroupBad()));

    QAction *unmark = menu.addAction("Unmark Selected Group(s)");
    connect(unmark, SIGNAL(triggered()), this, SLOT(unmarkSelectedGroups()));

    menu.addSeparator();

    QAction* z0 = menu.addAction("Copy to Clipboard");
    connect(z0, SIGNAL(triggered()), this ,SLOT(setClipboard()));

    QAction* z3 = menu.addAction("Align Groups");
    connect(z3, SIGNAL(triggered()), SLOT(align()));

    QAction* z4 = menu.addAction("Find Matching Compound");
    connect(z4, SIGNAL(triggered()), SLOT(findMatchingCompounds()));

    QAction* z5 = menu.addAction("Delete All Groups");
    connect(z5, SIGNAL(triggered()), SLOT(deleteAll()));

    QAction* z6 = menu.addAction("Show Hidden Groups");
    connect(z6, SIGNAL(triggered()), SLOT(unhideFocusedGroups()));

    QAction* z7 = menu.addAction("Rescore fragmentation");
    connect(z7, SIGNAL(triggered()), SLOT(rescoreFragmentation()));

    QMenu analysis("Cluster Analysis");
    QAction* zz0 = analysis.addAction("Cluster Groups by Retention Time");
    connect(zz0, SIGNAL(triggered()), this ,SLOT(clusterGroups()));
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
        for(int i=0; i < allgroups.size(); i++ )
            if(allgroups[i].label == 'g' || allgroups[i].label == 'b' )
                groups.push_back(&allgroups[i]);
        clsf->saveFeatures(groups,fileName.toStdString() + ".csv");
    }
}


void TableDockWidget::findMatchingCompounds() { 
    //matching compounds
    float ppm = _mainwindow->getUserPPM();
    float ionizationMode = _mainwindow->getIonizationMode();
    for(int i=0; i < allgroups.size(); i++ ) {
        PeakGroup& g = allgroups[i];
        vector<MassCalculator::Match> matches = DB.findMatchingCompounds(g.meanMz,ppm,ionizationMode);
        foreach(auto& m, matches) {
            g.compound = m.compoundLink;
            g.adduct   = m.adductLink;
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
    stream.writeAttribute("label",  QString::number(g->label ));
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
        for(int i=0; i < allgroups.size(); i++ ) writeGroupXML(stream,&allgroups[i]);
        stream.writeEndElement();
    }
}

void TableDockWidget::align() { 
    if ( allgroups.size() > 0 ) {
        vector<PeakGroup*> groups;
        for(int i=0; i <allgroups.size(); i++ ) groups.push_back(&allgroups[i]);
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
    PeakGroup* gp=NULL;

    g.groupId = xml.attributes().value("groupId").toString().toInt();
    g.tagString = xml.attributes().value("tagString").toString().toStdString();
    g.metaGroupId = xml.attributes().value("metaGroupId").toString().toInt();
    g.expectedRtDiff = xml.attributes().value("expectedRtDiff").toString().toDouble();
    g.groupRank = xml.attributes().value("groupRank").toString().toInt();
    g.label     =  xml.attributes().value("label").toString().toInt();
    g.setType( (PeakGroup::GroupType) xml.attributes().value("type").toString().toInt());

    string compoundId = xml.attributes().value("compoundId").toString().toStdString();
    string compoundDB = xml.attributes().value("compoundDB").toString().toStdString();
	string compoundName = xml.attributes().value("compoundName").toString().toStdString();

    string srmId = xml.attributes().value("srmId").toString().toStdString();
    if (!srmId.empty()) g.setSrmId(srmId);

	if (!compoundId.empty()){
        Compound* c = DB.findSpeciesById(compoundId,compoundDB);
		if (c) g.compound = c;
	} else if (!compoundName.empty() && !compoundDB.empty()) {
		vector<Compound*>matches = DB.findSpeciesByName(compoundName,compoundDB);
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
    for(int i=0; i < allgroups.size(); i++ ) allgroups[i].groupStatistics();
    showAllGroups();
}

void TableDockWidget::clearClusters() {
    for(unsigned int i=0; i<allgroups.size(); i++) allgroups[i].metaGroupId=0;
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
    for(unsigned int i=0; i<allgroups.size(); i++) allgroups[i].metaGroupId=0;

    vector<mzSample*> samples = _mainwindow->getSamples();

    //run clustering
    PeakGroup::clusterGroups(allgroups,samples,maxRtDiff,minSampleCorrelation,minRtCorrelation,ppm);

    _mainwindow->setProgressBar("Clustering., done!",allgroups.size(),allgroups.size());
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
    for(int i=0; i< allgroups.size();i++) {
        allgroups[i].isFocused=false;
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

void TableDockWidget::switchTableView() {
    viewType == groupView ? viewType=peakView: viewType=groupView;
    setupPeakTable();
    showAllGroups();
    updateTable();
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

    bool isShowThisNode = false;
    for (int i = 0; i < item->childCount(); ++i){
        if (traverseNode(item->child(i), needle)){
            isShowThisNode = true;
        }
    }

    if (isShowThisNode || item->text(0).contains(needle, Qt::CaseInsensitive)) {
        item->setHidden(false);
    } else {
        item->setHidden(true);
    }

    return !(item->isHidden());
}

void TableDockWidget::rescoreFragmentation() {
    float ppm=20.0;
    for(PeakGroup& grp: allgroups) {
        if(grp.compound) {
            grp.computeFragPattern(ppm);
            if(grp.fragmentationPattern.nobs() == 0) continue;
            Compound* cpd = grp.compound;
            grp.fragMatchScore = cpd->scoreCompoundHit(&grp.fragmentationPattern,ppm,false);
            grp.fragMatchScore.mergedScore = grp.fragMatchScore.hypergeomScore;
        }
    }
    updateTable();
}

void TableDockWidget::updateSelectedPeakGroup() {

    editPeakGroupDialog->hide();
    string updatedVal = editPeakGroupDialog->txtUpdateID->toPlainText().toStdString();

    if (!updatedVal.empty()) {

        PeakGroup *selectedPeakGroup = getSelectedGroup();
        if (!selectedPeakGroup) return; //Probably a cluster

        selectedPeakGroup->displayName = updatedVal;

        updateItem(treeWidget->currentItem());
    }

}

void TableDockWidget::showEditPeakGroupDialog() {

    //TODO: may want to handle bulk renaming
    //QList<PeakGroup*> selected = getSelectedGroups();

    PeakGroup *selectedPeakGroup = getSelectedGroup();
    if (!selectedPeakGroup) return; //Probably a cluster

    vector<string> suggestionSet;
    QString suggestionString = QString();

    QString compoundString = QString();
    if (selectedPeakGroup->compound){
        compoundString.append(selectedPeakGroup->compound->name.c_str());
    }

    if (compoundString.isEmpty() && _mainwindow->getProjectWidget()->currentProject){
        //try to get original compound string from matches (mzkitchen workflow)

        if (_mainwindow->getProjectWidget()->currentProject->topMatch.find(selectedPeakGroup->groupId)
                != _mainwindow->getProjectWidget()->currentProject->topMatch.end()){

            shared_ptr<mzrollDBMatch> originalCompoundNameAndScore = _mainwindow->getProjectWidget()->currentProject->topMatch.at(selectedPeakGroup->groupId);

            compoundString.append(originalCompoundNameAndScore->compoundName.c_str());
        }

    }

    editPeakGroupDialog->brsSuggestions->setText("");

    if (!compoundString.isEmpty()){

        //try to summarize lipids based on suggestions
        QString strippedCompoundName = compoundString.section(' ', 0, 0);

        string baseName = strippedCompoundName.toStdString();
        string acylChainLengthSummarized = LipidSummarizationUtils::getAcylChainLengthSummary(baseName);
        string acylChainCompositionSummarized = LipidSummarizationUtils::getAcylChainCompositionSummary(baseName);
        string lipidClassSummarized = LipidSummarizationUtils::getLipidClassSummary(baseName);

        suggestionSet.push_back(baseName);

        if (std::find(suggestionSet.begin(), suggestionSet.end(), acylChainLengthSummarized) == suggestionSet.end()) {
            suggestionSet.push_back(acylChainLengthSummarized);
        }

        if (std::find(suggestionSet.begin(), suggestionSet.end(), acylChainCompositionSummarized) == suggestionSet.end()) {
            suggestionSet.push_back(acylChainCompositionSummarized);
        }

        if (std::find(suggestionSet.begin(), suggestionSet.end(), lipidClassSummarized) == suggestionSet.end()) {
            suggestionSet.push_back(lipidClassSummarized);
        }

        for (unsigned int i = 0; i < suggestionSet.size(); i++) {

            QString hyperLink =
                    QString::fromStdString("<a href = \"") +
                    QString::fromStdString(suggestionSet.at(i).c_str()) +
                    QString::fromStdString("\" >") +
                    QString::fromStdString(suggestionSet.at(i).c_str()) +
                    QString::fromStdString("</a>");

            editPeakGroupDialog->brsSuggestions->append(hyperLink);

        }
    }

    QString adductString = QString();
    if (selectedPeakGroup->adduct){
        adductString.append(selectedPeakGroup->adduct->name.c_str());
    }

    editPeakGroupDialog->brsPreviousID->setText(groupTagString(selectedPeakGroup));
    editPeakGroupDialog->brsCompound->setText(compoundString);
    editPeakGroupDialog->brsAdduct->setText(adductString);
    editPeakGroupDialog->brsMz->setText(QString::number(selectedPeakGroup->meanMz, 'f', 4));
    editPeakGroupDialog->brsRT->setText(QString::number(selectedPeakGroup->meanRt, 'f', 2));
    editPeakGroupDialog->txtUpdateID->setText(QString());

    editPeakGroupDialog->show();
}

void TableDockWidget::hideEditPeakGroupDialog() {
    editPeakGroupDialog->hide();
}

