#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <psapi.h>
#include <iostream>
#include <STDLIB.H>
#include <fstream>
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")

using namespace std;

// Создание процесса командной строки, которая выполняет команду в невидимом режиме.
void execute(LPSTR command) {
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    CreateProcess (NULL, command, NULL, NULL, 0, 
    CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
}

// Получение пути до {Disk}:\Users\{Username}, отбрасывая часть с AppData\Roaming.
// Добавление имени вируса в конец пути.
string getPath(string target) {
    string appdata = getenv("APPDATA");
    int l = appdata.size();
    appdata = appdata.substr(0, l - 15);
    appdata.append(target);
    return appdata;
}

// Удаление вируса по пути {Disk}:\Users\{Username}.
void deleteTargetFromPath(string path) {
    string symbs = "CDEF";
    for (char c : symbs) {
        string s(1, c);
        path = path.replace(0, 1, s);
        remove(path.c_str());
    }
}

// Проверка полученного процесса.
void checkProcess(DWORD processID, string target, TCHAR targetName[MAX_PATH])
{
    HMODULE hMod;
    DWORD   cbNeeded;
    TCHAR   szProcessName[MAX_PATH] = TEXT("<unknown>");
    
    // Получение процесса.
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID);

    // Проверка является ли процесс <ничем>.
    if (NULL == hProcess) {
        CloseHandle(hProcess);
        return;
    }

    // Получение имени процесса.
    if ( EnumProcessModules( hProcess, &hMod, sizeof(hMod), &cbNeeded) )
    {
        GetModuleBaseName( hProcess, hMod, szProcessName,
         sizeof(szProcessName)/sizeof(TCHAR) );
    }
    
    // Сверка имени процесса и имени вируса.
    if (!memcmp(targetName, szProcessName, MAX_PATH)) {
        
        // Выключение вируса.
        HANDLE killProcess = OpenProcess(PROCESS_TERMINATE, 0,processID);
        TerminateProcess(killProcess, 1);
        CloseHandle(killProcess);
        cout << "Process " << targetName << " disabled" << endl;
    }
    // Закрытие процесса.
    CloseHandle(hProcess);
}

// Получение и проверка всех процессов.
void getProcesses(string target, TCHAR targetName[MAX_PATH]) {
    DWORD aProcesses[1024], cbNeeded, cProcesses;
    unsigned int i;

    // Получение всех процессов системы.
    if (!EnumProcesses( aProcesses, sizeof(aProcesses), &cbNeeded ))
    {
        cout << "EnumProcesses failed: " << GetLastError() << endl;
        return;
    }

    // Расчет сколько процессов будет возвращено.
    cProcesses = cbNeeded / sizeof(DWORD);

    // Прогон процессов.
    for (i = 0; i < cProcesses; i++)
    {
        if(aProcesses[i] != 0)
        {   
            checkProcess(aProcesses[i], target, targetName);
        }
    }
    cout << "Processes are checked." << endl;
    return;
}

/*
Изменения в regedit'e.
command - смена показа скрытых папок.
command2 - удаление автозапуска вируса.
command3 - добавление антивируса в автозапуск.
*/
void regeditChanges(string targetReg, string antiVirusReg, string hiddenDirs) {
    const LPSTR command = const_cast<char *>(hiddenDirs.c_str());
    const LPSTR command2 = const_cast<char *>(targetReg.c_str());
    const LPSTR command3 = const_cast<char *>(antiVirusReg.c_str());
    execute(command);
    execute(command2);
    execute(command3);
}

/*
Копирование антивируса.
command - создание папки антивируса на диске C.
command2 - перенос приложения антивируса.
*/
void copyAntiVirus(char * path, char * path2) {
    const string readme = "Антивирус сделан Statuxia (TG: Statuxia)\nЦель антивируса: Удалить вирус-червь smss.exe (Находтся по пути C:\\Users\\{user}), распространяющийся через флешки под видом папки.\nРабота антивируса: Копирует себя в путь C:\\antivirus, после чего ищет вирус и удаляет его. Также чистит реестр от автозапуска этого вируса.";
    const string mkdir = "cmd.exe /c mkdir C:\\antivirus";
    string copy = "cmd.exe /c copy ";
    copy.append(path);
    copy.append(" ");
    copy.append(path2);
    
    const LPSTR command = const_cast<char *>(mkdir.c_str());
    const LPSTR command2 = const_cast<char *>(copy.c_str());
    execute(command);
    Sleep(10);
    execute(command2);
    Sleep(10);
    ofstream out("C:\\antivirus\\README.txt");
    out << readme;
    out.close();
}

int main(int argc, char *argv[]) {
    const string target = "smss.exe";
    const string targetReg = "cmd.exe /c reg delete \"HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Run\" /v smss /f\"";
    const string antiVirusReg = "cmd.exe /c reg add \"HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Run\" /v antivirus /t reg_sz /d C:\\antivirus\\antivirus.exe /f\"";
    const string hiddenDirs = "cmd.exe /c reg add \"HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced\" /v Hidden /t reg_dword /d 1 /f\"";
    char * correctPath = "C:\\antivirus\\antivirus.exe";
    char * copyPath = "C:\\antivirus\\";
    argv[0][0] = toupper(argv[0][0]); // Смена регистра первого символа.

    // Получение имени процесса типа TCHAR.
    TCHAR targetName[MAX_PATH] = TEXT("");
    copy(target.begin(), target.end(), targetName);
    targetName[target.size()] = 0;
    
    /*
    Проверка запущен ли процесс из папки автозапуска. 
    В случае отрицательного результата копирует себя и запускает уже от туда.
    */
    if (strcmp(argv[0], correctPath) != 0) {
        copyAntiVirus(argv[0], copyPath);
        const string start = "cmd.exe /c start c:\\antivirus\\antivirus.exe";
        const LPSTR command = const_cast<char *>(start.c_str());
        execute(command);
        return 0;
    } else {
        // Цикл с 6 секундным тайм-аутом.
        while (true) {
            regeditChanges(targetReg, antiVirusReg, hiddenDirs);
            getProcesses(target, targetName);
            Sleep(10);
            deleteTargetFromPath(getPath(target));
            Sleep(6000);
        }
        return 0;
    }
}