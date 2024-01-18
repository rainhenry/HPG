#!/usr/bin/python3
import sys
sys.path.append("./HPG")

import cv2
import openvino_ocr as ovo

image = cv2.imread("./HPG/test1.bmp")
det_compiled_model = ovo.get_compile_model("./ch_PP-OCRv3_det_infer/inference.pdmodel")
det_output_layer = ovo.get_model_output_layer(det_compiled_model)
rec_compiled_model = ovo.get_compile_model("./ch_PP-OCRv3_rec_infer/inference.pdmodel")
rec_output_layer = ovo.get_model_output_layer(rec_compiled_model)
ret = ovo.ocr_ex(image, det_compiled_model, det_output_layer, rec_compiled_model, rec_output_layer, "./dict.txt")

print(ret)

