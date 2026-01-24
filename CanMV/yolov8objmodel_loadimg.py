
from libs.YOLO import YOLOv8
from libs.Utils import *
import os,sys,gc
import ulab.numpy as np
import image

if __name__=="__main__":
    # 这里仅为示例，自定义场景请修改为您自己的测试图片、模型路径、标签名称、模型输入大小
    img_path="/data/dataset/1651946904-8817422_png.rf.50172c2c9c96f3150c0ce9f5aaf1a682.jpg"
    kmodel_path="/data/kmodels/best_320_k230d.kmodel"
    labels = ["black","silver"]
    model_input_size=[320,320]

    confidence_threshold = 0.05
    nms_threshold=0.6
    img,img_ori=read_image(img_path)



#    print("type(img) =", type(img))
#    print("shape(img) =", getattr(img, "shape", None))
#    try:
#        print("dtype(img) =", img.dtype)
#    except:
#        pass





    rgb888p_size=[img.shape[2],img.shape[1]]
    # 初始化YOLOv8实例
    yolo=YOLOv8(task_type="detect",mode="image",kmodel_path=kmodel_path,labels=labels,rgb888p_size=rgb888p_size,model_input_size=model_input_size,conf_thresh=confidence_threshold,nms_thresh=nms_threshold,max_boxes_num=50,debug_mode=0)
    yolo.config_preprocess()
    res=yolo.run(img)
    print(res)

    yolo.draw_result(res,img_ori)
    yolo.deinit()
    gc.collect()
