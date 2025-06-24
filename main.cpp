#include "Installwizard.h"
#include <QApplication>
#include <QMessageBox>
#include <QFileInfo>
#include <errno.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    // Ensure the installer has the necessary privileges to run
    if (geteuid() != 0) {
        // Try to relaunch the application using pkexec which will
        // display a password prompt if needed.
        QString path = QFileInfo(argv[0]).absoluteFilePath();
        execlp("pkexec", "pkexec", path.toUtf8().constData(), (char*)nullptr);
        // If execlp returns, it failed

        QMessageBox::critical(nullptr, "Permissions Error",
                             "This installer must be run as root.\n"
                             "Please restart it using 'sudo' or 'pkexec'.");
        return 1;
    }

    Installwizard wizard;
    wizard.show();
    return a.exec();
}
