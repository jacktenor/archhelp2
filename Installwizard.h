#ifndef INSTALLWIZARD_H
#define INSTALLWIZARD_H

#include <QWizard>
#include <QProgressBar>
#include <QStringList>

QT_BEGIN_NAMESPACE
namespace Ui {
class Installwizard;
}
QT_END_NAMESPACE

class Installwizard : public QWizard {
    Q_OBJECT

public:
    explicit Installwizard(QWidget *parent = nullptr);
    ~Installwizard();

private:
    void installDependencies();
    Ui::Installwizard *ui;
    QString selectedDrive;  // 🧠 TRACK THE CURRENT DRIVE
    bool efiInstall = false; // track chosen boot mode
    QString getUserHome();
    void populateDrives(); // Populate the dropdown with available drives
    void downloadISO(QProgressBar *progressBar);
    void on_installButton_clicked();
    void unmountDrive(const QString &drive);
    void appendLog(const QString &message);
    // Declare the methods that were missing
    QStringList getAvailableDrives();        // Detect available drives
    void prepareDrive(const QString &drive);   // Prepare the selected drive
    void populatePartitionTable(const QString &drive); // new
    void prepareForEfi(const QString &drive); // use free space for EFI
    void setWizardButtonEnabled(QWizard::WizardButton which, bool enabled);
};

#endif // INSTALLWIZARD_H
