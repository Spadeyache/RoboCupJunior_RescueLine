# K230D YOLOv8n Object Detection Pipeline

End-to-end Docker-based development environment for training and deploying YOLOv8n models on the **Kendryte K230D Zero** (RISC-V) board with 128MB LPDDR4 RAM.

## 🎯 Project Overview

This pipeline enables:
- **Training**: YOLOv8n Nano model at 320×320 resolution
- **Export**: PyTorch → ONNX (Opset 11/12, simplified)
- **Quantization**: ONNX → INT8 .kmodel using nncase v2.10.0
- **Deployment**: CanMV (MicroPython) inference on K230D with KPU (SDK 2.0.0)

## 🏗️ Architecture

```
┌─────────────────┐
│  Training       │  PyTorch + Ultralytics
│  (train service)│  → Trains YOLOv8n @ 320x320
└────────┬────────┘
         │ .pt model
         ↓
┌─────────────────┐
│  Export         │  ONNX export + simplification
│  (train service)│  → Opset 11/12, no NMS
└────────┬────────┘
         │ .onnx model
         ↓
┌─────────────────┐
│  Conversion     │  nncase + .NET SDK
│  (convert svc)  │  → INT8 quantization
└────────┬────────┘
         │ .kmodel
         ↓
┌─────────────────┐
│  Deployment     │  CanMV (MicroPython)
│  (K230D)        │  → KPU inference
└─────────────────┘
```

## 📁 Project Structure

```
k230dev-yolo/
├── docker/
│   ├── train/
│   │   └── Dockerfile          # Training environment (PyTorch + Ultralytics)
│   └── convert/
│       └── Dockerfile          # Conversion environment (nncase)
├── workspace/
│   ├── train_yolov8n.py        # Training script
│   ├── export_to_onnx.py       # ONNX export script
│   ├── convert_to_kmodel.py    # nncase conversion script
│   ├── deploy_canmv.py         # CanMV deployment generator
│   ├── data.yaml.example       # Dataset configuration template
│   ├── requirements-train.txt  # Training dependencies
│   └── requirements-convert.txt # Conversion dependencies
├── datasets/                   # Your training datasets
├── models/                     # Exported models (.pt, .onnx)
├── output/                     # Final .kmodel files
├── runs/                       # Training runs
├── test_images/               # Test images for validation
├── docker-compose.yml         # Docker orchestration
└── README.md                  # This file
```

## 🚀 Quick Start

### 1. Prerequisites

- Docker & Docker Compose
- NVIDIA GPU with CUDA support (for training)
- NVIDIA Container Toolkit (nvidia-docker)

```bash
# Verify Docker
docker --version

# Verify NVIDIA Docker
docker run --rm --gpus all nvidia/cuda:12.1.0-base-ubuntu22.04 nvidia-smi
```

### 2. Setup

```bash
# Clone or navigate to project
cd k230dev-yolo

# Create necessary directories
mkdir -p datasets models output runs test_images

# Prepare your dataset configuration
cp workspace/data.yaml.example workspace/data.yaml
# Edit workspace/data.yaml with your dataset paths and classes
```

### 3. Build Docker Images

```bash
# Build both services
docker compose build

# Or build individually
docker compose build train
docker compose build convert
```

### 4. Prepare Your Dataset

Organize your dataset in YOLO format:

```
datasets/
└── my_dataset/
    ├── images/
    │   ├── train/
    │   │   ├── img1.jpg
    │   │   └── ...
    │   └── val/
    │       ├── img1.jpg
    │       └── ...
    └── labels/
        ├── train/
        │   ├── img1.txt
        │   └── ...
        └── val/
            ├── img1.txt
            └── ...
```

Update `workspace/data.yaml`:

```yaml
path: /datasets/my_dataset
train: images/train
val: images/val

nc: 2  # Number of classes
names:
  0: class1
  1: class2
```

## 📚 Complete Workflow

### Step 1: Train YOLOv8n Model

```bash
# Start training container
docker compose run --rm train

# Inside container
cd /workspace
python train_yolov8n.py \
    --data data.yaml \
    --epochs 100 \
    --batch 16 \
    --img 320 \
    --name my_model
```

**Key Parameters:**
- `--data`: Path to data.yaml
- `--epochs`: Training epochs (default: 100)
- `--batch`: Batch size (adjust based on GPU memory)
- `--img`: Image size (320 for K230D)
- `--name`: Run name for organization

**Output:** `/runs/my_model/weights/best.pt`

### Step 2: Export to ONNX

```bash
# Still in training container
python export_to_onnx.py \
    /runs/my_model/weights/best.pt \
    --output /models \
    --img 320 \
    --opset 11
```

**Output:** `/models/best_320.onnx`

### Step 3: Convert to .kmodel

```bash
# Exit training container, start conversion container
exit
docker compose run --rm convert

# Inside conversion container
cd /workspace
python convert_to_kmodel.py \
    /models/best_320.onnx \
    --output /output \
    --calib-data /datasets/my_dataset/images/val \
    --img 320 \
    --num-samples 100
```

