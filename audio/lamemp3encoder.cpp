#include "lamemp3encoder.h"


#define MIN_LAME_BUFFERSIZE (7200)
static int vbr_q = 4;

LameMp3Encoder::LameMp3Encoder()
    : flags_(NULL)
    , mp3buf(NULL)
    , fp_(NULL)
    , fileSize_(0)
{

}

LameMp3Encoder::~LameMp3Encoder()
{
    this->finish();
    if(flags_) lame_close(flags_);
    if(mp3buf) delete mp3buf;
    if(fp_) fclose(fp_);
}

quint64 LameMp3Encoder::fileSize()
{
    return fileSize_;
}

bool LameMp3Encoder::init(const char* filePath, uint32_t sampleRate, int bufferSize)
{
    fileSize_ = 0;
    strcpy(filePath_,filePath);
    fp_ = fopen(filePath,"wb+");
    if(fp_ == NULL) return false;

    bufferSize_ = qMax(bufferSize,MIN_LAME_BUFFERSIZE);
    mp3buf = (unsigned char*)malloc(bufferSize_);

    flags_ = lame_init();
    if(flags_==NULL) return false;
    int ret = 0;
    lame_set_in_samplerate(flags_,sampleRate);
    lame_set_num_channels(flags_,1);
    lame_set_VBR(flags_,vbr_default); //vbr_abr);
    lame_set_VBR_quality(flags_,vbr_q);
    //lame_set_VBR_min_bitrate_kbps(flags_,32);
    //lame_set_VBR_max_bitrate_kbps(flags_,96);  //最大极限是320 kbps
    //关闭tag自动写入,因为lame_mp3_tags_fid函数崩溃.
    //参考 http://mp3-encoding.31853.n2.nabble.com/Re-lame-mp3-tags-fid-and-file-access-callbacks-td34000.html
    lame_set_write_id3tag_automatic(flags_,0);
    //lame_set_brate(flags_,32);   //好像是影响最低比特率.
    lame_set_mode(flags_,MONO);  //单声道即可,源都是单声道.
    lame_set_quality(flags_,2);  //最佳效果,webrtc的效果占主导,其实听不出来差别.
    ret = lame_init_params(flags_);
    if(ret < 0) return false;
    ret = lame_get_id3v2_tag(flags_,mp3buf,bufferSize_);
    if(ret > 0){
        fwrite(mp3buf,1,ret,fp_);
    }
    tagPos_ = ftell(fp_);  //记录位置,结束前要回头写.
    fileSize_ = ret;
    return true;
}

bool LameMp3Encoder::handle(short* data, unsigned long frameCount)
{
    if(fp_ == NULL) return false;
    Q_ASSERT(frameCount*2 <= bufferSize_);
    //int mp3bytes  = lame_encode_buffer_interleaved(flags_,data,frameCount,mp3buf,bufferSize_);
    int mp3bytes = lame_encode_buffer(flags_,data,data,frameCount,mp3buf,bufferSize_);
    if(mp3bytes < 0) return false;
    fwrite(mp3buf,1,mp3bytes,fp_);
    fileSize_ = ftell(fp_);
    return true;
}

void LameMp3Encoder::drop()
{
    if(fp_){
        fclose(fp_);
        fp_ = NULL;
    }
    qInfo()<<"Drop mp3"<<filePath_;
    QFile::remove(QString::fromLocal8Bit(filePath_));
    fileSize_ = 0;
}

void LameMp3Encoder::finish()
{
    if(flags_==NULL || bufferSize_<7200 || fp_==NULL) return;
    int mp3bytes = lame_encode_flush(flags_,mp3buf,bufferSize_);
    if(mp3bytes>0){
        fwrite(mp3buf,1,mp3bytes,fp_);
        fileSize_ = ftell(fp_);
    }
    //lame_mp3_tags_fid(flags_,fp_);  此函数崩溃,看堆栈在fseek.使用手动tag写入解决.
    mp3bytes = lame_get_id3v1_tag(flags_,mp3buf,bufferSize_);
    if(mp3bytes>0){
        fwrite(mp3buf,1,mp3bytes,fp_);
    }
    mp3bytes = lame_get_lametag_frame(flags_,mp3buf,bufferSize_);
    if(mp3bytes>0){
        fseek(fp_,tagPos_,SEEK_SET);
        fwrite(mp3buf,1,mp3bytes,fp_);
    }
    fclose(fp_);
    fp_ = NULL;
}


