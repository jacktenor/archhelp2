#include "installerworker.h"
#include <QProcess>
#include <QThread>
#include <QFile>
#include <QDir>

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
    process.start("sudo", {"umount", "-l", "/mnt/*"}); process.waitForFinished();
    process.start("sudo", {"umount", "-l", "/mnt"});   process.waitForFinished();

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

    // Format
    emit logMessage("Formatting partition " + rootPart + " as ext4...");
    process.start("/bin/bash", {
        "-c", QString("sudo mkfs.ext4 -F -L ROOT %1").arg(rootPart)
    });
    process.waitForFinished();
    if (process.exitCode() != 0) {
        emit errorOccurred("Format failed.");
        return;
    }

    // Mount root
    emit logMessage("Mounting root partition...");
    process.start("sudo", {"mount", rootPart, "/mnt"});
    process.waitForFinished();

    emit logMessage("✅ Drive is ready.");

    // ── NEXT STAGES ───────────────────────────────────
    mountPartitions();
    mountISO();
    bindSystemDirectories();
    installArchBase();
    installGrub();

    emit logMessage("✅ All disk + rootfs steps complete.");
    emit installComplete();
}

void InstallerWorker::mountPartitions() {
    QProcess p;
    QString rootPart = QString("/dev/%1").arg(selectedDrive + "1");

    emit logMessage("Mounting root partition to /mnt...");
    p.start("sudo", {"mount", rootPart, "/mnt"});        p.waitForFinished();

    emit logMessage("Ensuring /mnt/boot exists...");
    p.start("sudo", {"mkdir", "-p", "/mnt/boot"});       p.waitForFinished();

    emit logMessage("Copying ISO to /mnt...");
    p.start("sudo", {"cp", "/tmp/archlinux.iso", "/mnt/archlinux.iso"});
    p.waitForFinished();
}

void InstallerWorker::mountISO() {
    QProcess p;
    const QString isoPath = "/mnt/archlinux.iso";

    if (!QFile::exists(isoPath)) {
        emit errorOccurred("Arch Linux ISO not found at: " + isoPath);
        return;
    }

    emit logMessage("Mounting ISO at /mnt/archiso...");
    QDir().mkpath("/mnt/archiso");
    p.start("sudo", {"mount","-o","loop",isoPath,"/mnt/archiso"});  p.waitForFinished();
    if (p.exitCode() != 0) {
        emit errorOccurred("Failed to mount ISO:\n" + p.readAllStandardError());
        return;
    }

    emit logMessage("Extracting airootfs.sfs...");
    QString sfs = "/mnt/archiso/arch/x86_64/airootfs.sfs";
    p.start("sudo", {"unsquashfs","-f","-d","/mnt",sfs});          p.waitForFinished();
    if (p.exitCode() != 0) {
        emit errorOccurred("Failed to extract rootfs:\n" + p.readAllStandardError());
    }
}

void InstallerWorker::bindSystemDirectories() {
    emit logMessage("Binding /proc, /sys, /dev, /run into /mnt...");
    QProcess p;
    for (const QString &d : { "/proc", "/sys", "/dev", "/run" }) {
        QString target = "/mnt" + d;
        QDir().mkpath(target);
        p.start("sudo", {"mount","--bind",d,target});
        p.waitForFinished();
        if (p.exitCode() != 0) {
            emit errorOccurred("Failed to bind " + d + ":\n" + p.readAllStandardError());
            return;
        }
    }
}

