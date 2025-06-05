#include "Installwizard.h"
#include "installerworker.h"
#include "ui_Installwizard.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QProcess>
#include <QNetworkAccessManager>
#include <QRegularExpression>
#include <QNetworkReply>
#include <unistd.h>
#include <QStandardPaths>
#include <QThread>

Installwizard::Installwizard(QWidget *parent) :
    QWizard(parent),
    ui(new Ui::Installwizard) {
    ui->setupUi(this);
    setWindowTitle("Arch Linux Installer");


    // Connect refreshButton to populate drives
    connect(ui->refreshButton, &QPushButton::clicked, this, &Installwizard::populateDrives);

    // Connect prepareButton to handle drive preparation
    connect(ui->prepareButton, &QPushButton::clicked, this, [=]() {
        QString selectedDrive = ui->driveDropdown->currentText();
        if (selectedDrive.isEmpty() || selectedDrive == "No drives found") {
            QMessageBox::warning(this, "Error", "Please select a valid drive.");
            return;
        }
        // Remove "/dev/" prefix for internal processing
        prepareDrive(selectedDrive.mid(5));
    });

    // Populate drives when the wizard starts
    populateDrives();

    // Inside Installwizard constructor
    connect(ui->downloadButton, &QPushButton::clicked, this, [=]() {
        downloadISO(ui->progressBar);  // Pass the progress bar to show download progress
    });

    connect(ui->installButton, &QPushButton::clicked,
        this, &Installwizard::on_installButton_clicked);

    connect(this, &QWizard::currentIdChanged, this, [this](int id) {
        if (id == 2) {
            if (ui->comboDesktopEnvironment->count() == 0) {
                ui->comboDesktopEnvironment->addItems({
                    "GNOME", "KDE Plasma", "XFCE", "LXQt", "Cinnamon", "MATE", "i3"
                });
            }
        }
    });
}
   QString Installwizard::getUserHome() {
    QString userHome;

    // Use HOME env variable if not root
    if (getuid() != 0) {
        userHome = QDir::homePath();
    } else {
        QByteArray userEnv = qgetenv("SUDO_USER");
        if (!userEnv.isEmpty()) {
            QString sudoUser = QString(userEnv);
            QProcess proc;
            proc.start("getent", QStringList() << "passwd" << sudoUser);
            proc.waitForFinished();
            QString output = proc.readAllStandardOutput();
            QStringList fields = output.split(':');
            if (fields.size() >= 6)
                userHome = fields[5]; // Home directory from /etc/passwd
        }
    }

    // Fallback
    if (userHome.isEmpty())
        userHome = QDir::homePath();

    return userHome;
}

void Installwizard::downloadISO(QProgressBar *progressBar) {
    networkManager = new QNetworkAccessManager(this);
    QUrl url("https://mirror.arizona.edu/archlinux/iso/latest/archlinux-x86_64.iso");
    QNetworkRequest request(url);
    QNetworkReply *reply = networkManager->get(request);

    QString finalIsoPath = QDir::tempPath() + "/archlinux.iso";
    QFile *file = new QFile(finalIsoPath);

    if (!file->open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, "Error", "Unable to open file for writing: " + finalIsoPath);
        reply->abort();
        reply->deleteLater();
        delete file;
        return;
    }

    connect(reply, &QNetworkReply::downloadProgress, this, [progressBar](qint64 bytesReceived, qint64 bytesTotal) {
        if (bytesTotal > 0) {
            progressBar->setValue(static_cast<int>((bytesReceived * 100) / bytesTotal));
        }
    });

    connect(reply, &QNetworkReply::readyRead, this, [file, reply]() {
        if (file->isOpen()) {
            file->write(reply->readAll());
        }
    });

    connect(reply, &QNetworkReply::finished, this, [this, file, reply, finalIsoPath]() {
        file->close();

        if (reply->error() == QNetworkReply::NoError) {
            // Set file permissions: readable by everyone
            QFile::setPermissions(finalIsoPath, QFile::ReadOwner | QFile::WriteOwner | QFile::ReadGroup | QFile::ReadOther);

            QMessageBox::information(this, "Success", "Arch Linux ISO downloaded successfully\nto: " + finalIsoPath + " \nNext is Installing depencies and extracting ISO...");
            installDependencies();

        } else {
            QFile::remove(finalIsoPath);
            QMessageBox::critical(this, "Error", "Failed to download ISO: " + reply->errorString());
        }

        reply->deleteLater();
        file->deleteLater();
    });

    connect(reply, &QNetworkReply::errorOccurred, this, [this, file, reply, finalIsoPath](QNetworkReply::NetworkError) {
        QFile::remove(finalIsoPath);
        QMessageBox::critical(this, "Error", "Network error while downloading ISO: " + reply->errorString());

        reply->deleteLater();
        file->deleteLater();
    });
}

