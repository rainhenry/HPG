#!/usr/bin/python3
#*************************************************************
#
#   程序名称:Python模块子程序
#   程序版本:REV 0.1
#   创建日期:20240117
#   设计编写:rainhenry
#   作者邮箱:rainhenry@savelife-tech.com
#   版    权:SAVELIFE Technology of Yunnan Co., LTD
#   开源协议:GPL
#
#   版本修订
#       REV 0.1   20240117      rainhenry    创建文档
#
#*************************************************************
from diffusers import StableDiffusionPipeline
import numpy as np
from PIL import Image
import time

def sd_init(model_id):
    pipe = StableDiffusionPipeline.from_pretrained(model_id).to("cpu")
    return pipe

def sd_ex(prompt, pipe):
    ##  标准512x512尺寸
    image = pipe(prompt, guidance_scale=10).images[0]  
    image.save('tmp.jpg')
    img = np.array(image)
    return img.astype(np.uint8)
    ##  用于测试的小尺寸128x128尺寸(无法正常产生图像，只能用于验证C++代码)
    ##image = pipe(prompt, guidance_scale=10, height=128, width=128).images[0]  
    ##image.save('tmp_s.jpg')
    ##img = np.array(image)
    ##return img.astype(np.uint8)
    ##  临时测试
    ##time.sleep(5)
    ##image = Image.open("tmp.jpg")
    ##img = np.array(image)
    ##return img.astype(np.uint8)
    
