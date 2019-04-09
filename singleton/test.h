#ifndef TEST_H
#define TEST_H

#include <QObject>
#include <QDebug>
#include <qthread.h>
#include "singleton.h"

#define gTest Test::instance()
//主要是把Singleton添加为友元，继承不是必要的

class Test : public Singleton<Test>
{
public:
    void doTest()
    {
        qInfo()<<"dotest"<<test;
    }

private:
    Test()
    {
        QThread::sleep(3);
        qInfo()<<"construct test";
        test = 100;
    }

private:
    //为了singleton访问test的私有构造函数
    friend class Singleton<Test>;
    int test;
};



#define gTest2 Singleton<Test2>::instance()
class Test2
{
public:
    void doTest()
    {
        qInfo()<<"dotest2";
    }

private:
    Test2()
    {
        qInfo()<<"construct test2";
    }

private:
    //为了singleton访问test的私有构造函数
    friend class Singleton<Test2>;
};

class Test3
{
public:
    void doTest()
    {
        qInfo()<<"dotest3";
    }

    Test3()
    {
        qInfo()<<"construct test3";
    }

};

#endif // TEST_H
