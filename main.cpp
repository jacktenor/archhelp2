#include "Installwizard.h"
#include <QApplication>
#include <QMessageBox>
#include <unistd.h>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    // Ensure the installer has the necessary privileges to run
    if (geteuid() != 0) {
        QMessageBox::critical(nullptr, "Permissions Error",
                             "This installer must be run as root.\n"
                             "Please restart it using 'sudo' or 'pkexec'.");
        return 1;
    }

    Installwizard wizard;
    wizard.show();
    return a.exec();
}
