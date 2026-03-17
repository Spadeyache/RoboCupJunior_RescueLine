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


def generate_calibration_data(dataset_path: str, img_height: int = 480, img_width: int = 640, num_samples: int = 100):
    """
    Generate calibration dataset for INT8 quantization
    
    Args:
        dataset_path: Path to validation images
        img_height: Input image height
        img_width: Input image width
        num_samples: Number of calibration samples
    
    Returns:
        List of preprocessed image arrays
    """
    import cv2
    from glob import glob
    
    print(f"\n[CALIB] Generating calibration data from: {dataset_path}")
    print(f"[CALIB] Target input size: {img_width}x{img_height} (WxH)")
    print(f"[CALIB] Requested samples: {num_samples}")
    
    # Find image files
    image_files = []
    for ext in ['*.jpg', '*.jpeg', '*.png', '*.bmp']:
        found = glob(os.path.join(dataset_path, '**', ext), recursive=True)
        print(f"[CALIB]   Found {len(found)} {ext} files")
        image_files.extend(found)
    
    if not image_files:
        raise ValueError(f"No images found in {dataset_path}")
    
    print(f"[CALIB] Total images found: {len(image_files)}")
    
    # Limit to num_samples
    image_files = image_files[:num_samples]
    print(f"[CALIB] Using first {len(image_files)} images for calibration")
    
    calibration_data = []
    failed = 0
    
    for idx, img_path in enumerate(image_files):
        try:
            # Read image
            img = cv2.imread(img_path)
            if img is None:
                print(f"[CALIB]   WARNING: Could not read {img_path}")
                failed += 1
                continue
            
            original_h, original_w = img.shape[:2]
            
            # Resize to model input size (width, height)
            img = cv2.resize(img, (img_width, img_height))
            
            # Convert BGR to RGB
            img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
            
            # FIX: Do NOT normalize to [0,1]. K230D ai2d feeds raw uint8 [0,255]
            # directly into the KPU at runtime — calibration must match that range.
            # Old (wrong): img = img.astype(np.float32) / 255.0
            img = img.astype(np.float32)  # keep as [0, 255] to match runtime input
            
            # Convert to NCHW format
            img = np.transpose(img, (2, 0, 1))
            img = np.expand_dims(img, axis=0)
            
            # Print stats for first 3 images so we can verify pipeline
            if idx < 3:
                print(f"[CALIB]   Sample {idx}: {os.path.basename(img_path)}")
                print(f"[CALIB]     Original size : {original_w}x{original_h}")
                print(f"[CALIB]     Resized shape : {img.shape}  (NCHW)")
                print(f"[CALIB]     dtype         : {img.dtype}")
                print(f"[CALIB]     min / max     : {img.min():.1f} / {img.max():.1f}  (should be ~0 / ~255)")
                print(f"[CALIB]     mean          : {img.mean():.2f}")
            
            calibration_data.append(img)
            
        except Exception as e:
            print(f"[CALIB]   WARNING: Failed to process {img_path}: {e}")
            failed += 1
            continue
    
    if not calibration_data:
        raise ValueError("Failed to generate any calibration data")
    
    # Summary stats across all calibration images
    all_data = np.concatenate(calibration_data, axis=0)
    print(f"\n[CALIB] ── Summary ──────────────────────────────")
    print(f"[CALIB]   Loaded successfully : {len(calibration_data)}")
    print(f"[CALIB]   Failed              : {failed}")
    print(f"[CALIB]   Final batch shape   : {all_data.shape}  (N, C, H, W)")
    print(f"[CALIB]   dtype               : {all_data.dtype}")
    print(f"[CALIB]   global min          : {all_data.min():.2f}  (should be ~0)")
    print(f"[CALIB]   global max          : {all_data.max():.2f}  (should be ~255)")
    print(f"[CALIB]   global mean         : {all_data.mean():.2f}  (typical ~100-150)")
    print(f"[CALIB] ────────────────────────────────────────────")
    del all_data

    return calibration_data


