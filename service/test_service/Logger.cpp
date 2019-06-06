#include "Logger.h"

//打开外部程序所需
#include <UserEnv.h>
#pragma comment(lib, "userenv.lib")
#include <WtsApi32.h>
#pragma comment(lib, "WtsApi32.lib")

//默认构造函数
Logger::Logger()
{
    //初始化
    memset(m_strLogPath, 0, MAX_STR_LEN);
    memset(m_strCurLogName, 0, MAX_STR_LEN);
    m_pFileStream = NULL;
    //设置默认的写日志级别
    m_nLogLevel = EnumLogLevel::LogLevelNormal;
    //初始化临界区变量
    InitializeCriticalSection(&m_cs);
    //创建日志文件名
    GenerateLogName();
}

//构造函数
Logger::Logger(const char * strLogPath, EnumLogLevel nLogLevel)
    :m_nLogLevel(nLogLevel)
{
    //初始化
    memset(m_strLogPath, 0, MAX_STR_LEN);
    memset(m_strCurLogName, 0, MAX_STR_LEN);
    m_pFileStream = NULL;
    strcpy(m_strLogPath, strLogPath);
    InitializeCriticalSection(&m_cs);
    CreateLogPath();
    GenerateLogName();
}


//析构函数
Logger::~Logger()
{
//    释放临界区
    DeleteCriticalSection(&m_cs);
    //关闭文件流
    if(m_pFileStream)
        fclose(m_pFileStream);
}

//写关键信息接口
void Logger::TraceKeyInfo(const char * strInfo, ...)
{
    if(!strInfo)
        return;

    char pTemp[MAX_STR_LEN] = {0};
    char pTempTime[MAX_STR_LEN] = {0};
    MyGetCurrentTime(pTempTime);
    strcpy(pTemp, pTempTime);
    strcat(pTemp, KEYINFOPREFIX);

    //获取可变形参
    va_list arg_ptr = NULL;
    va_start(arg_ptr, strInfo);
    vsprintf(pTemp + strlen(pTemp), strInfo, arg_ptr);
    va_end(arg_ptr);
    //写日志文件
    Trace(pTemp);
    arg_ptr = NULL;

}

//写错误信息
void Logger::TraceError(const char* strInfo, ...)
{
    if(m_strLogPath == "")
    {
        return;
    }
    //判断当前的写日志级别，若设置为不写日志则函数返回
    if(m_nLogLevel >= EnumLogLevel::LogLevelStop)
        return;
    if(!strInfo)
        return;

    char pTemp[MAX_STR_LEN] = {0};
    char pTempTime[MAX_STR_LEN] = {0};
    MyGetCurrentTime(pTempTime);
    strcpy(pTemp, pTempTime);
    strcat(pTemp, ERRORPREFIX);

    va_list arg_ptr = NULL;
    va_start(arg_ptr, strInfo);
    vsprintf(pTemp + strlen(pTemp), strInfo, arg_ptr);
    va_end(arg_ptr);
    Trace(pTemp);
    arg_ptr = NULL;
}

//写警告信息
void Logger::TraceWarning(const char * strInfo, ...)
{
    //判断当前的写日志级别，若设置为只写错误信息则函数返回
    if(m_nLogLevel >= EnumLogLevel::LogLevelNormal)
        return;
    if(!strInfo)
        return;

    char pTemp[MAX_STR_LEN] = {0};
    char pTempTime[MAX_STR_LEN] = {0};
    MyGetCurrentTime(pTempTime);
    strcpy(pTemp, pTempTime);
    strcat(pTemp, WARNINGPREFIX);

    va_list arg_ptr = NULL;
    va_start(arg_ptr, strInfo);
    vsprintf(pTemp + strlen(pTemp), strInfo, arg_ptr);
    va_end(arg_ptr);
    Trace(pTemp);
    arg_ptr = NULL;
}


//写一般信息
void Logger::TraceInfo(const char * strFmt, ...)
{
    //判断当前的写日志级别，若设置只写错误和警告信息则函数返回
    if(m_nLogLevel >= EnumLogLevel::LogLevelMid)
        return;
    if(!strFmt)
        return;

    char pTemp[MAX_STR_LEN] = {0};
    char pTempTime[MAX_STR_LEN] = {0};
    MyGetCurrentTime(pTempTime);
    strcpy(pTemp, pTempTime);
    strcat(pTemp, INFOPREFIX);

    va_list arg_ptr = NULL;
    va_start(arg_ptr, strFmt);
    vsprintf(pTemp + strlen(pTemp), strFmt, arg_ptr);
    va_end(arg_ptr);
    Trace(pTemp);
    arg_ptr = NULL;
}