Installwizard::~Installwizard() {
    delete ui;
}

void Installwizard::installDependencies() {


    QProcess process;
    QStringList packages = {
        "arch-install-scripts",  // includes arch-chroot, pacstrap
        "parted",
        "dosfstools",            // mkfs.vfat
        "e2fsprogs",             // mkfs.ext4
        "squashfs-tools",
        "os-prober",
        "wget"     // for downloading bootstrap if needed
    };

    QString installCmd = "pkexec apt install -y " + packages.join(" ");
    
    qDebug() << "Installing dependencies:" << installCmd;

    process.start("/bin/bash", QStringList() << "-c" << installCmd);
    process.waitForFinished(-1);
    
    QString output = process.readAllStandardOutput();
    QString error = process.readAllStandardError();

    qDebug() << "Dependency Install Output:" << output;
    qDebug() << "Dependency Install Errors:" << error;

    if (process.exitCode() != 0) {
        QMessageBox::critical(this, "Error", "Failed to install required dependencies:\n" + error);
        return;
    }

    // QMessageBox::information(this, "Dependencies Ready", "All required packages are installed.");

    getAvailableDrives();
}

QStringList Installwizard::getAvailableDrives() {
    QProcess process;

    // Use full path to lsblk
    process.start("/usr/bin/lsblk", QStringList() << "-o" << "NAME,SIZE,TYPE" << "-d" << "-n");
    process.waitForFinished();

    QString output = process.readAllStandardOutput();

    QStringList drives;

    // Split output into lines and process each line
    for (const QString &line : output.split('\n', Qt::SkipEmptyParts)) {
        QStringList  tokens = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);  // Split by whitespace

        if (tokens.size() >= 3 && tokens[2] == "disk") {  // Ensure it’s a disk
            QString deviceName = tokens[0];
            if (!deviceName.startsWith("loop")) {  // Skip loop devices
                drives << deviceName;  // Add the drive name (e.g., "sdb")
            }
        }
    }

    return drives;
}

void Installwizard::populateDrives() {
    ui->driveDropdown->clear();  // Clear existing items
    QStringList drives = getAvailableDrives();  // Get available drives

    if (drives.isEmpty()) {
        ui->driveDropdown->addItem("No drives found");
    } else {
        for (const QString &drive : std::as_const(drives)) {
            ui->driveDropdown->addItem(QString("/dev/%1").arg(drive));  // Add "/dev/" prefix
        }
    }

    qDebug() << "Drives added to ComboBox:" << drives;  // Debug: Confirm drives in ComboBox
}

