#include "widget.h"

#include <QApplication>
#include <cstdlib>
int main(int argc, char *argv[])
{
    qputenv("QT_IM_MODULE", QByteArray("qtvirtualkeyboard"));
    QApplication a(argc, argv);
    Widget w;
    w.show();
    return a.exec();
}
///usr/lib/arm-qt/plugins/platforminputcontexts/
