#include<QtGui>
#include<QDebug>
#include "mainWindow.h"

int main( int argc, char *argv[] ) {
    // Issue 858: render correctly on high-DPI (e.g. 4K) displays instead of
	// using tiny fonts/widgets sized for a 1x display. Must be set before
	// QApplication is constructed.
	QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

	QApplication app(argc, argv);

    // Issue 858: QStandardPaths::DataLocation (used for mzWatcher.db) is
	// derived from applicationName(), which otherwise defaults to the built
	// executable's own filename -- making the settings/database location
	// silently depend on build output naming instead of being stable across
	// versions. Set it explicitly so it can't drift again.
    QCoreApplication::setOrganizationName("MAVEN");
	QCoreApplication::setApplicationName("mzWatcher");

	MainWindow* mw = new MainWindow(0);
	mw->show();
	app.exec();
	return 1;
}