////获取系统当前时间,不要用它，会出错的
//char * Logger::MyGetCurrentTime()
//{
//    time_t curTime;
//    struct tm * pTimeInfo = NULL;
//    time(&curTime);
//    pTimeInfo = localtime(&curTime);
//    static char temp[MAX_STR_LEN] = {0};
//    sprintf(temp, "%02d:%02d:%02d", pTimeInfo->tm_hour, pTimeInfo->tm_min, pTimeInfo->tm_sec);
//    char * pTemp = temp;
//    return temp;
//}

//获取系统当前时间
void Logger::MyGetCurrentTime(char* strTime)
{
    time_t curTime;
    struct tm * pTimeInfo = NULL;
    time(&curTime);
    pTimeInfo = localtime(&curTime);
    sprintf(strTime, "%02d:%02d:%02d", pTimeInfo->tm_hour, pTimeInfo->tm_min, pTimeInfo->tm_sec);
}

//设置写日志级别
void Logger::SetLogLevel(EnumLogLevel nLevel)
{
    m_nLogLevel = nLevel;
}

//写文件操作
void Logger::Trace(const char * strInfo)
{
    if(!strInfo)
        return;
    try
    {
        //进入临界区
        EnterCriticalSection(&m_cs);
        //若文件流没有打开，则重新打开
        if(!m_pFileStream)
        {
            char temp[1024] = {0};
            strcat(temp, m_strLogPath);
            strcat(temp, m_strCurLogName);
            m_pFileStream = fopen(temp, "a+");
            if(!m_pFileStream)
            {
                return;
            }
        }
        //写日志信息到文件流
        fprintf(m_pFileStream, "%s\n", strInfo);
        fflush(m_pFileStream);
        fclose(m_pFileStream);
        m_pFileStream = NULL;

        //离开临界区
        LeaveCriticalSection(&m_cs);
    }
    //若发生异常，则先离开临界区，防止死锁
    catch(...)
    {
        LeaveCriticalSection(&m_cs);
    }
}

//创建日志文件的名称
void Logger::GenerateLogName()
{
    time_t curTime;
    struct tm * pTimeInfo = NULL;
    time(&curTime);
    pTimeInfo = localtime(&curTime);
    char temp[1024] = {0};
    //日志的名称如：2013-01-01.log
    sprintf(temp, "%04d-%02d-%02d.log", pTimeInfo->tm_year+1900, pTimeInfo->tm_mon + 1, pTimeInfo->tm_mday);
    if(m_strLogPath == "\0")
    {
        return;
    }
    if(0 != strcmp(m_strCurLogName, temp))
    {
        strcpy(m_strCurLogName,temp);
        if(m_pFileStream)
            fclose(m_pFileStream);
        strcat(temp, m_strLogPath);
        strcat(temp, m_strCurLogName);
        //以追加的方式打开文件流
//        m_pFileStream = fopen(temp, "a+");
    }
}

//创建日志文件的路径 结尾+ /还是\\ 待考究
void Logger::CreateLogPath()
{
//    if(0 != strlen(m_strLogPath))
//    {
//        strcat(m_strLogPath, "/");
//    }
//    USES_CONVERSION;
//    LPCWSTR lp = A2W(m_strLogPath);
//    CreateDirectory(lp,NULL);


//   // MakeSureDirectoryPathExists(m_strLogPath);
}

//在服务程序里GetEnvironmentVariable 是不准确的
char* Logger::GetUserPath()
{
//    std::string path= "";
    static wchar_t homePath[MAX_STR_LEN/2] = {0};
    unsigned int pathSize = GetEnvironmentVariable(L"USERPROFILE", homePath, 1024);
    if (pathSize == 0 || pathSize > MAX_STR_LEN/2)
    {
        // 获取失败 或者 路径太长
        int ret = GetLastError();
        return NULL;
    }
    else
    {
//        path = std::string(homePath);
//        USES_CONVERSION;
//        char* ch = W2A(homePath);
//        char str[MAX_STR_LEN] = {0};
//        sprintf(str,"%s",ch);
//        str[strlen(str)]='\\';
//        printf("%s,%d\n",str,strlen(str));
//        return str;
    }

    //尝试在服务里获取用户
//    DWORD sessionId = WTSGetActiveConsoleSessionId();
//    wchar_t* ppBuffer[100];
//    DWORD bufferSize;
//    WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE, sessionId, WTSUserName, ppBuffer, &bufferSize);
//    USES_CONVERSION;
//    char* ch = W2A(ppBuffer[0]);
//    char str[MAX_STR_LEN] = {0};
//    sprintf(str,"C:\\Users\\%s",ch);
//    str[strlen(str)]='\\';
//    printf("%s,%d\n",str,strlen(str));
//    return str;
}

void Logger::SetPath(const char* path)
{
    sprintf(m_strLogPath,"%s",path);
}

void Logger::SetPathAndCreate(const char* path)
{
    sprintf(m_strLogPath,"%s",path);
    CreateLogPath();
}

char* Logger::GetPath()
{
    return m_strLogPath;
}


