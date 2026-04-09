#include <QApplication>
#include <QFile>
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Set application info
    app.setApplicationName("PhotoTrick");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("PhotoTrick");

    // Load stylesheet
    QFile styleFile(":/styles/main.qss");
    if (styleFile.open(QFile::ReadOnly)) {
        QString styleSheet = QLatin1String(styleFile.readAll());
        app.setStyleSheet(styleSheet);
        styleFile.close();
    }

    MainWindow window;
    window.setWindowTitle("PhotoTrick");
    window.resize(1200, 800);
    window.show();

    return app.exec();
}