**Important:** Use `--calib-data` with validation images for accurate INT8 quantization.

**Output:** `/output/best_320_k230d.kmodel`

### Step 4: Generate CanMV Deployment Script

```bash
# Still in conversion container
python deploy_canmv.py \
    /output/best_320_k230d.kmodel \
    --output /output \
    --img 320 \
    --classes class1 class2 \
    --conf 0.25 \
    --iou 0.45
```

**Output:** `/output/deploy_k230d.py`

## 🎮 Deployment to K230D

### 1. Transfer Files to K230D

```bash
# Copy model and script to K230D
scp output/best_320_k230d.kmodel root@k230d-ip:/root/
scp output/deploy_k230d.py root@k230d-ip:/root/
```

### 2. Run on K230D

```bash
# SSH into K230D
ssh root@k230d-ip

# Run inference
cd /root
python3 deploy_k230d.py
```

The script will:
- Load the .kmodel
- Initialize camera
- Run real-time inference
- Display FPS and detections
- Apply memory-efficient garbage collection

## ⚙️ Advanced Configuration

### Training Optimization

**For Better Accuracy:**
```bash
python train_yolov8n.py \
    --epochs 200 \
    --batch 32 \
    --img 320 \
    --cache  # Cache images in RAM (if enough memory)
```

**For Faster Training:**
```bash
python train_yolov8n.py \
    --epochs 50 \
    --batch 32 \
    --workers 16
```

### Model Quantization

**High-Quality Quantization:**
```bash
python convert_to_kmodel.py model.onnx \
    --calib-data /datasets/my_dataset/images/val \
    --num-samples 200  # More samples = better accuracy
```

**Quick Test (NOT for production):**
```bash
python convert_to_kmodel.py model.onnx
# Uses random calibration data
```

### Memory Optimization for K230D

The deployment script includes several optimizations for 128MB RAM:

1. **Manual Garbage Collection:** Every 10 frames
2. **Efficient NMS:** Memory-optimized implementation
3. **No Frame Buffering:** Process frames immediately
4. **Threshold-based Memory GC:** Adaptive garbage collection

## 📊 Model Performance

### Expected Metrics

| Metric | Target | Notes |
|--------|--------|-------|
| Model Size | < 3 MB | INT8 quantized .kmodel |
| RAM Usage | < 50 MB | During inference on K230D |
| FPS | 10-30 | Depends on num classes & detections |
| mAP50 | > 0.8 | Dataset dependent |
| Latency | 30-100ms | Per frame on K230D KPU |

### Benchmarking

```bash
# On K230D, monitor performance
python3 -c "
import gc
print(f'Free RAM: {gc.mem_free() / 1024 / 1024:.2f} MB')
print(f'Used RAM: {gc.mem_alloc() / 1024 / 1024:.2f} MB')
"
```

## 🔧 Troubleshooting

### Training Issues

**Out of Memory:**
```bash
# Reduce batch size
python train_yolov8n.py --batch 8
```

**Slow Training:**
```bash
# Increase workers
python train_yolov8n.py --workers 16
```

### Conversion Issues

**nncase Not Found:**
```bash
# Inside convert container
pip3 install nncase==2.10.0 nncase-kpu==2.10.0
```

**Quantization Accuracy Loss:**
- Use more calibration samples (--num-samples 200)
- Ensure calibration data is representative
- Try different calibration methods in convert_to_kmodel.py

### K230D Runtime Issues

**Out of Memory:**
- Reduce confidence threshold (fewer detections)
- Increase GC frequency in deploy_k230d.py
- Ensure model size < 5 MB

**Low FPS:**
- Verify KPU is being used (not CPU)
- Check camera resolution settings
- Simplify post-processing

## 📦 Docker Services Reference

### Training Service (`train`)
- Base: `pytorch/pytorch:2.1.0-cuda12.1-cudnn8-runtime`
- GPU: Required (CUDA 12.1)
- Mounts: workspace, datasets, models, runs

### Conversion Service (`convert`)
- Base: `ubuntu:22.04`
- Dependencies: .NET SDK 8.0, nncase 2.10.0
- Mounts: workspace, models, output, test_images

## 🤝 Contributing

Improvements welcome! Focus areas:
- nncase version compatibility
- Memory optimization techniques
- Post-processing speedups
- Alternative quantization methods

## 📄 License

Project code is open source. Please respect licenses of dependencies:
- Ultralytics YOLOv8: AGPL-3.0
- nncase: Apache-2.0
- PyTorch: BSD-style

## 🔗 Resources

- [K230D Documentation](https://developer.canaan-creative.com/)
- [YOLOv8 Docs](https://docs.ultralytics.com/)
- [nncase GitHub](https://github.com/kendryte/nncase)
- [CanMV Documentation](https://developer.canaan-creative.com/canmv)

## 📞 Support

For issues specific to:
- **Training/Export**: Check Ultralytics documentation
- **nncase**: Check Kendryte nncase GitHub issues
- **K230D Hardware**: Check CanMV/Kendryte forums

---

**Built with ❤️ for the K230D RISC-V community**

*Optimized for 320×320 inference on 128MB RAM*
