#!/usr/bin/env python3
"""
Convert ONNX model to K230D .kmodel format using nncase
Applies INT8 quantization for KPU v3.0 deployment
"""

import os
import sys
import gc
import numpy as np
from pathlib import Path
import onnx


def generate_calibration_data(dataset_path: str, img_size: int = 320, num_samples: int = 100):
    """
    Generate calibration dataset for INT8 quantization
    
    Args:
        dataset_path: Path to validation images
        img_size: Input image size
        num_samples: Number of calibration samples
    
    Returns:
        List of preprocessed image arrays
    """
    import cv2
    from glob import glob
    
    print(f"Generating calibration data from: {dataset_path}")
    
    # Find image files
    image_files = []
    for ext in ['*.jpg', '*.jpeg', '*.png', '*.bmp']:
        image_files.extend(glob(os.path.join(dataset_path, '**', ext), recursive=True))
    
    if not image_files:
        raise ValueError(f"No images found in {dataset_path}")
    
    # Limit to num_samples
    image_files = image_files[:num_samples]
    print(f"Using {len(image_files)} images for calibration")
    
    calibration_data = []
    
    for img_path in image_files:
        try:
            # Read image
            img = cv2.imread(img_path)
            if img is None:
                continue
            
            # Resize to model input size
            img = cv2.resize(img, (img_size, img_size))
            
            # Convert BGR to RGB
            img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
            
            # Normalize to [0, 1]
            img = img.astype(np.float32) / 255.0
            
            # Convert to NCHW format
            img = np.transpose(img, (2, 0, 1))
            img = np.expand_dims(img, axis=0)
            
            calibration_data.append(img)
            
        except Exception as e:
            print(f"Warning: Failed to process {img_path}: {e}")
            continue
    
    if not calibration_data:
        raise ValueError("Failed to generate calibration data")
    
    print(f"✓ Generated {len(calibration_data)} calibration samples")
    return calibration_data


