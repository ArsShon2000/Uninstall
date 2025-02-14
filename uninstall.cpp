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
    // ui->stackedWidget->setCurrentIndex(0);

    ui->checkBoxIts->hide();
    ui->stackedWidget->setCurrentIndex(0);

    QString style = R"(
QPushButton:hover{
    background-color: rgba(1, 87, 253, 220);
}
QPushButton{
    background-color: rgb(1, 87, 253);
    border-radius: 4px;
    color: white;
    font: 16px;
}
)";
    ui->pushButton->setStyleSheet(style);
    ui->pushButton->setEnabled(true);


        subkeyPath = readRegistryValue("SOFTWARE\\ITSolutions\\SCUD", "ITSPath");

        if (!subkeyPath.isEmpty()) {
            qDebug() << "Значение ITSPath:" << subkeyPath;
        } else {
            qDebug() << "Не удалось прочитать значение ITSPath.";
        }

        QDir directory(subkeyPath + "\\ITS");
        if (directory.exists()) ui->checkBoxIts->show();


        // Проверяем наличие значений ClientPath, ITSPath, ServerPath
        // if (checkRegistryValue(hKey, "ClientPath")) ui->checkBoxClient->show();
        // if (checkRegistryValue(hKey, "ITSPath")) ui->checkBoxIts->show();
        // if (checkRegistryValue(hKey, "ServerPath")) ui->checkBoxServer->show();

        // Закрываем ключ реестра

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
        "ITS_client_launch_keeper",
        "ITS_server_launch_keeper",
        "ITS_server",
        "ITS_server_updater",
        "ITS_client_updater",
        "ITS_license_activator",
        "ITS_license_service"
    };

    QStringList programms = {
        "AvtoSCUDGuardantChecker.exe",
        "AvtoScudService.exe",
        "AvtoSCUD.exe",
        "tracker.exe",
        "InterfaceConfigEditer.exe",
        "MasterActivation.exe",
        "AvtoSCUDConfigEditer.exe"
    };

    // Остановка и удаление служб
    QString service = "eventlog";       // ЖУРНАЛ СОБЫТИЙ
    QProcess stopProcess;
    stopProcess.start("sc", QStringList() << "stop" << service);
    if (stopProcess.waitForFinished())
    {
        QString stopOutput = stopProcess.readAllStandardOutput();
        qDebug() << "Остановка службы" << service << ":" << stopOutput;
    }
    stopAndDeleteServices(services);            // Закрытие служб

    foreach (const QString &program, programms)  // Закрытие программ
    {
        killProcessByName(program);
        QThread::msleep(100); // Задержка 100 мс
    }

    QString currentExecutablePath = QCoreApplication::applicationFilePath();
    QString currentDirPath = QFileInfo(currentExecutablePath).absolutePath();
    // if (removeDirectory(currentDirPath)) {
    //     qDebug() << "Папка" << currentDirPath << "успешно удалена.";
    // } else {
    //     qDebug() << "Не удалось удалить папку" << currentDirPath;
    // }


    if (ui->checkBoxIts->isChecked())
    {
        excludedDirs << "ITS"; // исключение
    }
    deleteDesktopShortcut();
    deleter->deleteFilesWithProgress(subkeyPath, excludedDirs);
    QTimer::singleShot(500, this, [&]() mutable {

        QString uninstallPath = QCoreApplication::applicationDirPath();  // "Client"
        QDir clientDir(uninstallPath);
        QDir parentDir = clientDir;
        parentDir.cdUp();

        // QFile eFile("path.txt");
        // if (!eFile.exists()
        //         ? eFile.open(QIODevice::WriteOnly | QIODevice::Text)
        //         : eFile.open(QIODevice::Append | QIODevice::Text)) {
        //     QTextStream out(&eFile);

        //     out << QDateTime::currentDateTime().toString("dd-MM-yyyy hh:mm") << "\t: " << uninstallPath << clientDir.absolutePath() << parentDir.path() << Qt::endl;

        //     eFile.close();
        // }

        // QString pathToDelete = subkeyPath;
        // if (removeDirectory(QDir::cleanPath(pathToDelete), excludedDirs)) {
        // } else {
        //     qDebug() << "Не удалось удалить папку" << pathToDelete;


        // }

        if (!isClearRegisterValue())
        {
            HKEY hKey;
            LONG result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\ITSolutions\\SCUD"), 0, KEY_ALL_ACCESS, &hKey);

            if (result == ERROR_SUCCESS) {
                // Удаляем значение ClientPath
                result = RegDeleteValue(hKey, TEXT("ITSPath"));
                if (result == ERROR_SUCCESS) {
                    qDebug() << "Значение ITSPath успешно удалено.\n";
                } else {
                    qDebug() << "Не удалось удалить значение ITSPath. Ошибка: " << result << "\n";
                }

                // Закрываем ключ реестра
                RegCloseKey(hKey);
            } else {
                qDebug() << "Не удалось открыть ключ SCUD. Ошибка: " << result << "\n";
            }
        }

        ui->stackedWidget->setCurrentIndex(2);

    });


    // if (ui->checkBoxServer->isChecked() && ui->checkBoxIts->isChecked() && ui->checkBoxClient->isChecked()) reecterClear();

}


