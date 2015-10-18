#include <QApplication>
#include "TheMainWindow.h"

int main(int argc, char * argv[])
{
    QApplication app(argc, argv);
    TheMainWindow w;
    w.show();
    return app.exec();
}
