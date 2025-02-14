#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::removeDirectory(const QString &dirPath) {
    QDir dir(dirPath);
    if (!dir.exists()) {
        return false;
    }

    foreach (QString file, dir.entryList(QDir::Files)) {
        dir.remove(file);
    }

    foreach (QString subDir, dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        removeDirectory(dir.absoluteFilePath(subDir));
    }

    return dir.rmdir(dirPath);
}

void MainWindow::scheduleSelfDeletion(const QString &executablePath) {
#ifdef Q_OS_WIN
    QString batchScript = QString(
                              "@echo off\n"
                              "ping 127.0.0.1 -n 5 > nul\n"
                              "del /F /Q \"%1\"\n"
                              "del /F /Q \"%2\"\n"
                              "exit\n"
                              ).arg(executablePath).arg(QCoreApplication::applicationFilePath());

    QString batchFilePath = QDir::temp().filePath("uninstall.bat");

    QFile batchFile(batchFilePath);
    if (batchFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        batchFile.write(batchScript.toUtf8());
        batchFile.close();

        QProcess::startDetached("cmd.exe", {"/C", batchFilePath});
    } else {
        qDebug() << "Не удалось создать batch-файл для удаления.";
    }
#else \
    // Для других ОС можно использовать аналогичный подход с использованием shell-скриптов
    qDebug() << "Функция удаления для данной ОС не реализована.";
#endif
}
