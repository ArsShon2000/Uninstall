#include "uninstall.h"
#include "ProgressBar/filedeleter.h"
#include "ui_uninstall.h"
#include <QStandardPaths>
#include <tlhelp32.h>
#include <QThread>

UnInstall::UnInstall(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::UnInstall)
{
    ui->setupUi(this);
    ui->stackedWidget->setCurrentIndex(0);
    ui->checkBoxIts->hide();

    // Устанавливаем стиль кнопки
    QString style = R"(
        QPushButton {
            background-color: rgb(1, 87, 253);
            border-radius: 4px;
            color: white;
            font: 16px;
        }
        QPushButton:hover {
            background-color: rgba(1, 87, 253, 220);
        }
    )";
    ui->pushButton->setStyleSheet(style);
    ui->pushButton->setEnabled(true);

    // Чтение пути из реестра
    subkeyPath = readRegistryValue("SOFTWARE\\ITSolutions\\SCUD", "ITSPath");
    qDebug() << (subkeyPath.isEmpty() ? "Не удалось прочитать значение ITSPath."
                                      : "Значение ITSPath: " + subkeyPath);

    // Проверка существования директории ITS
    QDir directory(subkeyPath + "\\ITS");
    if (directory.exists()) {
        ui->checkBoxIts->show();
    }
}

UnInstall::~UnInstall()
{
    delete ui;
}

void UnInstall::on_pushButton_clicked()
{
    FileDeleter *deleter = new FileDeleter(this);
    ui->gridLayout_4->addWidget(deleter);
    ui->stackedWidget->setCurrentIndex(1);

    QStringList services = {
        "ITS_client_launch_keeper", "ITS_server_launch_keeper", "ITS_server",
        "ITS_server_updater", "ITS_client_updater", "ITS_license_activator",
        "ITS_license_service"
    };

    QStringList programs = {
        "AvtoSCUDGuardantChecker.exe", "AvtoScudService.exe", "AvtoSCUD.exe",
        "tracker.exe", "InterfaceConfigEditer.exe", "MasterActivation.exe",
        "AvtoSCUDConfigEditer.exe"
    };

    stopAndDeleteServices(services);
    for (const QString &program : programs) {
        killProcessByName(program);
        QThread::msleep(100);
    }

    if (ui->checkBoxIts->isChecked()) {
        excludedDirs << "ITS";
    }

    deleteDesktopShortcut();
    deleter->deleteFilesWithProgress(subkeyPath, excludedDirs);

    QTimer::singleShot(500, this, [this]() {
        if (!isClearRegisterValue()) {
            reecterClear();
        }
        ui->stackedWidget->setCurrentIndex(2);
    });
}


QString UnInstall::readRegistryValue(const QString &keyPath, const QString &valueName) {
    HKEY hKey;
    LONG result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, reinterpret_cast<LPCWSTR>(keyPath.utf16()), 0, KEY_READ, &hKey);

    if (result != ERROR_SUCCESS) {
        qDebug() << "Не удалось открыть ключ реестра. Ошибка:" << result;
        return QString();
    }

    DWORD valueType;
    wchar_t valueData[1024]; // Буфер для хранения значения
    DWORD valueSize = sizeof(valueData);

    result = RegQueryValueEx(hKey, reinterpret_cast<LPCWSTR>(valueName.utf16()), nullptr, &valueType, reinterpret_cast<LPBYTE>(valueData), &valueSize);

    if (result == ERROR_SUCCESS && valueType == REG_SZ) {
        RegCloseKey(hKey);
        return QString::fromWCharArray(valueData); // Преобразуем значение в QString
    } else {
        qDebug() << "Не удалось прочитать значение реестра. Ошибка:" << result;
        RegCloseKey(hKey);
        return QString();
    }
}

