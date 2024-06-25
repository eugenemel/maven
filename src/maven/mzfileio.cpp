#include "mzfileio.h"
#include <QStringList>
#include <QTextStream>

mzFileIO::mzFileIO(QWidget*) {
    _mainwindow = NULL;
    _stopped = true;
}

void mzFileIO::setMainWindow(MainWindow* mw) {
    _mainwindow=mw;
    connect(this,SIGNAL(updateProgressBar(QString,int,int)), _mainwindow, SLOT(setProgressBar(QString, int,int)));
    connect(this,SIGNAL(finished()),_mainwindow,SLOT(setupSampleColors()));
    connect(this,SIGNAL(finished()),_mainwindow->projectDockWidget,SLOT(updateSampleList()));
    connect(this,SIGNAL(finished()),_mainwindow,SLOT(showSRMList()));
    connect(this,SIGNAL(finished()),_mainwindow,SLOT(updateSampleComboBoxes()));
}


void mzFileIO::loadSamples(QStringList& filenames) {
    emptyFiles = QStringList(); //reset empty file list as a part of updating file names
    setFileList(filenames);
    start();
}

QString mzFileIO::shortSampleName(QString fileName) {
    QString sampleName = fileName;
    sampleName.replace(QRegExp(".*/"),"");
    sampleName.replace(".mzCSV","",Qt::CaseInsensitive);
    sampleName.replace(".mzdata","",Qt::CaseInsensitive);
    sampleName.replace(".mzXML","",Qt::CaseInsensitive);
    sampleName.replace(".mzML","",Qt::CaseInsensitive);
    sampleName.replace(".mz5","",Qt::CaseInsensitive);
    sampleName.replace(".pepXML","",Qt::CaseInsensitive);
    sampleName.replace(".xml","",Qt::CaseInsensitive);
    sampleName.replace(".cdf","",Qt::CaseInsensitive);
    sampleName.replace(".raw","",Qt::CaseInsensitive);
    return sampleName;
}

bool mzFileIO::checkSampleAlreadyLoaded(QString filename) {
    bool loaded=false;

    if(_mainwindow) {
        for(mzSample* s: _mainwindow->getSamples()) {
         if (s->fileName == filename.toStdString()) return true;
        }
    }

    return loaded;
}

mzSample* mzFileIO::loadSample(QString filename){

    //check if file exists
    QFile file(filename);

    if (!file.exists() ) { 	//couldn't find this file. check local directory
        qDebug() << "Can't find file " << filename; return 0;
    }

    if (checkSampleAlreadyLoaded(filename)) {
        qDebug() <<  "Sample already leaded " << filename; return 0;
    }

    mzSample* sample = nullptr;
    QTime timer; timer.start();

    try {
        if (filename.contains("pepxml",Qt::CaseInsensitive)) {
           	mzFileIO::loadPepXML(filename);
        } else  if (filename.contains("mzdata",Qt::CaseInsensitive)) {
            sample = mzFileIO::parseMzData(filename);
        } else {
            sample = new mzSample();
            sample->loadSample( filename.toLatin1().data() );
            if ( sample->scans.size() == 0 ) {
                delete(sample);
                sample=nullptr;
            }
        }
    } catch(...) {
        qDebug() << "loadSample() " << filename << " failed..";
    }

    qDebug() << "loadSample time (msec) = " << timer.elapsed();

    if ( sample && sample->scans.size() > 0 ) {
        sample->enumerateSRMScans();

        //set min and max values for rt
        sample->calculateMzRtRange();

        //set file path
        sample->fileName = file.fileName().toStdString();

        if (filename.contains("blan",Qt::CaseInsensitive)) sample->isBlank = true;
        return sample;
    }

    return nullptr;
}

