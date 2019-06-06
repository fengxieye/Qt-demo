#include <Windows.h>
#include <QCoreApplication>
#include <iostream>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QDir>
#include <QSettings>
#include <Logger.h>
#include <QStandardPaths>
#include <define.h>
using namespace std;

//sc service所需
#pragma comment(lib,"Advapi32.lib")
#pragma comment(lib,"user32.lib")

//IsExistProcess 需要
#include <tlhelp32.h>

//打开外部程序所需
#include <UserEnv.h>
#pragma comment(lib, "userenv.lib")
#include <WtsApi32.h>
#pragma comment(lib, "WtsApi32.lib")

#include <Wtsapi32.h>
#include <userenv.h>

//写日志类
Logger* logger = NULL;

//使用时为了方便获取程序路径或用户路径，不使用qt那service获取路径太费劲
//因为service和用户不是在同一个会话,也是不能获取用户路径，留着吧，有些东西用Qt还是
//比较方便
int argc2 = 0;
char** argv2 = 0;
QCoreApplication a(argc2,argv2);

//服务相关内容
SERVICE_STATUS_HANDLE g_serviceStatusHandle = NULL;
SERVICE_STATUS serviceStatus;

//调试下使用这个，不调试就没必要了
int WriteToLog(const char * str)
{
    FILE* log;
    log = fopen(LOGFILE,"a+");
    if(log == NULL)
    {
        return -1;
    }
    fprintf(log,"%s\n",str);
    fclose(log);
    return 0;
}

//#define SAFE_CALL(FuncCall, ErrorCode)		                        \
//    if (FuncCall == ErrorCode) {\
//        DWORD num = GetLastError();\
//        QFile file("E://tmemstatus.txt");                     \
//        file.open(QFile::ReadWrite|QFile::Append); \
//        QTextStream stream(&file); \
//        stream<<__FUNCTION__<<num<<"  "<<__LINE__<<"\r\n";  \
//        file.flush();\
//        file.close();\
//        exit(-1);							                        \
//    }

#define SAFE_CALL(FuncCall, ErrorCode)		                        \
    if (FuncCall == ErrorCode) {\
        DWORD num = GetLastError();\
        QString str = QString("error %1 fun %2 line %3").arg(num).arg(__FUNCTION__).arg(__LINE__);  \
        if(logger!=NULL){ \
                logger->TraceError(str.toStdString().c_str());   \
        } \
        exit(-1);							                        \
    }


//原本是在用户目录写日志(担心权限问题)，如果无法创建，就在程序目录下写日志
//后发现qt获取的用户目录还是变了，倒不如直接在程序目录下写算了
void setLog()
{
    if(logger == NULL)
    {
        logger = new Logger();
        logger->SetLogLevel(LogLevelAll);

        QString appdir = QCoreApplication::applicationDirPath();
        QString logPath = appdir+"/log-s/";
//        QString usrdir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
//        usrdir = usrdir+"/."+PROGRAMNAME+"/log-s/";

        QDir dirLog;
        if(!dirLog.exists(logPath))
        {
            if(dirLog.mkdir(logPath))
            {
                logger->SetPath(logPath.toStdString().c_str());
            }
        }
        else
        {
            logger->SetPath(logPath.toStdString().c_str());
        }

        QDir dir(logPath);
        QFileInfoList fileList = dir.entryInfoList(QStringList() << "*.log", QDir::NoFilter, QDir::Time);
        if (fileList.size() >= LOG_FILE_KEEP_NUM)
        {
            int i = 0;
            foreach(QFileInfo fileInfo , fileList)
            {
                if (i >= LOG_FILE_KEEP_NUM)
                {
                    QString fileName = fileInfo.absoluteFilePath();
                    QFile::remove(fileName);
                }
                i++;
            }
        }

        QString strPath = QCoreApplication::applicationDirPath()+"/congig-s.ini";
        QSettings set(strPath,QSettings::IniFormat);
        set.setValue("Path",QString(logger->GetPath()));
    }
}

