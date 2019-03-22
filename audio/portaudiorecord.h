#ifndef PORTAUDIORECORDER_H
#define PORTAUDIORECORDER_H

//通过PortAudio录音的类.
//这个类特别之处是维护设备热插拔和热更替,提供缺数据(比如蓝牙话筒关话筒)解决方案
//如果设备中途断开,则持续提供静音录制.力保时间同步性.

#include "portaudio.h"
#include <QTimer>
#include <QElapsedTimer>

class ISinkForPA
{
public:
    enum ErrorType{
        ETStreamOpen,    //流打开,代表正常工作.
        ETError,         //发生中断,可能是设备断连,可以重插或等待
        ETNoDevice,
    };

    //常规数据处理
    virtual bool tPaHandle(const void *input,unsigned long frameCount)=0;
    //宣布放弃处理, 设计为一段时间都没有设备或不正常.
    virtual void tPaNotice(ErrorType et)=0; //发生故障,time是遗失的时间.如果要继续需要补相应时间的空白.
    //需要补足0数据
    virtual void tPaNeedPad(double time)=0; //需要补上这么多时间的空数据,否则时间长度不对.(蓝牙话筒关话筒的时候出现!)
};

//这个类没有做成单例,但是按单例来实现.
class PortAudioRecord
{
public:
    PortAudioRecord(ISinkForPA *cb);
    ~PortAudioRecord();
    void set(int sampleRate, int framePerBuffer);
    int defaultInputDevice();
    //除非初始化失败,否则都是成功(哪怕没设备)
    bool start();
    void stop();

public:
    static int PACallback(
            const void *input, void *output,
            unsigned long frameCount,
            const PaStreamCallbackTimeInfo* timeInfo,
            PaStreamCallbackFlags statusFlags,
            void *userData );
    int dataCallBack(
            const void *input,
            unsigned long frameCount,
            const PaStreamCallbackTimeInfo* timeInfo,
            PaStreamCallbackFlags statusFlags);
protected:
    bool openStream();
    void closeStream();
    void checkDataFeed();
private:
    ISinkForPA *cb_;
    PaStream *paStream_;
    QElapsedTimer etimer_;
    QTimer chktimer_;
    qint64 feedTime_; //已经喂饱数据的时间,以etimer_参照.
    double lastDacTime_;  //最后一次提供数据的portaudio时间.注意起始时间是不确定的.(小于0代表未知.)
    int deviceIdx_;
    int sampleRate_;
    int framePerBuffer_;
    volatile bool haltFlag_; //pa录制异常的标记.
};

#endif // PORTAUDIORECORDER_H
