#ifndef BACKGROUND_DIRECTINFUSION_UPDATE_H
#define BACKGROUND_DIRECTINFUSION_UPDATE_H

#include "stable.h"
#include "mzSample.h"
#include "mainwindow.h"
#include "database.h"

#include "directinfusionprocessor.h"

class MainWindow;
class Database;
class TableDockWidget;

class BackgroundDirectInfusionUpdate : public QThread {

    Q_OBJECT

public:
    BackgroundDirectInfusionUpdate(QWidget*);
    ~BackgroundDirectInfusionUpdate();

    /**
     * @brief params
     * sets all to defaults.
     */
    shared_ptr<DirectInfusionSearchParameters> params = shared_ptr<DirectInfusionSearchParameters>(new DirectInfusionSearchParameters);

private:
    vector<mzSample*> samples;
    vector<Compound*> compounds;
    vector<Adduct*> adducts;

public:
    void setCompounds(vector<Compound*>& set) {compounds=set; }
    void setSamples(vector<mzSample*>& set) {samples=set; }
    void setAdducts(vector<Adduct*>& set) {adducts=set;}

protected:
  void run(void);

signals:
    void updateProgressBar(QString, int, int);
    void newDirectInfusionAnnotation(unique_ptr<DirectInfusionGroupAnnotation>, int);
    void closeDialog();

};

#endif // BACKGROUND_DIRECTINFUSION_UPDATE_H
