#ifndef LAMEMP3ENCODER_H
#define LAMEMP3ENCODER_H
#include "lame.h"
#include <stdint.h>
#include <QtGlobal>
#include <QFile>
#include <QDebug>

//采样频率可以选大的,保证原始质量.
#define SAMPLE_RATE  (32000)
//这个数越大,回调的时机会越晚,因为要填满buff才触发.我们做录音不实时播放无所谓,故取值较大.
//如果取值小,实时性高,但是会有多线程保护和额外开销的问题.
#define FRAMES_PER_BUFFER (6400)  //640
class LameMp3Encoder
{
public:
    LameMp3Encoder();
    ~LameMp3Encoder();
    quint64 fileSize();
    bool init(const char *filePath, uint32_t sampleRate, int bufferSize = FRAMES_PER_BUFFER*2);
    bool handle(short *data, unsigned long frameCount);
    void drop();
    void finish();

private:
    lame_global_flags* flags_;
    unsigned bufferSize_;
    unsigned char* mp3buf;
    FILE* fp_;
    quint64 fileSize_;
    int tagPos_;
    char filePath_[350];
};

#endif // LAMEMP3ENCODER_H
