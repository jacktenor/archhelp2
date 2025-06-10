#ifndef INSTALLERWORKER_H
#define INSTALLERWORKER_H

#include <QObject>
#include <QString>

class InstallerWorker : public QObject {
    Q_OBJECT
public:
    explicit InstallerWorker(QObject *parent = nullptr);
    void setDrive(const QString &drive);

signals:
    void logMessage(const QString &message);
    void errorOccurred(const QString &message);
    void installComplete();

public slots:
    void run();

private:
    QString selectedDrive;
};

#endif // INSTALLERWORKER_H
