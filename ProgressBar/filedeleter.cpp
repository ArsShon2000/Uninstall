#include "filedeleter.h"


FileDeleter::FileDeleter(QWidget *parent) : QWidget(parent), progressBar(new QProgressBar(this)) {
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(progressBar);
    setLayout(layout);
    setWindowTitle("Удаление файлов");
    resize(300, 100);
}

bool FileDeleter::deleteFilesWithProgress(const QString &dirPath, const QStringList &excludedDirs) {
    QDir dir(dirPath);
    if (!dir.exists()) {
        QMessageBox::critical(this, "Ошибка", "Директория не существует.");
        return false;
    }

    // Список всех файлов и папок в директории
    QFileInfoList fileList = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    int totalFiles = fileList.size();
    if (totalFiles == 0) {
        QMessageBox::information(this, "Информация", "Нет файлов для удаления.");
        return false;
    }

    // Настраиваем прогресс-бар
    progressBar->setMinimum(0);
    progressBar->setMaximum(totalFiles);
    progressBar->setValue(0);

    int deletedFiles = 0;
    for (const QFileInfo &fileInfo : fileList) {
        QString fileName = fileInfo.fileName();
        QString absoluteFilePath = fileInfo.absoluteFilePath();

        // Проверяем, находится ли файл или папка в списке исключений
        if (excludedDirs.contains(fileName)) {
            qDebug() << "Папка" << fileName << "в списке исключений, пропускаем...";
            continue;
        }

        if (fileInfo.isDir()) {
            removeDirectory(absoluteFilePath, excludedDirs); // Используем рекурсивное удаление
        } else {
            if (!QFile::remove(absoluteFilePath)) {
                qDebug() << "Не удалось удалить файл:" << absoluteFilePath;
            }
        }

        // Обновляем прогресс-бар
        deletedFiles++;
        progressBar->setValue(deletedFiles);

        // Позволяет интерфейсу обновляться
        QApplication::processEvents();
    }
    return true;
}

bool FileDeleter::removeDirectory(const QString &dirPath, const QStringList &excludedDirs) {
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
