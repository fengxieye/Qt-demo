#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->btn_start,&QPushButton::clicked,this,&MainWindow::start);
    connect(ui->btn_stop,&QPushButton::clicked,this,&MainWindow::stop);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::start()
{
    audio_.start();
}

void MainWindow::stop()
{
    audio_.stop();
}


