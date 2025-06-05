#include "Installwizard.h"
#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    Installwizard wizard;
    wizard.show();
    return a.exec();
}
