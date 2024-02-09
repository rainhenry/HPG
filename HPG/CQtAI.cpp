/*************************************************************

    程序名称:基于Qt线程类的AI类
    程序版本:REV 0.2
    创建日期:20240117
    设计编写:rainhenry
    作者邮箱:rainhenry@savelife-tech.com
    版    权:SAVELIFE Technology of Yunnan Co., LTD
    开源协议:GPL

    版本修订
        REV 0.1   20240117      rainhenry    创建文档
        REV 0.2   20240209      rainhenry    修复c_str()返回空指针问题

*************************************************************/

//  包含头文件
#include "CQtAI.h"

//  构造函数
CQtAI::CQtAI():
    sem_cmd(0)
{
    //  初始化私有数据
    cur_st = EAISt_Ready;
    cur_cmd = EAIcmd_Null;
    confid_th = 0.8;
    confid_low_flag = true;

    //  创建Python的OCR对象
    py_ocr = new CPyAI();
}

//  析构函数
CQtAI::~CQtAI()
{
    Release();   //  通知线程退出
    this->msleep(1000);
    this->quit();
    this->wait(500);

    delete py_ocr;
}

//  初始化
void CQtAI::Init(void)
{
    //  暂时没有
}

//  配置
void CQtAI::Configure(double th,       //  信心阈值
                       bool low_flag    //  低阈值是否画图标记
                      )
{
    cmd_mutex.lock();
    this->confid_th = th;
    this->confid_low_flag = low_flag;
    cmd_mutex.unlock();
}