def verify_onnx_model(onnx_path: str, img_height: int, img_width: int):
    """
    Verify ONNX model input/output shapes match our expectations
    """
    print(f"\n[ONNX] Verifying model: {onnx_path}")
    model = onnx.load(onnx_path)
    onnx.checker.check_model(model)
    print(f"[ONNX]   ✓ Model is valid")

    # Input info
    for inp in model.graph.input:
        shape = [d.dim_value for d in inp.type.tensor_type.shape.dim]
        dtype = inp.type.tensor_type.elem_type
        print(f"[ONNX]   Input  '{inp.name}': shape={shape}, dtype_id={dtype}")
        # Check if shape matches our target
        if len(shape) == 4:
            _, c, h, w = shape
            if h != img_height or w != img_width:
                print(f"[ONNX]   ⚠ WARNING: ONNX input {w}x{h} does not match "
                      f"target {img_width}x{img_height} — make sure these match!")
            else:
                print(f"[ONNX]   ✓ Input size matches target {img_width}x{img_height}")
            if c != 3:
                print(f"[ONNX]   ⚠ WARNING: Expected 3 channels (RGB), got {c}")
            else:
                print(f"[ONNX]   ✓ Channels = 3 (RGB)")

    # Output info
    for out in model.graph.output:
        shape = [d.dim_value for d in out.type.tensor_type.shape.dim]
        print(f"[ONNX]   Output '{out.name}': shape={shape}")
        # For YOLOv8 detect: expected (1, 4+num_classes, num_anchors)
        if len(shape) == 3:
            _, rows, anchors = shape
            num_classes = rows - 4
            print(f"[ONNX]   ✓ YOLOv8 output detected: {anchors} anchors, "
                  f"{rows} rows = 4 box + {num_classes} classes")

    print(f"[ONNX]   Model opset: {model.opset_import[0].version}")
    print(f"[ONNX]   IR version : {model.ir_version}")