void Installwizard::forceUnmount(const QString &mountPoint) {
    QProcess process;

    process.start("/bin/bash", QStringList() << "-c" << " sudo umount -Rfl /mnt");

    process.waitForFinished();
    if (process.exitCode() == 0) {
        qDebug() << "Recursive unmount succeeded.";
    } else {
        qDebug() << "Recursive unmount failed.";
        qDebug() << "stderr:" << process.readAllStandardError();
    }

    // Find and kill processes using the mount point
    process.start("sudo fuser -vk " + mountPoint);
    process.waitForFinished();
    qDebug() << "Killed processes using" << mountPoint;

    // Try unmounting normally
    process.start("sudo umount " + mountPoint);
    process.waitForFinished();
    if (process.exitCode() == 0) {
        qDebug() << "Unmounted successfully: " << mountPoint;
        return;
    }

    // If normal unmount failed, try lazy unmount
    process.start("sudo umount -l " + mountPoint);
    process.waitForFinished();
    if (process.exitCode() == 0) {
        qDebug() << "Lazy unmounted: " << mountPoint;
        return;
    }

    // If still fails, force unmount
    process.start("sudo umount -f " + mountPoint);
    process.waitForFinished();
    if (process.exitCode() != 0) {
        //  QMessageBox::critical(nullptr, "Error", "Failed to unmount " + mountPoint);
    } else {
        qDebug() << "Force unmounted: " << mountPoint;
    }
}

void Installwizard::prepareDrive(const QString &drive) {
    selectedDrive = drive;

    InstallerWorker *worker = new InstallerWorker;
    worker->setDrive(drive);

    QThread *thread = new QThread;
    worker->moveToThread(thread);

    connect(thread, &QThread::started, worker, &InstallerWorker::run);
    connect(worker, &InstallerWorker::logMessage, ui->logWidget, &QPlainTextEdit::appendPlainText);
    connect(worker, &InstallerWorker::errorOccurred, this, [this](const QString &msg) {
        QMessageBox::critical(this, "Error", msg);
    });
    connect(worker, &InstallerWorker::installComplete, thread, &QThread::quit);
    connect(worker, &InstallerWorker::installComplete, worker, &QObject::deleteLater);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);

    connect(worker, &InstallerWorker::installComplete, this, [this]() {
        on_installButton_clicked();
    });

    thread->start();
}

void Installwizard::on_installButton_clicked() {
    const QString username       = ui->lineEditUsername->text().trimmed();
    const QString password       = ui->lineEditPassword->text();
    const QString passwordAgain  = ui->lineEditPasswordAgain->text();
    const QString rootPassword   = ui->lineEditRootPassword->text();
    const QString rootPasswordAgain = ui->lineEditRootPasswordAgain->text();
    const QString desktopEnv     = ui->comboDesktopEnvironment->currentText();

    // Basic validation
    if (username.isEmpty() || password.isEmpty() || rootPassword.isEmpty()) {
        QMessageBox::warning(this, "Input Error", "Please fill out all fields.");
        return;
    }
    if (password != passwordAgain) {
        QMessageBox::warning(this, "Password Mismatch", "User passwords do not match.");
        return;
    }
    if (rootPassword != rootPasswordAgain) {
        QMessageBox::warning(this, "Password Mismatch", "Root passwords do not match.");
        return;
    }

    // Disable the button so they can't click again
    ui->installButton->setEnabled(false);

    // Launch the user+desktop install in a thread
    InstallerWorker *worker = new InstallerWorker;
    worker->setDrive(selectedDrive);  // keep from page 1
    worker->setUserInfo(username, password, rootPassword, desktopEnv);

    QThread *thread = new QThread;
    worker->moveToThread(thread);

    connect(thread, &QThread::started,
            worker, &InstallerWorker::installUserAndDesktop);
    connect(worker, &InstallerWorker::logMessage,
            ui->logWidget_2, &QPlainTextEdit::appendPlainText);
    connect(worker, &InstallerWorker::errorOccurred, this, [&](const QString &e){
        QMessageBox::critical(this, "Error", e);
        ui->installButton->setEnabled(true);
        thread->quit();
    });
    connect(worker, &InstallerWorker::installComplete, this, [&](){
        QMessageBox::information(this, "All Done",
                                 "Your user and desktop environment are ready!");
        ui->installButton->setEnabled(true);
    });

    // Clean up
    connect(worker, &InstallerWorker::installComplete,
            thread, &QThread::quit);
    connect(thread, &QThread::finished,
            worker, &QObject::deleteLater);
    connect(thread, &QThread::finished,
            thread, &QObject::deleteLater);

    thread->start();
}
