#include "Installwizard.h"
#include <QApplication>
#include <QMessageBox>
#include <QFileInfo>
#include <QProcess>
#include <errno.h>
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
            argBytes << QByteArray("XAUTHORITY=") + xauth;
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
        QMessageBox::critical(nullptr, "Permissions Error",
                             QString("Failed to run pkexec: %1").arg(strerror(errno)));
        return 1;
    }

    Installwizard wizard;
    wizard.show();
    return a.exec();
}
