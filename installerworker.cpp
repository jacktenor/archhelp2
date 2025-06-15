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
    QString bootPart = QString("/dev/%1").arg(selectedDrive + "1");
    QString rootPart = QString("/dev/%1").arg(selectedDrive + "2");

    emit logMessage("🧙 Starting disk preparation in thread...");

    // Unmount
    emit logMessage("Unmounting existing /mnt...");
    process.start("sudo", {"umount", "-l", "/mnt/*"});
    process.waitForFinished();
    process.start("sudo", {"umount", "-l", "/mnt"});
    process.waitForFinished();
    process.start("/bin/bash", {"-c",
                                QString("lsblk -nr -o MOUNTPOINT /dev/%1").arg(selectedDrive)});
    process.waitForFinished();
    QStringList mps = QString(process.readAllStandardOutput()).split('\n', Qt::SkipEmptyParts);
    for (const QString &mp : mps) {
        process.start("sudo", {"umount", "-f", mp.trimmed()});
        process.waitForFinished();
    }

    // Partition
    emit logMessage("Creating new partition table...");
    QStringList cmds = {
        QString("sudo parted /dev/%1 --script mklabel msdos").arg(selectedDrive),
        QString("sudo parted /dev/%1 --script mkpart primary ext4 1MiB 100%").arg(selectedDrive)

        QString("sudo parted /dev/%1 --script mklabel gpt").arg(selectedDrive),
        QString("sudo parted /dev/%1 --script mkpart ESP fat32 1MiB 513MiB").arg(selectedDrive),
        QString("sudo parted /dev/%1 --script set 1 esp on").arg(selectedDrive),
        QString("sudo parted /dev/%1 --script mkpart primary ext4 513MiB 100%").arg(selectedDrive)

        QString("sudo parted /dev/%1 --script mkpart primary ext4 513MiB 100%%").arg(selectedDrive)

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
        "-c",
        QString("sudo partprobe /dev/%1 && sudo udevadm settle").arg(selectedDrive)
    });
    process.waitForFinished();

    // Format partitions
    emit logMessage("Formatting boot partition " + bootPart + " as FAT32...");
    process.start("/bin/bash", {
        "-c", QString("sudo mkfs.fat -F32 %1").arg(bootPart)
    });
    process.waitForFinished();
    if (process.exitCode() != 0) {
        emit errorOccurred("Format failed.");
        return;
    }

    emit logMessage("Formatting partition " + rootPart + " as ext4...");
    process.start("/bin/bash", {
        "-c", QString("sudo mkfs.ext4 -F %1").arg(rootPart)
    });
    process.waitForFinished();
    if (process.exitCode() != 0) {
        emit errorOccurred("Format failed.");
        return;
    }

    // Mount
    emit logMessage("Mounting partitions...");
    process.start("sudo", {"mount", rootPart, "/mnt"});
    process.waitForFinished();
    process.start("sudo", {"mkdir", "-p", "/mnt/boot"});
    process.waitForFinished();

    process.start("sudo", {"mount", bootPart, "/mnt/boot"});
    process.waitForFinished();

    emit logMessage("✅ Drive is ready.");
    emit installComplete();
}
