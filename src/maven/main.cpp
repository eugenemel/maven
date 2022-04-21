#include "stable.h"
#include "globals.h"
#include "mainwindow.h"
#include "database.h"
#include "mzfileio.h"



Database DB;
void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);

int main(int argc, char *argv[])
{

    QApplication app(argc, argv);
    QPixmap pixmap(":/images/splash.png","PNG",Qt::ColorOnly);
    QSplashScreen splash(pixmap);
    splash.setMask(pixmap.mask());
    splash.show();
    app.processEvents();
    app.setApplicationName("Maven2");

    MainWindow* mainWindow = new MainWindow();
    //qInstallMessageHandler(customMessageHandler);

    QStringList filelist;
 	for (int i = 1; i < argc; ++i) {
        QString filename(argv[i]);

        if (filename.endsWith(".mzrollDB",Qt::CaseInsensitive) ) {
            mainWindow->projectDockWidget->loadProjectSQLITE(filename);
		}

        if (filename.endsWith("mzxml",Qt::CaseInsensitive) ||
            filename.endsWith("mzdata",Qt::CaseInsensitive) ||
            filename.endsWith("mzdata.xml",Qt::CaseInsensitive) ||
            filename.endsWith("cdf",Qt::CaseInsensitive) ||
            filename.endsWith("netcdf",Qt::CaseInsensitive) ||
            filename.endsWith("nc",Qt::CaseInsensitive) ||
            filename.endsWith("mzML",Qt::CaseInsensitive) ||
            filename.endsWith("mzcsv",Qt::CaseInsensitive))  {
            filelist << filename;
            splash.showMessage("Loading " + filename, Qt::AlignLeft, Qt::white );
        }
	}

    splash.finish(mainWindow);
    mainWindow->show();

    bool isNotifyNewerMaven = true;
    if (mainWindow->getSettings() && mainWindow->getSettings()->contains("chkNotifyNewerMaven")) {
        isNotifyNewerMaven = static_cast<Qt::CheckState>(mainWindow->getSettings()->value("chkNotifyNewerMaven").toInt()) == Qt::CheckState::Checked;
    }

    if (isNotifyNewerMaven) {

        QString currentVersion(MAVEN_VERSION);

        QProcess process;
        process.start("curl https://github.com/eugenemel/maven/releases/latest/");
        process.waitForFinished(10000); // wait for 10 seconds

        QString stdoutString = process.readAllStandardOutput();

        QRegularExpression regex("(?<=tag/).*(?=\")");
        QRegularExpressionMatch match = regex.match(stdoutString);
        QString latestVersion = match.captured(0);

        //Issue 445
        //if the current version is not a development version,
        //the latest version could be retrieved,
        //and a more recent version exists,
        //ask the user if they'd like to visit the download page
        if (!currentVersion.contains("-") && !latestVersion.isEmpty() && latestVersion != currentVersion){

            QMessageBox versionMsgBox;
            versionMsgBox.setText("A more recent version of MAVEN is available.");
            versionMsgBox.setInformativeText("Would you like to update MAVEN to the latest version?");
            QPushButton *visitButton = versionMsgBox.addButton("Visit Download Page", QMessageBox::ActionRole);
            QPushButton *okButton = versionMsgBox.addButton("Skip for now", QMessageBox::ActionRole);

            versionMsgBox.exec();

            if (versionMsgBox.clickedButton() == visitButton) {
                QDesktopServices::openUrl(QUrl("https://github.com/eugenemel/maven/releases/latest/"));
            } else if (versionMsgBox.clickedButton() == okButton) {
                versionMsgBox.close();
            }
        }
    }

    if ( filelist.size() > 0 ) {
        mzFileIO* fileLoader  = new mzFileIO(mainWindow);
        fileLoader->setMainWindow(mainWindow);
        fileLoader->loadSamples(filelist);
        fileLoader->start();
    }

    int rv = app.exec();
    return rv;


}


void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
        switch (type) {
        	case QtDebugMsg:
                cerr << "Debug: " << msg.toStdString() << endl;
                break;
        	case QtWarningMsg:
                cerr << "Warning: " << msg.toStdString() << endl;
                break;
        	case QtCriticalMsg:
                cerr << "Critical: " << msg.toStdString() << endl;
                break;
        	case QtFatalMsg:
                cerr << "Fatal: " << msg.toStdString() << endl;
		break;

                default:
                cerr << "Debug: " << msg.toStdString() << endl;

                //abort();
        }
        //QFile outFile("debuglog.txt");
        //outFile.open(QIODevice::WriteOnly | QIODevice::Append);
        //QTextStream ts(&outFile);
        //ts << txt << endl;
}

