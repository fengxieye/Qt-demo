#ifndef LOGGER_H
#define LOGGER_H

/******************
 *
    网上搜了一下就拿来用了
    然而效果并不理想,各种隐患
    后面最好改进

*****************/

#include <Windows.h>
#include <stdio.h>
//#include <imagehlp.h>
#include <time.h>
//#include <string.h>
//#include <stdarg.h>
//w2a 需要
//#include <atlconv.h>

//日志级别的提示信息
static const char * KEYINFOPREFIX   = " Key: \n";
static const char * ERRORPREFIX = " Error: \n";
static const char * WARNINGPREFIX   = " Warning: \n";
static const char * INFOPREFIX      = " Info: \n";

static const int MAX_STR_LEN = 1024;
//日志级别枚举
enum EnumLogLevel
{
    LogLevelAll = 0,    //所有信息都写日志
    LogLevelMid,        //写错误、警告信息
    LogLevelNormal,     //只写错误信息
    LogLevelStop        //不写日志

};

class Logger
{
public:
    Logger();
    //构造函数
    Logger(const char * strLogPath, EnumLogLevel nLogLevel = EnumLogLevel::LogLevelNormal);
    //析构函数
    virtual ~Logger();
public:
    //写关键信息
    void TraceKeyInfo(const char * strInfo, ...);
    //写错误信息
    void TraceError(const char* strInfo, ...);
    //写警告信息
    void TraceWarning(const char * strInfo, ...);
    //写一般信息
    void TraceInfo(const char * strInfo, ...);
    //设置写日志级别
    void SetLogLevel(EnumLogLevel nLevel);
    char* GetUserPath();
//    char * MyGetCurrentTime();
    void MyGetCurrentTime(char* strTime);
    void SetPath(const char *path);
    void SetPathAndCreate(const char *path);
    char* GetPath();
private:
    //写文件操作
    void Trace(const char * strInfo);
    //获取当前系统时间

    //创建日志文件名称
    void GenerateLogName();
    //创建日志路径
    void CreateLogPath();
private:
    //写日志文件流
    FILE * m_pFileStream;
//    //写日志级别
    EnumLogLevel m_nLogLevel;
//    //日志的路径
    char m_strLogPath[MAX_STR_LEN];
//    //日志的名称
    char m_strCurLogName[MAX_STR_LEN];
//    //线程同步的临界区变量
    CRITICAL_SECTION m_cs;
};

#endif // LOGGER_H
