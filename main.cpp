#include "uninstall.h"
#include <QApplication>
#include <QMessageBox>
#include <QProcess>
#include <windows.h>

bool isRunningAsAdmin() {
    BOOL isAdmin = FALSE;
    PSID adminGroup = NULL;

    // Получаем SID группы администраторов
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
                                 0, 0, 0, 0, 0, 0, &adminGroup)) {
        // Проверяем, входит ли текущий процесс в группу администраторов
        CheckTokenMembership(NULL, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }

    return isAdmin;
}

void restartWithAdminRights() {
    QString applicationPath = QCoreApplication::applicationFilePath();

    // Структура для запроса запуска с правами администратора
    SHELLEXECUTEINFO shExecInfo;
    memset(&shExecInfo, 0, sizeof(SHELLEXECUTEINFO));
    shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
    shExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
    shExecInfo.hwnd = NULL;
    shExecInfo.lpVerb = L"runas";  // Используем широкую строку для Unicode
    shExecInfo.lpFile = (LPCWSTR)applicationPath.utf16();  // Преобразуем QString в LPCWSTR
    shExecInfo.lpParameters = NULL;  // Аргументы (если есть)
    shExecInfo.lpDirectory = NULL;
    shExecInfo.nShow = SW_SHOWNORMAL;

    if (ShellExecuteEx(&shExecInfo)) {
        exit(0);  // Завершаем текущий процесс, если запущено с правами администратора
    } else {
        exit(0);
        QMessageBox::critical(nullptr, "Ошибка", "Не удалось перезапустить приложение с правами администратора.");
    }
}


int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    if (!isRunningAsAdmin()) {
        restartWithAdminRights();
    }
    // restartWithAdminRights();

    UnInstall unInstall;


    unInstall.show();

    return app.exec();
}
