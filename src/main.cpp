#include "mainwindow.h"
#include <QApplication>
#include <QMessageBox>
#include <QSettings>
#include <QStandardPaths>
#include <QDir>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Set application information for QSettings
    QCoreApplication::setOrganizationName("mxrcode");
    QCoreApplication::setApplicationName("KeyGhost");
    
    try {
        // Create application data directory if it doesn't exist
        QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir dir(dataPath);
        if (!dir.exists()) {
            dir.mkpath(".");
        }
        
        MainWindow window;
        window.show();
        
        return app.exec();
    } catch (const std::exception& e) {
        QMessageBox::critical(nullptr, "Fatal Error", 
            QString("An error occurred during startup: %1").arg(e.what()));
        return 1;
    } catch (...) {
        QMessageBox::critical(nullptr, "Fatal Error", 
            "An unknown error occurred during startup. The application will now exit.");
        return 1;
    }
}
