#ifndef  BODE_INCLUDED
#define  BODE_INCLUDED

#include "stable.h"
#include "globals.h"

#include "scatterplot.h"
#include "eicwidget.h"
#include "settingsform.h"
#include "spectrawidget.h"
#include "masscalcgui.h"
#include "ligandwidget.h"
#include "isotopeswidget.h"
#include "treedockwidget.h"
#include "tabledockwidget.h"
#include "peakdetectiondialog.h"
#include "directinfusiondialog.h"
#include "selectadductsdialog.h"
#include "alignmentdialog.h"
#include "calibratedialog.h"
//#include "rconsoledialog.h"
#include "background_peaks_update.h"
#include "heatmap.h"
#include "note.h"
#include "history.h"
#include "suggest.h"
#include "gallerywidget.h"
#include "projectdockwidget.h"
#include "spectramatching.h"
#include "mzfileio.h"
#include "spectralhit.h"
#include "rconsolewidget.h"
#include "librarydialog.h"
#include "setrumsdbdialog.h"
#include "samplebarplotwidget.h"
#include "Peptide.h"

class SettingsForm;
class EicWidget;
class PlotDockWidget;
class BackgroundPeakUpdate;
class PeakDetectionDialog;
class DirectInfusionDialog;
class SelectAdductsDialog;
class AlignmentDialog;
class CalibrateDialog;
//class RConsoleDialog;
class SpectraWidget;
class LigandWidget;
class IsotopeWidget;
class MassCalcWidget;
class TreeDockWidget;
class TableDockWidget;
class Classifier;
class ClassifierNeuralNet;
class ClassifierNaiveBayes;
class HeatMap;
class ScatterPlot;
class TreeMap;
class History;
class QSqlDatabase;
class SuggestPopup;
class NotesWidget;
class GalleryWidget;
class mzFileIO;
class ProjectDockWidget;
class SpectraMatching;
class LogWidget;
class RconsoleWidget;
class SpectralHit;
class LibraryMangerDialog;
class SampleBarPlotWidget;
class SetRumsDBDialog;
class SRMTransition;

extern Database DB; 


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    QSettings* getSettings() { return settings; }
    vector <mzSample*> samples;		//list of loaded samples
    static mzSample* loadSampleStatic(QString filename);
    vector<Adduct*> availableAdducts; //loaded from file, or fall back to default
    QString rumsDBDatabaseName;

    //Issue 271 compound loading upon mzrollDB open options
    bool isAttemptToLoadDB = true;
    bool isLoadPGCompoundMatches = true;

    QDoubleSpinBox 	  *ppmWindowBox;
    QLineEdit         *searchText;
    QComboBox		  *quantType = nullptr;
    QComboBox         *adductType;
    QLabel			  *statusText;
    QLabel            *percentageText;

    SpectraWidget     *spectraWidget;                   //ms1
    SpectraWidget     *fragmentationSpectraWidget;      //ms2
    SpectraWidget     *ms3SpectraWidget;                //ms3

    MassCalcWidget       *massCalcWidget;
    LigandWidget     *ligandWidget;
    IsotopeWidget    *isotopeWidget;
    TreeDockWidget 	 *covariantsPanel;
    TreeDockWidget	 *ms2ScansListWidget;
    TreeDockWidget   *ms1ScansListWidget;
    TreeDockWidget   *ms3ScansListWidget;
    TreeDockWidget   *ms2ConsensusScansListWidget;
    TreeDockWidget	 *srmDockWidget;
    TreeDockWidget *srmTransitionDockWidget; //Issue 347

    //TreeDockWidget   *peaksPanel;

    QDockWidget         *spectraDockWidget;                 //ms1
    QDockWidget         *fragmentationSpectraDockWidget;    //ms2
    QDockWidget         *ms3SpectraDockWidget;              //ms3

    QDockWidget		 *heatMapDockWidget;
    QDockWidget		 *scatterDockWidget;
    QDockWidget		 *treeMapDockWidget;
    QDockWidget	 	 *galleryDockWidget;
    QDockWidget      *barPlotDockWidget;
    LogWidget            *logWidget;
    NotesWidget		 *notesDockWidget;
    ProjectDockWidget    *projectDockWidget;
    SpectraMatching      *spectraMatchingForm;

    TableDockWidget      *bookmarkedPeaks;
    SuggestPopup	 *suggestPopup;
    HeatMap		 *heatmap;
    GalleryWidget	 *galleryWidget;
    ScatterPlot		 *scatterplot;
    TreeMap		 *treemap;
    SampleBarPlotWidget     *barPlotWidget;

    SettingsForm   *settingsForm;
    PeakDetectionDialog *peakDetectionDialog;
    DirectInfusionDialog *directInfusionDialog;
    SelectAdductsDialog *selectAdductsDialog;
    AlignmentDialog*	  alignmentDialog;
    CalibrateDialog*	  calibrateDialog;
    RconsoleWidget*       rconsoleDockWidget;
    LibraryMangerDialog*       libraryDialog;
    SetRumsDBDialog*           setRumsDBDialog;

    QProgressBar  *progressBar;

    int sampleCount() { return samples.size(); }
    EicWidget* getEicWidget() { return eicWidget; }
    SpectraWidget*  getSpectraWidget() { return spectraWidget; }
    ProjectDockWidget* getProjectWidget() { return projectDockWidget; }
    RconsoleWidget*    getRconsoleWidget() { return rconsoleDockWidget; }
    TableDockWidget*    getBookmarkTable() { return bookmarkedPeaks; }

    Classifier* getClassifier() { return clsf; }

    MatrixXf getIsotopicMatrix(PeakGroup* group);
    void isotopeC13Correct(MatrixXf& MM, int numberofCarbons);

    mzSample* getSample(int i) { assert(i < samples.size()); return(samples[i]);  }
    inline vector<mzSample*> getSamples() { return samples; }
    vector<mzSample*> getVisibleSamples();
    float getVisibleSamplesMaxRt();

    PeakGroup::QType getUserQuantType();

    bool isSampleFileType(QString filename);
    bool isProjectFileType(QString filename);

    QColor getBackgroundAdjustedBlack(QWidget* widget);
    QToolButton *btnLibrary = nullptr;
    IsotopeParameters getIsotopeParameters();

