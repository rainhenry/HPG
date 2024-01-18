#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //  初始化UI
    QImage img_out(ui->label_out_photo->width(), ui->label_out_photo->height(), QImage::Format_RGB888);
    img_out.fill(QColor(0, 0, 255));
    ui->label_out_photo->setPixmap(QPixmap::fromImage(img_out));
    QImage img_in(ui->label_in_hand->width(), ui->label_in_hand->height(), QImage::Format_RGB888);
    img_in.fill(QColor(255, 255, 255));
    ui->label_in_hand->setPixmap(QPixmap::fromImage(img_in));

    //  配置手写图片
    hand_img = new QImage(ui->label_in_hand->width(), ui->label_in_hand->height(), QImage::Format_RGB888);
    hand_img->fill(QColor(255, 255, 255));

    //  开启鼠标功能
    press_mouse_left_btn = false;
    setMouseTracking(true);
    centralWidget()->setMouseTracking(true);
    ui->label_in_hand->setMouseTracking(true);

    //  设置当前忙状态
    is_cur_busy = true;

    //  提示
    ui->progressBar->setFormat("Initializing environment, please wait...");

    //  关闭控件
    ui->pushButton_by_text ->setEnabled(false);
    ui->pushButton_by_ocr  ->setEnabled(false);
    ui->pushButton_only_ocr->setEnabled(false);
    ui->pushButton_clear   ->setEnabled(false);

    //  初始化OCR环境
    qRegisterMetaType<CQtAI::SOCRout>("CQtAI::SOCRout");
    qRegisterMetaType<CQtAI::SSDout>("CQtAI::SSDout");
    connect(&ocr, &CQtAI::send_ocr_result,        this, &MainWindow::slot_OnOCRFinish);
    connect(&ocr, &CQtAI::send_sd_result,         this, &MainWindow::slot_OnSDFinish);
    connect(&ocr, &CQtAI::send_environment_ready, this, &MainWindow::slot_OnOCREnvReady);
    ocr.Init();
    ocr.Configure(0.8, true);    //  配置默认的信心阈值与是否低阈值画图
    ocr.start();

    //  定时器初始化
    timer = new QTimer(this);
    timer->setInterval(30);
    connect(timer, SIGNAL(timeout()), this, SLOT(slot_timeout()));
    timer->start();

}

MainWindow::~MainWindow()
{
    //  释放定时器
    if(timer != nullptr)
    {
        timer->stop();
        delete timer;
        timer = nullptr;
    }

    //  释放手写图片
    if(hand_img != nullptr)
    {
        delete hand_img;
        hand_img = nullptr;
    }

    //  释放OCR资源
    ocr.Release();

    //  释放UI资源
    delete ui;
}


//  当OCR环境就绪
void MainWindow::slot_OnOCREnvReady(void)
{
    //  开启UI
    ui->pushButton_by_text ->setEnabled(true);
    ui->pushButton_by_ocr  ->setEnabled(true);
    ui->pushButton_only_ocr->setEnabled(true);
    ui->pushButton_clear   ->setEnabled(true);

    //  提示
    is_cur_busy = false;
    ui->progressBar->setFormat("Ready!!");
}

