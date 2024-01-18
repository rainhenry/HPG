/*************************************************************

    程序名称:基于Python3原生C接口的AI C++类(阻塞)
    程序版本:REV 0.1
    创建日期:20240117
    设计编写:rainhenry
    作者邮箱:rainhenry@savelife-tech.com
    版    权:SAVELIFE Technology of Yunnan Co., LTD
    开源协议:GPL

    版本修订
        REV 0.1   20240117      rainhenry    创建文档

*************************************************************/
//------------------------------------------------------------
//  重定义保护
#ifndef __CPYAI_H__
#define __CPYAI_H__

//------------------------------------------------------------
//  宏定义
#define USE_SD_OPENVINO                      0         //  是否使用OpenVINO加速Stable Diffusion
#define USE_OCR_OPENVINO                     0         //  是否使用OpenVINO加速OCR(不建议开启)

//------------------------------------------------------------
//  包含头文件
#include <string>
#include <list>

//------------------------------------------------------------
//  类定义
class CPyAI
{
public:
    //  模型的种类
    typedef enum
    {
        EModeType_Fast = 0,    //  快速模型
        EModeType_Precise,     //  精确模型
    }EModeType;

    //  输出的OCR数据结构
    typedef struct
    {
        std::string str;       //  成功识别的字符串结果
        double confidence;     //  信心值
        long postion[8];       //  该字符串所在图像的4个点的坐标
    }SOCRdata;

    //  输出的SD数据结构
    typedef struct
    {
        unsigned char* dat;    //  数据
        long width;            //  宽度
        long height;           //  高度
        long channel;          //  通道数
    }SSDdata;

public:
    //  构造与析构函数
    CPyAI();
    ~CPyAI();

    //  释放资源
    //  注意！该释放必须和执行本体在同一线程中！
    void Release(void);

    //  初始化
    //  注意！该初始化必须和执行本体在同一线程中！
    void Init(void);

    //  执行OCR，只支持输入RGB888格式的数据
    //  images (list[numpy.ndarray]): 图片数据，ndarray.shape 为 [H, W, C]
    std::list<SOCRdata> OCR_Ex(void* buf, int w, int h, EModeType mode_type);

    //  执行SD
    SSDdata SD_Ex(const char* prompt);

private:
    //  为了兼容Python C的原生API，独立封装numpy的C初始化接口
    int import_array_init(void);

    //  静态python环境初始化标志
    static bool Py_Initialize_flag;

    //  OCR相关私有数据
    void* py_ocr_module;                 //  OCR模块
    void* py_ocr_ex;                     //  执行OCR
#if USE_OCR_OPENVINO
    void* py_ocr_get_compile_model;      //  得到编译后的模型
    void* py_ocr_get_model_output_layer; //  得到模型的输出层
    void* py_ocr_det_model_handle;       //  检测模型句柄
    void* py_ocr_rec_model_handle;       //  识别模型句柄
    void* py_ocr_det_out_layer_handle;   //  检测模型输出层句柄
    void* py_ocr_rec_out_layer_handle;   //  识别模型输出层句柄
    void* py_ocr_dict_handle;            //  字典句柄
#else
    void* py_ocr_init_fast;              //  快速模型库的初始化
    void* py_ocr_init_precise;           //  精确模型库的初始化
    void* py_ocr_fast_handle;            //  快速OCR句柄
    void* py_ocr_precise_handle;         //  精确OCR句柄
#endif

    //  Stable Diffusion相关的私有数据
    void* py_sd_module;                  //  SD模块
    void* py_sd_init;                    //  SD的初始化
    void* py_sd_handle;                  //  SD句柄
    void* py_sd_ex;                      //  执行SD
};


//------------------------------------------------------------
#endif  //  __CPYAI_H__


