#include "Installwizard.h"
#include <QApplication>
#include <QMessageBox>
#include <QFileInfo>
#include <QProcess>
#include <errno.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    // Ensure the installer has the necessary privileges to run
    if (geteuid() != 0) {
        // Try to relaunch the application using pkexec which will
        // display a password prompt. Preserve DISPLAY and XAUTHORITY
        // so the GUI can connect to the X server.
        QString path = QFileInfo(argv[0]).absoluteFilePath();
        QStringList args{"env"};
        QByteArray disp = qgetenv("DISPLAY");
        if (!disp.isEmpty())
            args << QString("DISPLAY=%1").arg(QString::fromLocal8Bit(disp));
        QByteArray xauth = qgetenv("XAUTHORITY");
        if (!xauth.isEmpty())
            args << QString("XAUTHORITY=%1").arg(QString::fromLocal8Bit(xauth));
        QByteArray qpa = qgetenv("QT_QPA_PLATFORMTHEME");
        if (!qpa.isEmpty())
            args << QString("QT_QPA_PLATFORMTHEME=%1").arg(QString::fromLocal8Bit(qpa));
        args << path;

        if (QProcess::startDetached("pkexec", args))
            return 0;

        // If startDetached() failed, notify the user
        QMessageBox::critical(nullptr, "Permissions Error",
                             "This installer must be run as root.\n"
                             "Please restart it using 'sudo' or 'pkexec'.");
        return 1;
    }

    Installwizard wizard;
    wizard.show();
    return a.exec();
}
