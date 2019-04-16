#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    m_download = new GCHttpDownLoad();
    //测试下载百度云盘
    QString strUrl = "http://issuecdn.baidupcs.com/issue/netdisk/yunguanjia/BaiduNetdisk_6.7.3.exe";
    GCHttpDownLoad::AddResult result = m_download->downLoad(strUrl);
    qInfo()<<"添加下载任务"<<((result>=GCHttpDownLoad::AddResult::DownLoading)?"成功":"失败")<<strUrl;
}

MainWindow::~MainWindow()
{
    delete ui;
}
