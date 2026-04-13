#include <QApplication>
#include <QFile>
#include <QIcon>
#include <QSplashScreen>
#include <QTimer>
#include <QPainter>
#include <QPropertyAnimation>
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Set application info
    app.setApplicationName("PhotoTrick");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("PhotoTrick");

    // Create splash screen with gradient background
    QPixmap splashPixmap(400, 300);
    splashPixmap.fill(Qt::transparent);

    QPainter painter(&splashPixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    // Draw gradient background
    QLinearGradient gradient(0, 0, 0, 300);
    gradient.setColorAt(0, QColor(45, 45, 65));
    gradient.setColorAt(1, QColor(25, 25, 45));
    painter.fillRect(0, 0, 400, 300, gradient);

    // Draw app name
    painter.setPen(QColor(100, 200, 255));
    QFont titleFont("Segoe UI", 32, QFont::Bold);
    painter.setFont(titleFont);
    painter.drawText(QRect(0, 100, 400, 60), Qt::AlignCenter, "PhotoTrick");

    // Draw subtitle
    painter.setPen(QColor(180, 180, 200));
    QFont subFont("Segoe UI", 12);
    painter.setFont(subFont);
    painter.drawText(QRect(0, 170, 400, 30), Qt::AlignCenter, "OCR & Invoice Recognition");

    // Draw loading indicator
    painter.setPen(QColor(100, 200, 255));
    QFont loadingFont("Segoe UI", 10);
    painter.setFont(loadingFont);
    painter.drawText(QRect(0, 250, 400, 30), Qt::AlignCenter, "Loading...");

    painter.end();

    QSplashScreen splash(splashPixmap);
    splash.show();
    app.processEvents();

    // Set application icon
    app.setWindowIcon(QIcon(":/icons/app.svg"));

    // Load stylesheet
    QFile styleFile(":/styles/main.qss");
    if (styleFile.open(QFile::ReadOnly)) {
        QString styleSheet = QLatin1String(styleFile.readAll());
        app.setStyleSheet(styleSheet);
        styleFile.close();
    }

    // Create main window
    MainWindow window;
    window.setWindowTitle("PhotoTrick");
    window.resize(1200, 800);

    // Show splash for 1.5 seconds then show main window
    QTimer::singleShot(1500, [&]() {
        splash.finish(&window);
        window.show();
    });

    return app.exec();
}
