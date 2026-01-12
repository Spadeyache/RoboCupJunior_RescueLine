#!/usr/bin/env python3
"""
YOLOv8n Training Script for K230D Deployment
Optimized for embedded deployment on RISC-V K230D chip
"""

from ultralytics import YOLO
import torch

def train_yolov8n_for_k230d(
    data_yaml='data.yaml',
    epochs=100,
    imgsz=320,
    batch=16,
    device=0,
    project='runs/detect',
    name='k230d_train'
):
    """
    Train YOLOv8n model optimized for K230D deployment
    
    Args:
        data_yaml: Path to dataset configuration file
        epochs: Number of training epochs
        imgsz: Input image size (320 recommended for K230D)
        batch: Batch size
        device: GPU device (0 for first GPU, 'cpu' for CPU)
        project: Project directory
        name: Training run name
    """
    
    print("=" * 70)
    print("YOLOv8n Training for K230D Deployment")
    print("=" * 70)
    print(f"Dataset:     {data_yaml}")
    print(f"Epochs:      {epochs}")
    print(f"Image size:  {imgsz}x{imgsz}")
    print(f"Batch size:  {batch}")
    print(f"Device:      {device}")
    print("=" * 70)
    
    # Initialize YOLOv8n model
    model = YOLO('yolov8n.pt')
    
    # Check if CUDA is available
    if torch.cuda.is_available():
        print(f"\n✓ GPU Available: {torch.cuda.get_device_name(0)}")
    else:
        print("\n⚠ No GPU detected, training will use CPU (slower)")
        device = 'cpu'
    
    # Training configuration optimized for embedded deployment
    results = model.train(
        data=data_yaml,
        epochs=epochs,
        imgsz=imgsz,
        batch=batch,
        device=device,
        project=project,
        name=name,
        
        # Optimization settings for embedded deployment
        patience=50,           # Early stopping patience
        save=True,            # Save checkpoints
        save_period=10,       # Save checkpoint every 10 epochs
        cache=False,          # Don't cache images (memory constraint)
        
        # Model optimization
        optimizer='AdamW',    # Optimizer
        lr0=0.01,            # Initial learning rate
        lrf=0.01,            # Final learning rate factor
        momentum=0.937,      # Momentum
        weight_decay=0.0005, # Weight decay
        
        # Augmentation (moderate for embedded)
        hsv_h=0.015,         # HSV-Hue augmentation
        hsv_s=0.7,           # HSV-Saturation augmentation
        hsv_v=0.4,           # HSV-Value augmentation
        degrees=0.0,         # Rotation augmentation
        translate=0.1,       # Translation augmentation
        scale=0.5,           # Scale augmentation
        shear=0.0,           # Shear augmentation
        perspective=0.0,     # Perspective augmentation
        flipud=0.0,          # Vertical flip probability
        fliplr=0.5,          # Horizontal flip probability
        mosaic=1.0,          # Mosaic augmentation
        mixup=0.0,           # Mixup augmentation
        
        # Validation settings
        val=True,
        plots=True,
        verbose=True
    )
    
    print("\n" + "=" * 70)
    print("Training Complete!")
    print("=" * 70)
    print(f"Best model saved at: {project}/{name}/weights/best.pt")
    print(f"Last model saved at: {project}/{name}/weights/last.pt")
    
    return results


def export_to_onnx(
    model_path,
    output_path='best.onnx',
    imgsz=320,
    simplify=True
):
    """
    Export trained model to ONNX format for K230D conversion
    
    Args:
        model_path: Path to trained .pt model
        output_path: Output ONNX file path
        imgsz: Input image size
        simplify: Simplify ONNX model
    """
    
    print("\n" + "=" * 70)
    print("Exporting to ONNX format")
    print("=" * 70)
    print(f"Model:  {model_path}")
    print(f"Output: {output_path}")
    print(f"Size:   {imgsz}x{imgsz}")
    print("=" * 70)
    
    # Load the trained model
    model = YOLO(model_path)
    
    # Export to ONNX
    model.export(
        format='onnx',
        imgsz=imgsz,
        simplify=simplify,
        dynamic=False,      # Static shape for K230D
        opset=11,          # ONNX opset version
    )
    
    print(f"\n✓ ONNX export complete: {output_path}")
    print("\nNext steps:")
    print("1. Copy the ONNX file to the convert container")
    print("2. Run the conversion script: python3 convert_to_k230.py")
    print("3. Deploy the .kmodel file to your K230D device")


def main():
    """Main training pipeline"""
    
    import argparse
    
    parser = argparse.ArgumentParser(description='Train YOLOv8n for K230D')
    parser.add_argument('--data', type=str, default='data.yaml', help='Path to data.yaml')
    parser.add_argument('--epochs', type=int, default=100, help='Number of epochs')
    parser.add_argument('--imgsz', type=int, default=320, help='Image size')
    parser.add_argument('--batch', type=int, default=16, help='Batch size')
    parser.add_argument('--device', default=0, help='GPU device or cpu')
    parser.add_argument('--name', type=str, default='k230d_train', help='Run name')
    parser.add_argument('--export', action='store_true', help='Export to ONNX after training')
    
    args = parser.parse_args()
    
    # Train the model
    results = train_yolov8n_for_k230d(
        data_yaml=args.data,
        epochs=args.epochs,
        imgsz=args.imgsz,
        batch=args.batch,
        device=args.device,
        name=args.name
    )
    
    # Export to ONNX if requested
    if args.export:
        best_model = f"runs/detect/{args.name}/weights/best.pt"
        export_to_onnx(
            model_path=best_model,
            output_path='/workspace/best.onnx',
            imgsz=args.imgsz
        )


if __name__ == "__main__":
    main()