bool UnInstall::removeDirectory(const QString &dirPath, const QStringList &excludedDirs) {
    QDir dir(dirPath);
    if (!dir.exists()) {
        return false; // Папка не существует
    }

    // Удаление файлов в текущей папке
    foreach (const QString &file, dir.entryList(QDir::Files)) {
        if (!dir.remove(file)) {
            qDebug() << "Не удалось удалить файл:" << dir.absoluteFilePath(file);
        }
    }

    // Удаление подпапок, если они не входят в список исключений
    foreach (const QString &subDir, dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        QString absoluteSubDirPath = dir.absoluteFilePath(subDir);
        if (!excludedDirs.contains(subDir)) {
            if (!removeDirectory(absoluteSubDirPath, excludedDirs)) {
                qDebug() << "Не удалось удалить папку:" << absoluteSubDirPath;
            }
        }
    }

    // Удаление самой папки, если она не в списке исключений
    QString folderName = QFileInfo(dirPath).fileName();
    if (excludedDirs.contains(folderName)) {
        qDebug() << "Папка" << folderName << "в списке исключений и не будет удалена.";
        return true; // Папка остается, так как она исключена
    }

    // Удаляем саму папку
    bool result = dir.rmdir(dirPath);
    if (!result) {
        qDebug() << "Не удалось удалить папку:" << dirPath;
    }
    return result;
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
    foreach (const QString &service, services) {
        // Остановка службы
        QProcess stopProcess;
        stopProcess.start("sc", QStringList() << "stop" << service);
        if (stopProcess.waitForFinished()) {
            QString stopOutput = stopProcess.readAllStandardOutput();
            qDebug() << "Остановка службы" << service << ":" << stopOutput;
        } else {
            qDebug() << "Не удалось остановить службу" << service;
            QFile eFile("path.txt");
            if (!eFile.exists()
                    ? eFile.open(QIODevice::WriteOnly | QIODevice::Text)
                    : eFile.open(QIODevice::Append | QIODevice::Text)) {
                QTextStream out(&eFile);

                out << QDateTime::currentDateTime().toString("dd-MM-yyyy hh:mm") << "\t: " << "Не удалось остановить службу" << Qt::endl;

                eFile.close();
            }
        }

        // Удаление службы
        QProcess deleteProcess;
        deleteProcess.start("sc", QStringList() << "delete" << service);
        if (deleteProcess.waitForFinished()) {
            QString deleteOutput = deleteProcess.readAllStandardOutput();
            qDebug() << "Удаление службы" << service << ":" << deleteOutput;
        } else {
            qDebug() << "Не удалось удалить службу" << service;
            QFile eFile("path.txt");
            if (!eFile.exists()
                    ? eFile.open(QIODevice::WriteOnly | QIODevice::Text)
                    : eFile.open(QIODevice::Append | QIODevice::Text)) {
                QTextStream out(&eFile);

                out << QDateTime::currentDateTime().toString("dd-MM-yyyy hh:mm") << "\t: " << "Не удалось удалить службу" << Qt::endl;

                eFile.close();
            }
        }
    }
}

