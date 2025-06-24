#include "Installwizard.h"
#include <QApplication>
#include <QMessageBox>
#include <QFileInfo>
#include <QProcess>
#include <errno.h>

#include <errno.h>
#include <QProcess>
#include <QFileInfo>

#include <unistd.h>
#include <vector>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    // Ensure the installer has the necessary privileges to run
    if (geteuid() != 0) {
        // Relaunch the program through pkexec which will open a password
        // dialog. We use execvp so the current process is replaced and
        // polkit can communicate with the authentication agent.
        QString path = QFileInfo(argv[0]).absoluteFilePath();

        QList<QByteArray> argBytes{"pkexec", "env"};
        QByteArray disp = qgetenv("DISPLAY");
        if (!disp.isEmpty())
            argBytes << QByteArray("DISPLAY=") + disp;
        QByteArray xauth = qgetenv("XAUTHORITY");
        if (!xauth.isEmpty())
            argBytes << QByteArray("./ArchHelp
Refusing to render service to dead parents.
XAUTHORITY=") + xauth;
        QByteArray qpa = qgetenv("QT_QPA_PLATFORMTHEME");
        if (!qpa.isEmpty())
            argBytes << QByteArray("QT_QPA_PLATFORMTHEME=") + qpa;
        argBytes << path.toLocal8Bit();

        std::vector<char*> execArgs;
        execArgs.reserve(argBytes.size() + 1);
        for (QByteArray &a : argBytes)
            execArgs.push_back(a.data());
        execArgs.push_back(nullptr);

        execvp("pkexec", execArgs.data());
        // If execvp returns, it failed

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
        // display a password prompt if needed.
        QString path = QFileInfo(argv[0]).absoluteFilePath();
        execlp("pkexec", "pkexec", path.toUtf8().constData(), (char*)nullptr);
        // If execlp returns, it failed
        QStringList args;
        args << QFileInfo(argv[0]).absoluteFilePath();

        if (QProcess::startDetached("pkexec", args)) {
            return 0; // Child process launched; exit current instance
        }

        QMessageBox::critical(nullptr, "Permissions Error",
                             QString("Failed to run pkexec: %1").arg(strerror(errno)));
        return 1;
    }

    Installwizard wizard;
    wizard.show();
    return a.exec();
}