protected:
    void closeEvent(QCloseEvent *event);
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);

public slots:
    QDockWidget* createDockWidget(QString title, QWidget* w);
    void showPeakInfo(Peak*);
    void setProgressBar(QString, int step, int totalSteps);
    void setStatusText(QString text = QString::null);
    void setMzValue();
    void setMzValue(float mz);
    void loadModel();



    bool addSample(mzSample* sample);
    void compoundDatabaseSearch();
    void setUrl(QString url,QString link=QString::null);
    void setUrl(Compound*);
    void setFormulaFocus(QString formula);
    void setPeptideFocus(QString peptideSequence);
    void Align();
    void UndoAlignment();
    void doGuidedAligment(QString filename);
    void spectraFocused(Peak* _peak);
    void setCompoundFocus(Compound* c);
    void setAdductFocus(Adduct *adduct);
    void showFragmentationScans(float pmz);
    void showConsensusFragmentationScans(float pmz);
    void showMs1Scans(float pmz);
    void showMs3Scans(float preMs1Mz, float preMs2Mz);
    QString groupTextExport(PeakGroup* group);
    void bookmarkPeakGroup(PeakGroup* group);
    void setClipboardToGroup(PeakGroup* group);
    void bookmarkSelectedPeakGroup();
    void findCovariants(Peak* _peak);
    void reportBugs();
    void updateEicSmoothingWindow(int value);
    pair<vector<mzSlice*>, vector<SRMTransition*>> getSrmSlices();

    void open();
    void print();
    void exportPDF();
    void exportSVG();
    void setPeakGroup(PeakGroup* group);
    void setMs3PeakGroup(PeakGroup* parentGroup, PeakGroup* childGroup);
    void showDockWidgets();
    void hideDockWidgets();
    //void terminateTheads();
    void doSearch(QString needle);
    void setupSampleColors();
    void showMassSlices();
    void showDirectInfusionDialog();
    void showSelectAdductsDialog();
    void showSRMList();
    void addToHistory(const mzSlice& slice);
    void historyNext();
    void historyLast();
    void getCovariants(Peak* peak);
    void markGroup(PeakGroup* group,char label);
    void updateAdductComboBox(vector<Adduct*> enabledAdducts);

    void changeUserAdduct();
    Adduct* getUserAdduct();

    void setIonizationMode( int x );
    int  getIonizationMode() { return _ionizationMode; }

    void setUserPPM( double x);
    double getUserPPM() { return _ppmWindow; }

    void updateGUIWithLastSelectedPeakGroup();

    TableDockWidget* addPeaksTable(QString title, QString encodedTableInfo=QString(""), QString displayTableInfo=QString(""));
    TableDockWidget* findPeakTable(QString title);
    QString getUniquePeakTableTitle(QString title);

    QList< QPointer<TableDockWidget> > getAllPeakTables() { return groupTables; }

    void deletePeakTable(TableDockWidget* x);
    void deleteAllPeakTables();
    BackgroundPeakUpdate* newWorkerThread(QString funcName);
    QWidget* eicWidgetController();

    //Issue 376
    void unsetLastSelectedPeakGroup(){_lastSelectedPeakGroup = nullptr;}