int mzFileIO::loadPepXML(QString fileName) {

    qDebug() << "Loading pepXML sample: " << fileName;
    QFile data(fileName);
    string dbname = mzUtils::cleanFilename(fileName.toStdString());

    if (!data.open(QFile::ReadOnly) ) {
        qDebug() << "Can't open " << fileName; return 0;
    }

    QXmlStreamReader xml(&data);
    xml.setNamespaceProcessing(false);
    QList<QStringRef> taglist;
/*
    <spectrum_query spectrum="BSA_run_120909192952.2.2.2" spectrumNativeID="controllerType=0 controllerNumber=1 scan=2" start_scan="2" end_scan="2" precursor_neutral_mass="887.14544706624" assumed_charge="2" index="2">
      <search_result num_target_comparisons="0" num_decoy_comparisons="0">
        <search_hit hit_rank="1" peptide="SCHTGLGR" peptide_prev_aa="K" peptide_next_aa="S" protein="sp|P02787|TRFE_HUMAN" num_tot_proteins="1" calc_neutral_pep_mass="886.94606" massdiff="-0.19938706624" num_tol_term="2" num_missed_cleavages="0" num_matched_ions="5" tot_num_ions="42">
          <modification_info>
            <mod_aminoacid_mass position="2" mass="160.0306444778"/>
          </modification_info>
          <search_score name="number of matched peaks" value="5"/>
          <search_score name="number of unmatched peaks" value="37"/>
*/
 
    int hitCount=0;
    int charge=0;
    float precursorMz=0;
    //int  scannum=0;

    Compound* cpd = NULL;
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement()) {
                taglist << xml.name();
                if (xml.name() == "spectrum_query") {
                    //scannum = xml.attributes().value("start_scan").toString().toInt();
                    charge = xml.attributes().value("charge").toString().toInt();
                    precursorMz = xml.attributes().value("precursor_neutral_mass").toString().toInt();
		    precursorMz = (precursorMz-charge)/charge;	

                } else if (xml.name() == "search_hit") {
		    hitCount++;
                    //int hit_rank = xml.attributes().value("hit_rank").toString().toInt();
                    QString peptide = xml.attributes().value("peptide").toString();
                    QString protein = xml.attributes().value("protein").toString();
		    QString formula = "";

		    cpd = new Compound(
				    protein.toStdString() + "_" + peptide.toStdString(),
				    peptide.toStdString(),
				    formula.toStdString(),
				    charge);

            cpd->setExactMass(precursorMz); // Potential problem, should use peptide aa sequence for calculation
		    cpd->precursorMz=precursorMz;
		    cpd->db=dbname;
		    //cpd->fragment_mzs = mzs;
		    //cpd->fragment_intensity = intest;
		    DB.addCompound(cpd);

                } else if (xml.name() == "mod_aminoacid_mass" ) {
                    //int pos =          xml.attributes().value("position").toString().toInt();
                    //double massshift = xml.attributes().value("mass").toString().toDouble();
                } else if (xml.name() == "search_score" ) {
                    QString name = xml.attributes().value("name").toString();
                    QString value = xml.attributes().value("value").toString();
                }
        } else if (xml.isEndElement()) {
               if (!taglist.isEmpty()) taglist.pop_back();
               if (xml.name() == "search_hit") {

               }
        }
    }

    data.close();
    return hitCount;
}

mzSample* mzFileIO::parseMzData(QString fileName) {

    qDebug() << "Loading mzData sample: " << fileName;
    QFile data(fileName);

    if (!data.open(QFile::ReadOnly) ) {
        qDebug() << "Can't open " << fileName; return NULL;
    }

    QXmlStreamReader xml(&data);
    xml.setNamespaceProcessing(false);
    QList<QStringRef> taglist;

    int scannum=0;

    mzSample* currentSample=NULL;
    Scan* currentScan=NULL;

    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement()) {
                taglist << xml.name();
                if (xml.name() == "spectrum") {
                    scannum++;
                    if (!currentSample) currentSample = new mzSample();
                    currentScan = new Scan(currentSample,scannum,1,0,0,0);
                } else if (xml.name() == "cvParam" && currentScan) {
                    QString _name = xml.attributes().value("name").toString();
                    QString _value = xml.attributes().value("value").toString();
                   //qDebug() << _name << "->" << _value;

                    if(_name.contains("TimeInMinutes",Qt::CaseInsensitive))  currentScan->rt = _value.toFloat();
                    else if(_name.contains("time in seconds",Qt::CaseInsensitive))  currentScan->rt = _value.toFloat();
                    else if(_name.contains("Polarity",Qt::CaseInsensitive)) {
                        if ( _value[0] == 'p' || _value[0] == 'P') {
                            currentScan->setPolarity(+1);
                        } else {
                            currentScan->setPolarity(-1);
                        }
                    }
                } else if (xml.name() == "spectrumInstrument" && currentScan) {
                    currentScan->mslevel = xml.attributes().value("msLevel").toString().toInt();
                    if (currentScan->mslevel <= 0 ) currentScan->mslevel=1;
                 } else if (xml.name() == "data" && taglist.size() >= 2 && currentScan) {
                     int precision = xml.attributes().value("precision").toString().toInt();


                     if (taglist.at(taglist.size()-2) == "mzArrayBinary") {
                      currentScan->mz=
                               base64::decode_base64(xml.readElementText().toStdString(),precision/8,false,false);
                     }

                     if (taglist.at(taglist.size()-2) == "intenArrayBinary") {
                        currentScan->intensity =
                                base64::decode_base64(xml.readElementText().toStdString(),precision/8,false,false);
                     }
                }

        } else if (xml.isEndElement()) {
               if (!taglist.isEmpty()) taglist.pop_back();
               if (xml.name() == "spectrum" && currentScan) {
                   currentSample->addScan(currentScan);
                   emit (updateProgressBar( "FileImport", scannum%100, 110));
               }
        }
    }

    data.close();
    return currentSample;
}

void mzFileIO::run(void) {
    fileImport();
    quit();
}

void mzFileIO::fileImport(void) {

    if ( filelist.size() == 0 ) return;
    emit (updateProgressBar( "Importing files", filelist.size()+0.01, filelist.size()));

    for (int i = 0; i < filelist.size(); i++) {
        QString filename = filelist.at(i);
        emit (updateProgressBar( tr("Importing file %1").arg(filename), i+1, filelist.size()));

        if(_mainwindow) {
            qDebug() << "Loading sample:" << filename;
            mzSample* sample = this->loadSample(filename);

            if (sample) {

                sample->enumerateSRMScans();

                if (sample->scans.size()>0) _mainwindow->addSample(sample);

            } else {
                emptyFiles.append(filename);
            }
        }
    }

    emit(updateProgressBar( "Done importing", filelist.size(), filelist.size()));
}

QStringList mzFileIO::getEmptyFiles() {
    return emptyFiles;
}

