#ifndef BACKGROUND_DIRECTINFUSION_UPDATE_H
#define BACKGROUND_DIRECTINFUSION_UPDATE_H

#include "stable.h"
#include "mzSample.h"
#include "mainwindow.h"
#include "database.h"

class MainWindow;
class Database;

class BackgroundDirectInfusionUpdate : public QThread {

    Q_OBJECT

public:
    BackgroundDirectInfusionUpdate(QWidget*);
    ~BackgroundDirectInfusionUpdate();

private:
    vector<mzSample*> samples;
    vector<Compound*> compounds;

public:
    void setCompounds(vector<Compound*>& set) {compounds=set; }
    void setSamples(vector<mzSample*>& set) {samples=set; }

protected:
  void run(void);

signals:
    void updateProgressBar(QString, int, int);
};

#endif // BACKGROUND_DIRECTINFUSION_UPDATE_H
