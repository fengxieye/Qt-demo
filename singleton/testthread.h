#ifndef TESTTHREAD_H
#define TESTTHREAD_H
#include <QThread>
#include <QObject>
#include <test.h>

class TestThread : public QThread
{
    Q_OBJECT
public:
    explicit TestThread(QThread *parent = nullptr);

    void run();
signals:

public slots:
};

#endif // TESTTHREAD_H
