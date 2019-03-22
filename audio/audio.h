#ifndef AUDIO_H
#define AUDIO_H
#include "lamemp3encoder.h"
#include "PortAudioRecord.h"
#include <QDateTime>
#include <QDir>
//采样频率可以选大的,保证原始质量.
#define SAMPLE_RATE  (32000)

#define FRAMES_PER_BUFFER (6400)  //6400
class Audio: public ISinkForPA
{
public:
    Audio();
    void start();
    void stop();

protected:
    //线程调用.返回true继续,否则中止.
    bool tPaHandle(const void *input,unsigned long frameCount);
    void tPaNotice(ISinkForPA::ErrorType et);
    void tPaNeedPad(double time);

private:
    PortAudioRecord* parcd_;
    LameMp3Encoder* lame_;
    QMutex mutex_;
    volatile bool stopFlag_;
};

#endif // AUDIO_H