void UnInstall::stopAndDeleteServices(const QStringList &services) {
    for (const QString &service : services) {
        // Лямбда-функция с явным указанием возвращаемого типа
        auto executeCommand = [](const QString &command, const QStringList &args) -> QString {
            QProcess process;
            process.start(command, args);
            if (process.waitForFinished()) {
                return process.readAllStandardOutput();
            }
            return "Ошибка выполнения команды.";
        };

        QString stopOutput = executeCommand("sc", {"stop", service});
        qDebug() << "Остановка службы" << service << ":" << stopOutput;

        QString deleteOutput = executeCommand("sc", {"delete", service});
        qDebug() << "Удаление службы" << service << ":" << deleteOutput;

        if (stopOutput.contains("FAILED") || deleteOutput.contains("FAILED")) {
            QFile file("error.log");
            file.open(file.exists() ? QIODevice::Append : QIODevice::WriteOnly);
            QTextStream out(&file);
            out << QDateTime::currentDateTime().toString("dd-MM-yyyy hh:mm")
                << ": Ошибка с службой " << service << "\n";
            file.close();
        }
    }
}

bool UnInstall::killProcessByName(const QString &processName) {
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) return false;

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hSnap, &pe)) {
        do {
            QString currentProcessName = QString::fromWCharArray(pe.szExeFile);
            if (currentProcessName.compare(processName, Qt::CaseInsensitive) == 0) {
                HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
                if (hProcess) {
                    if (TerminateProcess(hProcess, 0)) {
                        qDebug() << "Процесс" << processName << "завершен.";
                    } else {
                        qDebug() << "Ошибка завершения процесса:" << GetLastError();
                    }
                    CloseHandle(hProcess);
                    CloseHandle(hSnap);
                    return true;
                } else {
                    qDebug() << "Ошибка открытия процесса:" << GetLastError();
                }
            }
        } while (Process32Next(hSnap, &pe));
    }

    CloseHandle(hSnap);
    return false;
}

void UnInstall::reecterClear()
{
    HKEY hKey;
    LONG result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\ITSolutions\\SCUD"),
                               0, KEY_ALL_ACCESS, &hKey);
    if (result == ERROR_SUCCESS) {
        result = RegDeleteValue(hKey, TEXT("ITSPath"));
        qDebug() << (result == ERROR_SUCCESS ? "Значение ITSPath успешно удалено."
                                             : "Не удалось удалить значение ITSPath. Ошибка: " + QString::number(result));
        RegCloseKey(hKey);
    } else {
        qDebug() << "Не удалось открыть ключ SCUD. Ошибка: " << result;
    }
}

bool UnInstall::isClearRegisterValue()
{
    bool isEmpty = false;
    QDir directory(subkeyPath + "\\ITS");
    if (directory.exists()) isEmpty = true;
    return isEmpty;
}

void UnInstall::on_pushButton_2_clicked()
{
    exit(1);
}

std::string UnInstall::GetRegistryValue(const std::string& keyPath, const std::string& valueName) {

    HKEY HKEYVALUE = HKEY_LOCAL_MACHINE;
    HKEY hKey;
    LONG result = RegOpenKeyExA(HKEYVALUE, keyPath.c_str(), 0, KEY_READ, &hKey);
    if (result != ERROR_SUCCESS) {
        qDebug() << "Ошибка при открытии ключа реестра: " << result;
        return "";
    }

    DWORD dataSize = 0;
    result = RegQueryValueExA(hKey, valueName.c_str(), nullptr, nullptr, nullptr, &dataSize);
    if (result != ERROR_SUCCESS) {
        qDebug() << "Ошибка при запросе значения из реестра: " << result;
        RegCloseKey(hKey);
        return "";
    }

    std::string value(dataSize, '\0');
    result = RegQueryValueExA(hKey, valueName.c_str(), nullptr, nullptr, reinterpret_cast<LPBYTE>(&value[0]), &dataSize);
    if (result != ERROR_SUCCESS) {
        qDebug() << "Ошибка при получении значения из реестра: " << result;
        RegCloseKey(hKey);
        return "";
    }

    RegCloseKey(hKey);

    value.erase(value.end() - 1);
    return value;
}


