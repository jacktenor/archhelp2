#include "Installwizard.h"
#include "installerworker.h"
#include "systemworker.h"
#include "ui_Installwizard.h"
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTextStream>
#include <QThread>
#include <QComboBox>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <algorithm>
#include <unistd.h>

Installwizard::Installwizard(QWidget *parent)
    : QWizard(parent), ui(new Ui::Installwizard) {
  ui->setupUi(this);
  setWindowTitle("Arch Linux Installer");

  // Initially disable navigation buttons until each page completes its work
  // setWizardButtonEnabled(QWizard::NextButton, false);
  setWizardButtonEnabled(QWizard::FinishButton, false);

  // Connect refreshButton to populate drives
  connect(ui->partRefreshButton, &QPushButton::clicked, this,
          &Installwizard::populateDrives);

  connect(ui->comboInstallMode, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, [this](int idx) {
            switch (idx) {
            case 0:
              installMode = InstallerWorker::InstallMode::WipeDrive;
              break;
            case 1:
              installMode = InstallerWorker::InstallMode::UsePartition;
              break;
            case 2:
              installMode = InstallerWorker::InstallMode::UseFreeSpace;
              break;
            }
          });

  connect(ui->treePartitions, &QTreeWidget::itemClicked, this,
          [this](QTreeWidgetItem *item, int) {
            if (item)
              selectedPartition = item->text(0);
          });

  // Connect prepareButton to handle drive preparation
  connect(ui->prepareButton, &QPushButton::clicked, this, [=]() {
    QString d = ui->driveDropdown->currentText();
    if (d.isEmpty() || d == "No drives found") {
      QMessageBox::warning(this, "Error", "Please select a valid drive.");
      return;
    }

    // Re-read the chosen install mode so we always act on the UI state
    installMode = static_cast<InstallerWorker::InstallMode>(
        ui->comboInstallMode->currentIndex());

    QString msg;
    if (installMode == InstallerWorker::InstallMode::WipeDrive)
      msg = tr("You are about to destroy all data on %1!!! Are you absolutely sure this is correct?").arg(d);
    else if (installMode == InstallerWorker::InstallMode::UsePartition)
      msg = tr("Format partition %1 as root?").arg(selectedPartition);
    else
      msg = tr("Create a new partition in free space on %1?").arg(d);

    QMessageBox::StandardButton confirm = QMessageBox::question(this, tr("Confirm"), msg, QMessageBox::Yes | QMessageBox::No);
    if (confirm != QMessageBox::Yes)
      return;

    efiInstall = false; // legacy mode

    setWizardButtonEnabled(QWizard::NextButton, true);

    QString drive = d.mid(5); // remove /dev/
    switch (installMode) {
    case InstallerWorker::InstallMode::WipeDrive:
      prepareDrive(drive);
      break;
    case InstallerWorker::InstallMode::UsePartition:
      if (selectedPartition.isEmpty()) {
        QMessageBox::warning(this, "Error", "Please select a partition in the table.");
        return;
      }
      prepareExistingPartition("/dev/" + selectedPartition);
      break;
    case InstallerWorker::InstallMode::UseFreeSpace:
      prepareFreeSpace(drive);
      break;
    }
  });

  // Populate drives when the wizard starts
  populateDrives();

  // Inside Installwizard constructor
  connect(ui->downloadButton, &QPushButton::clicked, this, [=]() {
    setWizardButtonEnabled(QWizard::NextButton, true);

    downloadISO(
        ui->progressBar); // Pass the progress bar to show download progress
  });

  connect(ui->installButton, &QPushButton::clicked, this,
          &Installwizard::on_installButton_clicked);

  connect(this, &QWizard::currentIdChanged, this, [this](int id) {
    if (id == 0) {
      setWizardButtonEnabled(QWizard::NextButton, true);
    } else if (id == 1) {
      setWizardButtonEnabled(QWizard::NextButton, false);

      QString drive = ui->driveDropdown->currentText().mid(5);
      if (!drive.isEmpty())
        populatePartitionTable(drive);
    } else if (id == 2) {
      setWizardButtonEnabled(QWizard::FinishButton, false);

      if (ui->comboDesktopEnvironment->count() == 0) {
        ui->comboDesktopEnvironment->addItems(
            {"GNOME", "KDE Plasma", "XFCE", "LXQt", "Cinnamon", "MATE", "i3"});
      }
    }
  });

  connect(ui->partRefreshButton, &QPushButton::clicked, this, [this]() {
    QString drive = ui->driveDropdown->currentText().mid(5);

    populatePartitionTable(drive);      
  });

  connect(ui->createPartButton, &QPushButton::clicked, this, [this]() {
    QString drive = ui->driveDropdown->currentText().mid(5);
    if (drive.isEmpty())
      return;

    // Ensure we react to the currently selected install mode even if
    // the combo box signal did not fire for some reason
    installMode = static_cast<InstallerWorker::InstallMode>(
        ui->comboInstallMode->currentIndex());

    setWizardButtonEnabled(QWizard::NextButton, false);
    if (installMode == InstallerWorker::InstallMode::UsePartition) {
      if (selectedPartition.isEmpty()) {
        QMessageBox::warning(this, "Error",
                             "Please select a partition in the table.");
        setWizardButtonEnabled(QWizard::NextButton, true);
        return;
      }
      splitPartitionForEfi(selectedPartition);
    } else {
      prepareForEfi(drive);



  };
    if (!drive.isEmpty()) {
      setWizardButtonEnabled(QWizard::NextButton, false);
      if (installMode == InstallerWorker::InstallMode::UsePartition) {
        if (selectedPartition.isEmpty()) {
          QMessageBox::warning(this, "Error",
                               "Please select a partition in the table.");
          setWizardButtonEnabled(QWizard::NextButton, true);
          return;
        }
        splitPartitionForEfi(selectedPartition);
      } else {
        prepareForEfi(drive);
      }
    }
  });

  connect(ui->driveDropdown, &QComboBox::currentTextChanged, this,
          &Installwizard::handleDriveChange);


          [this](const QString &text) {
            if (!text.isEmpty() && text != "No drives found") {
              selectedDrive = text.mid(5);
              if (currentId() == 1)
                populatePartitionTable(selectedDrive);
            }
          };
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

static QString locatePartedBinary() {
  QString p = QStandardPaths::findExecutable("parted");
  if (!p.isEmpty())
    return p;
  const QStringList fallbacks{"/usr/sbin/parted", "/sbin/parted"};
  for (const QString &path : fallbacks)
    if (QFileInfo::exists(path))
      return path;
  return QString();
}


void Installwizard::downloadISO(QProgressBar *progressBar) {
  QNetworkAccessManager *networkManager = new QNetworkAccessManager(this);
  QUrl url(
      "https://mirror.arizona.edu/archlinux/iso/latest/archlinux-x86_64.iso");
  QNetworkRequest request(url);
  QNetworkReply *reply = networkManager->get(request);

  QString finalIsoPath = QDir::tempPath() + "/archlinux.iso";
  QFile *file = new QFile(finalIsoPath);

  if (!file->open(QIODevice::WriteOnly)) {
    QMessageBox::critical(this, "Error",
                          "Unable to open file for writing: " + finalIsoPath);
    reply->abort();
    reply->deleteLater();
    delete file;
    return;
  }
  appendLog("Downloading ISO...");

  connect(reply, &QNetworkReply::downloadProgress, this,
          [progressBar](qint64 bytesReceived, qint64 bytesTotal) {
            if (bytesTotal > 0) {
              progressBar->setValue(
                  static_cast<int>((bytesReceived * 100) / bytesTotal));
            }
          });

  connect(reply, &QNetworkReply::readyRead, this, [file, reply]() {
    if (file->isOpen()) {
      file->write(reply->readAll());
    }
  });

  connect(
      reply, &QNetworkReply::finished, this,
      [this, file, reply, finalIsoPath]() {
        file->close();

        if (reply->error() == QNetworkReply::NoError) {
          // Set file permissions: readable by everyone
          QFile::setPermissions(finalIsoPath,
                                QFile::ReadOwner | QFile::WriteOwner |
                                    QFile::ReadGroup | QFile::ReadOther);

          QMessageBox::information(
              this, "Success",
              "Arch Linux ISO downloaded successfully\nto: " + finalIsoPath +
                  " \nNext is Installing dependencies and extracting ISO...");
          installDependencies();

        } else {
          QFile::remove(finalIsoPath);
          QMessageBox::critical(
              this, "Error", "Failed to download ISO: " + reply->errorString());
        }

        reply->deleteLater();
        file->deleteLater();
      });

  connect(reply, &QNetworkReply::errorOccurred, this,
          [this, file, reply, finalIsoPath](QNetworkReply::NetworkError) {
            QFile::remove(finalIsoPath);
            QMessageBox::critical(this, "Error",
                                  "Network error while downloading ISO: " +
                                      reply->errorString());

            reply->deleteLater();
            file->deleteLater();
          });
}

Installwizard::~Installwizard() { delete ui; }

void Installwizard::setWizardButtonEnabled(QWizard::WizardButton which,
                                           bool enabled) {
  if (QAbstractButton *btn = button(which))
    btn->setEnabled(enabled);
}


void Installwizard::installDependencies() {

  QProcess process;
  QStringList packages = {
      "arch-install-scripts", // includes arch-chroot, pacstrap
      "parted",
      "dosfstools", // mkfs.vfat
      "e2fsprogs",  // mkfs.ext4
      "squashfs-tools",
      "os-prober",
      "wget" // for downloading bootstrap if needed
  };

  // Detect distribution by reading /etc/os-release
  QString distro;
  QFile osRelease("/etc/os-release");
  if (osRelease.open(QIODevice::ReadOnly | QIODevice::Text)) {
    QTextStream in(&osRelease);
    while (!in.atEnd()) {
      QString line = in.readLine();
      if (line.startsWith("ID=")) {
        distro = line.section('=', 1).remove('"').trimmed();
        break;
      }
    }
  }

  QString installCmd;
  if (distro == "fedora") {
    installCmd = "pkexec dnf install -y " + packages.join(" ");
  } else if (distro == "arch" || distro == "archlinux") {
    installCmd = "pkexec pacman -S --noconfirm " + packages.join(" ");
  } else {
    installCmd = "pkexec apt install -y " + packages.join(" ");
  }

  qDebug() << "Installing dependencies:" << installCmd;
  appendLog("Installing dependencies:...");

  process.start("/bin/bash", QStringList() << "-c" << installCmd);
  process.waitForFinished(-1);

  QString output = process.readAllStandardOutput();
  QString error = process.readAllStandardError();

  qDebug() << "Dependency Install Output:" << output;
  qDebug() << "Dependency Install Errors:" << error;

  if (process.exitCode() != 0) {
    QMessageBox::critical(this, "Error",
                          "Failed to install required dependencies:\n" + error);
    return;
  }

  appendLog("Dependencies installed, click next to proceed.");

  // Allow user to advance to partitioning page
  setWizardButtonEnabled(QWizard::NextButton, true);

  getAvailableDrives();
}

QStringList Installwizard::getAvailableDrives() {
  QProcess process;

  // Use lsblk from PATH for better portability
  process.start("lsblk", QStringList()
                             << "-o" << "NAME,SIZE,TYPE" << "-d" << "-n");
  process.waitForFinished();

    // Use lsblk from PATH for better portability
    process.start("lsblk", QStringList() << "-o" << "NAME,SIZE,TYPE" << "-d" << "-n");
    process.waitForFinished();

  QString output = process.readAllStandardOutput();

  QStringList drives;

  // Split output into lines and process each line
  for (const QString &line : output.split('\n', Qt::SkipEmptyParts)) {
    QStringList tokens = line.split(QRegularExpression("\\s+"),
                                    Qt::SkipEmptyParts); // Split by whitespace

    if (tokens.size() >= 3 && tokens[2] == "disk") { // Ensure it’s a disk
      QString deviceName = tokens[0];
      if (!deviceName.startsWith("loop")) { // Skip loop devices
        drives << deviceName;               // Add the drive name (e.g., "sdb")
      }
    }
  }

  return drives;
}

void Installwizard::populateDrives() {
  ui->driveDropdown->clear();                // Clear existing items
  QStringList drives = getAvailableDrives(); // Get available drives

  if (drives.isEmpty()) {
    ui->driveDropdown->addItem("No drives found");
  } else {
    for (const QString &drive : std::as_const(drives)) {
      ui->driveDropdown->addItem(
          QString("/dev/%1").arg(drive)); // Add "/dev/" prefix
    }
  }

  qDebug() << "Drives added to ComboBox:"
           << drives; // Debug: Confirm drives in ComboBox
}


void Installwizard::unmountDrive(const QString &drive) {
  QProcess process;
  process.start("lsblk", QStringList() << "-nr" << "-o" << "MOUNTPOINT"
                                       << QString("/dev/%1").arg(drive));
  process.waitForFinished();
  QStringList points =
      QString(process.readAllStandardOutput()).split('\n', Qt::SkipEmptyParts);
  for (const QString &pt : points) {
    QString trimmed = pt.trimmed();
    if (!trimmed.isEmpty() && trimmed != "[SWAP]") {
      QProcess::execute("sudo", {"umount", "-f", trimmed});
    }
  }
}

void Installwizard::appendLog(const QString &message) {
  if (ui->logWidget3)
    ui->logWidget3->appendPlainText(message);
  if (ui->logView1)
    ui->logView1->appendPlainText(message);
  if (ui->logView2)
    ui->logView2->appendPlainText(message);
}

void Installwizard::prepareDrive(const QString &drive) {
  selectedDrive = drive;

  InstallerWorker *worker = new InstallerWorker;
  worker->setDrive(drive);
  worker->setMode(InstallerWorker::InstallMode::WipeDrive);

  QThread *thread = new QThread;
  worker->moveToThread(thread);

  connect(thread, &QThread::started, worker, &InstallerWorker::run);
  connect(worker, &InstallerWorker::logMessage, this,
          [this](const QString &msg) { appendLog(msg); });
  connect(worker, &InstallerWorker::errorOccurred, this,
          [this](const QString &msg) {
            QMessageBox::critical(this, "Error", msg);
          });

  connect(worker, &InstallerWorker::installComplete, thread, &QThread::quit);
  connect(worker, &InstallerWorker::installComplete, this, [this]() {
    appendLog("\xE2\x9C\x85 Drive preparation complete.");
    setWizardButtonEnabled(QWizard::NextButton, true);
  });

  connect(worker, &InstallerWorker::installComplete, worker,
          &QObject::deleteLater);
  connect(thread, &QThread::finished, thread, &QObject::deleteLater);

  thread->start();
}

void Installwizard::prepareExistingPartition(const QString &partition) {
  // Derive the parent drive so grub-install knows where to install
  QProcess proc;
  proc.start("lsblk", QStringList() << "-nr" << "-o" << "PKNAME" << partition);
  proc.waitForFinished();
  QString parent = QString(proc.readAllStandardOutput()).trimmed();
  if (!parent.isEmpty())
    selectedDrive = parent;

  InstallerWorker *worker = new InstallerWorker;
  worker->setDrive(selectedDrive);
  worker->setMode(InstallerWorker::InstallMode::UsePartition);
  worker->setTargetPartition(partition);

  QThread *thread = new QThread;
  worker->moveToThread(thread);

  connect(thread, &QThread::started, worker, &InstallerWorker::run);
  connect(worker, &InstallerWorker::logMessage, this,
          [this](const QString &msg) { appendLog(msg); });
  connect(worker, &InstallerWorker::errorOccurred, this,
          [this](const QString &msg) {
            QMessageBox::critical(this, "Error", msg);
          });
  connect(worker, &InstallerWorker::installComplete, thread, &QThread::quit);
  connect(worker, &InstallerWorker::installComplete, this, [this]() {
    appendLog("\xE2\x9C\x85 Partition prepared.");
    setWizardButtonEnabled(QWizard::NextButton, true);
  });
  connect(worker, &InstallerWorker::installComplete, worker, &QObject::deleteLater);
  connect(thread, &QThread::finished, thread, &QObject::deleteLater);

  thread->start();
}

void Installwizard::prepareFreeSpace(const QString &drive) {
  selectedDrive = drive;
  InstallerWorker *worker = new InstallerWorker;
  worker->setDrive(drive);
  worker->setMode(InstallerWorker::InstallMode::UseFreeSpace);

  QThread *thread = new QThread;
  worker->moveToThread(thread);

  connect(thread, &QThread::started, worker, &InstallerWorker::run);
  connect(worker, &InstallerWorker::logMessage, this,
          [this](const QString &msg) { appendLog(msg); });
  connect(worker, &InstallerWorker::errorOccurred, this,
          [this](const QString &msg) {
            QMessageBox::critical(this, "Error", msg);
          });
  connect(worker, &InstallerWorker::installComplete, thread, &QThread::quit);
  connect(worker, &InstallerWorker::installComplete, this, [this]() {
    appendLog("\xE2\x9C\x85 Free space partition created.");
    setWizardButtonEnabled(QWizard::NextButton, true);
  });
  connect(worker, &InstallerWorker::installComplete, worker, &QObject::deleteLater);
  connect(thread, &QThread::finished, thread, &QObject::deleteLater);

  thread->start();
}

void Installwizard::populatePartitionTable(const QString &drive) {
  if (drive.isEmpty())
    return;

  QProcess process;
  QString device = QString("/dev/%1").arg(drive);
  process.start("lsblk", QStringList()
                             << "-r" << "-n" << "-o"
                             << "NAME,SIZE,TYPE,MOUNTPOINT" << device);
  process.waitForFinished();
  QString output = process.readAllStandardOutput();

  ui->treePartitions->clear();
  QStringList lines = output.split('\n', Qt::SkipEmptyParts);
  for (const QString &line : lines.mid(1)) { // skip header
    QStringList cols =
        line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    if (cols.size() >= 4) {
      QTreeWidgetItem *item = new QTreeWidgetItem(ui->treePartitions);
      item->setText(0, cols.at(0));
      item->setText(1, cols.at(1));
      item->setText(2, cols.at(2));
      item->setText(3, cols.at(3));
    }
  }
}

void Installwizard::prepareForEfi(const QString &drive) {
  efiInstall = true; // remember choice for grub
  selectedDrive = drive;
  unmountDrive(drive);

  QString device = QString("/dev/%1").arg(drive);

  QString partedBin = locatePartedBinary();
  if (partedBin.isEmpty()) {
    QMessageBox::critical(this, "Partition Error", "parted not found");
    return;
  }

  // Run all partition commands in a single parted invocation so the kernel
  // sees the new table before we name and flag the ESP
  QStringList args{partedBin, device,  "--script", "mklabel", "gpt",  "mkpart",
                   "primary", "fat32", "1MiB",     "513MiB",  "name", "1",
                   "ESP",     "set",   "1",        "esp",     "on",   "mkpart",
                   "primary", "ext4",  "513MiB",   "100%"};
  if (QProcess::execute("sudo", args) != 0) {
    QMessageBox::critical(this, "Partition Error", tr("Failed to run parted."));
    return;
  }

  QProcess::execute("sudo", {"partprobe", device});
  QProcess::execute("sudo", {"udevadm", "settle"});

  populatePartitionTable(drive);
  appendLog("\xE2\x9C\x85 Partitions ready for EFI install.");
  setWizardButtonEnabled(QWizard::NextButton, true);
}

void Installwizard::handleDriveChange(const QString &text) {
  if (!text.isEmpty() && text != "No drives found") {
    selectedDrive = text.mid(5);
    if (currentId() == 1)
      populatePartitionTable(selectedDrive);
  }
}

void Installwizard::splitPartitionForEfi(const QString &partition) {
  efiInstall = true;

  QString part = partition;
  if (part.startsWith("/dev/"))
    part = part.mid(5);

  QRegularExpression rx("([a-zA-Z]+)(\\d+)$");
  QRegularExpressionMatch m = rx.match(part);
  if (!m.hasMatch()) {
    QMessageBox::critical(this, "Partition Error",
                         "Unable to determine partition number.");
    return;
  }

  QString drive = m.captured(1);
  int partNum = m.captured(2).toInt();
  selectedDrive = drive;

  // Make sure nothing from the drive is mounted
  unmountDrive(drive);

  QProcess proc;
  proc.start("lsblk",
             QStringList() << "-bnro" << "START,SIZE" << QString("/dev/%1").arg(part));
  proc.waitForFinished();
  QStringList vals = QString(proc.readAllStandardOutput())
                          .split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
  if (vals.size() < 2) {
    QMessageBox::critical(this, "Partition Error",
                         "Could not query partition info.");
    return;
  }

  long long startB = vals.at(0).toLongLong();
  long long sizeB = vals.at(1).toLongLong();
  long long startMiB = startB / (1024 * 1024);
  long long endMiB = startMiB + sizeB / (1024 * 1024);

  if (endMiB - startMiB <= 600) {
    QMessageBox::warning(this, "Partition Error",
                         "Partition too small to split for EFI.");
    return;
  }

  long long newEnd = endMiB - 512; // leave 512MiB for ESP
  QString device = QString("/dev/%1").arg(drive);
  QString partedBin = locatePartedBinary();
  if (partedBin.isEmpty()) {
    QMessageBox::critical(this, "Partition Error", "parted not found");
    return;
  }

  QString partPath = QString("/dev/%1").arg(part);
  long long newSize = newEnd - startMiB;

  // Shrink the filesystem before resizing the partition
  if (QProcess::execute("sudo", {"e2fsck", "-f", partPath}) != 0) {
    QMessageBox::critical(this, "Partition Error",
                         "Filesystem check failed before resize.");
    return;
  }
  if (QProcess::execute("sudo", {"resize2fs", partPath,
                                  QString("%1M").arg(newSize)}) != 0) {
    QMessageBox::critical(this, "Partition Error",
                         "Failed to shrink filesystem.");
    return;
  }

  // Resize the existing partition
  if (QProcess::execute("sudo",
                        {partedBin, device, "--script", "resizepart",
                         QString::number(partNum), QString("%1MiB").arg(newEnd)}) != 0) {
    QMessageBox::critical(this, "Partition Error",
                         "Failed to resize selected partition.");
    return;
  }

  // Create the ESP in the freed space
  if (QProcess::execute("sudo",
                        {partedBin, device, "--script", "mkpart", "primary",
                         "fat32", QString("%1MiB").arg(newEnd),
                         QString("%1MiB").arg(endMiB)}) != 0) {
    QMessageBox::critical(this, "Partition Error",
                         "Failed to create EFI partition.");
    return;
  }

  QProcess::execute("sudo", {"partprobe", device});
  QProcess::execute("sudo", {"udevadm", "settle"});

  // Determine new partition name (assume highest number)
  proc.start("lsblk", QStringList() << "-nr" << "-o" << "NAME" << device);
  proc.waitForFinished();
  QStringList names = QString(proc.readAllStandardOutput()).split('\n', Qt::SkipEmptyParts);
  QString newPart = names.last();
  QProcess::execute("sudo",
                    {"mkfs.fat", "-F32", QString("/dev/%1").arg(newPart)});

  QProcess::execute("sudo",
                    {partedBin, device, "--script", "name",
                     QString::number(partNum + 1), "ESP"});
  QProcess::execute("sudo",
                    {partedBin, device, "--script", "set",
                     QString::number(partNum + 1), "esp", "on"});

  populatePartitionTable(drive);
  appendLog("\xE2\x9C\x85 Partition adjusted for EFI.");
  setWizardButtonEnabled(QWizard::NextButton, true);
}

void Installwizard::on_installButton_clicked() {


  QString username = ui->lineEditUsername->text().trimmed();
  QString password = ui->lineEditPassword->text();
  QString passwordAgain = ui->lineEditPasswordAgain->text();

  QString rootPassword = ui->lineEditRootPassword->text();
  QString rootPasswordAgain = ui->lineEditRootPasswordAgain->text();

  QString desktopEnv = ui->comboDesktopEnvironment->currentText();

  ui->comboDesktopEnvironment->addItems(
      {"GNOME", "KDE Plasma", "XFCE", "LXQt", "Cinnamon", "MATE", "i3"});

  if (username.isEmpty() || password.isEmpty() || rootPassword.isEmpty()) {
    QMessageBox::warning(this, "Input Error", "Please fill out all fields.");
    return;
  }

  if (password != passwordAgain) {
    QMessageBox::warning(this, "Password Mismatch",
                         "User passwords do not match.");
    return;
  }

  if (rootPassword != rootPasswordAgain) {
    QMessageBox::warning(this, "Password Mismatch",
                         "Root passwords do not match.");
    return;
  }

  SystemWorker *worker = new SystemWorker;
  worker->setParameters(selectedDrive, username, password, rootPassword,
                        desktopEnv, efiInstall);

  // Prevent finishing until the background install completes
  setWizardButtonEnabled(QWizard::FinishButton, false);

  QThread *thread = new QThread;
  worker->moveToThread(thread);
  appendLog("Starting system installation…");

  connect(thread, &QThread::started, worker, &SystemWorker::run);
  connect(worker, &SystemWorker::logMessage, this,
          [this](const QString &msg) { appendLog(msg); });
  connect(worker, &SystemWorker::errorOccurred, this,
          [this](const QString &msg) {
            QMessageBox::critical(this, "Error", msg);
          });

  connect(worker, &SystemWorker::finished, this, [this]() {
    appendLog("\xE2\x9C\x85 Installation complete.");
    setWizardButtonEnabled(QWizard::FinishButton, true);
    QMessageBox::information(this, "Complete", "System installation finished.");
  });
  connect(worker, &SystemWorker::finished, thread, &QThread::quit);
  connect(worker, &SystemWorker::finished, worker, &QObject::deleteLater);
  connect(thread, &QThread::finished, thread, &QObject::deleteLater);

  thread->start();
}
