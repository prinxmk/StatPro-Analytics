#include <QApplication>
#include <QStyleFactory>
#include "ui/MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("StatPro Analytics");
    QApplication::setApplicationDisplayName("StatPro Analytics");
    QApplication::setApplicationVersion("0.4.0");
    QApplication::setOrganizationName("StatPro");
    app.setStyle(QStyleFactory::create("Fusion"));
    StatPro::MainWindow window;
    window.show();
    return app.exec();
}