void UnInstall::on_pushButton_3_clicked()
{
    QString service = "eventlog";       // ЖУРНАЛ СОБЫТИЙ
    QProcess stopProcess;
    stopProcess.start("net", QStringList() << "start" << service);
    if (stopProcess.waitForFinished()) {
        QString stopOutput = stopProcess.readAllStandardOutput();
        qDebug() << "Остановка службы" << service << ":" << stopOutput;
    }

    FileDeleter deleter;
    QString currentExecutablePath = QCoreApplication::applicationFilePath();
    QString currentDirPath = QFileInfo(currentExecutablePath).absolutePath();
    // deleter.deleteFilesWithProgress(currentDirPath, excludedDirs);
    uninstallApp(excludedDirs);
    // QTimer::singleShot(50, this, [=]() {
    //     // scheduleSelfDeletion(currentDirPath);       // удалекние самого анинстала

    //     // QTimer::singleShot(1000, this, [=]() {
    //     //     removeDirectory(QDir::cleanPath(subkeyPath), excludedDirs);
    //     // });

    //     exit(1);
    // });
}

void UnInstall::uninstallApp(const QStringList &excludedDirs) {
    QString uninstallPath = QCoreApplication::applicationDirPath();  // "Utils"
    QDir clientDir(uninstallPath);
    QDir parentDir = clientDir;
    parentDir.cdUp();

    if (!clientDir.exists()) {
        qDebug() << "Client folder does not exist!";
        return;
    }

    bool removeClientOnly = excludedDirs.contains("ITS", Qt::CaseInsensitive);

    QString batFilePath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/uninstall_script.bat";

    QFile batFile(batFilePath);
    if (batFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&batFile);
        out << "@echo off\n";
        out << "chcp 65001 > nul\n";  // Кодировка UTF-8
        out << "cd /d C:\\Windows\\Temp\n";
        out << "timeout /t 2 /nobreak > nul\n";

        // Завершаем процессы
        out << "echo Checking for running processes...\n";
        out << "tasklist | findstr /I \"AvtoSCUD_Uninstall.exe\" && taskkill /F /IM AvtoSCUD_Uninstall.exe\n";
        out << "tasklist | findstr /I \"AvtoSCUD.exe\" && taskkill /F /IM AvtoSCUD.exe\n";

        // Принудительное удаление файлов внутри Client
        out << "echo Deleting files from Utils folder...\n";
        out << "del /F /Q \"" << uninstallPath << "/*.*\" > nul 2>&1\n";

        // Удаление папки Client
        out << "echo Removing Utils folder...\n";
        out << "rmdir /S /Q \"" << uninstallPath << "\"\n";

        // Удаление AvtoSCUD, если нет "ITS"
        if (!removeClientOnly) {
            out << "echo Removing AvtoSCUD folder...\n";
            out << "rmdir /S /Q \"" << parentDir.path() << "\"\n";
        }

        // Самоудаление .bat
        out << "timeout /t 2 /nobreak > nul\n";
        out << "del /F /Q \"" << batFilePath << "\"\n";

        batFile.close();
    } else {
        qDebug() << "Failed to create bat file!";
        return;
    }

    if (!QFile::exists(batFilePath)) {
        qDebug() << "Bat file was not created!";
        return;
    }

    bool success = QProcess::startDetached("cmd.exe", {"/c", batFilePath});
    if (!success) {
        qDebug() << "Failed to start bat file!";
    } else {
        qDebug() << "Bat file started successfully!";
    }

    QCoreApplication::exit(0);
}

void UnInstall::deleteDesktopShortcut() {
    // Получаем путь к рабочему столу
    QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);

    // Путь к ярлыку
    QString shortcutPath = desktopPath + "/AvtoSCUD.lnk";

    // Проверяем, существует ли ярлык
    if (QFile::exists(shortcutPath)) {
        if (QFile::remove(shortcutPath)) {
            qDebug() << "Ярлык успешно удален!";
        } else {
            qDebug() << "Ошибка удаления ярлыка!";
        }
    } else {
        qDebug() << "Ярлык не найден!";
    }
}

