#include "gchttpdownload.h"
#include <QDir>
#include <QDebug>
#include <QUrl>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QFileInfo>
#include <QFile>
#include <QThread>
//#define USETHREADDOWNLOAD 1 //if 0, all functhion will run into main thread.
#define INFO_PRINT 0

GCHttpDownLoad::GCHttpDownLoad(QThread *thread, QObject *parent): QObject(parent)
{
    if(thread){
        m_downLoadThread = 0;
    }else{
        qDebug()<<"CTLHttpDownLoad:"<<(qint64)thread;
        m_downLoadThread = new QThread(this);
        m_downLoadThread->start();
        thread = m_downLoadThread;
    }
    this->moveToThread(thread);
    //m_downLoadManger must run into thread ,too. otherwise unexpected crash will happened.
    m_downLoadManger = new QNetworkAccessManager();
    m_downLoadManger->moveToThread(thread);

    m_tempPath = QDir::homePath()+"/downloadtemp";
    QDir::home().mkpath(m_tempPath);
    qDebug()<< __FUNCTION__ << "m_tempPath = " <<m_tempPath;
    connect(this,
            SIGNAL(sigToDownLoad(QString, QString, QString)),
            this,
            SLOT(onDownLoad(QString, QString, QString)));
    connect(this,
            SIGNAL(sigToStopDownLoad(QString)),
            this,
            SLOT(onStopDownLoad(QString))
            );
    connect(this,
            SIGNAL(sigToStopAllDownLoad()),
            this,
            SLOT(onStopAllDownLoad())
            );
    connect(this,
            SIGNAL(sigToGetUrlSize(QString)),
            this,
            SLOT(onGetUrlSize(QString))
            );
    connect(this, SIGNAL(sigToDownLoad(QNetworkReply*,QString)),
            this, SLOT(onDownLoad(QNetworkReply*,QString)));

}

GCHttpDownLoad::~GCHttpDownLoad()
{
    if(m_downLoadThread){
        m_downLoadThread->quit();
        if(m_downLoadThread->wait(100)){
            m_downLoadThread->terminate();
        }
        delete m_downLoadThread;
        m_downLoadThread = 0;
    }
    if(m_downLoadManger){
        delete m_downLoadManger;
        m_downLoadManger = 0;
    }
    toClearDownLoadList();
}

GCHttpDownLoad::AddResult GCHttpDownLoad::downLoad(QString url, QString name, QString dir, bool bcontinue)
{
    qDebug()<<__FUNCTION__;
    AddResult result;
    result = Unknow;
    dir = dir.isEmpty()? m_tempPath:dir;
    QDir fdir(dir);

    if(!fdir.exists()){
        qWarning()<<"false: GCHttpDownLoad :downLoad (), dir is empty!"<<dir;
        return DirError;
    }

    name = name.isEmpty()? splitUrlToName(url):name;
    if(!bcontinue && !isUrlDownloading(url)){
        QString temp = dir + "/" + name + ".temp";
        if(QFile::exists(temp)){
            QFile::remove(temp);//删除下载缓存，而不是文件
        }
    }

    QUrl durl(url);
    if(!durl.isValid() || durl.isLocalFile() || durl.isEmpty()){
        qWarning()<<"GCHttpDownLoad: Invalid url."<<url;
        return UrlInvalid;
    }

    if(isDownLoading(url)){
        qWarning()<<"GCHttpDownLoad: Url is downloading."<<url;
        return DownLoading;
    }


    if(QFile::exists(dir + "/" + name)){
        if(bcontinue){
            qWarning()<<"GCHttpDownLoad: File is download finished already."<<url;
            result = DownLoadFinished;
            emit sigFinished(url, 1, dir + "/" + name);
        }else{
            QFileInfo info(dir + "/" + name);
            while(info.exists()){
                name = info.completeBaseName().append(QString::number(qrand()%10));
                if(!info.suffix().isEmpty()){
                    name.append("." + info.suffix());
                }
                info.setFile(dir + "/" + name);
            }
            qWarning()<<"GCHttpDownLoad: File is download exist already."<<url<<
                        "Turn to"<<info.fileName();
        }
    }

    if(result == Unknow ){
        QString downfilepath = dir + "/" + name + ".temp";
        QFile file(downfilepath);
        if(file.open(QFile::Append)){
            result = AddOK;
            file.close();
            //一切妥当，下载去
            emit sigToDownLoad(url, name, dir);
        }else{
            result = FileError;
        }
    }

    return result;
}

