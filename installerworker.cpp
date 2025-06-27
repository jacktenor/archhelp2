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

    QString partedBin = locatePartedBinary();
    if (partedBin.isEmpty()) {
        emit errorOccurred("parted not found");
        return;
    }

    // Partition the drive in one go to avoid kernel race conditions
    emit logMessage("Creating new partition table...");
    QStringList args{partedBin, QString("/dev/%1").arg(selectedDrive), "--script",
                     "mklabel", "msdos",
                     "mkpart", "primary", "ext4", "1MiB", "513MiB",
                     "set", "1", "boot", "on",
                     "mkpart", "primary", "ext4", "513MiB", "100%"};
    if (QProcess::execute("sudo", args) != 0) {
        emit errorOccurred("Partition command failed");
        return;

    QString partedBin = QStandardPaths::findExecutable("parted");
    if (partedBin.isEmpty()) {
        emit errorOccurred("parted not found in PATH");
        return;
    }

    // Partition
    emit logMessage("Creating new partition table...");
    QStringList cmd1{partedBin, QString("/dev/%1").arg(selectedDrive), "--script", "mklabel", "msdos"};
    QStringList cmd2{partedBin, QString("/dev/%1").arg(selectedDrive), "--script", "mkpart", "primary", "ext4", "1MiB", "513MiB"};
    QStringList cmd3{partedBin, QString("/dev/%1").arg(selectedDrive), "--script", "set", "1", "boot", "on"};
    QStringList cmd4{partedBin, QString("/dev/%1").arg(selectedDrive), "--script", "mkpart", "primary", "ext4", "513MiB", "100%"};

    for (const QStringList &args : {cmd1, cmd2, cmd3, cmd4}) {
        int ret = QProcess::execute("sudo", args);
        if (ret != 0) {
            emit errorOccurred("Partition command failed");


    QStringList cmds = {
        // Legacy BIOS layout: 512MiB boot partition + remainder root
        QString("sudo %1 /dev/%2 --script mklabel msdos").arg(partedBin, selectedDrive),
        QString("sudo %1 /dev/%2 --script mkpart primary ext4 1MiB 513MiB").arg(partedBin, selectedDrive),
        QString("sudo %1 /dev/%2 --script set 1 boot on").arg(partedBin, selectedDrive),
        QString("sudo %1 /dev/%2 --script mkpart primary ext4 513MiB 100%").arg(partedBin, selectedDrive)
    };
    for (const QString &cmd : cmds) {
        process.start("/bin/bash", {"-c", cmd});
        process.waitForFinished(-1);
        QString stdOut = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
        QString errOut = QString::fromUtf8(process.readAllStandardError()).trimmed();
        if (!stdOut.isEmpty())
            emit logMessage(stdOut);
        if (process.exitCode() != 0) {
            emit errorOccurred(QString("Partition error: %1\n%2").arg(cmd, errOut));
            return;
        }
    }

    // Refresh table
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

    // Mount
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
