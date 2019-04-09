#include <QApplication>
#include <singleton.h>
#include <test.h>
#include <testthread.h>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    //最原始的使用，非模板
//    Singleton::instance();

    //全局静态类,验证是单例了
//    Test::instance()->doTest();
//    gTest->doTest();

    Singleton<Test2>::instance()->doTest();
    gTest2->doTest();

    //验证线程安全
//    TestThread thread1;
//    TestThread thread2;
//    thread1.start();
//    thread2.start();

    //typedef Singleton<_class> class用于按需创建的单例对象
    Singleton<Test3> t3;
    t3.instance()->doTest();
    t3.instance()->doTest();
    //t4 不构建
    Singleton<Test3> t4;
    t4.instance()->doTest();

    return a.exec();
}


