#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_CreatQRcode.h"
#include <opencv2/opencv.hpp>
#include <opencv2/objdetect.hpp>
#include <String>
#include <QPixmap>
#include <QFileDialog>
#include <QImage>
#include <QMessageBox>
#include <QTimer>

using namespace cv;

class CreatQRcode : public QMainWindow
{
    Q_OBJECT

public:
    CreatQRcode(QWidget *parent = nullptr);
    ~CreatQRcode();
    QString getSystemTime();                 //获取系统时间
    void reflashTime();                      //刷新时间

public slots:
    void clearContent();                     //清空文本与图片
    void showQRPic();                        //显示二维码
    void saveImage();                        //保存图片
    void openQRcode();                       //打开二维码图片
    void showTime();                         //显示时间

public:
    QLabel *statusLabel;                      //状态提示
    QLabel* time;                             //时间标签
    QTimer* systemTime;

private:
    void creatQRcode(String text,Mat& qr);   //生成二维码
    String readQRcode();                     //读取二维码数据

private:
    Ui::CreatQRcodeClass ui;
};

