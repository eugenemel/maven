#include "stable.h"
#include "globals.h"
#include "mainwindow.h"
#include "database.h"
#include "mzfileio.h"



Database DB;
void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);

//Issue 865: versions are "major.minor.patch" or "major.minor.patch.letter"
//(e.g. "1.2.3" or "1.2.3.A"), where the lettered form is a later release than
//its plain "major.minor.patch" counterpart, and later letters are later still.
struct MavenVersion {
    int major = 0, minor = 0, patch = 0;
    QChar letter; // null QChar() if there is no letter suffix
};

static MavenVersion parseMavenVersion(const QString &versionStr) {
    QRegularExpression versionRegex("(\\d+)\\.(\\d+)\\.(\\d+)(?:\\.([A-Za-z]))?");
    QRegularExpressionMatch m = versionRegex.match(versionStr);

    MavenVersion v;
    if (m.hasMatch()) {
        v.major = m.captured(1).toInt();
        v.minor = m.captured(2).toInt();
        v.patch = m.captured(3).toInt();
        if (!m.captured(4).isEmpty()) v.letter = m.captured(4).at(0).toUpper();
    }
    return v;
}

//true if "a" is a later release than "b" (null letter sorts before any letter)
static bool isMavenVersionNewer(const MavenVersion &a, const MavenVersion &b) {
    if (a.major != b.major) return a.major > b.major;
    if (a.minor != b.minor) return a.minor > b.minor;
    if (a.patch != b.patch) return a.patch > b.patch;
    return a.letter > b.letter;
}

int main(int argc, char *argv[])
{

    QApplication app(argc, argv);
    QPixmap pixmap(":/images/maven_2_img.png","PNG",Qt::ColorOnly);
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
        process.start("curl -L https://github.com/eugenemel/maven/releases/latest/");
        process.waitForFinished(10000); // wait for 10 seconds

        QString stdoutString = process.readAllStandardOutput();

        //Issue 865: New verisons are either number.number.number or number.number.number.letter
        // regex needs to handle both

        QRegularExpression regex("(?<=eugenemel/maven/releases/tag/)\\d+\\.\\d+\\.\\d+(?:\\.[A-Za-z])?");
        QRegularExpressionMatch match = regex.match(stdoutString);
        QString latestVersion = match.captured(0);

        //Issue 445
        //if the current version is not a development version,
        //the latest version could be retrieved,
        //and a more recent version exists,
        //ask the user if they'd like to visit the download page
        qDebug() << "currentVersion:" << currentVersion;
        qDebug() << "latestVersion:" << latestVersion;

        // if (!latestVersion.isEmpty() && latestVersion != currentVersion){
        if (!currentVersion.contains("-") && !latestVersion.isEmpty() &&
                isMavenVersionNewer(parseMavenVersion(latestVersion), parseMavenVersion(currentVersion))){

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

