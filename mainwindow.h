#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QCoreApplication>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    // Функция для рекурсивного удаления директории
    bool removeDirectory(const QString &dirPath);

    // Функция для удаления текущего исполняемого файла
    void scheduleSelfDeletion(const QString &executablePath);
};
#endif // MAINWINDOW_H