void InstallerWorker::installArchBase() {
    QProcess p;

    emit logMessage("Installing keys...");
    // Initialize pacman
    QProcess::execute("sudo", {"arch-chroot", "/mnt", "pacman-key", "--init"});
    QProcess::execute("sudo", {"arch-chroot", "/mnt", "pacman-key", "--populate", "archlinux"});
    QProcess::execute("sudo", {"arch-chroot", "/mnt", "pacman", "-Sy", "--noconfirm", "archlinux-keyring"});

    emit logMessage("💧 Copying resolv.conf into chroot…");
    p.start("sudo", {"rm","-f","/mnt/etc/resolv.conf"}); p.waitForFinished();
    p.start("sudo", {"cp","/etc/resolv.conf","/mnt/etc/resolv.conf"}); p.waitForFinished();

    // bootstrap if needed
    if (!QFile::exists("/mnt/usr/bin/pacman")) {
        emit logMessage("📥 Downloading bootstrap…");
        p.start("sudo", {"wget","-O","/tmp/arch-bootstrap.tar.gz",
                         "https://mirrors.edge.kernel.org/archlinux/iso/latest/archlinux-bootstrap-x86_64.tar.gz"});
        p.waitForFinished();
        if (p.exitCode() != 0) {
            emit errorOccurred("Failed to download bootstrap.");
            return;
        }
        emit logMessage("📂 Extracting bootstrap…");
        p.start("sudo", {"tar","-xzf","/tmp/arch-bootstrap.tar.gz",
                         "-C","/mnt","--strip-components=1"});
        p.waitForFinished();
        if (p.exitCode() != 0) {
            emit errorOccurred("Failed to extract bootstrap.");
            return;
        }
    }

    emit logMessage("🔧 Installing base, linux, linux-firmware…");
    p.start("sudo", {"arch-chroot","/mnt","pacman","-Sy","--noconfirm",
                     "base","linux","linux-firmware"});
    p.waitForFinished();
    if (p.exitCode() != 0) {
        emit errorOccurred("Base system installation failed.");
        return;
    }

    emit logMessage("🛠️ Writing mkinitcpio preset…");
    // write the preset file
    QString preset =
        "[mkinitcpio preset file for the 'linux' package]\n"
        "ALL_config=\"/etc/mkinitcpio.conf\"\n"
        "ALL_kver=\"/boot/vmlinuz-linux\"\n"
        "\n"
        "PRESETS=(\n"
        "  default\n"
        "  fallback\n"
        ")\n"
        "\n"
        "default_image=\"/boot/initramfs-linux.img\"\n"
        "fallback_image=\"/boot/initramfs-linux-fallback.img\"\n"
        "fallback_options=\"-S autodetect\"\n";
    QFile f("/tmp/linux.preset");
    if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        f.write(preset.toUtf8());
        f.close();
        p.start("sudo", {"cp","/tmp/linux.preset","/mnt/etc/mkinitcpio.d/linux.preset"});
        p.waitForFinished();
    }

    emit logMessage("⏱️ Enabling timesyncd…");
    p.start("sudo", {"arch-chroot","/mnt","systemctl","enable","systemd-timesyncd.service"});
    p.waitForFinished();

    // clean up archiso configs and regen
    emit logMessage("🧹 Cleaning old initramfs & configs…");
    p.start("sudo", {"arch-chroot","/mnt","rm","-f","/boot/initramfs-linux*"});
    p.waitForFinished();
    p.start("sudo", {"arch-chroot","/mnt","rm","-f","/etc/mkinitcpio.conf.d/archiso.conf"});
    p.waitForFinished();
    p.start("sudo", {"arch-chroot","/mnt","sed","-i",
                     "s/archiso[^ ]* *//g","/etc/mkinitcpio.conf"});
    p.waitForFinished();

    emit logMessage("🔄 Regenerating fstab & initramfs…");
    p.start("sudo", {"bash","-c","genfstab -U /mnt > /mnt/etc/fstab"}); p.waitForFinished();
    p.start("sudo", {"arch-chroot","/mnt","mkinitcpio","-P"});   p.waitForFinished();

    emit logMessage("✅ Base system installed");
}

void InstallerWorker::installGrub() {
    QProcess p;
    QString disk = QString("/dev/%1").arg(selectedDrive);

    emit logMessage("🍀 Installing GRUB packages inside chroot…");
    p.start("sudo", {"arch-chroot","/mnt","pacman","-Sy","--noconfirm",
                     "grub","os-prober"});
    p.waitForFinished();
    if (p.exitCode() != 0) {
        emit errorOccurred("Failed to install GRUB packages");
        return;
    }

    emit logMessage("📥 Running grub-install…");
    p.start("sudo", {"arch-chroot","/mnt","grub-install","--target=i386-pc",disk});
    p.waitForFinished();
    if (p.exitCode() != 0) {
        emit errorOccurred("grub-install failed");
        return;
    }

    emit logMessage("⚙️ Generating grub.cfg…");
    p.start("sudo", {"arch-chroot","/mnt","grub-mkconfig","-o","/boot/grub/grub.cfg"});
    p.waitForFinished();
    if (p.exitCode() != 0) {
        emit errorOccurred("grub-mkconfig failed");
        return;
    }

    emit logMessage("✅ GRUB installed");
}

