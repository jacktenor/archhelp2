/********************************************************************************
** Form generated from reading UI file 'Installwizard.ui'
**
** Created by: Qt User Interface Compiler version 6.9.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_INSTALLWIZARD_H
#define UI_INSTALLWIZARD_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QWizard>
#include <QtWidgets/QWizardPage>

QT_BEGIN_NAMESPACE

class Ui_Installwizard
{
public:
    QWizardPage *wizardPage;
    QLabel *label;
    QPushButton *downloadButton;
    QProgressBar *progressBar;
    QPlainTextEdit *logView1;
    QLabel *label_6;
    QWizardPage *partitionPage;
    QTreeWidget *treePartitions;
    QPushButton *partRefreshButton;
    QPushButton *createPartButton;
    QLabel *label_5;
    QLabel *label_2;
    QComboBox *driveDropdown;
    QPushButton *prepareButton;
    QPlainTextEdit *logView2;
    QWizardPage *wizardPage_3;
    QPushButton *installButton;
    QPlainTextEdit *logWidget3;
    QComboBox *comboDesktopEnvironment;
    QLabel *label_4;
    QLabel *label_3;
    QLabel *labelUsername;
    QLabel *labelPassword;
    QLabel *labelAgain;
    QLabel *labelRootPassword;
    QLabel *labelRootPasswordAgain;
    QLineEdit *lineEditUsername;
    QLineEdit *lineEditPassword;
    QLineEdit *lineEditPasswordAgain;
    QLineEdit *lineEditRootPassword;
    QLineEdit *lineEditRootPasswordAgain;

    void setupUi(QWizard *Installwizard)
    {
        if (Installwizard->objectName().isEmpty())
            Installwizard->setObjectName("Installwizard");
        Installwizard->resize(500, 420);
        Installwizard->setMinimumSize(QSize(500, 420));
        Installwizard->setMaximumSize(QSize(500, 420));
        Installwizard->setWizardStyle(QWizard::WizardStyle::ClassicStyle);
        wizardPage = new QWizardPage();
        wizardPage->setObjectName("wizardPage");
        label = new QLabel(wizardPage);
        label->setObjectName("label");
        label->setGeometry(QRect(90, 14, 301, 25));
        QFont font;
        font.setFamilies({QString::fromUtf8("Noto Sans")});
        font.setPointSize(12);
        font.setWeight(QFont::DemiBold);
        font.setItalic(false);
        label->setFont(font);
        label->setStyleSheet(QString::fromUtf8("font: 600 12pt \"Noto Sans\";"));
        label->setAlignment(Qt::AlignmentFlag::AlignCenter);
        downloadButton = new QPushButton(wizardPage);
        downloadButton->setObjectName("downloadButton");
        downloadButton->setGeometry(QRect(190, 92, 100, 30));
        progressBar = new QProgressBar(wizardPage);
        progressBar->setObjectName("progressBar");
        progressBar->setGeometry(QRect(30, 144, 420, 30));
        progressBar->setValue(0);
        logView1 = new QPlainTextEdit(wizardPage);
        logView1->setObjectName("logView1");
        logView1->setGeometry(QRect(30, 192, 420, 150));
        label_6 = new QLabel(wizardPage);
        label_6->setObjectName("label_6");
        label_6->setGeometry(QRect(142, 48, 207, 17));
        label_6->setStyleSheet(QString::fromUtf8("font: 600 12pt \"Noto Sans\";"));
        Installwizard->addPage(wizardPage);
        partitionPage = new QWizardPage();
        partitionPage->setObjectName("partitionPage");
        treePartitions = new QTreeWidget(partitionPage);
        treePartitions->setObjectName("treePartitions");
        treePartitions->setGeometry(QRect(32, 58, 420, 125));
        treePartitions->setMinimumSize(QSize(420, 0));
        partRefreshButton = new QPushButton(partitionPage);
        partRefreshButton->setObjectName("partRefreshButton");
        partRefreshButton->setGeometry(QRect(270, 316, 67, 30));
        createPartButton = new QPushButton(partitionPage);
        createPartButton->setObjectName("createPartButton");
        createPartButton->setGeometry(QRect(344, 316, 103, 30));
        label_5 = new QLabel(partitionPage);
        label_5->setObjectName("label_5");
        label_5->setGeometry(QRect(192, 6, 99, 25));
        label_5->setStyleSheet(QString::fromUtf8("font: 600 12pt \"Noto Sans\";"));
        label_2 = new QLabel(partitionPage);
        label_2->setObjectName("label_2");
        label_2->setGeometry(QRect(116, 30, 251, 22));
        label_2->setStyleSheet(QString::fromUtf8("font: 600 12pt \"Noto Sans\";"));
        label_2->setAlignment(Qt::AlignmentFlag::AlignCenter);
        driveDropdown = new QComboBox(partitionPage);
        driveDropdown->addItem(QString());
        driveDropdown->setObjectName("driveDropdown");
        driveDropdown->setGeometry(QRect(32, 316, 125, 30));
        prepareButton = new QPushButton(partitionPage);
        prepareButton->setObjectName("prepareButton");
        prepareButton->setGeometry(QRect(164, 316, 100, 30));
        logView2 = new QPlainTextEdit(partitionPage);
        logView2->setObjectName("logView2");
        logView2->setGeometry(QRect(32, 184, 420, 125));
        Installwizard->addPage(partitionPage);
        wizardPage_3 = new QWizardPage();
        wizardPage_3->setObjectName("wizardPage_3");
        installButton = new QPushButton(wizardPage_3);
        installButton->setObjectName("installButton");
        installButton->setGeometry(QRect(200, 172, 80, 25));
        logWidget3 = new QPlainTextEdit(wizardPage_3);
        logWidget3->setObjectName("logWidget3");
        logWidget3->setGeometry(QRect(30, 210, 420, 150));
        comboDesktopEnvironment = new QComboBox(wizardPage_3);
        comboDesktopEnvironment->setObjectName("comboDesktopEnvironment");
        comboDesktopEnvironment->setGeometry(QRect(308, 44, 125, 25));
        label_4 = new QLabel(wizardPage_3);
        label_4->setObjectName("label_4");
        label_4->setGeometry(QRect(304, 16, 132, 21));
        QFont font1;
        font1.setFamilies({QString::fromUtf8("Noto Serif")});
        font1.setPointSize(11);
        font1.setBold(true);
        label_4->setFont(font1);
        label_3 = new QLabel(wizardPage_3);
        label_3->setObjectName("label_3");
        label_3->setGeometry(QRect(50, 12, 219, 27));
        label_3->setFont(font1);
        labelUsername = new QLabel(wizardPage_3);
        labelUsername->setObjectName("labelUsername");
        labelUsername->setGeometry(QRect(68, 46, 65, 20));
        labelPassword = new QLabel(wizardPage_3);
        labelPassword->setObjectName("labelPassword");
        labelPassword->setGeometry(QRect(72, 70, 58, 20));
        labelAgain = new QLabel(wizardPage_3);
        labelAgain->setObjectName("labelAgain");
        labelAgain->setGeometry(QRect(94, 92, 36, 20));
        labelRootPassword = new QLabel(wizardPage_3);
        labelRootPassword->setObjectName("labelRootPassword");
        labelRootPassword->setGeometry(QRect(42, 116, 87, 20));
        labelRootPasswordAgain = new QLabel(wizardPage_3);
        labelRootPasswordAgain->setObjectName("labelRootPasswordAgain");
        labelRootPasswordAgain->setGeometry(QRect(92, 138, 36, 20));
        lineEditUsername = new QLineEdit(wizardPage_3);
        lineEditUsername->setObjectName("lineEditUsername");
        lineEditUsername->setGeometry(QRect(136, 42, 125, 25));
        lineEditPassword = new QLineEdit(wizardPage_3);
        lineEditPassword->setObjectName("lineEditPassword");
        lineEditPassword->setGeometry(QRect(136, 64, 125, 25));
        lineEditPassword->setEchoMode(QLineEdit::EchoMode::Password);
        lineEditPasswordAgain = new QLineEdit(wizardPage_3);
        lineEditPasswordAgain->setObjectName("lineEditPasswordAgain");
        lineEditPasswordAgain->setGeometry(QRect(136, 88, 125, 24));
        lineEditPasswordAgain->setEchoMode(QLineEdit::EchoMode::Password);
        lineEditRootPassword = new QLineEdit(wizardPage_3);
        lineEditRootPassword->setObjectName("lineEditRootPassword");
        lineEditRootPassword->setGeometry(QRect(136, 110, 125, 25));
        lineEditRootPassword->setEchoMode(QLineEdit::EchoMode::Password);
        lineEditRootPasswordAgain = new QLineEdit(wizardPage_3);
        lineEditRootPasswordAgain->setObjectName("lineEditRootPasswordAgain");
        lineEditRootPasswordAgain->setGeometry(QRect(136, 134, 125, 25));
        lineEditRootPasswordAgain->setEchoMode(QLineEdit::EchoMode::Password);
        Installwizard->addPage(wizardPage_3);

        retranslateUi(Installwizard);

        QMetaObject::connectSlotsByName(Installwizard);
    } // setupUi

    void retranslateUi(QWizard *Installwizard)
    {
        Installwizard->setWindowTitle(QCoreApplication::translate("Installwizard", "Installwizard", nullptr));
        label->setText(QCoreApplication::translate("Installwizard", "Download the latest Arch Linux ISO", nullptr));
        downloadButton->setText(QCoreApplication::translate("Installwizard", "Download", nullptr));
        label_6->setText(QCoreApplication::translate("Installwizard", "and install dependencies.", nullptr));
        QTreeWidgetItem *___qtreewidgetitem = treePartitions->headerItem();
        ___qtreewidgetitem->setText(3, QCoreApplication::translate("Installwizard", "Mount", nullptr));
        ___qtreewidgetitem->setText(2, QCoreApplication::translate("Installwizard", "Type", nullptr));
        ___qtreewidgetitem->setText(1, QCoreApplication::translate("Installwizard", "Size", nullptr));
        ___qtreewidgetitem->setText(0, QCoreApplication::translate("Installwizard", "Name", nullptr));
        partRefreshButton->setText(QCoreApplication::translate("Installwizard", "Refresh", nullptr));
        createPartButton->setText(QCoreApplication::translate("Installwizard", "Prepare For EFI", nullptr));
        label_5->setText(QCoreApplication::translate("Installwizard", "Partitioning", nullptr));
        label_2->setText(QCoreApplication::translate("Installwizard", "Select a drive for installation", nullptr));
        driveDropdown->setItemText(0, QCoreApplication::translate("Installwizard", "Select a Drive", nullptr));

        prepareButton->setText(QCoreApplication::translate("Installwizard", "Prepare Drive", nullptr));
        installButton->setText(QCoreApplication::translate("Installwizard", "Submit", nullptr));
        label_4->setText(QCoreApplication::translate("Installwizard", "Choose a desktop", nullptr));
        label_3->setText(QCoreApplication::translate("Installwizard", "Add user and set passwords", nullptr));
        labelUsername->setText(QCoreApplication::translate("Installwizard", "Username:", nullptr));
        labelPassword->setText(QCoreApplication::translate("Installwizard", "Password:", nullptr));
        labelAgain->setText(QCoreApplication::translate("Installwizard", "Again:", nullptr));
        labelRootPassword->setText(QCoreApplication::translate("Installwizard", "Root Password:", nullptr));
        labelRootPasswordAgain->setText(QCoreApplication::translate("Installwizard", "Again:", nullptr));
    } // retranslateUi

};

namespace Ui {
    class Installwizard: public Ui_Installwizard {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_INSTALLWIZARD_H
