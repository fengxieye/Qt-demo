#ifndef SINGLETON_H
#define SINGLETON_H
/**
 *
 *  static 构造是线程安全的，但里面的变量不是线程安全的
 *  假如构造函数里面对变量进行了操作，需要加锁或单例提前初始化
 *
**/

//最原始方式
//class Singleton
//{
//public:
//    static Singleton* instance()
//    {
//        static Singleton ins;
//        return &ins;
//    }

//private:
//    Singleton(){}
//};


/* ***************************************
 *
 * 有两种方式使用这个类,一种是在自己类里通过friend添加为友元
 * 一种是typedef Singleton<_class> class用于按需创建的单例对象
 *
 * ***************************************/
template<typename T>
class Singleton
{
public:
    static T* instance()
    {
        static T ins;
        return &ins;
    }

//如果要让继承类访问,就不要实现此构造函数
private:
    Singleton(){}
};


#endif // SINGLETON_H