//  当完成一次OCR
void MainWindow::slot_OnOCRFinish(CQtAI::SOCRout ocr_info)
{
    //  保存图像
    ocr_img_cache = ocr_info.out_img;

    //  绘制OCR输出的标记图像
    QImage img = ocr_info.out_img;
    img = img.scaled(ui->label_in_hand->width(), ui->label_in_hand->height(), Qt::KeepAspectRatio);
    ui->label_in_hand->setPixmap(QPixmap::fromImage(img));

    //  将输出的文本写入编辑框(将所有条目拼接到一起,用空格隔离)
    QString out_text;

    //  遍历每一条结果
    int count = 0;
    std::list<CPyAI::SOCRdata> ocr_text_info = ocr_info.out_dat;
    while(ocr_text_info.size() > 0)
    {
        //  统计识别个数
        count++;

        //  获取当前信息
        CPyAI::SOCRdata cur_dat = *(ocr_text_info.begin());

        //  显示信息
        out_text += QString::asprintf("%s ", cur_dat.str.c_str());

        //  完成当前信息的处理
        ocr_text_info.erase(ocr_text_info.begin());
    }

    //  输出
    ui->lineEdit_prompt->setText(out_text);

    //  当本次为只进行OCR操作
    if(only_ocr_flag)
    {
        //  忙完
        is_cur_busy = false;

        //  开启UI
        ui->pushButton_by_text ->setEnabled(true);
        ui->pushButton_by_ocr  ->setEnabled(true);
        ui->pushButton_only_ocr->setEnabled(true);
        ui->pushButton_clear   ->setEnabled(true);

        //  设置状态
        long long ms = ocr_info.run_time_ns / 1000000LL;
        int sec = static_cast<int>(ms / 1000);
        int min = sec / 60;
        int hour = min / 60;
        QString str = QString::asprintf("OCR Done(%d:%02d:%02d.%03lld)[By:%s]",
                                        hour, min % 60, sec % 60, ms % 1000LL,
                                    #if USE_OCR_OPENVINO
                                        "OpenVINO"
                                    #else
                                        "Normal"
                                    #endif
                                        );
        ui->progressBar->setFormat(str);
    }
    //  否则为直接进行SD
    else
    {
        //  调用SD按钮
        this->on_pushButton_by_text_clicked();
    }
}

//  当完成一次SD
void MainWindow::slot_OnSDFinish(CQtAI::SSDout sd_info)
{
    //  将输出图像显示到控件上
    QImage out_img = sd_info.out_img;
    out_img = out_img.scaled(ui->label_out_photo->width(), ui->label_out_photo->height(), Qt::KeepAspectRatio);
    ui->label_out_photo->setPixmap(QPixmap::fromImage(out_img));

    //  忙完
    is_cur_busy = false;

    //  开启UI
    ui->pushButton_by_text ->setEnabled(true);
    ui->pushButton_by_ocr  ->setEnabled(true);
    ui->pushButton_only_ocr->setEnabled(true);
    ui->pushButton_clear   ->setEnabled(true);

    //  设置状态
    long long ms = sd_info.run_time_ns / 1000000LL;
    int sec = static_cast<int>(ms / 1000);
    int min = sec / 60;
    int hour = min / 60;
    QString str = QString::asprintf("Stable Diffusion Done(%d:%02d:%02d.%03lld)[By:%s]",
                                    hour, min % 60, sec % 60, ms % 1000LL,
                                #if USE_SD_OPENVINO
                                    "OpenVINO"
                                #else
                                    "Normal"
                                #endif
                                    );
    ui->progressBar->setFormat(str);
}

//  定时器
void MainWindow::slot_timeout()
{
    //  进度变量
    static int cur_proc = 100;

    //  当忙
    if(is_cur_busy)
    {
        if(cur_proc >= 100) cur_proc = 0;
        else                cur_proc = cur_proc + 1;
    }
    //  当忙完
    else
    {
        cur_proc = 100;
    }

    //  更新到控件
    ui->progressBar->setValue(cur_proc);
}

//  单击通过文本生成
void MainWindow::on_pushButton_by_text_clicked()
{
    //  当文本不为空
    if(ui->lineEdit_prompt->text().length() != 0)
    {
        //  执行一次SD
        ocr.ExSD(ui->lineEdit_prompt->text());

        //  设置忙
        is_cur_busy = true;

        //  设置状态
        ui->progressBar->setFormat("Processing...");

        //  关闭控件
        ui->pushButton_by_text ->setEnabled(false);
        ui->pushButton_by_ocr  ->setEnabled(false);
        ui->pushButton_only_ocr->setEnabled(false);
        ui->pushButton_clear   ->setEnabled(false);
    }
    //  为空
    else
    {
        //  提示错误，并恢复状态
        ui->progressBar->setFormat("Error:Prompt is Null!!");
        is_cur_busy = false;
        ui->pushButton_by_text ->setEnabled(true);
        ui->pushButton_by_ocr  ->setEnabled(true);
        ui->pushButton_only_ocr->setEnabled(true);
        ui->pushButton_clear   ->setEnabled(true);
    }
}

