#include "PortAudioRecord.h"
#include <QDebug>

//说明:input是从设备读到的数据,字节长度是frameCount*sizeof(SAMPLE)
//如果要echo,需要在初始化的时候指定好设备,把input拷到output即可.
//#define NODEVICEGIVEUP (5*60*1000) //无设备放弃时间.

//定义pa的sample类型为int16,这个可以配合webrtc模块
#define PA_SAMPLE_TYPE paInt16
//对应的sample单位是short,占2字节.
typedef short SAMPLE;

class PaInitMana
{
public:
    PaInitMana():initOK(false){}
    bool set()  { if(!initOK) initOK = (paNoError == Pa_Initialize()); return initOK;  }
    bool reset(){ if(initOK) unset(); return set();  }
    void unset(){ if(initOK) Pa_Terminate();  initOK = false; }
    operator bool(){ return initOK; }
private:
    bool initOK;
} gPaInit;


PortAudioRecord::PortAudioRecord(ISinkForPA *cb)
    : cb_(NULL)
    , paStream_(NULL)
    , feedTime_(0)
    , lastDacTime_(-1.0) //小于0代表未知.
    , deviceIdx_(paNoDevice)
    , sampleRate_    (32000)
    , framePerBuffer_(6400)
    , haltFlag_(false)
{
    Q_ASSERT(cb);
    cb_ = cb;

    chktimer_.setInterval(500);
    chktimer_.setSingleShot(false);
    QObject::connect(&chktimer_,&QTimer::timeout,[=](){
        //检查数据缺失, 或者没设备导致的数据缺失.
        checkDataFeed();
    });
}

PortAudioRecord::~PortAudioRecord()
{
    this->stop();
}

void PortAudioRecord::set(int sampleRate, int framePerBuffer)
{
    sampleRate_ = sampleRate;
    framePerBuffer_ = framePerBuffer;
}

bool PortAudioRecord::start()
{
    qInfo()<<"Pa_start";
    etimer_.restart();
    chktimer_.start();
    if(!gPaInit.set())return false;
    lastDacTime_ = -1.0;
    Q_ASSERT(deviceIdx_==paNoDevice);
    deviceIdx_ = defaultInputDevice();
    qInfo()<<"defaultInputDevice"<<deviceIdx_;
    if(deviceIdx_ != paNoDevice)
    {
        const PaDeviceInfo *dinfo = Pa_GetDeviceInfo(deviceIdx_);
        qInfo()<<"input device name:"<<dinfo->name;
        PaError err;
        if(openStream()==false){
            paStream_ = NULL;
            deviceIdx_ = paNoDevice;
            goto start_end;
        }

        err = Pa_StartStream( paStream_ );
        qInfo()<<"Pa_StartStream"<<Pa_GetErrorText(err);
        if(err != paNoError){
            //Pa_StopStream
            Pa_CloseStream(paStream_);
            paStream_ = NULL;
            deviceIdx_ = paNoDevice;
            goto start_end;
        }
        haltFlag_ = false;
        //qInfo()<<"Pa_startStream1 succ!";
        cb_->tPaNotice(ISinkForPA::ETStreamOpen);
        return true;
    }

start_end:
    if(deviceIdx_ == paNoDevice)
    {
        cb_->tPaNotice(ISinkForPA::ETNoDevice);
        gPaInit.unset();
    }
    return true;
}
void PortAudioRecord::stop()
{
    qInfo()<<"Pa_stop";
    chktimer_.stop();
    if(paStream_){
        closeStream();
    }
    gPaInit.unset();
    lastDacTime_ = -1.0;
    feedTime_ = 0;
    haltFlag_ = false;
    deviceIdx_ = paNoDevice;
}


int PortAudioRecord::dataCallBack(
        const void *input,
        unsigned long frameCount,
        const PaStreamCallbackTimeInfo *timeInfo,
        PaStreamCallbackFlags statusFlags)
{
    if(statusFlags==0)
    {
        lastDacTime_ = timeInfo->currentTime;
        bool rt = cb_->tPaHandle(input,frameCount);
        feedTime_ = etimer_.elapsed();
        return rt?paContinue:paComplete;
    }
    else
    {
        qInfo()<<"PACallback statusFlags:"<<statusFlags<<"Abort!";
        haltFlag_ = true;
        return paAbort;
    }
}

bool PortAudioRecord::openStream()
{
    Q_ASSERT(gPaInit);
    if(!gPaInit) return false;
    PaError err;

    PaStreamParameters inputDev;
    inputDev.device = deviceIdx_; //Pa_GetDefaultInputDevice();
    inputDev.channelCount = 1;
    inputDev.sampleFormat = PA_SAMPLE_TYPE;
    inputDev.suggestedLatency = 1;
    inputDev.hostApiSpecificStreamInfo = NULL;

    PaStreamParameters outputDev;
    outputDev.device = Pa_GetDefaultOutputDevice(); //paNoDevice;  //Pa_GetDefaultOutputDevice();
    outputDev.channelCount = 1;
    outputDev.sampleFormat = PA_SAMPLE_TYPE;
    outputDev.suggestedLatency = 1;
    outputDev.hostApiSpecificStreamInfo = NULL;

    err = Pa_OpenStream(
                &paStream_,
                &inputDev,
                &outputDev,
                sampleRate_,
                framePerBuffer_, /* frames per buffer */
                paDitherOff,    /* paDitherOff, // flags */
                PortAudioRecord::PACallback,
                this);

    qInfo()<<"Pa_openStream"<<Pa_GetErrorText(err);
    return (paNoError==err);
}

