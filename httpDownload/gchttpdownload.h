#ifndef GCHTTPDOWNLOAD_H
#define GCHTTPDOWNLOAD_H
#include <QObject>

/**
 * @brief The GCHttpDownLoad class
 * pro 加入 network
 * 利用QNetworkRequest的下载类，支持断点和重新下载，支持多任务下载。不能设置parent，因为这个类在线程中;
 * 只有槽函数才会在线程里执行
 * @waring 如果在线程中创建，GCHttpDownLoad和创建此类的类不能设置parent
 * @author
 */
#include <QNetworkReply>
#include <QNetworkAccessManager>
class QFile;
class QThread;

class  GCHttpDownLoad : public QObject
{
    Q_OBJECT
public:
    explicit GCHttpDownLoad(QThread *thread = 0, QObject *parent = 0);
    ~GCHttpDownLoad();
    enum AddResult{
        Unknow,//初始化
        DirError,//存放路径不存在
        FileError,//文件无法打开/生成
        UrlInvalid,//下载链接非法
        DownLoading,//正在下载
        DownLoadFinished,//已经下载完成
        AddOK,
    };
    /**
     * @brief downLoad：根据下载链接下载文件
     * @param url：下载链接
     * @param name： 设置下载的文件名，默认自动解析下载url，如果url不支持自动解析，请设置文件名
     * @param dir: 设置下载文件的存放路径，默认使用tempDir();
     * @param bcontinue：如果false将会删除缓存文件重新下载；设置为true将会断点下载
     */
    AddResult downLoad(QString url, QString name = QString(), QString dir = QString(), bool bcontinue = true);

    AddResult downLoad(QNetworkReply *reply, QString dir = QString());

    /**
     * @brief setTempDir：设置下载文件保存路径
     * @param tempdir：文件保存路径
     * @return bool：如果设置成功返回true，否返回false
     */
    bool setTempDir(QString tempdir);
    /**
     * @brief tempDir: 下载文件保存路径
     * @return QString
     */
    inline QString tempDir(){return m_tempPath;}
    /**
     * @brief stopDownLoad：停止下载
     * @param url：停止下载的链接
     */
    void stopDownLoad(QString url);
    /**
     * @brief stopAllDownLoad：停止下载所有链接
     */
    void stopAllDownLoad();
    /**
     * @brief isDownLoading: url是否正在下载。
     * @param url
     * @return : bool
     */
    bool isDownLoading(QString url);
    /**
     * @brief splitUrlToName：静态函数，解析下载链接，获取文件名
     * @param url：下载链接
     * @return QString：要下载的文件名
     */
    static QString splitUrlToName(QString url);
    /**
     * @brief downLoadingTempFile：返回下载中的temp文件,如果不是下载中，返回空QString()
     * @param url：下载链接
     * @return
     */
    QString downLoadingTempFile(QString url);

    /**
     * @brief getUrlSize：获取url链接的文件大小
     * @param url
     * @return url是否有效
     */
    bool getUrlSize(QString url);

//    void setNetworkAccessManager(QNetworkAccessManager * manager);
signals:
    /**
     * @brief sigFinished:信号，文件下载完成会返回下载文件的下载链接以及下载到本地的文件路径
     * @param url
     * @param file:下载完成文件路径，下载到一半暂停返回temp文件路径，下载出错file为空,
     * @param percent：文件下载进度，当暂停的时候，percent为当前进度
     */
    void sigFinished(QString url, qreal percent, QString file);
    /**
     * @brief sigDownloadProgress：信号，发送下载中的进度
     * @param url:下载链接
     * @param receive：当前下载接收了多少kb
     * @param total：当次下载需要下载多少kb，断点续传中total = 文件总大小 - 文件断点位置文件大小;
     * @param file:文件路径
     */
    void sigDownloadProgress(QString url, qint64 receive, qint64 total, QString file);
    /**
     * @brief sigDownLoadPercent：当前下载百分比
     * @param url
     * @param percent
     * @param file
     */
    void sigDownLoadPercent(QString url, qreal percent, QString file);
    /**
     * @brief sigUrlSize:信号，发送url链接的文件大小
     * @param url：链接
     * @param size：大小bit为单位
     */
    void sigUrlSize(QString url, qint64 size);

    void sigError(QString url, int type);
    //发送下载信号，让下载动作在线程运行；内部使用, 多此一举传path，防止万一槽在等待队列的时候temppath改变了；
    void sigToDownLoad(QString url, QString name, QString path);
    void sigToDownLoad(QNetworkReply *reply, QString path);
    void sigToStopDownLoad(QString url);
    void sigToStopAllDownLoad();
    void sigToGetUrlSize(QString url);
private:
    struct DownLoadFileInfo{
        QFile *file;
        int totalSize;//文件总大小
        QNetworkReply *reply;//为了[取消下载]功能
        int broken;//开始下载的位置
    };
    QThread *m_downLoadThread;//下载线程
    QMap<QNetworkReply *,DownLoadFileInfo *> m_downLoadMap;
    QNetworkAccessManager *m_downLoadManger;
    QString m_tempPath;
    QMutex mutex;
private:
    void toDownLoad(QString file, QString name, QString path);
    void toClearDownLoadList();
    void deleteInfo(DownLoadFileInfo *info);
    void toGetUrlSize(QString url);
    bool isUrlDownloading(QString url);
private slots:
    void onReadyRead();
    void onError(QNetworkReply::NetworkError);
#ifndef Q_OS_IOS
    //void onSslErrors(QList<QSslError>);
#endif
    void onDownloadProgress(qint64, qint64);
    void onFinished();
    void onDownLoad(QString file, QString name, QString path);
    void onDownLoad(QNetworkReply *reply, QString path);

    void onStopDownLoad(QString);
    void onStopAllDownLoad();

    void onGetUrlSize(QString url);
    void onDownloadProgressGetSize(qint64, qint64);
};


#endif // GCHTTPDOWNLOAD_H