//  单击通过OCR生成
void MainWindow::on_pushButton_by_ocr_clicked()
{
    //  执行
    is_cur_busy = true;
#if USE_OCR_FAST
    ocr.ExOCR(*hand_img, CPyOCR::EModeType_Fast);
#else
    ocr.ExOCR(*hand_img, CPyAI::EModeType_Precise);
#endif

    //  设置状态
    ui->progressBar->setFormat("Processing...");

    //  关闭控件
    ui->pushButton_by_text ->setEnabled(false);
    ui->pushButton_by_ocr  ->setEnabled(false);
    ui->pushButton_only_ocr->setEnabled(false);
    ui->pushButton_clear   ->setEnabled(false);

    //  设置标志
    only_ocr_flag = false;
}

//  单击清除书写区域
void MainWindow::on_pushButton_clear_clicked()
{
    hand_img->fill(QColor(255, 255, 255));
    ui->label_in_hand->setPixmap(QPixmap::fromImage(*hand_img));
}

//  单击只进行OCR操作
void MainWindow::on_pushButton_only_ocr_clicked()
{
    //  执行
    is_cur_busy = true;
#if USE_OCR_FAST
    ocr.ExOCR(*hand_img, CPyOCR::EModeType_Fast);
#else
    ocr.ExOCR(*hand_img, CPyAI::EModeType_Precise);
#endif

    //  设置状态
    ui->progressBar->setFormat("Processing...");

    //  关闭控件
    ui->pushButton_by_text ->setEnabled(false);
    ui->pushButton_by_ocr  ->setEnabled(false);
    ui->pushButton_only_ocr->setEnabled(false);
    ui->pushButton_clear   ->setEnabled(false);

    //  设置标志
    only_ocr_flag = true;
}

//  鼠标按下处理
void MainWindow::mousePressEvent(QMouseEvent* event)
{
    if(Qt::LeftButton == event->button())
    {
        //  记录按下状态
        press_mouse_left_btn = true;

    }
}

//  鼠标抬起处理
void MainWindow::mouseReleaseEvent(QMouseEvent* event)
{
    if(Qt::LeftButton == event->button())
    {
        //  释放按下状态
        press_mouse_left_btn = false;
        ui->label_in_hand->setPixmap(QPixmap::fromImage(*hand_img));
    }
}

//  鼠标双击处理
void MainWindow::mouseDoubleClickEvent(QMouseEvent* event)
{
    (void)event;
}

//  鼠标移动处理
void MainWindow::mouseMoveEvent(QMouseEvent* event)
{
    //  当OCR处理忙
    if(ocr.GetStatus() == CQtAI::EAISt_Busy) return;

    //  当忙
    if(is_cur_busy) return;

    //  在输出图像控件上的处理
    QRect rect_out_img = ui->label_in_hand->geometry();                //  获取该图像控件的位置信息
    bool on_out_img = rect_out_img.contains(event->x(), event->y());   //  判断鼠标是否在该控件内
    int out_img_pos_x = event->x() - rect_out_img.x();                 //  计算相对坐标x
    int out_img_pos_y = event->y() - rect_out_img.y();                 //  计算相对坐标y

    //  上一次的坐标
    static int last_out_img_pos_x = -1;
    static int last_out_img_pos_y = -1;

    //  处理上一次的坐标值
    if(last_out_img_pos_x == -1) last_out_img_pos_x = out_img_pos_x;
    if(last_out_img_pos_y == -1) last_out_img_pos_y = out_img_pos_y;

    //  当在该图像中，并且为按下
    if(on_out_img && press_mouse_left_btn)
    {
        //  创建画板
        QPainter* ppainter = new QPainter();
        ppainter->begin(hand_img);
        ppainter->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);    //  抗锯齿

        //  画图
        ppainter->setPen(QPen(QColor(0,0,0), 3, Qt::SolidLine));
        ppainter->drawLine(last_out_img_pos_x, last_out_img_pos_y,
                           out_img_pos_x, out_img_pos_y);

        //  显示到控件
        ppainter->end();
        ui->label_in_hand->setPixmap(QPixmap::fromImage(*hand_img));

        //  释放画板
        delete ppainter;
    }

    //  更新坐标
    last_out_img_pos_x = out_img_pos_x;
    last_out_img_pos_y = out_img_pos_y;
}

//  鼠标滚轮处理
void MainWindow::wheelEvent(QWheelEvent* event)
{
    (void)event;
}