GCHttpDownLoad::AddResult GCHttpDownLoad::downLoad(QNetworkReply *reply, QString dir)
{
    AddResult result = AddOK;
    emit sigToDownLoad(reply, dir);
    return result;
}

bool GCHttpDownLoad::setTempDir(QString tempdir)
{
    QDir dir(tempdir);
    if(dir.exists() || dir.mkpath(tempdir)){
        m_tempPath = tempdir;
        return true;
    }else{
        return false;
    }

}

void GCHttpDownLoad::stopDownLoad(QString url)
{
    emit sigToStopDownLoad(url);
}

void GCHttpDownLoad::stopAllDownLoad()
{
    emit sigToStopAllDownLoad();
}

bool GCHttpDownLoad::isDownLoading(QString url)
{
    return isUrlDownloading(url);
}

QString GCHttpDownLoad::splitUrlToName(QString url)
{
    int nIndex = url.lastIndexOf("/");
    QString name = url.right(url.size() - nIndex - 1);
    if(name.contains("?")){
        name = name.left(name.indexOf("?"));
    }
    return name;
}

QString GCHttpDownLoad::downLoadingTempFile(QString url)
{
    QMap<QNetworkReply *,DownLoadFileInfo *>::iterator itor = m_downLoadMap.begin();
    while(itor != m_downLoadMap.end()){
        if(itor.key()->url().toString() == url){
            DownLoadFileInfo *info = *itor;
            return info->file->fileName();
        }
        itor++;
    }
    return QString();
}

bool GCHttpDownLoad::getUrlSize(QString url)
{
    QUrl durl(url);
    if(!durl.isValid() || durl.isLocalFile() || durl.isEmpty()){
        qWarning()<<"GCHttpDownLoad : Invalid url."<<url;
        return false;
    }
    emit sigToGetUrlSize(url);//也可以用阻塞的方式，但是会延时--用QEventLoop阻塞在这个函数里；
    return true;
}

#include <QEventLoop>
#include <QElapsedTimer>
void GCHttpDownLoad::toDownLoad(QString url, QString name, QString path)
{
    qDebug()<<__FUNCTION__;
    QString downfilepath = path + "/" + name + ".temp";
    QFile *file = new QFile(downfilepath);
    bool bopen = file->open(QFile::Append);
    if(!bopen){
        Q_ASSERT(bopen);
        return;
    }
    DownLoadFileInfo *info = new DownLoadFileInfo;

    info->file = file;
    int downBroken = file->size();
    QNetworkRequest req;
    req.setUrl(url);
    if( downBroken > 0){
        QString Range = "bytes=" + QString::number(downBroken) + "-";//download pos;
        req.setRawHeader("Range" ,Range.toLatin1());
    }
    QNetworkReply *reply = m_downLoadManger->get(QNetworkRequest(req));
    info->reply = reply;
    info->broken = downBroken;
    /**attention!
         * This slots must be run in the thread same of @m_downLoadManger be created,
         *      otherwise will get warning and failed to connect slots;
         * warn:QObject: Cannot create children for a parent that is in a different thread.
         */
    mutex.lock();
    m_downLoadMap.insert(reply, info);
    mutex.unlock();
    //Qt::BlockingQueuedConnection will blocked the thread of signal-sender;
    connect(reply, SIGNAL(downloadProgress(qint64,qint64)),
            this, SLOT(onDownloadProgress(qint64,qint64)));
    connect(reply, SIGNAL(finished()),
            this, SLOT(onFinished()));
    connect(reply, SIGNAL(readyRead()),
            this, SLOT(onReadyRead()));
}

