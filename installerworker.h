#ifndef INSTALLERWORKER_H
#define INSTALLERWORKER_H

#include <QObject>
#include <QString>

class InstallerWorker : public QObject {
    Q_OBJECT
public:
    explicit InstallerWorker(QObject *parent = nullptr);
    void setDrive(const QString &drive);
    void setUserInfo(const QString &username,
                     const QString &password,
                     const QString &rootPassword,
                     const QString &desktopEnv);

signals:
    void logMessage(const QString &message);
    void errorOccurred(const QString &message);
    void installComplete();

public slots:
    void run();
    void installUserAndDesktop();

private:
    QString selectedDrive;
    void mountPartitions();
    void mountISO();
    void bindSystemDirectories();
    void installArchBase();
    void installGrub();
    QString selUser;
    QString selPass;
    QString selRootPass;
    QString selDesktop;
};

#endif // INSTALLERWORKER_H
