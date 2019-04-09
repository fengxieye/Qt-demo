#include "testthread.h"
//class Cnum
//{
//public:
//    Cnum()
//    {
//        qInfo()<< "construct start";
//        QThread::sleep(3);
//        num++;
//        qInfo() << "construct stop";
//    }
//    void Test()
//    {
//        qInfo() << "Test";
//        num++;
//        qInfo() <<num;
//    }

//    static int num;
//};
//int Cnum::num=0;

TestThread::TestThread(QThread *parent) : QThread(parent)
{

}

void TestThread::run()
{
    gTest->doTest(); //会绕过构造函数直接调用doTest,
                       //但其中的变量不是线程安全的

//    static Cnum a;
//    qInfo()<<"ok";
//    a.Test();
}