wstring getExeFullFilename()
{
    static wchar_t buffer[1024];
    SAFE_CALL(GetModuleFileNameW(NULL, buffer, 1024), 0);
    return wstring(buffer);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void installService()
{
    //打开scm
    auto scmHandle = OpenSCManagerW(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    SAFE_CALL(scmHandle, NULL);

    //创建服务
    auto serviceHandle = CreateServiceW(scmHandle,
                                        SERVICENAME,
                                        SERVICENAME,
                                        SERVICE_ALL_ACCESS,
                                        SERVICE_WIN32_OWN_PROCESS,
                                        SERVICE_AUTO_START,
                                        SERVICE_ERROR_NORMAL,
                                        getExeFullFilename().c_str(),
                                        NULL, NULL, L"", NULL, L"");
    if( serviceHandle == NULL)
    {
        DWORD errCode = GetLastError();
        //1073 说明服务已存在，直接启动
        if(errCode!=1073)
        {
            char str[100] = {0};
            sprintf(str,"installService wrong %d",errCode);
            WriteToLog(str);
            logger->TraceError(str);
            exit(-1);
        }
    }
    else
    {
        logger->TraceError("CreateServiceW ok");
    }

    if(scmHandle != NULL)
    {
        CloseServiceHandle(scmHandle);
    }
    if(serviceHandle != NULL)
    {
        CloseServiceHandle(serviceHandle);
    }

}

/////////////////////////////////////////////////////////////////////////////////////////////////

void uninstallService()
{
    auto scmHandle = OpenSCManagerW(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    SAFE_CALL(scmHandle, NULL);

    auto serviceHandle = OpenServiceW(	scmHandle,
                                        SERVICENAME,
                                        SERVICE_ALL_ACCESS);
    SAFE_CALL(serviceHandle, NULL);

    SERVICE_STATUS serviceStatus;
    SAFE_CALL(QueryServiceStatus(serviceHandle, &serviceStatus), 0);
    if (serviceStatus.dwCurrentState == SERVICE_RUNNING) {
        SAFE_CALL(ControlService(serviceHandle, SERVICE_CONTROL_STOP, &serviceStatus), 0);
        SAFE_CALL(serviceStatus.dwCurrentState, NO_ERROR);

        do {
            SAFE_CALL(QueryServiceStatus(serviceHandle, &serviceStatus), 0);
            Sleep(1000);
        } while (serviceStatus.dwCurrentState != SERVICE_STOPPED);
    }

    SAFE_CALL(DeleteService(serviceHandle), FALSE);

    CloseServiceHandle(scmHandle);
    CloseServiceHandle(serviceHandle);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void startService()
{
    auto scmHandle = OpenSCManagerW(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    SAFE_CALL(scmHandle, NULL);

    auto serviceHandle = OpenServiceW(	scmHandle,
                                        SERVICENAME,
                                        SERVICE_ALL_ACCESS);
    SAFE_CALL(serviceHandle, NULL);
    logger->TraceInfo("start open service");
    SERVICE_STATUS serviceStatus;
    SAFE_CALL(QueryServiceStatus(serviceHandle, &serviceStatus), 0);
    if (serviceStatus.dwCurrentState == SERVICE_START &&
        serviceStatus.dwCurrentState != SERVICE_START_PENDING)
        return;

    logger->TraceInfo(" query status");
    SAFE_CALL(StartServiceW(serviceHandle, 0, NULL), FALSE);

    logger->TraceInfo("startservice over");
    CloseServiceHandle(scmHandle);
    CloseServiceHandle(serviceHandle);
}


void setServiceStatus(DWORD status)
{
    serviceStatus.dwCurrentState = status;
    SAFE_CALL(SetServiceStatus(g_serviceStatusHandle, &serviceStatus), 0);
}


VOID WINAPI ServiceHandler(DWORD controlCode)
{
    switch (controlCode)
    {
        case SERVICE_CONTROL_CONTINUE:
            setServiceStatus(SERVICE_START_PENDING);
            break;
        case SERVICE_CONTROL_INTERROGATE:
            break;
        case SERVICE_CONTROL_PAUSE:
            setServiceStatus(SERVICE_PAUSED);
            break;
        case SERVICE_CONTROL_SHUTDOWN:
            setServiceStatus(SERVICE_STOPPED);
            break;
        case SERVICE_CONTROL_STOP:
            setServiceStatus(SERVICE_STOPPED);
            break;
        default:
            break;
    }

    logger->TraceInfo("state change %d ",controlCode);
}

BOOL IsExistProcess(const char*  szProcessName)
{

    //PROCESSENTRY32结构体，保存进程具体信息
    PROCESSENTRY32 processEntry32;
    //获取进程句柄
    HANDLE toolHelp32Snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (((int)toolHelp32Snapshot) != -1)
    {
        processEntry32.dwSize = sizeof(processEntry32);
        //先获取第一个进程
        if (Process32First(toolHelp32Snapshot, &processEntry32))
        {
            //循环进程
            do
            {
                int iLen = 2 * wcslen(processEntry32.szExeFile);
                char* chRtn = new char[iLen + 1];
                //转换成功返回为非负值
                wcstombs(chRtn, processEntry32.szExeFile, iLen + 1);
                if (strcmp(szProcessName, chRtn) == 0)
                {
                    delete[] chRtn;
                    chRtn = NULL;
                    CloseHandle(toolHelp32Snapshot);
                    return TRUE;
                }
                if(chRtn!=NULL)
                {
                    delete[] chRtn;
                }

            } while (Process32Next(toolHelp32Snapshot, &processEntry32));
        }
        CloseHandle(toolHelp32Snapshot);
    }
    //
    return FALSE;
}




VOID WINAPI ServiceMain(DWORD argc, LPWSTR *argv)
{
    serviceStatus.dwServiceType              = SERVICE_WIN32_OWN_PROCESS;
    serviceStatus.dwWin32ExitCode            = NO_ERROR;
    serviceStatus.dwServiceSpecificExitCode  = 0;
    serviceStatus.dwWaitHint                 = 2000;
    serviceStatus.dwCheckPoint               = 0;
    serviceStatus.dwControlsAccepted         =  SERVICE_ACCEPT_PAUSE_CONTINUE |
                                                SERVICE_ACCEPT_SHUTDOWN |
                                                SERVICE_ACCEPT_STOP;

    g_serviceStatusHandle = RegisterServiceCtrlHandlerW(SERVICENAME, &ServiceHandler);
    WriteToLog("regist over");
    if (g_serviceStatusHandle == 0)
    {
        cout << "RegisterServiceCtrlHandlerW error, code:" << GetLastError()
            << " ,line:" << __LINE__ << "\n";
        exit(-1);
    }

    setServiceStatus(SERVICE_START_PENDING);
    WriteToLog("PENDING over");
    setServiceStatus(SERVICE_RUNNING);
    WriteToLog("SERVICE_RUNNING over");

    bool isNewStart = true;
    while(serviceStatus.dwCurrentState == SERVICE_RUNNING)
    {
        setLog();
        if(!isNewStart)
        {
            Sleep(SLEEP_TIME);
        }
        else {
            logger->TraceInfo("not sleep start");
        }

        isNewStart = false;

        char* rtn = getenv("USERPROFILE");
        WriteToLog(rtn);
        if(IsExistProcess("CControl.exe"))
        {
            WriteToLog("exist");
            continue;
        }
        else
        {
            WriteToLog("not exist");
        }


        DWORD dwSessionId = WTSGetActiveConsoleSessionId();
        if(dwSessionId == 0xFFFFFFFF)
        {
            logger->TraceInfo("no active console session");
            WriteToLog("no active console session");
            continue;
        }
        else
        {
            dwSessionId;
            logger->TraceInfo("console session %d",dwSessionId);
        }

        HANDLE hToken = NULL;
        if(!WTSQueryUserToken(dwSessionId,&hToken))
        {
            logger->TraceInfo("WTSQueryUserToken erong %d",GetLastError());
            continue;
        }
        else
        {
            logger->TraceInfo("WTSQueryUserToken ok");
        }

        STARTUPINFO si;
        PROCESS_INFORMATION pi;
        ZeroMemory(&si,sizeof(STARTUPINFO));
        ZeroMemory(&pi,sizeof(PROCESS_INFORMATION));
        si.cb = sizeof(STARTUPINFO);
        wchar_t desktopName[] = L"WinSta0\\Default";
        si.lpDesktop = desktopName;
        si.wShowWindow = TRUE;
        si.dwFlags     = STARTF_USESHOWWINDOW;

        std::wstring wstr;
        std::string str = (QCoreApplication::applicationDirPath()+"/process/"+PROGRAMNAME).toStdString();
        wstr = std::wstring(str.begin(),str.end());
        LPWSTR lp = (LPWSTR)wstr.c_str();

        LPVOID pEnv = NULL;
        if(FALSE == CreateEnvironmentBlock(&pEnv,hToken,FALSE))
        {
            logger->TraceInfo("CreateEnvironmentBlock failed");
            WriteToLog("CreateEnvironmentBlock failed");
            continue;
        }
        else
        {
            logger->TraceInfo("CreateEnvironmentBlock ok");
        }

        DWORD dwCreationFlag = NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE|CREATE_UNICODE_ENVIRONMENT;
        if(!CreateProcessAsUser(hToken,NULL,lp,NULL,NULL,FALSE,dwCreationFlag,pEnv,NULL,&si,&pi))
        {
            logger->TraceInfo("CreateProcessAsUser erong %d",GetLastError());
            WriteToLog("CreateProcessAsUser failed");
            continue;
        }
        else
        {
             logger->TraceInfo("open control sucess");
        }

        CloseHandle(pi.hProcess);

        CloseHandle(pi.hThread);

        if (hToken != NULL)
            CloseHandle(hToken);
        if (pEnv != NULL)
            DestroyEnvironmentBlock(pEnv);

        WriteToLog("finish");
    }
}




void runService()
{
    WriteToLog("runService");
    logger->TraceInfo("%s",__FUNCTION__);
    const SERVICE_TABLE_ENTRYW serviceTable[] = {
        {LPWSTR(L""), (LPSERVICE_MAIN_FUNCTION)ServiceMain },
        { NULL, NULL }
    };
    SAFE_CALL(StartServiceCtrlDispatcherW(&serviceTable[0]), 0);
    WriteToLog("run over");
}


// -install 启动 先进入argc2，然后代码里的start  会使程序进入argc1
int main(int argc, char** argv)
{
//    printf("start");
    setLog();
    logger->TraceInfo("main start");

//    printf("start1");
    if (argc == 1)
    {
        logger->TraceInfo("argc 1");
        runService();
    }
    else if (argc == 2)
    {
        // -install 如果已存在直接启动
        logger->TraceInfo("argc 2");
        logger->TraceInfo(argv[1]);
//        if (argv[1] == ("-install"))
        if(strcmp(argv[1],"-install")==0)
        {
            logger->TraceInfo("into -install");
            installService();
            startService();
        }
        if(strcmp(argv[1],"-start")==0)
        {
            startService();
        }
        if(strcmp(argv[1],"-uninstall")==0)
        {
            uninstallService();
        }
    }
    else
    {
//        std::cout << "usage: a.exe [-install/-uninstall]";
    }

    return 0;
}
