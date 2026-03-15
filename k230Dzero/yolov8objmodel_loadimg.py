
#from libs.YOLO import YOLOv8
#from libs.Utils import *
#import os,sys,gc
#import ulab.numpy as np
#import image

#if __name__=="__main__":
#    # 这里仅为示例，自定义场景请修改为您自己的测试图片、模型路径、标签名称、模型输入大小
#    img_path="/sdcard/dataset/victim/victim_0013.jpg"
#    kmodel_path="/data/kmodels/best_640x480_k230d.kmodel"
#    labels = ["black","silver"]
#    model_input_size=[640,480]

#    confidence_threshold = 0.05
#    nms_threshold=0.6
#    img,img_ori=read_image(img_path)



##    print("type(img) =", type(img))
##    print("shape(img) =", getattr(img, "shape", None))
##    try:
##        print("dtype(img) =", img.dtype)
##    except:
##        pass





#    rgb888p_size=[img.shape[2],img.shape[1]]
#    # 初始化YOLOv8实例
#    yolo=YOLOv8(task_type="detect",mode="image",kmodel_path=kmodel_path,labels=labels,rgb888p_size=rgb888p_size,model_input_size=model_input_size,conf_thresh=confidence_threshold,nms_thresh=nms_threshold,max_boxes_num=50,debug_mode=0)
#    yolo.config_preprocess()
#    res=yolo.run(img)
#    print(res)

#    yolo.draw_result(res,img_ori)
#    yolo.deinit()
#    gc.collect()
from libs.YOLO import YOLOv8
from libs.Utils import *
import os, sys, gc
import ulab.numpy as np
import image
from media.display import *
from media.media import *

if __name__ == "__main__":
    img_path = "/sdcard/dataset/victim/victim_0013.jpg"
    kmodel_path = "/data/kmodels/best_640x480_k230d.kmodel"
    labels = ["black", "silver"]
    model_input_size = [640, 480]
    confidence_threshold = 0.05
    nms_threshold = 0.6

    # Init display
    display = Display()
    display.init()
    MediaManager.init()

    print("Loading image...")
    img, img_ori = read_image(img_path)
    print("Image shape:", img.shape)

    rgb888p_size = [img.shape[2], img.shape[1]]
    print("rgb888p_size:", rgb888p_size)

    print("Loading model...")
    try:
        yolo = YOLOv8(
            task_type="detect",
            mode="image",
            kmodel_path=kmodel_path,
            labels=labels,
            rgb888p_size=rgb888p_size,
            model_input_size=model_input_size,
            conf_thresh=confidence_threshold,
            nms_thresh=nms_threshold,
            max_boxes_num=50,
            debug_mode=1
        )
        print("Model loaded OK")
    except Exception as e:
        print("Failed to load model:", e)
        sys.exit(1)

    print("Configuring preprocess...")
    yolo.config_preprocess()

    print("Running inference...")
    try:
        res = yolo.run(img)
        print("Results:", res)
    except Exception as e:
        print("Inference failed:", e)
        yolo.deinit()
        gc.collect()
        sys.exit(1)

    # Draw boxes on img_ori
    yolo.draw_result(res, img_ori)
    print("Detection complete.")

    # Show on display
    display.show(img_ori)

    # Wait so you can see it
    import time
    time.sleep(5)

    # Cleanup
    display.deinit()
    MediaManager.deinit()
    yolo.deinit()
    gc.collect()
    print("Done.")
