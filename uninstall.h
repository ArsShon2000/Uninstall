#ifndef UNINSTALL_H
#define UNINSTALL_H

#include <QDialog>
#include <QFile>
#include <QDir>
#include <QProcess>
#include <QDebug>
#include <QTimer>
#include <windows.h>

namespace Ui {
class UnInstall;
}

class UnInstall : public QDialog
{
    Q_OBJECT

public:
    explicit UnInstall(QWidget *parent = nullptr);
    ~UnInstall();

private slots:
    void on_pushButton_clicked();
    QString readRegistryValue(const QString &keyPath, const QString &valueName);
    void reecterClear();
    bool isClearRegisterValue();
    bool checkRegistryValue(HKEY hKey, const char* valueName);
    std::string GetRegistryValue(const std::string& keyPath, const std::string& valueName);;


    bool checkers();

    void on_checkBoxIts_stateChanged(int arg1);

    void on_pushButton_2_clicked();

    void on_pushButton_3_clicked();
    void uninstallApp(const QStringList &excludedDirs);
    bool removeDirectory(const QString &dirPath, const QStringList &excludedDirs = {});
    void deleteDesktopShortcut();
    void stopAndDeleteServices(const QStringList &services);
    void scheduleSelfDeletion(QString higherLevelDir);
    bool killProcessByName(const QString &processName);

private:
    Ui::UnInstall *ui;
    QString subkeyPath;

    QStringList excludedDirs;
};

#endif // UNINSTALL_H
