#ifndef FILEDELETER_H
#define FILEDELETER_H

#include <QApplication>
#include <QProcess>
#include <QWidget>
#include <QVBoxLayout>
#include <QProgressBar>
#include <QDir>
#include <QMessageBox>

class FileDeleter : public QWidget
{
    Q_OBJECT
public:
    explicit FileDeleter(QWidget *parent = nullptr);
    bool deleteFilesWithProgress(const QString &dirPath, const QStringList &excludedDirs);
    bool removeDirectory(const QString &dirPath, const QStringList &excludedDirs);

private:
    QProgressBar *progressBar;

signals:
};

#endif // FILEDELETER_H