void GCHttpDownLoad::toClearDownLoadList()
{
    QMap<QNetworkReply *,DownLoadFileInfo *> tempMap = m_downLoadMap;
    QMap<QNetworkReply *,DownLoadFileInfo *>::iterator it = tempMap.begin();
    while(it != tempMap.end()){
        DownLoadFileInfo *info = it.value();
        if(info && info->reply){
            //abort() will trigger onFinished() slot, and run deleteInfo().
            info->reply->abort();//直接跳到onFinished
        }
        it++;
    }
}

void GCHttpDownLoad::deleteInfo(GCHttpDownLoad::DownLoadFileInfo *info)
{
    info->reply->abort();
    info->reply->close();
    info->reply->deleteLater();
    info->reply = 0;
    info->file->close();
    delete info->file;
    info->file = 0;
    delete info;
}

void GCHttpDownLoad::toGetUrlSize(QString url)
{
    QNetworkRequest req;
    req.setUrl(url);
    QNetworkReply *reply = m_downLoadManger->get(QNetworkRequest(req));
    connect(reply, SIGNAL(downloadProgress(qint64,qint64)),
            this, SLOT(onDownloadProgressGetSize(qint64,qint64)));
}

bool GCHttpDownLoad::isUrlDownloading(QString url)
{
    QMap<QNetworkReply *,DownLoadFileInfo *>::iterator itor = m_downLoadMap.begin();
    while(itor != m_downLoadMap.end()){
        if(itor.key()->url().toString() == url){
            return true;
        }else{
            itor++;
        }
    }
    return false;
}


void GCHttpDownLoad::onReadyRead()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    DownLoadFileInfo *info = m_downLoadMap.value(reply, Q_NULLPTR);
    if(info){
        Q_ASSERT(info->file->isOpen());
        Q_ASSERT(info->reply == reply);
        info->file->write(reply->readAll());
    }
}

void GCHttpDownLoad::onError(QNetworkReply::NetworkError error)
{
    qDebug()<<"GCHttpDownLoad::onError"<<error;
}


void GCHttpDownLoad::onDownloadProgress(qint64 rcv, qint64 total)
{
    if(rcv == 0){
        return;
    }
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    QString url = reply->request().url().toString();
    DownLoadFileInfo *info = m_downLoadMap.value(reply, Q_NULLPTR);
    if(info){
        Q_ASSERT(info->file->isOpen());
        emit sigDownloadProgress(url, rcv, total, info->file->fileName());
        int fsize = info->file->size();
        int ftotal = total + info->broken;
        if(ftotal<=0){
            return;//出错了，服务器文件大小为0？还是也可能是broken为0，同时total也未
        }
        info->totalSize = ftotal;//更新info->totalSize
        qreal percent = fsize*1.0/ftotal;
        emit sigDownLoadPercent(url, percent, info->file->fileName());
        qDebug() << __FUNCTION__  << "file Name:" << info->file->fileName() << "percent:" << percent*100;
#if INFO_PRINT
        qDebug()<<"-[fileName:"<<info->file->fileName()
               <<"\n  cur bytesReceived:"<<rcv
              <<"\n  total receive: "<<total
             <<"\n  file size: "<<info->file->size()
            <<"\n  info->broken: "<<info->broken
           <<"]-";
#endif
    }
}

void GCHttpDownLoad::onFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    Q_ASSERT(reply);
    QString url = reply->request().url().toString();
    DownLoadFileInfo *info = m_downLoadMap.value(reply, Q_NULLPTR);
    Q_ASSERT(info);
    QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    if(info){
        int code = 404;
        if(statusCode.isValid()){
            code = statusCode.toInt();
            if((code/200) != 1){//1xx || 3xx || 4xx || 5xx 都是错误码
                emit sigFinished(url, -1, QString());
                emit sigError(url, code);
                Q_ASSERT(info->file);
                info->file->remove();
            }else{
                Q_ASSERT(info->file->isOpen());
                QString filename = info->file->fileName();
                Q_ASSERT(filename.contains(".temp"));
                qreal percent = (info->file->size()*1.0f) / info->totalSize;
                if(percent >= 1){//真正下载完成
                    filename = filename.left(filename.size() - QString(".temp").size());
                    info->file->rename(filename);
                }
                emit sigFinished(url, percent, info->file->fileName());

#if INFO_PRINT
                qDebug()<<"-[DownloadFinish: \n"<<
                          "  Url: "<<url<<code<<
                          "\n  FilePath: "<<info->file->fileName()<<"]-";
#endif
            }
        }else{
            emit sigFinished(url, -1, QString());
            emit sigError(url, -1);
        }

        mutex.lock();
        qDebug()<< __FUNCTION__ << m_downLoadMap.size();
        if(m_downLoadMap.size()){
            if(m_downLoadMap.contains(reply)){
                m_downLoadMap.remove(reply);
            }
        }
        mutex.unlock();
        deleteInfo(info);
    }
}