void InstallerWorker::setUserInfo(const QString &username,
                                  const QString &password,
                                  const QString &rootPassword,
                                  const QString &desktopEnv)
{
    selUser     = username;
    selPass     = password;
    selRootPass = rootPassword;
    selDesktop  = desktopEnv;
}

// ── NEW: page-3 logic runs in the worker thread ──
void InstallerWorker::installUserAndDesktop() {
    QProcess p;

    emit logMessage("👤 Adding user and setting password…");
    p.start("sudo", {"arch-chroot","/mnt","useradd","-m","-G","wheel", selUser});
    p.waitForFinished();
    p.start("sudo", {"arch-chroot","/mnt","bash","-c",
                     QString("echo '%1:%2' | chpasswd").arg(selUser, selPass)});
    p.waitForFinished();

    emit logMessage("🔒 Setting root password…");
    p.start("sudo", {"arch-chroot","/mnt","bash","-c",
                     QString("echo 'root:%1' | chpasswd").arg(selRootPass)});
    p.waitForFinished();

    emit logMessage("🛡️ Enabling sudo for wheel group…");
    p.start("sudo", {"arch-chroot","/mnt","sed","-i",
                     "s/^# %wheel ALL=(ALL:ALL) ALL/%wheel ALL=(ALL:ALL) ALL/",
                     "/etc/sudoers"});
    p.waitForFinished();

    // desktop packages map
    QMap<QString, QStringList> desktopPackages = {
        {"GNOME",      {"xorg","gnome","gdm"}},
        {"KDE Plasma", {"xorg","plasma","sddm","kde-applications"}},
        {"XFCE",       {"xorg","xfce4","xfce4-goodies","lightdm","lightdm-gtk-greeter"}},
        {"LXQt",       {"xorg","lxqt","sddm"}},
        {"Cinnamon",   {"xorg","cinnamon","lightdm","lightdm-gtk-greeter"}},
        {"MATE",       {"xorg","mate","mate-extra","lightdm","lightdm-gtk-greeter"}},
        {"i3",         {"xorg","i3","lightdm","lightdm-gtk-greeter"}}
    };

    if (!desktopPackages.contains(selDesktop)) {
        emit errorOccurred("Unknown desktop env: " + selDesktop);
        return;
    }

    emit logMessage("💻 Installing desktop environment: " + selDesktop);
    QStringList args = {"arch-chroot","/mnt","pacman","-Sy","--noconfirm"};
    args.append(desktopPackages[selDesktop]);
    p.start("sudo", args);
    p.waitForFinished();
    if (p.exitCode() != 0) {
        emit errorOccurred("Failed to install " + selDesktop + ":\n"
                           + p.readAllStandardError());
        return;
    }

    // enable the right display manager
    QString dm = (selDesktop == "GNOME")             ? "gdm.service"
                 : (selDesktop == "KDE Plasma"
                    || selDesktop == "LXQt")         ? "sddm.service"
                     :                                       "lightdm.service";

    emit logMessage("🔌 Enabling " + dm);
    p.start("sudo", {"arch-chroot","/mnt","systemctl","enable",dm});
    p.waitForFinished();

    emit logMessage("🔄 Regenerating fstab & initramfs…");
    p.start("sudo", {"bash","-c","genfstab -U /mnt > /mnt/etc/fstab"}); p.waitForFinished();
    p.start("sudo", {"arch-chroot","/mnt","mkinitcpio","-P"});          p.waitForFinished();

    emit logMessage("✅ User & desktop setup complete");
    emit installComplete();  // final wake-up to GUI
}