def convert_to_kmodel(
    onnx_path: str,
    output_dir: str = "/output",
    calibration_data_path: str = None,
    img_size: int = 320,
    target: str = "k230",
    num_calibration_samples: int = 100
):
    """
    Convert ONNX model to K230D .kmodel format
    
    Args:
        onnx_path: Path to ONNX model
        output_dir: Output directory for .kmodel
        calibration_data_path: Path to calibration images
        img_size: Input image size
        target: Target device (k230)
        num_calibration_samples: Number of samples for calibration
    
    Returns:
        Path to generated .kmodel file
    """
    
    print("=" * 80)
    print("Converting ONNX to K230D .kmodel")
    print("=" * 80)
    
    # Verify ONNX model exists
    if not os.path.exists(onnx_path):
        raise FileNotFoundError(f"ONNX model not found: {onnx_path}")
    
    # Create output directory
    os.makedirs(output_dir, exist_ok=True)
    
    # Generate output filename
    model_name = Path(onnx_path).stem
    kmodel_filename = f"{model_name}_k230d.kmodel"
    kmodel_path = os.path.join(output_dir, kmodel_filename)
    
    print(f"\nConfiguration:")
    print(f"  - Input ONNX: {onnx_path}")
    print(f"  - Output kmodel: {kmodel_path}")
    print(f"  - Target: {target}")
    print(f"  - Input Size: {img_size}x{img_size}")
    print(f"  - Quantization: INT8")
    
    # Import nncase
    try:
        import nncase
        print(f"\n✓ nncase version: {getattr(nncase, '__version__', 'unknown')}")
    except ImportError:
        raise ImportError("nncase not found. Please install: pip install nncase nncase-kpu")
    
    # Generate or load calibration data
    if calibration_data_path:
        print(f"\nGenerating calibration data...")
        calibration_data = generate_calibration_data(
            calibration_data_path, 
            img_size, 
            num_calibration_samples
        )
    else:
        print("\n⚠ No calibration data provided. Using random data (NOT RECOMMENDED for production)")
        calibration_data = [np.random.rand(1, 3, img_size, img_size).astype(np.float32) for _ in range(10)]
    
    print("\n" + "=" * 80)
    print("Starting Compilation...")
    print("=" * 80 + "\n")
    
    try:
        # Configure compiler
        compile_options = nncase.CompileOptions()
        compile_options.target = target
        compile_options.dump_ir = False
        compile_options.dump_asm = False
        compile_options.dump_dir = "tmp"
        
        # Configure quantization options for INT8
        ptq_options = nncase.PTQTensorOptions()
        ptq_options.quant_type = "uint8"  # INT8 quantization
        ptq_options.w_quant_type = "uint8"
        ptq_options.calibrate_method = "Kld"  # KL divergence method
        ptq_options.finetune_weights_method = "NoFineTuneWeights"
        ptq_options.samples_count = len(calibration_data)
        
        # Set calibration data
        print("Setting up calibration data...")
        ptq_data = [[img] for img in calibration_data]
        ptq_options.set_tensor_data(ptq_data)
        
        compile_options.quant_options = ptq_options
        
        # Create compiler
        compiler = nncase.Compiler(compile_options)
        
        # Import ONNX model
        print("Importing ONNX model...")
        import_options = nncase.ImportOptions()
        model_content = open(onnx_path, 'rb').read()
        compiler.import_onnx(model_content, import_options)
        
        compiler.use_ptq(ptq_options)
        
        # Compile the model
        print("\nCompiling to kmodel (this may take several minutes)...")
        compiler.compile()
        
        # Generate kmodel
        print("Generating kmodel file...")
        kmodel = compiler.gencode_tobytes()
        
        # Save kmodel
        with open(kmodel_path, 'wb') as f:
            f.write(kmodel)
        
        print("\n" + "=" * 80)
        print("Conversion Complete!")
        print("=" * 80)
        
        # Print file info
        kmodel_size_mb = os.path.getsize(kmodel_path) / (1024 * 1024)
        print(f"\n✓ kmodel saved: {kmodel_path}")
        print(f"✓ Model size: {kmodel_size_mb:.2f} MB")
        
        # Verify it's small enough for K230D (128MB RAM)
        if kmodel_size_mb > 10:
            print(f"\n⚠ WARNING: Model size is {kmodel_size_mb:.2f} MB")
            print(f"  This may be too large for K230D's 128MB RAM during inference")
            print(f"  Consider reducing model complexity or using smaller input size")
        else:
            print(f"\n✓ Model size is suitable for K230D deployment")
        
        print(f"\n✓ Next step: Deploy to K230D using deploy_canmv.py script")
        
        # Clean up
        gc.collect()
        
        return kmodel_path
        
    except Exception as e:
        print(f"\n✗ Compilation failed: {e}")
        raise


if __name__ == "__main__":
    import argparse
    
    parser = argparse.ArgumentParser(description="Convert ONNX to K230D kmodel")
    parser.add_argument("onnx", type=str, help="Path to ONNX model")
    parser.add_argument("--output", type=str, default="/output", help="Output directory")
    parser.add_argument("--calib-data", type=str, default=None, 
                        help="Path to calibration images (required for good accuracy)")
    parser.add_argument("--img", type=int, default=320, help="Image size")
    parser.add_argument("--target", type=str, default="k230", help="Target device")
    parser.add_argument("--num-samples", type=int, default=100, 
                        help="Number of calibration samples")
    
    args = parser.parse_args()
    
    if not args.calib_data:
        print("\n⚠ WARNING: No calibration data provided!")
        print("  For best accuracy, provide calibration images with --calib-data")
        print("  Example: --calib-data /datasets/val/images\n")
        response = input("Continue with random calibration? [y/N]: ")
        if response.lower() != 'y':
            sys.exit(1)
    
    kmodel_path = convert_to_kmodel(
        onnx_path=args.onnx,
        output_dir=args.output,
        calibration_data_path=args.calib_data,
        img_size=args.img,
        target=args.target,
        num_calibration_samples=args.num_samples
    )
    
    print(f"\n✓ Conversion complete: {kmodel_path}")