void GCHttpDownLoad::onDownLoad(QString url, QString name, QString path)
{
    toDownLoad(url, name, path);
}
#include <QPair>
void GCHttpDownLoad::onDownLoad(QNetworkReply *reply, QString dir)
{
    QString name;
    QList<QNetworkReply::RawHeaderPair> pairs = reply->rawHeaderPairs();
    if(!pairs.isEmpty()){
        foreach (QNetworkReply::RawHeaderPair pair, pairs) {
            if(pair.first == "Content-disposition"){
                qDebug()<<"C4Browser::onReplyReadyRead() : pair.second"<< pair.second;
                name = pair.second;
                name.remove("filename=");
                name.remove("attachment; ");
            }
        }
    }
    if(name.isEmpty()){
        name = splitUrlToName(reply->url().toString());
    }
    QString tpath = dir.isEmpty() ? m_tempPath:dir;
    if(QFile::exists(tpath + "/" + name)){
        QFileInfo info(tpath + "/" + name);
        while(info.exists()){
            name = info.completeBaseName().append(QString::number(qrand()%10));
            if(!info.suffix().isEmpty()){
                name.append("." + info.suffix());
            }
            info.setFile(tpath + "/" + name);
        }
        qWarning()<<"GCHttpDownLoad: File is download exist already."<<reply->url().toString()<<
                    "Turn to"<<info.fileName();
    }

    if(!isUrlDownloading(reply->url().toString())){
        QString temp = tpath + "/" + name + ".temp";
        if(QFile::exists(temp)){
            QFile::remove(temp);//删除下载缓存，而不是文件
        }
    }
    QString downfilepath = tpath + "/" + name + ".temp";
    QFile *file = new QFile(downfilepath);
    bool bopen = file->open(QFile::Append);
    if(!bopen){
        Q_ASSERT(bopen);
        return;
    }
    DownLoadFileInfo *info = new DownLoadFileInfo;

    info->file = file;
    info->reply = reply;
    info->broken = 0;
    mutex.lock();
    m_downLoadMap.insert(reply, info);
    mutex.unlock();
    connect(reply, SIGNAL(downloadProgress(qint64,qint64)),
            this, SLOT(onDownloadProgress(qint64,qint64)));
    connect(reply, SIGNAL(finished()),
            this, SLOT(onFinished()));
    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)),
            this, SLOT(onError(QNetworkReply::NetworkError)));
    connect(reply, SIGNAL(readyRead()),
            this, SLOT(onReadyRead()));
}

void GCHttpDownLoad::onStopDownLoad(QString url)
{
    QMap<QNetworkReply *,DownLoadFileInfo *>::iterator itor = m_downLoadMap.begin();
    while(itor != m_downLoadMap.end()){
        if(itor.key()->url().toString() == url){
            DownLoadFileInfo *info = *itor;
            if(info && info->reply){
                //abort() will trigger onFinished() slot, and run deleteInfo().;
                info->reply->abort();
                return;
            }
        }
        itor++;
    }
}

void GCHttpDownLoad::onStopAllDownLoad()
{
    toClearDownLoadList();
}

void GCHttpDownLoad::onGetUrlSize(QString url)
{
    toGetUrlSize(url);
}

void GCHttpDownLoad::onDownloadProgressGetSize(qint64 rcv, qint64 total)
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    QString url = reply->request().url().toString();
    if(rcv>0 && total>0){
        emit sigUrlSize(url, total);
        reply->deleteLater();
    }
}