void PortAudioRecord::closeStream()
{
    Q_ASSERT(gPaInit);
    if(!gPaInit) return ;
    if(paStream_){
        PaError err;
        err = Pa_StopStream( paStream_ );
        qDebug()<<"Pa_StopStream:"<<Pa_GetErrorText(err);
        err = Pa_CloseStream( paStream_ );
        qDebug()<<"Pa_CloseStream:"<<Pa_GetErrorText(err);
        paStream_ = NULL;
    }
    deviceIdx_ = paNoDevice;
}

int PortAudioRecord::PACallback(
        const void *input,
        void *output,
        unsigned long frameCount,
        const PaStreamCallbackTimeInfo *timeInfo,
        PaStreamCallbackFlags statusFlags,
        void *userData)
{
    Q_UNUSED(output);
    PortAudioRecord *media=(PortAudioRecord*)userData;
    return media->dataCallBack(input,frameCount,timeInfo,statusFlags);
}

void PortAudioRecord::checkDataFeed()
{
    if(haltFlag_)
    {
        if(paStream_){
            PaError err;
            err = Pa_CloseStream( paStream_ );
            qInfo()<<"Pa_CloseStream:"<<Pa_GetErrorText(err);
            paStream_ = NULL;
        }
        deviceIdx_ = paNoDevice;
        lastDacTime_ = -1.0;
        haltFlag_ = false;
        Q_ASSERT(gPaInit);
        gPaInit.unset();
        cb_->tPaNotice(ISinkForPA::ETError);
    }

    if(paStream_ == NULL)
    {   //枚举设备,处理热插拔.
        if(gPaInit.set())
        {
            int didx = defaultInputDevice();
            if(didx != deviceIdx_)
            {
                qInfo()<<"Pa_device changed!"<<deviceIdx_<<"->"<<didx;
                closeStream();
                deviceIdx_ = didx;

                if(openStream()==false){
                    paStream_ = NULL;
                    deviceIdx_ = paNoDevice;
                }
                else
                {
                    PaError err;
                    err = Pa_StartStream( paStream_ );
                    qInfo()<<"Pa_StartStream"<<Pa_GetErrorText(err);
                    if(err != paNoError){
                        Pa_CloseStream(paStream_);
                        paStream_ = NULL;
                        deviceIdx_ = paNoDevice;
                    }
                    else
                    {
                        cb_->tPaNotice(ISinkForPA::ETStreamOpen);
                        qInfo()<<"Pa_startStream2 succ!";
                    }
                }
            }
            if(paStream_==NULL)
            {//如果不成功,继续释放Pa
                gPaInit.unset();
            }
        }
    }

    qint64 tnow = etimer_.elapsed();
    //检查数据喂养情况,如果有较大出入,补静音.
    qint64 feedGap = tnow-feedTime_;
    if(feedGap>0 && feedGap > 1000)
    {
        if(deviceIdx_==paNoDevice)
        { //如果没有录音设备,直接补静音
//            if(lastDacTime_ >0 && tnow > NODEVICEGIVEUP)
//            {  //太久了,放弃本次录音. 只针对从来没录过音的情况.
//                cb_->tPaNotice(ISinkForPA::ETHalt);
//            }
//            else
            {
                cb_->tPaNeedPad((double)(feedGap)/1000);
                feedTime_ = tnow;
            }
        }
        else
        {
            if(lastDacTime_<=0)
            {   //如果没有来过有效数据,补静音
                cb_->tPaNeedPad((double)(feedGap)/1000);
                feedTime_ = tnow;
            }
            else
            {   //来过有效数据,补到当前时间之前.
                Q_ASSERT(paStream_);
                double unit = (double)framePerBuffer_/sampleRate_;
                double dnow = Pa_GetStreamTime(paStream_);
                qDebug()<<"dnow"<<dnow<<"lastDacTime_"<<lastDacTime_;
                double gap = dnow-lastDacTime_-unit;
                Q_ASSERT(gap>0);
                Q_ASSERT(gap*1000<feedGap);
                if(gap>0){
                    cb_->tPaNeedPad(gap);
                    lastDacTime_ += gap;  //dnow-unit;
                    feedTime_ += qint64(gap*1000);
                }
            }
        }
    }
}

int PortAudioRecord::defaultInputDevice()
{
    //注意,经过试验,如果不调用Pa_Terminate,Pa_GetDefaultInputDevice的结果不会变化.
    //如果没有调用Pa_Terminate,Pa_Initialize也不能刷新DefaultInputDevice.
    if(!gPaInit) return paNoDevice;
    return Pa_GetDefaultInputDevice();
}
