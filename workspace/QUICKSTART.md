# K230D YOLOv8n Quick Start Guide

## 🚀 Getting Started in 5 Minutes

### Step 1: Start Docker Containers

From the project root directory:

```bash
docker-compose up -d
```

Wait for both containers to start (first time may take a few minutes to download images and install nncase).

### Step 2: Prepare Your Dataset

Organize your dataset in this structure:

```
workspace/
└── datasets/
    ├── images/
    │   ├── train/
    │   │   ├── img1.jpg
    │   │   ├── img2.jpg
    │   │   └── ...
    │   └── val/
    │       ├── img1.jpg
    │       └── ...
    └── labels/
        ├── train/
        │   ├── img1.txt
        │   ├── img2.txt
        │   └── ...
        └── val/
            ├── img1.txt
            └── ...
```

Create your `data.yaml` from the example:

```bash
cp data.yaml.example data.yaml
# Edit data.yaml to match your classes
```

### Step 3: Train YOLOv8n Model

Enter the training container:

```bash
docker exec -it k230d_train bash
```

Run training:

```bash
# Quick training (for testing)
python3 train_yolov8n.py --data data.yaml --epochs 10 --batch 8 --export

# Full training
python3 train_yolov8n.py --data data.yaml --epochs 100 --batch 16 --export
```

Or use direct YOLO CLI:

```bash
yolo train model=yolov8n.pt data=data.yaml epochs=100 imgsz=320 batch=16
yolo export model=runs/detect/train/weights/best.pt format=onnx imgsz=320
```

### Step 4: Convert to K230D Format

Exit the training container (Ctrl+D) and enter the conversion container:

```bash
docker exec -it k230d_convert bash
```

Run conversion:

```bash
python3 convert_to_k230.py /workspace/best.onnx /workspace/yolov8n_k230.kmodel
```

Or with custom settings:

```bash
python3 convert_to_k230.py \
    /workspace/runs/detect/train/weights/best.onnx \
    /workspace/my_model_k230.kmodel
```

### Step 5: Deploy to K230D

The `.kmodel` file in the workspace folder is now ready for deployment to your K230D device!

## 🎯 For RoboCup Rescue Line

### Typical Classes to Detect

```yaml
names:
  0: line           # Black rescue line
  1: victim_silver  # Silver/reflective victim marker
  2: victim_heated  # Heated victim (IR detection)
  3: checkpoint     # Green checkpoint area
  4: obstacle       # Obstacles on path
  5: ramp           # Ramp detection
  6: intersection   # Line intersections
  7: gap            # Line gaps
```

### Recommended Settings

- **Image size**: 320x320 (balance between speed and accuracy)
- **Batch size**: 16 (adjust based on GPU memory)
- **Epochs**: 100-200 (depending on dataset size)
- **Input type**: RGB camera feed at 30+ FPS

### K230D Deployment Code (MicroPython Example)

```python
from maix import nn, image, camera

# Load the kmodel
model = nn.NN("yolov8n_k230.kmodel")

# Initialize camera
cam = camera.Camera()

while True:
    # Capture frame
    img = cam.read()
    
    # Preprocess (resize to 320x320)
    input_img = img.resize(320, 320)
    
    # Run inference
    result = model.forward(input_img)
    
    # Process detections
    # ... your detection handling code ...
    
    # Display
    img.show()
```

## 🔧 Troubleshooting

### Training Issues

**GPU not detected:**
```bash
# Check if nvidia-docker is installed
docker run --rm --gpus all nvidia/cuda:11.8.0-base-ubuntu22.04 nvidia-smi
```

**Out of memory:**
- Reduce batch size: `--batch 8` or `--batch 4`
- Reduce image size: `--imgsz 224`

### Conversion Issues

**nncase import error:**
```bash
# Reinstall nncase in convert container
docker exec -it k230d_convert bash
pip3 install --upgrade nncase nncase-kpu
```

**ONNX opset version error:**
```bash
# Export with specific opset version
yolo export model=best.pt format=onnx opset=11 imgsz=320
```

### Performance Optimization

**Quantization for faster inference:**

Edit `convert_to_k230.py` and uncomment:

```python
compile_options.quant_type = 'uint8'
compile_options.w_quant_type = 'uint8'
```

**Adjust input size for speed:**
- 224x224: Faster, less accurate
- 320x320: Balanced (recommended)
- 416x416: Slower, more accurate

## 📊 Expected Performance on K230D

| Model | Input Size | FPS | mAP50 | Notes |
|-------|-----------|-----|-------|-------|
| YOLOv8n | 224x224 | ~25 | 0.65 | Fastest |
| YOLOv8n | 320x320 | ~15 | 0.75 | Recommended |
| YOLOv8n | 416x416 | ~8  | 0.80 | Most accurate |

*Performance varies based on model complexity and number of classes*

## 🎓 Learning Resources

- [YOLOv8 Documentation](https://docs.ultralytics.com/)
- [K230D SDK](https://developer.canaan-creative.com/k230)
- [nncase GitHub](https://github.com/kendryte/nncase)
- [RoboCup Junior Rules](https://junior.robocup.org/)

## 🤝 Tips for RoboCup Success

1. **Collect diverse data**: Various lighting, angles, and environments
2. **Label accurately**: Precise bounding boxes improve performance
3. **Test iteratively**: Train small, test on robot, refine dataset
4. **Optimize for speed**: K230D has limited compute, prioritize FPS
5. **Use data augmentation**: Helps generalize to competition environment

---

**Good luck with your RoboCup competition! 🏆🤖**
