/********************************************************************************
** Form generated from reading UI file 'mzWatcherGui.ui'
**
** Created by: Qt User Interface Compiler version 5.15.15
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MZWATCHERGUI_H
#define UI_MZWATCHERGUI_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDial>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTextBrowser>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_mzWatcherGui
{
public:
    QGridLayout *gridLayout_2;
    QTreeWidget *treeWidget;
    QTextBrowser *logWidget;
    QProgressBar *progressBar;
    QTabWidget *tabWidget;
    QWidget *tabWidgetPage1;
    QGridLayout *gridLayout;
    QLabel *label_4;
    QLineEdit *extensionEdit;
    QLineEdit *sourceFolderEdit;
    QLabel *label_5;
    QSpinBox *dayDiffBox;
    QLineEdit *commandEdit;
    QLabel *label_3;
    QLabel *label_7;
    QComboBox *destFileFormatBox;
    QPushButton *sourceFolderButton;
    QPushButton *destFolderButton;
    QCheckBox *makeCopyCheckBox;
    QLineEdit *destFolderEdit;
    QWidget *tab;
    QFormLayout *formLayout;
    QSpinBox *monitorTimeout;
    QLabel *label_6;
    QComboBox *instrumentBox;
    QPushButton *clearDatabaseButton;
    QLabel *label;
    QSpinBox *converter_waitTime;
    QLabel *label_2;
    QSpinBox *minimumsFileSize;
    QLabel *label_10;
    QLabel *label_settingsLocation;
    QLineEdit *settingsLocationEdit;
    QWidget *tab_2;
    QGridLayout *gridLayout_4;
    QLabel *label_11;
    QPushButton *addGcsSyncTargetButton;
    QScrollArea *gcsSyncTargetsScrollArea;
    QWidget *gcsSyncTargetsContainer;
    QVBoxLayout *gcsSyncTargetsContainerLayout;
    QWidget *tab_3;
    QGridLayout *gridLayout_5;
    QCheckBox *automaticWarningsCheckBox;
    QLabel *label_17;
    QLineEdit *mailerConfigFileEdit;
    QPushButton *mailerConfigFileButton;
    QLabel *mailerConfigStatusLabel;
    QLabel *label_13;
    QLineEdit *warningComputerNameEdit;
    QLabel *label_14;
    QLineEdit *warningEmailAddressesEdit;
    QLabel *label_15;
    QPushButton *addWarningSizeRuleButton;
    QWidget *warningSizeRulesContainer;
    QVBoxLayout *warningSizeRulesContainerLayout;
    QFrame *frame;
    QGridLayout *gridLayout_3;
    QPushButton *refreshButton;
    QSpacerItem *horizontalSpacer;
    QPushButton *convertButton;
    QDial *monitorDial;
    QPushButton *watchButton;

    void setupUi(QWidget *mzWatcherGui)
    {
        if (mzWatcherGui->objectName().isEmpty())
            mzWatcherGui->setObjectName(QString::fromUtf8("mzWatcherGui"));
        mzWatcherGui->resize(570, 599);
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(mzWatcherGui->sizePolicy().hasHeightForWidth());
        mzWatcherGui->setSizePolicy(sizePolicy);
        gridLayout_2 = new QGridLayout(mzWatcherGui);
        gridLayout_2->setObjectName(QString::fromUtf8("gridLayout_2"));
        treeWidget = new QTreeWidget(mzWatcherGui);
        QTreeWidgetItem *__qtreewidgetitem = new QTreeWidgetItem();
        __qtreewidgetitem->setText(0, QString::fromUtf8("Date"));
        treeWidget->setHeaderItem(__qtreewidgetitem);
        treeWidget->setObjectName(QString::fromUtf8("treeWidget"));
        QSizePolicy sizePolicy1(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(treeWidget->sizePolicy().hasHeightForWidth());
        treeWidget->setSizePolicy(sizePolicy1);
        treeWidget->setMinimumSize(QSize(0, 100));
        treeWidget->setSelectionMode(QAbstractItemView::SelectionMode::MultiSelection);
        treeWidget->setSortingEnabled(true);
        treeWidget->setAnimated(true);
        treeWidget->setHeaderHidden(false);
        treeWidget->header()->setMinimumSectionSize(5);
        treeWidget->header()->setDefaultSectionSize(100);

        gridLayout_2->addWidget(treeWidget, 2, 0, 1, 3);

        logWidget = new QTextBrowser(mzWatcherGui);
        logWidget->setObjectName(QString::fromUtf8("logWidget"));
        logWidget->setEnabled(true);
        QSizePolicy sizePolicy2(QSizePolicy::Expanding, QSizePolicy::Fixed);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(logWidget->sizePolicy().hasHeightForWidth());
        logWidget->setSizePolicy(sizePolicy2);
        logWidget->setMinimumSize(QSize(0, 0));
        logWidget->setMaximumSize(QSize(16777215, 100));
        logWidget->setAutoFillBackground(false);
        logWidget->setStyleSheet(QString::fromUtf8(""));
        logWidget->setFrameShape(QFrame::Shape::Box);
        logWidget->setFrameShadow(QFrame::Shadow::Sunken);
        logWidget->setReadOnly(true);
        logWidget->setOpenLinks(false);

        gridLayout_2->addWidget(logWidget, 3, 0, 1, 3);

        progressBar = new QProgressBar(mzWatcherGui);
        progressBar->setObjectName(QString::fromUtf8("progressBar"));
        sizePolicy.setHeightForWidth(progressBar->sizePolicy().hasHeightForWidth());
        progressBar->setSizePolicy(sizePolicy);
        progressBar->setValue(24);

        gridLayout_2->addWidget(progressBar, 4, 1, 1, 2);

        tabWidget = new QTabWidget(mzWatcherGui);
        tabWidget->setObjectName(QString::fromUtf8("tabWidget"));
        tabWidget->setEnabled(true);
        QSizePolicy sizePolicy3(QSizePolicy::Expanding, QSizePolicy::Minimum);
        sizePolicy3.setHorizontalStretch(0);
        sizePolicy3.setVerticalStretch(0);
        sizePolicy3.setHeightForWidth(tabWidget->sizePolicy().hasHeightForWidth());
        tabWidget->setSizePolicy(sizePolicy3);
        tabWidget->setDocumentMode(false);
        tabWidget->setMovable(false);
        tabWidgetPage1 = new QWidget();
        tabWidgetPage1->setObjectName(QString::fromUtf8("tabWidgetPage1"));
        gridLayout = new QGridLayout(tabWidgetPage1);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        label_4 = new QLabel(tabWidgetPage1);
        label_4->setObjectName(QString::fromUtf8("label_4"));

        gridLayout->addWidget(label_4, 4, 0, 1, 1);

        extensionEdit = new QLineEdit(tabWidgetPage1);
        extensionEdit->setObjectName(QString::fromUtf8("extensionEdit"));

        gridLayout->addWidget(extensionEdit, 4, 2, 1, 1);

        sourceFolderEdit = new QLineEdit(tabWidgetPage1);
        sourceFolderEdit->setObjectName(QString::fromUtf8("sourceFolderEdit"));
        sourceFolderEdit->setReadOnly(false);

        gridLayout->addWidget(sourceFolderEdit, 0, 2, 1, 3);

        label_5 = new QLabel(tabWidgetPage1);
        label_5->setObjectName(QString::fromUtf8("label_5"));

        gridLayout->addWidget(label_5, 4, 3, 1, 1);

        dayDiffBox = new QSpinBox(tabWidgetPage1);
        dayDiffBox->setObjectName(QString::fromUtf8("dayDiffBox"));
        dayDiffBox->setMaximum(1000000000);
        dayDiffBox->setValue(5);

        gridLayout->addWidget(dayDiffBox, 4, 4, 1, 1);

        commandEdit = new QLineEdit(tabWidgetPage1);
        commandEdit->setObjectName(QString::fromUtf8("commandEdit"));

        gridLayout->addWidget(commandEdit, 2, 2, 1, 3);

        label_3 = new QLabel(tabWidgetPage1);
        label_3->setObjectName(QString::fromUtf8("label_3"));

        gridLayout->addWidget(label_3, 2, 0, 1, 1);

        label_7 = new QLabel(tabWidgetPage1);
        label_7->setObjectName(QString::fromUtf8("label_7"));

        gridLayout->addWidget(label_7, 6, 0, 1, 1);

        destFileFormatBox = new QComboBox(tabWidgetPage1);
        destFileFormatBox->addItem(QString());
        destFileFormatBox->addItem(QString());
        destFileFormatBox->addItem(QString());
        destFileFormatBox->setObjectName(QString::fromUtf8("destFileFormatBox"));

        gridLayout->addWidget(destFileFormatBox, 6, 2, 1, 1);

        sourceFolderButton = new QPushButton(tabWidgetPage1);
        sourceFolderButton->setObjectName(QString::fromUtf8("sourceFolderButton"));

        gridLayout->addWidget(sourceFolderButton, 0, 0, 1, 1);

        destFolderButton = new QPushButton(tabWidgetPage1);
        destFolderButton->setObjectName(QString::fromUtf8("destFolderButton"));

        gridLayout->addWidget(destFolderButton, 1, 0, 1, 1);

        makeCopyCheckBox = new QCheckBox(tabWidgetPage1);
        makeCopyCheckBox->setObjectName(QString::fromUtf8("makeCopyCheckBox"));

        gridLayout->addWidget(makeCopyCheckBox, 6, 3, 1, 2);

        destFolderEdit = new QLineEdit(tabWidgetPage1);
        destFolderEdit->setObjectName(QString::fromUtf8("destFolderEdit"));
        destFolderEdit->setReadOnly(false);

        gridLayout->addWidget(destFolderEdit, 1, 2, 1, 3);

        tabWidget->addTab(tabWidgetPage1, QString());
        tab = new QWidget();
        tab->setObjectName(QString::fromUtf8("tab"));
        formLayout = new QFormLayout(tab);
        formLayout->setObjectName(QString::fromUtf8("formLayout"));
        formLayout->setFieldGrowthPolicy(QFormLayout::FieldGrowthPolicy::AllNonFixedFieldsGrow);
        monitorTimeout = new QSpinBox(tab);
        monitorTimeout->setObjectName(QString::fromUtf8("monitorTimeout"));
        monitorTimeout->setMinimum(1);
        monitorTimeout->setMaximum(999999999);
        monitorTimeout->setSingleStep(60);
        monitorTimeout->setValue(60);

        formLayout->setWidget(3, QFormLayout::FieldRole, monitorTimeout);

        label_6 = new QLabel(tab);
        label_6->setObjectName(QString::fromUtf8("label_6"));

        formLayout->setWidget(7, QFormLayout::LabelRole, label_6);

        instrumentBox = new QComboBox(tab);
        instrumentBox->addItem(QString());
        instrumentBox->addItem(QString());
        instrumentBox->addItem(QString());
        instrumentBox->addItem(QString());
        instrumentBox->setObjectName(QString::fromUtf8("instrumentBox"));
        instrumentBox->setEnabled(false);
        instrumentBox->setEditable(false);

        formLayout->setWidget(8, QFormLayout::LabelRole, instrumentBox);

        clearDatabaseButton = new QPushButton(tab);
        clearDatabaseButton->setObjectName(QString::fromUtf8("clearDatabaseButton"));

        formLayout->setWidget(8, QFormLayout::FieldRole, clearDatabaseButton);

        label = new QLabel(tab);
        label->setObjectName(QString::fromUtf8("label"));

        formLayout->setWidget(5, QFormLayout::LabelRole, label);

        converter_waitTime = new QSpinBox(tab);
        converter_waitTime->setObjectName(QString::fromUtf8("converter_waitTime"));
        converter_waitTime->setValue(5);

        formLayout->setWidget(5, QFormLayout::FieldRole, converter_waitTime);

        label_2 = new QLabel(tab);
        label_2->setObjectName(QString::fromUtf8("label_2"));

        formLayout->setWidget(6, QFormLayout::LabelRole, label_2);

        minimumsFileSize = new QSpinBox(tab);
        minimumsFileSize->setObjectName(QString::fromUtf8("minimumsFileSize"));
        minimumsFileSize->setMaximum(1000000000);
        minimumsFileSize->setValue(10);

        formLayout->setWidget(6, QFormLayout::FieldRole, minimumsFileSize);

        label_10 = new QLabel(tab);
        label_10->setObjectName(QString::fromUtf8("label_10"));

        formLayout->setWidget(3, QFormLayout::LabelRole, label_10);

        label_settingsLocation = new QLabel(tab);
        label_settingsLocation->setObjectName(QString::fromUtf8("label_settingsLocation"));

        formLayout->setWidget(9, QFormLayout::LabelRole, label_settingsLocation);

        settingsLocationEdit = new QLineEdit(tab);
        settingsLocationEdit->setObjectName(QString::fromUtf8("settingsLocationEdit"));
        settingsLocationEdit->setReadOnly(true);

        formLayout->setWidget(9, QFormLayout::FieldRole, settingsLocationEdit);

        tabWidget->addTab(tab, QString());
        tab_2 = new QWidget();
        tab_2->setObjectName(QString::fromUtf8("tab_2"));
        gridLayout_4 = new QGridLayout(tab_2);
        gridLayout_4->setObjectName(QString::fromUtf8("gridLayout_4"));
        label_11 = new QLabel(tab_2);
        label_11->setObjectName(QString::fromUtf8("label_11"));

        gridLayout_4->addWidget(label_11, 0, 0, 1, 2);

        addGcsSyncTargetButton = new QPushButton(tab_2);
        addGcsSyncTargetButton->setObjectName(QString::fromUtf8("addGcsSyncTargetButton"));

        gridLayout_4->addWidget(addGcsSyncTargetButton, 0, 2, 1, 1);

        gcsSyncTargetsScrollArea = new QScrollArea(tab_2);
        gcsSyncTargetsScrollArea->setObjectName(QString::fromUtf8("gcsSyncTargetsScrollArea"));
        gcsSyncTargetsScrollArea->setWidgetResizable(true);
        gcsSyncTargetsContainer = new QWidget();
        gcsSyncTargetsContainer->setObjectName(QString::fromUtf8("gcsSyncTargetsContainer"));
        gcsSyncTargetsContainerLayout = new QVBoxLayout(gcsSyncTargetsContainer);
        gcsSyncTargetsContainerLayout->setContentsMargins(0, 0, 0, 0);
        gcsSyncTargetsContainerLayout->setObjectName(QString::fromUtf8("gcsSyncTargetsContainerLayout"));
        gcsSyncTargetsScrollArea->setWidget(gcsSyncTargetsContainer);

        gridLayout_4->addWidget(gcsSyncTargetsScrollArea, 1, 0, 1, 3);

        tabWidget->addTab(tab_2, QString());
        tab_3 = new QWidget();
        tab_3->setObjectName(QString::fromUtf8("tab_3"));
        gridLayout_5 = new QGridLayout(tab_3);
        gridLayout_5->setObjectName(QString::fromUtf8("gridLayout_5"));
        automaticWarningsCheckBox = new QCheckBox(tab_3);
        automaticWarningsCheckBox->setObjectName(QString::fromUtf8("automaticWarningsCheckBox"));

        gridLayout_5->addWidget(automaticWarningsCheckBox, 0, 0, 1, 4);

        label_17 = new QLabel(tab_3);
        label_17->setObjectName(QString::fromUtf8("label_17"));

        gridLayout_5->addWidget(label_17, 1, 0, 1, 1);

        mailerConfigFileEdit = new QLineEdit(tab_3);
        mailerConfigFileEdit->setObjectName(QString::fromUtf8("mailerConfigFileEdit"));

        gridLayout_5->addWidget(mailerConfigFileEdit, 1, 1, 1, 2);

        mailerConfigFileButton = new QPushButton(tab_3);
        mailerConfigFileButton->setObjectName(QString::fromUtf8("mailerConfigFileButton"));

        gridLayout_5->addWidget(mailerConfigFileButton, 1, 3, 1, 1);

        mailerConfigStatusLabel = new QLabel(tab_3);
        mailerConfigStatusLabel->setObjectName(QString::fromUtf8("mailerConfigStatusLabel"));

        gridLayout_5->addWidget(mailerConfigStatusLabel, 2, 0, 1, 4);

        label_13 = new QLabel(tab_3);
        label_13->setObjectName(QString::fromUtf8("label_13"));

        gridLayout_5->addWidget(label_13, 3, 0, 1, 1);

        warningComputerNameEdit = new QLineEdit(tab_3);
        warningComputerNameEdit->setObjectName(QString::fromUtf8("warningComputerNameEdit"));

        gridLayout_5->addWidget(warningComputerNameEdit, 3, 1, 1, 3);

        label_14 = new QLabel(tab_3);
        label_14->setObjectName(QString::fromUtf8("label_14"));

        gridLayout_5->addWidget(label_14, 4, 0, 1, 1);

        warningEmailAddressesEdit = new QLineEdit(tab_3);
        warningEmailAddressesEdit->setObjectName(QString::fromUtf8("warningEmailAddressesEdit"));

        gridLayout_5->addWidget(warningEmailAddressesEdit, 4, 1, 1, 3);

        label_15 = new QLabel(tab_3);
        label_15->setObjectName(QString::fromUtf8("label_15"));

        gridLayout_5->addWidget(label_15, 5, 0, 1, 3);

        addWarningSizeRuleButton = new QPushButton(tab_3);
        addWarningSizeRuleButton->setObjectName(QString::fromUtf8("addWarningSizeRuleButton"));

        gridLayout_5->addWidget(addWarningSizeRuleButton, 5, 3, 1, 1);

        warningSizeRulesContainer = new QWidget(tab_3);
        warningSizeRulesContainer->setObjectName(QString::fromUtf8("warningSizeRulesContainer"));
        warningSizeRulesContainerLayout = new QVBoxLayout(warningSizeRulesContainer);
        warningSizeRulesContainerLayout->setContentsMargins(0, 0, 0, 0);
        warningSizeRulesContainerLayout->setObjectName(QString::fromUtf8("warningSizeRulesContainerLayout"));
        warningSizeRulesContainerLayout->setContentsMargins(0, 0, 0, 0);

        gridLayout_5->addWidget(warningSizeRulesContainer, 6, 0, 1, 4);

        tabWidget->addTab(tab_3, QString());

        gridLayout_2->addWidget(tabWidget, 1, 0, 1, 3);

        frame = new QFrame(mzWatcherGui);
        frame->setObjectName(QString::fromUtf8("frame"));
        frame->setMinimumSize(QSize(0, 36));
        frame->setFrameShape(QFrame::Shape::NoFrame);
        frame->setFrameShadow(QFrame::Shadow::Sunken);
        gridLayout_3 = new QGridLayout(frame);
        gridLayout_3->setContentsMargins(1, 1, 1, 1);
        gridLayout_3->setObjectName(QString::fromUtf8("gridLayout_3"));
        refreshButton = new QPushButton(frame);
        refreshButton->setObjectName(QString::fromUtf8("refreshButton"));
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/images/refresh.png"), QSize(), QIcon::Normal, QIcon::Off);
        refreshButton->setIcon(icon);

        gridLayout_3->addWidget(refreshButton, 0, 0, 1, 1);

        horizontalSpacer = new QSpacerItem(144, 27, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout_3->addItem(horizontalSpacer, 0, 3, 1, 1);

        convertButton = new QPushButton(frame);
        convertButton->setObjectName(QString::fromUtf8("convertButton"));
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/images/export.png"), QSize(), QIcon::Normal, QIcon::Off);
        convertButton->setIcon(icon1);

        gridLayout_3->addWidget(convertButton, 0, 1, 1, 1);

        monitorDial = new QDial(frame);
        monitorDial->setObjectName(QString::fromUtf8("monitorDial"));
        monitorDial->setEnabled(true);
        QSizePolicy sizePolicy4(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy4.setHorizontalStretch(0);
        sizePolicy4.setVerticalStretch(0);
        sizePolicy4.setHeightForWidth(monitorDial->sizePolicy().hasHeightForWidth());
        monitorDial->setSizePolicy(sizePolicy4);
        monitorDial->setMinimumSize(QSize(30, 30));
        monitorDial->setMaximumSize(QSize(30, 30));
        QPalette palette;
        QBrush brush(QColor(255, 255, 255, 255));
        brush.setStyle(Qt::SolidPattern);
        palette.setBrush(QPalette::Active, QPalette::Button, brush);
        palette.setBrush(QPalette::Inactive, QPalette::Button, brush);
        palette.setBrush(QPalette::Disabled, QPalette::Button, brush);
        monitorDial->setPalette(palette);
        monitorDial->setFocusPolicy(Qt::FocusPolicy::NoFocus);
        monitorDial->setContextMenuPolicy(Qt::ContextMenuPolicy::NoContextMenu);
        monitorDial->setAutoFillBackground(false);
        monitorDial->setMaximum(1);
        monitorDial->setPageStep(1);
        monitorDial->setTracking(false);
        monitorDial->setInvertedAppearance(false);
        monitorDial->setInvertedControls(false);
        monitorDial->setWrapping(true);
        monitorDial->setNotchesVisible(false);

        gridLayout_3->addWidget(monitorDial, 0, 4, 1, 1);

        watchButton = new QPushButton(frame);
        watchButton->setObjectName(QString::fromUtf8("watchButton"));
        QIcon icon2;
        icon2.addFile(QString::fromUtf8(":/images/train.png"), QSize(), QIcon::Normal, QIcon::Off);
        watchButton->setIcon(icon2);

        gridLayout_3->addWidget(watchButton, 0, 2, 1, 1);


        gridLayout_2->addWidget(frame, 0, 0, 1, 3);


        retranslateUi(mzWatcherGui);

        tabWidget->setCurrentIndex(0);
        destFileFormatBox->setCurrentIndex(1);


        QMetaObject::connectSlotsByName(mzWatcherGui);
    } // setupUi

    void retranslateUi(QWidget *mzWatcherGui)
    {
        mzWatcherGui->setWindowTitle(QCoreApplication::translate("mzWatcherGui", "Form", nullptr));
        QTreeWidgetItem *___qtreewidgetitem = treeWidget->headerItem();
        ___qtreewidgetitem->setText(2, QCoreApplication::translate("mzWatcherGui", "Filename", nullptr));
        ___qtreewidgetitem->setText(1, QCoreApplication::translate("mzWatcherGui", "Size Change", nullptr));
#if QT_CONFIG(tooltip)
        logWidget->setToolTip(QCoreApplication::translate("mzWatcherGui", "Process Log", nullptr));
#endif // QT_CONFIG(tooltip)
        logWidget->setDocumentTitle(QCoreApplication::translate("mzWatcherGui", "Log", nullptr));
        label_4->setText(QCoreApplication::translate("mzWatcherGui", "Extension", nullptr));
        extensionEdit->setText(QString());
        sourceFolderEdit->setText(QString());
        label_5->setText(QCoreApplication::translate("mzWatcherGui", " Skip Files Older than", nullptr));
        dayDiffBox->setSuffix(QCoreApplication::translate("mzWatcherGui", " days", nullptr));
        commandEdit->setText(QString());
        label_3->setText(QCoreApplication::translate("mzWatcherGui", "Converter Command", nullptr));
        label_7->setText(QCoreApplication::translate("mzWatcherGui", "Dest File Format", nullptr));
        destFileFormatBox->setItemText(0, QCoreApplication::translate("mzWatcherGui", "mzXML", nullptr));
        destFileFormatBox->setItemText(1, QCoreApplication::translate("mzWatcherGui", "mzML", nullptr));
        destFileFormatBox->setItemText(2, QCoreApplication::translate("mzWatcherGui", "mzData", nullptr));

        sourceFolderButton->setText(QCoreApplication::translate("mzWatcherGui", "Source Folder", nullptr));
        destFolderButton->setText(QCoreApplication::translate("mzWatcherGui", "Dest Folder", nullptr));
        makeCopyCheckBox->setText(QCoreApplication::translate("mzWatcherGui", "Make a copy of original file", nullptr));
        destFolderEdit->setText(QString());
        tabWidget->setTabText(tabWidget->indexOf(tabWidgetPage1), QCoreApplication::translate("mzWatcherGui", "File Converter Settings", nullptr));
        monitorTimeout->setSuffix(QCoreApplication::translate("mzWatcherGui", " sec", nullptr));
        monitorTimeout->setPrefix(QCoreApplication::translate("mzWatcherGui", "Interval ", nullptr));
        label_6->setText(QCoreApplication::translate("mzWatcherGui", "File Converter Presets", nullptr));
        instrumentBox->setItemText(0, QCoreApplication::translate("mzWatcherGui", "Triple Quad -  ABI 4000", nullptr));
        instrumentBox->setItemText(1, QCoreApplication::translate("mzWatcherGui", "Ion Trap - Thermo LTQ XL", nullptr));
        instrumentBox->setItemText(2, QCoreApplication::translate("mzWatcherGui", "Triple Quad - Thermo TSQ Vantage", nullptr));
        instrumentBox->setItemText(3, QCoreApplication::translate("mzWatcherGui", "Other", nullptr));

        clearDatabaseButton->setText(QCoreApplication::translate("mzWatcherGui", "Clear File Database", nullptr));
        label->setText(QCoreApplication::translate("mzWatcherGui", "Wait period before file conversion", nullptr));
        converter_waitTime->setSuffix(QCoreApplication::translate("mzWatcherGui", " min", nullptr));
        converter_waitTime->setPrefix(QString());
        label_2->setText(QCoreApplication::translate("mzWatcherGui", "Do not convert files small than", nullptr));
        minimumsFileSize->setSuffix(QCoreApplication::translate("mzWatcherGui", " bytes", nullptr));
        minimumsFileSize->setPrefix(QString());
        label_10->setText(QCoreApplication::translate("mzWatcherGui", "Monitor Folder and Automatically Convert ", nullptr));
        label_settingsLocation->setText(QCoreApplication::translate("mzWatcherGui", "Settings Location", nullptr));
#if QT_CONFIG(tooltip)
        settingsLocationEdit->setToolTip(QCoreApplication::translate("mzWatcherGui", "Where mzWatcher's saved settings live on this machine. An expert user can open this location (the Windows Registry, if this is a path starting with HKEY_CURRENT_USER) to inspect, back up, or delete these settings.", nullptr));
#endif // QT_CONFIG(tooltip)
        tabWidget->setTabText(tabWidget->indexOf(tab), QCoreApplication::translate("mzWatcherGui", "Advanced Settings", nullptr));
        label_11->setText(QCoreApplication::translate("mzWatcherGui", "GCS Sync Targets:", nullptr));
        addGcsSyncTargetButton->setText(QCoreApplication::translate("mzWatcherGui", "Add Bucket", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(tab_2), QCoreApplication::translate("mzWatcherGui", "Remote Server Updates", nullptr));
        automaticWarningsCheckBox->setText(QCoreApplication::translate("mzWatcherGui", "Enable Automatic Warnings", nullptr));
        label_17->setText(QCoreApplication::translate("mzWatcherGui", "Mailer Config File:", nullptr));
        mailerConfigFileButton->setText(QCoreApplication::translate("mzWatcherGui", "Browse...", nullptr));
        mailerConfigStatusLabel->setText(QCoreApplication::translate("mzWatcherGui", "Mail service not configured", nullptr));
        label_13->setText(QCoreApplication::translate("mzWatcherGui", "Computer Name:", nullptr));
        label_14->setText(QCoreApplication::translate("mzWatcherGui", "Recipients:", nullptr));
        label_15->setText(QCoreApplication::translate("mzWatcherGui", "Trigger Automatic Warning File Size Rules:", nullptr));
        addWarningSizeRuleButton->setText(QCoreApplication::translate("mzWatcherGui", "Add Rule", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(tab_3), QCoreApplication::translate("mzWatcherGui", "Automatic Warnings", nullptr));
        refreshButton->setText(QCoreApplication::translate("mzWatcherGui", "Refresh", nullptr));
        convertButton->setText(QCoreApplication::translate("mzWatcherGui", "Convert", nullptr));
#if QT_CONFIG(statustip)
        monitorDial->setStatusTip(QString());
#endif // QT_CONFIG(statustip)
        watchButton->setText(QCoreApplication::translate("mzWatcherGui", "Watch Folder", nullptr));
    } // retranslateUi

};

namespace Ui {
    class mzWatcherGui: public Ui_mzWatcherGui {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MZWATCHERGUI_H
