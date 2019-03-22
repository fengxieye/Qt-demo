#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <audio.h>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void start();
    void stop();


private:
    Ui::MainWindow *ui;
    Audio audio_;
};

#endif // MAINWINDOW_H
