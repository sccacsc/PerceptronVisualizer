#include <QApplication>
#include <QStyleFactory>
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setStyle(QStyleFactory::create("Fusion"));
    MainWindow window;
    window.resize(1280, 780);
    window.show();
    return app.exec();
}