void UnInstall::scheduleSelfDeletion(QString higherLevelDir) {
#ifdef Q_OS_WIN
    // Преобразуем слэши для Windows в стандартные обратные слэши
    higherLevelDir = QDir::toNativeSeparators(higherLevelDir);  // Преобразуем путь в формат с обратными слэшами
    QString appFilePath = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());  // Путь к самому приложению

    // Отладочные сообщения для проверки путей
    qDebug() << "higherLevelDir (после преобразования):" << higherLevelDir;
    qDebug() << "appFilePath (после преобразования):" << appFilePath;

    QString batchScript = QString(
                              "@echo off\n"
                              "echo Удаление файлов...\n"  // Отладочное сообщение
                              "ping 127.0.0.1 -n 5 > nul\n"  // Небольшая задержка
                              "del /F /Q \"%1\"\n"  // Удаление самого uninstall.exe
                              "if exist \"%1\" echo Не удалось удалить %1\n"  // Проверка, удалось ли удалить
                              "rmdir /S /Q \"%2\"\n"  // Удаление директории после удаления всех файлов
                              "if exist \"%2\" echo Не удалось удалить %2\n"
                              ).arg(appFilePath, higherLevelDir);  // Используем корректные пути

    QString batchFilePath = QDir::temp().filePath("uninstall.bat");

    // Отладка: проверка пути к batch-файлу
    qDebug() << "Batch file path:" << batchFilePath;

    QFile batchFile(batchFilePath);
    if (batchFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "batchFile.open успешен";
        batchFile.write(batchScript.toUtf8());
        batchFile.close();

        // Запуск batch-файла для удаления
        if (!QProcess::startDetached("cmd.exe", {"/C", batchFilePath})) {
            qDebug() << "Ошибка: Не удалось запустить batch-файл.";
        }
    } else {
        qDebug() << "Ошибка: Не удалось создать batch-файл для удаления.";
    }

#else
    qDebug() << "Функция удаления для данной ОС не реализована.";
#endif
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
    LONG result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE"), 0, KEY_ALL_ACCESS, &hKey);

    if (result == ERROR_SUCCESS) {
        // Удаляем ключ ITSolutions
        result = RegDeleteKey(hKey, TEXT("ITSolutions"));
        if (result == ERROR_SUCCESS) {
            qDebug() << "Ключ ITSolutions успешно удален.\n";
        } else {
            qDebug() << "Не удалось удалить ключ ITSolutions. Ошибка: " << result << "\n";
        }

    } else {
        qDebug() << "Не удалось открыть ключ SOFTWARE. Ошибка: " << result << "\n";
    }
}

bool UnInstall::checkers()
{
    bool isChecked = ( ui->checkBoxIts->isChecked() );
    if (isChecked)
    {
        QString style = R"(
QPushButton:hover{
    background-color: rgba(1, 87, 253, 220);
}
QPushButton{
    background-color: rgb(1, 87, 253);
    border-radius: 4px;
    color: white;
    font: 16px;
}
)";
        ui->pushButton->setStyleSheet(style);
    }
    else
    {
        QString style = R"(
QPushButton:hover{
    background-color: rgba(152, 155, 163, 220);
}
QPushButton{
    background-color: rgb(152, 155, 163);
    border-radius: 4px;
    color: white;
    font: 16px;
}
)";
        ui->pushButton->setStyleSheet(style);
    }
    return isChecked;
}

bool UnInstall::isClearRegisterValue()
{
    bool isEmpty = false;
    QDir directory(subkeyPath + "\\ITS");
    if (directory.exists()) isEmpty = true;
    return isEmpty;
}


void UnInstall::on_checkBoxIts_stateChanged(int arg1)
{

}

bool UnInstall::checkRegistryValue(HKEY hKey, const char* valueName) {
    // Преобразуем имя значения в формат TCHAR (для работы с WinAPI)
    TCHAR valueNameT[MAX_PATH];
    MultiByteToWideChar(CP_ACP, 0, valueName, -1, valueNameT, MAX_PATH);

    // Проверяем наличие значения с помощью RegQueryValueEx
    LONG result = RegQueryValueEx(hKey, valueNameT, NULL, NULL, NULL, NULL);

    if (result == ERROR_SUCCESS) {
        qDebug() << "Значение " << valueName << " существует в реестре.\n";
        return true;
    } else if (result == ERROR_FILE_NOT_FOUND) {
        qDebug() << "Значение " << valueName << " не найдено в реестре.\n";
    } else {
        qDebug() << "Ошибка при проверке значения " << valueName << ". Код ошибки: " << result << "\n";
    }

    return false;
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

