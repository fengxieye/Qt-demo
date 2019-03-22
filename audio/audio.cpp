#include "audio.h"

Audio::Audio():
    parcd_(NULL)
  , lame_(NULL)
  , stopFlag_(false)
{

}

void Audio::start()
{
    stopFlag_ = false;
    mutex_.lock();
    if(parcd_ == NULL)
    {
        parcd_ = new PortAudioRecord(this);
    }
    parcd_->set(SAMPLE_RATE,FRAMES_PER_BUFFER);
    parcd_->start();

    if(lame_ == NULL)
    {
        QDateTime curTime = QDateTime::currentDateTime();
        QString timeStr = curTime.toString("yyMMddhhmmss");
        QString audioname = QDir::currentPath()+"/"+timeStr+QString(".mp3");
        lame_ = new LameMp3Encoder;
        if(false == lame_->init(audioname.toLocal8Bit().data(),SAMPLE_RATE))
        {
            qInfo()<<"init lame wrong";
        }
    }
    mutex_.unlock();
}

void Audio::stop()
{
    stopFlag_ = true;
    if(parcd_){
        parcd_->stop();
    }
    if(lame_)
    {
        lame_->finish();
        delete lame_;
        lame_ = NULL;
    }
}

bool Audio::tPaHandle(const void *input, unsigned long frameCount)
{
    char internalBuf[FRAMES_PER_BUFFER*2];
    Q_ASSERT(frameCount == FRAMES_PER_BUFFER);
    if(frameCount != FRAMES_PER_BUFFER) return false;
    mutex_.lock();
    if(stopFlag_){
        mutex_.unlock();
        return false;
    }
    memcpy(internalBuf,input,FRAMES_PER_BUFFER*2);
    lame_->handle((short*)internalBuf,frameCount);
    mutex_.unlock();
    if(stopFlag_) return false;
    return true;
}

void Audio::tPaNotice(ISinkForPA::ErrorType et)
{

}

void Audio::tPaNeedPad(double time)
{
    char internalBuf[FRAMES_PER_BUFFER*2]={0};
    int total = time*SAMPLE_RATE;
    while(total > 0){
        if(total>=FRAMES_PER_BUFFER){
            lame_->handle((short*)internalBuf,FRAMES_PER_BUFFER);
            total-=FRAMES_PER_BUFFER;
        }
        else{
            lame_->handle((short*)internalBuf,total);
            total = 0;
        }
    }
}
