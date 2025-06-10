#include "installerworker.h"
#include <QProcess>
#include <QThread>
#include <QFile>

InstallerWorker::InstallerWorker(QObject *parent) : QObject(parent) {}

void InstallerWorker::setDrive(const QString &drive) {
    selectedDrive = drive;
}

void InstallerWorker::run() {
    QProcess process;
    QString rootPart = QString("/dev/%1").arg(selectedDrive + "1");

    emit logMessage("🧙 Starting disk preparation in thread...");

    // Unmount
    emit logMessage("Unmounting existing /mnt...");
    process.start("sudo", {"umount", "-l", "/mnt/*"});
    process.waitForFinished();
    process.start("sudo", {"umount", "-l", "/mnt"});
    process.waitForFinished();

    // Partition
    emit logMessage("Creating new partition table...");
    QStringList cmds = {
        QString("sudo parted /dev/%1 --script mklabel msdos").arg(selectedDrive),
        QString("sudo parted /dev/%1 --script mkpart primary ext4 1MiB 100%").arg(selectedDrive),
        QString("sudo parted /dev/%1 --script set 1 boot on").arg(selectedDrive)
    };
    for (const QString &cmd : cmds) {
        process.start("/bin/bash", {"-c", cmd});
        process.waitForFinished();
        if (process.exitCode() != 0) {
            emit errorOccurred("Partition error: " + cmd);
            return;
        }
    }

    // Refresh table
    emit logMessage("Refreshing partition table...");
    process.start("/bin/bash", {
        "-c", QString("sudo partprobe /dev/%1 && sudo udevadm trigger").arg(selectedDrive)
    });
    process.waitForFinished();

    // Format partition
    emit logMessage("Formatting partition " + rootPart + " as ext4...");
    process.start("/bin/bash", {
        "-c", QString("sudo mkfs.ext4 -F -L ROOT %1").arg(rootPart)
    });
    process.waitForFinished();
    if (process.exitCode() != 0) {
        emit errorOccurred("Format failed.");
        return;
    }

    // Mount
    emit logMessage("Mounting partition...");
    process.start("sudo", {"mount", rootPart, "/mnt"});
    process.waitForFinished();

    emit logMessage("✅ Drive is ready.");
    emit installComplete();
}