private slots:
    void createMenus();
    void createToolBars();
    void readSettings();
    void writeSettings();

private:
    QSettings* settings;
    Classifier* clsf;
    QList< QPointer<TableDockWidget> > groupTables;
    EicWidget *eicWidget; //plot of extractred EIC
    History history;

    int _ionizationMode;
    double _ppmWindow;
    PeakGroup* _lastSelectedPeakGroup = nullptr;

    QToolBar* sideBar;
    QToolButton* addDockWidgetButton( QToolBar*, QDockWidget*, QIcon, QString);

signals:
    void updatedAvailableAdducts();
};

struct FileLoader {
		FileLoader(){};
		typedef mzSample* result_type;

		mzSample* operator()(const QString filename) {
                mzSample* sample = MainWindow::loadSampleStatic(filename);
				return sample;
		}
};

struct EicLoader {
		enum   PeakDetectionFlag { NoPeakDetection=0, PeakDetection=1 };

                EicLoader(mzSlice* islice,
                          PeakDetectionFlag iflag=NoPeakDetection,
                          int smoothingWindow=5,
                          int smoothingAlgorithm=0,
                          float amuQ1=0.1,
                          float amuQ2=0.5,
                          int baselineSmoothingWindow=5,
                          int baselineDropTopX=40
                                    ) {

				slice=islice; 
				pdetect=iflag; 
				eic_smoothingWindow=smoothingWindow;
                eic_smoothingAlgorithm=smoothingAlgorithm;
                eic_amuQ1=amuQ1;
                eic_amuQ2=amuQ2;
                eic_baselne_dropTopX=baselineDropTopX;
                eic_baselne_smoothingWindow=baselineSmoothingWindow;
		}

		typedef EIC* result_type;

		EIC* operator()(mzSample* sample) {
                    EIC* e = NULL;
                    Compound* c = slice->compound;

                    if ( ! slice->srmId.empty() ) {
                        //cout << "computeEIC srm:" << slice->srmId << endl;
                        e = sample->getEIC(slice->srmId);
                    } else if ( c && c->precursorMz >0 && c->productMz >0 ) {
                        //cout << "computeEIC qqq: " << c->precursorMz << "->" << c->productMz << endl;
                        e = sample->getEIC(c->precursorMz, c->collisionEnergy, c->productMz,eic_amuQ1, eic_amuQ2);
                    } else {
                        //cout << "computeEIC mzrange" << setprecision(7) << slice->mzmin  << " " << slice->mzmax << slice->rtmin  << " " << slice->rtmax << endl;
                        e = sample->getEIC(slice->mzmin, slice->mzmax,slice->rtmin,slice->rtmax,1);
                    }

                    if (e) {
                        e->setBaselineSmoothingWindow(eic_baselne_smoothingWindow);
                        e->setBaselineDropTopX(eic_baselne_dropTopX);
                        e->setSmootherType((EIC::SmootherType) (eic_smoothingAlgorithm));
                    }

                    if(pdetect == PeakDetection && e){
                        e->getPeakPositions(eic_smoothingWindow);
                    }
                    return e;
		}

		mzSlice* slice;
		PeakDetectionFlag pdetect;
		int eic_smoothingWindow;
        int eic_smoothingAlgorithm;
        float eic_amuQ1;
        float eic_amuQ2;
        int eic_baselne_smoothingWindow;
        int eic_baselne_dropTopX;
};

#endif