void CQtAI::run()
{
    //  初始化python环境
    py_ocr->Init();

    //  通知运行环境就绪
    emit send_environment_ready();

    //  现成主循环
    while(1)
    {
        //  获取信号量
        sem_cmd.acquire();

        //  获取当前命令和数据
        cmd_mutex.lock();
        EAIcmd now_cmd = this->cur_cmd;
        QImage now_img = this->in_img_dat;
        double now_confid_th = this->confid_th;
        bool now_low_flag = this->confid_low_flag;
        CPyAI::EModeType now_type = this->mode_type;
        std::string tmp_str = this->sd_prompt.toStdString();
        cmd_mutex.unlock();

        //  当为空命令
        if(now_cmd == EAIcmd_Null)
        {
            //  释放CPU
            this->sleep(1);
        }
        //  当为退出命令
        else if(now_cmd == EAIcmd_Release)
        {
            py_ocr->Release();
            qDebug("Thread is exit!!");
            return;
        }
        //  当为OCR命令
        else if(now_cmd == EAIcmd_ExOCR)
        {
            //  设置忙
            cmd_mutex.lock();
            this->cur_st = EAISt_Busy;
            cmd_mutex.unlock();

            //  转换图像格式为RGB888
            now_img.convertTo(QImage::Format_RGB888);

            //  去掉图像中每一行后面的多余对齐内存空间
            unsigned char* pimg = now_img.bits();
            unsigned char* pimg_buf = new unsigned char[now_img.width() * now_img.height() * 3];
            int y=0;
            for(y=0;y<now_img.height();y++)
            {
                memcpy(pimg_buf + y*now_img.width()*3,
                       pimg + y*now_img.bytesPerLine(),
                       static_cast<size_t>(now_img.width()*3)
                      );
            }

            //  执行OCR
            QElapsedTimer run_time;
            run_time.start();
            std::list<CPyAI::SOCRdata> ocr_info = py_ocr->OCR_Ex(pimg_buf, now_img.width(), now_img.height(), now_type);
            std::list<CPyAI::SOCRdata> ocr_buf_info = ocr_info;  //  保存副本，用于输出给用户
            qint64 run_time_ns = run_time.nsecsElapsed();

            //  释放缓存资源
            delete [] pimg_buf;

            //  创建画板
            QPainter* ppainter = new QPainter();
            ppainter->begin(&now_img);
            ppainter->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);    //  抗锯齿

            //  将OCR识别到的结果标记 
            while(ocr_info.size()>0)
            {
                //  获取当前条目数据
                CPyAI::SOCRdata cur_info;
                cur_info = *(ocr_info.begin());

                //  当达到信心值
                if(cur_info.confidence >= now_confid_th)
                {
                    ppainter->setPen(QPen(QColor(255,0,0), 5, Qt::SolidLine));
                    ppainter->drawLine(static_cast<int>(cur_info.postion[0]), static_cast<int>(cur_info.postion[1]), 
                                       static_cast<int>(cur_info.postion[2]), static_cast<int>(cur_info.postion[3]));
                    ppainter->drawLine(static_cast<int>(cur_info.postion[2]), static_cast<int>(cur_info.postion[3]), 
                                       static_cast<int>(cur_info.postion[4]), static_cast<int>(cur_info.postion[5]));
                    ppainter->drawLine(static_cast<int>(cur_info.postion[4]), static_cast<int>(cur_info.postion[5]), 
                                       static_cast<int>(cur_info.postion[6]), static_cast<int>(cur_info.postion[7]));
                    ppainter->drawLine(static_cast<int>(cur_info.postion[6]), static_cast<int>(cur_info.postion[7]), 
                                       static_cast<int>(cur_info.postion[0]), static_cast<int>(cur_info.postion[1]));
                }
                //  未达到
                else
                {
                    //  当低于阈值也表示画图
                    if(now_low_flag)
                    {
                        ppainter->setPen(QPen(QColor(0,0,255), 3, Qt::DashLine));
                        ppainter->drawLine(static_cast<int>(cur_info.postion[0]), static_cast<int>(cur_info.postion[1]), 
                                           static_cast<int>(cur_info.postion[2]), static_cast<int>(cur_info.postion[3]));
                        ppainter->drawLine(static_cast<int>(cur_info.postion[2]), static_cast<int>(cur_info.postion[3]), 
                                           static_cast<int>(cur_info.postion[4]), static_cast<int>(cur_info.postion[5]));
                        ppainter->drawLine(static_cast<int>(cur_info.postion[4]), static_cast<int>(cur_info.postion[5]), 
                                           static_cast<int>(cur_info.postion[6]), static_cast<int>(cur_info.postion[7]));
                        ppainter->drawLine(static_cast<int>(cur_info.postion[6]), static_cast<int>(cur_info.postion[7]), 
                                           static_cast<int>(cur_info.postion[0]), static_cast<int>(cur_info.postion[1]));
                    }
                }

                //  已完成该数据的操作，擦除该数据
                ocr_info.erase(ocr_info.begin());
            }
            ppainter->end();

            //  构造输出结果
            SOCRout out_info;
            out_info.out_img = now_img;
            out_info.out_dat = ocr_buf_info;
            out_info.run_time_ns = run_time_ns;
            out_info.mode_type = now_type;

            //  发出通知信号
            emit send_ocr_result(out_info);

            //  完成一帧处理
            cmd_mutex.lock();
            this->cur_st = EAISt_Ready;
            now_cmd = EAIcmd_Null;
            cmd_mutex.unlock();

            //  释放画板
            delete ppainter;
        }
        //  当为执行SD命令
        else if(now_cmd == EAIcmd_ExSD)
        {
            //  设置忙
            cmd_mutex.lock();
            this->cur_st = EAISt_Busy;
            cmd_mutex.unlock();

            //  执行SD
            QElapsedTimer run_time;
            run_time.start();
            CPyAI::SSDdata sd_dat = py_ocr->SD_Ex(tmp_str.c_str());
            qint64 run_time_ns = run_time.nsecsElapsed();

            //  构造输出图像
            QImage img(static_cast<int>(sd_dat.width), static_cast<int>(sd_dat.height), QImage::Format_RGB888);

            //  复制图像数据
            int y=0;
            for(y=0;y<static_cast<int>(sd_dat.height);y++)
            {
                memcpy(img.bits() + (img.bytesPerLine() * y),
                       sd_dat.dat + (sd_dat.width * y * sd_dat.channel),
                       static_cast<size_t>(sd_dat.width * sd_dat.channel));
            }

            //  释放资源
            delete [] sd_dat.dat;

            //  构造输出结构体
            SSDout sd_info;
            sd_info.out_img = img;
            sd_info.run_time_ns = run_time_ns;

            //  通知完成
            emit send_sd_result(sd_info);

            //  完成处理
            cmd_mutex.lock();
            this->cur_st = EAISt_Ready;
            now_cmd = EAIcmd_Null;
            cmd_mutex.unlock();
        }
        //  非法命令
        else
        {
            //  释放CPU
            QThread::sleep(1);
            qDebug("Unknow cmd code!!");
        }
    }
}

CQtAI::EAISt CQtAI::GetStatus(void)
{
    EAISt re;
    cmd_mutex.lock();
    re = this->cur_st;
    cmd_mutex.unlock();
    return re;
}

void CQtAI::ExOCR(QImage origin, CPyAI::EModeType type)
{
    if(GetStatus() == EAISt_Busy) return;

    cmd_mutex.lock();
    this->cur_cmd = EAIcmd_ExOCR;
    this->in_img_dat = origin;
    this->mode_type = type;
    this->cur_st = EAISt_Busy;    //  设置忙
    cmd_mutex.unlock();

    sem_cmd.release();
}

//  执行一次SD
void CQtAI::ExSD(QString prompt)
{
    if(GetStatus() == EAISt_Busy) return;

    cmd_mutex.lock();
    this->cur_cmd = EAIcmd_ExSD;
    this->sd_prompt = prompt;
    this->cur_st = EAISt_Busy;    //  设置忙
    cmd_mutex.unlock();

    sem_cmd.release();
}

void CQtAI::Release(void)
{
    cmd_mutex.lock();
    this->cur_cmd = EAIcmd_Release;
    cmd_mutex.unlock();

    sem_cmd.release();
}