def convert_to_kmodel(
    onnx_path: str,
    output_dir: str = "/output",
    calibration_data_path: str = None,
    img_height: int = 480,
    img_width: int = 640,
    target: str = "k230",
    num_calibration_samples: int = 100
):
    """
    Convert ONNX model to K230D .kmodel format
    
    Args:
        onnx_path: Path to ONNX model
        output_dir: Output directory for .kmodel
        calibration_data_path: Path to calibration images
        img_height: Input image height
        img_width: Input image width
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
    
    print(f"\n[CONFIG] Input ONNX  : {onnx_path}")
    print(f"[CONFIG] Output kmodel: {kmodel_path}")
    print(f"[CONFIG] Target       : {target}")
    print(f"[CONFIG] Input size   : {img_width}x{img_height} (WxH)")
    print(f"[CONFIG] Quantization : INT8 (uint8 activations + weights)")
    print(f"[CONFIG] Calib samples: {num_calibration_samples}")

    # Verify ONNX model shape matches our target before doing anything else
    verify_onnx_model(onnx_path, img_height, img_width)
    
    # Import nncase
    try:
        import nncase
        print(f"\n[NNCASE] ✓ nncase version: {getattr(nncase, '__version__', 'unknown')}")
    except ImportError:
        raise ImportError("nncase not found. Please install: pip install nncase nncase-kpu")
    
    # Generate or load calibration data
    if calibration_data_path:
        calibration_data = generate_calibration_data(
            calibration_data_path,
            img_height,
            img_width,
            num_calibration_samples
        )
    else:
        # FIX: random data also uses [0, 255] range to match runtime
        # Old (wrong): np.random.rand(...).astype(np.float32) -> [0, 1]
        print("\n[CALIB] ⚠ No calibration data provided — using random data (NOT RECOMMENDED)")
        print(f"[CALIB]   Shape per sample: (1, 3, {img_height}, {img_width})")
        print(f"[CALIB]   Range: [0, 255] float32 to match K230D runtime")
        calibration_data = [
            (np.random.rand(1, 3, img_height, img_width) * 255).astype(np.float32)
            for _ in range(10)
        ]
        print(f"[CALIB]   Generated 10 random samples")
        print(f"[CALIB]   Sample min/max: {calibration_data[0].min():.1f} / {calibration_data[0].max():.1f}")
    
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
        print(f"[COMPILE] target     : {compile_options.target}")
        print(f"[COMPILE] dump_ir    : {compile_options.dump_ir}")
        
        # Configure quantization options for INT8
        ptq_options = nncase.PTQTensorOptions()
        ptq_options.quant_type = "uint8"
        ptq_options.w_quant_type = "uint8"
        ptq_options.calibrate_method = "Kld"  # KL divergence method
        ptq_options.finetune_weights_method = "NoFineTuneWeights"
        ptq_options.samples_count = len(calibration_data)
        print(f"[COMPILE] quant_type : {ptq_options.quant_type}")
        print(f"[COMPILE] w_quant    : {ptq_options.w_quant_type}")
        print(f"[COMPILE] calib_method: {ptq_options.calibrate_method}")
        print(f"[COMPILE] calib samples: {ptq_options.samples_count}")
        
        # Set calibration data
        print("\n[COMPILE] Setting up calibration data...")
        ptq_data = [[img] for img in calibration_data]
        print(f"[COMPILE]   ptq_data length : {len(ptq_data)}")
        print(f"[COMPILE]   ptq_data[0] shape: {ptq_data[0][0].shape}")
        print(f"[COMPILE]   ptq_data[0] dtype: {ptq_data[0][0].dtype}")
        print(f"[COMPILE]   ptq_data[0] min  : {ptq_data[0][0].min():.1f}")
        print(f"[COMPILE]   ptq_data[0] max  : {ptq_data[0][0].max():.1f}")
        ptq_options.set_tensor_data(ptq_data)
        print(f"[COMPILE] ✓ Calibration data set")
        
        compile_options.quant_options = ptq_options
        
        # Create compiler
        compiler = nncase.Compiler(compile_options)
        print(f"[COMPILE] ✓ Compiler created")
        
        # Import ONNX model
        print("\n[COMPILE] Importing ONNX model...")
        import_options = nncase.ImportOptions()
        model_content = open(onnx_path, 'rb').read()
        print(f"[COMPILE]   ONNX file size: {len(model_content)/1024/1024:.2f} MB")
        compiler.import_onnx(model_content, import_options)
        print(f"[COMPILE] ✓ ONNX imported")
        
        compiler.use_ptq(ptq_options)
        print(f"[COMPILE] ✓ PTQ options applied")
        
        # Compile the model
        print("\n[COMPILE] Compiling to kmodel (this may take several minutes)...")
        compiler.compile()
        print(f"[COMPILE] ✓ Compilation done")
        
        # Generate kmodel
        print("[COMPILE] Generating kmodel bytes...")
        kmodel = compiler.gencode_tobytes()
        print(f"[COMPILE]   kmodel bytes: {len(kmodel)/1024/1024:.2f} MB")
        
        # Save kmodel
        with open(kmodel_path, 'wb') as f:
            f.write(kmodel)
        
        print("\n" + "=" * 80)
        print("Conversion Complete!")
        print("=" * 80)
        
        kmodel_size_mb = os.path.getsize(kmodel_path) / (1024 * 1024)
        print(f"\n[OUTPUT] ✓ kmodel saved : {kmodel_path}")
        print(f"[OUTPUT] ✓ Model size   : {kmodel_size_mb:.2f} MB")
        
        # Verify it's small enough for K230D (128MB RAM)
        if kmodel_size_mb > 10:
            print(f"[OUTPUT] ⚠ WARNING: Model size {kmodel_size_mb:.2f} MB may be too large "
                  f"for K230D's 128MB RAM")
            print(f"[OUTPUT]   Consider reducing model complexity or using smaller input size")
        else:
            print(f"[OUTPUT] ✓ Model size is suitable for K230D deployment")
        
        print(f"\n[OUTPUT] ✓ Next step: Deploy to K230D using deploy_canmv.py script")
        
        # Clean up
        gc.collect()
        
        return kmodel_path
        
    except Exception as e:
        print(f"\n[COMPILE] ✗ Compilation failed: {e}")
        raise


if __name__ == "__main__":
    import argparse
    
    parser = argparse.ArgumentParser(description="Convert ONNX to K230D kmodel")
    parser.add_argument("onnx", type=str, help="Path to ONNX model")
    parser.add_argument("--output", type=str, default="/output", help="Output directory")
    parser.add_argument("--calib-data", type=str, default=None, 
                        help="Path to calibration images (required for good accuracy)")
    parser.add_argument("--img-height", type=int, default=480, help="Image height")
    parser.add_argument("--img-width", type=int, default=640, help="Image width")
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
        img_height=args.img_height,
        img_width=args.img_width,
        target=args.target,
        num_calibration_samples=args.num_samples
    )
    
    print(f"\n✓ Conversion complete: {kmodel_path}")