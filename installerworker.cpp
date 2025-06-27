#include "installerworker.h"
#include <QProcess>
#include <QThread>
#include <QFile>
#include <QStandardPaths>
#include <QFileInfo>

static QString locatePartedBinary()
{
    QString p = QStandardPaths::findExecutable("parted");
    if (!p.isEmpty())
        return p;
    const QStringList fallbacks{"/usr/sbin/parted", "/sbin/parted"};
    for (const QString &path : fallbacks)
        if (QFileInfo::exists(path))
            return path;
    return QString();
}

InstallerWorker::InstallerWorker(QObject *parent) : QObject(parent) {}

void InstallerWorker::setDrive(const QString &drive) {
    selectedDrive = drive;
}


void InstallerWorker::run() {
    QProcess process;
    QString suffix = (selectedDrive.startsWith("nvme") || selectedDrive.startsWith("mmc")) ? "p" : "";
    QString bootPart = QString("/dev/%1%2%3").arg(selectedDrive, suffix, "1");
    QString rootPart = QString("/dev/%1%2%3").arg(selectedDrive, suffix, "2");

    emit logMessage("🧙 Starting disk preparation in thread...");

    // Unmount any existing mounts
    emit logMessage("Unmounting existing /mnt...");
    process.start("sudo", {"umount", "-l", "/mnt/*"});
    process.waitForFinished();
    process.start("sudo", {"umount", "-l", "/mnt"});
    process.waitForFinished();
    process.start("/bin/bash", {"-c", QString("lsblk -nr -o MOUNTPOINT /dev/%1").arg(selectedDrive)});
    process.waitForFinished();
    QStringList mps = QString(process.readAllStandardOutput()).split('\n', Qt::SkipEmptyParts);
    for (const QString &mp : mps) {
        process.start("sudo", {"umount", "-f", mp.trimmed()});
        process.waitForFinished();
    }

    QString partedBin = locatePartedBinary();
    if (partedBin.isEmpty()) {
        emit errorOccurred("parted not found");
        return;
    }

    emit logMessage("Creating new partition table...");
    QStringList args{partedBin, QString("/dev/%1").arg(selectedDrive), "--script",
                     "mklabel", "msdos",
                     "mkpart", "primary", "ext4", "1MiB", "513MiB",
                     "set", "1", "boot", "on",
                     "mkpart", "primary", "ext4", "513MiB", "100%"};
    if (QProcess::execute("sudo", args) != 0) {
        emit errorOccurred("Partition command failed");
        return;
    }

    // Refresh table so the new partitions are visible
    emit logMessage("Refreshing partition table...");
    QProcess::execute("sudo", {"partprobe", QString("/dev/%1").arg(selectedDrive)});
    QProcess::execute("sudo", {"udevadm", "settle"});

    // Format partitions
    emit logMessage("Formatting boot partition " + bootPart + " as ext4...");
    if (QProcess::execute("sudo", {"mkfs.ext4", "-F", bootPart}) != 0) {
        emit errorOccurred("Format failed.");
        return;
    }

    emit logMessage("Formatting partition " + rootPart + " as ext4...");
    if (QProcess::execute("sudo", {"mkfs.ext4", "-F", rootPart}) != 0) {
        emit errorOccurred("Format failed.");
        return;
    }

    // Mount the newly created partitions
    emit logMessage("Mounting partitions...");
    process.start("sudo", {"mount", rootPart, "/mnt"});
    process.waitForFinished();
    process.start("sudo", {"mkdir", "-p", "/mnt/boot"});
    process.waitForFinished();
    process.start("sudo", {"mount", bootPart, "/mnt/boot"});
    process.waitForFinished();

    // Copy ISO for later installation if available
    process.start("/bin/bash", {"-c", "if [ -f /tmp/archlinux.iso ]; then sudo cp /tmp/archlinux.iso /mnt/archlinux.iso; fi"});
    process.waitForFinished();

    emit logMessage("✅ Drive is ready.");
    emit installComplete();
}
