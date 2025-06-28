#ifndef INSTALLERWORKER_H
#define INSTALLERWORKER_H

#include <QObject>
#include <QString>

class InstallerWorker : public QObject {
    Q_OBJECT
public:
    explicit InstallerWorker(QObject *parent = nullptr);
    void setDrive(const QString &drive);
    enum class InstallMode { WipeDrive, UsePartition, UseFreeSpace };
    void setMode(InstallMode mode);
    void setTargetPartition(const QString &partition);

signals:
    void logMessage(const QString &message);
    void errorOccurred(const QString &message);
    void installComplete();

public slots:
    void run();

private:
    QString selectedDrive;
    InstallMode mode = InstallMode::WipeDrive;
    QString targetPartition; // used when mode == UsePartition
};

#endif // INSTALLERWORKER_H
