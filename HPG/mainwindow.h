/*************************************************************

    程序名称:主窗体
    程序版本:REV 0.1
    创建日期:20240117
    设计编写:rainhenry
    作者邮箱:rainhenry@savelife-tech.com
    版    权:SAVELIFE Technology of Yunnan Co., LTD
    开源协议:GPL

    版本修订
        REV 0.1   20240117      rainhenry    创建文档

*************************************************************/


#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QImage>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QTimer>


#include "CQtAI.h"

//  使用OCR模型的类型
//  当0，表示使用高精度模型
//  当1，表示使用快速模型
#define USE_OCR_FAST    0

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    //  OCR对象
    CQtAI ocr;

    //  手写的图片
    QImage* hand_img;

    //  定时器
    QTimer* timer;

    //  当前忙
    bool is_cur_busy;

    //  只进行OCR
    bool only_ocr_flag;

    //  OCR输出图像缓存
    QImage ocr_img_cache;

    //  鼠标左键按下的状态
    bool press_mouse_left_btn;
    int press_mouse_pos_x;
    int press_mouse_pos_y;

private slots:
    //  当OCR环境就绪
    void slot_OnOCREnvReady(void);

    //  当完成一次OCR
    void slot_OnOCRFinish(CQtAI::SOCRout ocr_info);

    //  当完成一次SD
    void slot_OnSDFinish(CQtAI::SSDout sd_info);

    //  定时器
    void slot_timeout();


    //  单击通过文本生成
    void on_pushButton_by_text_clicked();

    //  单击通过OCR生成
    void on_pushButton_by_ocr_clicked();

    //  单击清除书写区域
    void on_pushButton_clear_clicked();

    void on_pushButton_only_ocr_clicked();

protected:
    //  鼠标相关
    void mousePressEvent(QMouseEvent* event);          //  按下
    void mouseReleaseEvent(QMouseEvent* event);        //  抬起
    void mouseDoubleClickEvent(QMouseEvent* event);    //  双击
    void mouseMoveEvent(QMouseEvent* event);           //  移动
    void wheelEvent(QWheelEvent* event);               //  滚轮

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
