#!/usr/bin/env python3
"""
K230D Model Conversion Script
Converts ONNX YOLOv8n models to K230D kmodel format using nncase
"""

import os
import sys
import nncase

def convert_onnx_to_kmodel(
    onnx_path,
    output_path,
    input_shape=(1, 3, 320, 320),
    target='k230',
    dump_dir='./tmp'
):
    """
    Convert ONNX model to K230D kmodel format
    
    Args:
        onnx_path: Path to input ONNX model
        output_path: Path to output kmodel file
        input_shape: Input tensor shape [batch, channels, height, width]
        target: Target platform (k230)
        dump_dir: Directory for intermediate files
    """
    
    print(f"Converting {onnx_path} to K230D kmodel...")
    print(f"Input shape: {input_shape}")
    
    # Check if input file exists
    if not os.path.exists(onnx_path):
        print(f"Error: Input file {onnx_path} not found!")
        sys.exit(1)
    
    # Create dump directory
    os.makedirs(dump_dir, exist_ok=True)
    
    # Configure compilation options for K230D
    compile_options = nncase.CompileOptions()
    compile_options.target = target
    compile_options.dump_ir = True
    compile_options.dump_asm = True
    compile_options.dump_dir = dump_dir
    
    # Input configuration
    compile_options.input_type = 'float32'
    compile_options.input_shape = list(input_shape)
    compile_options.input_range = [0, 1]  # Normalized input range
    
    # Output configuration
    compile_options.output_type = 'float32'
    
    # Quantization options (optional - uncomment for INT8)
    # compile_options.quant_type = 'uint8'
    # compile_options.w_quant_type = 'uint8'
    # compile_options.use_mix_quant = False
    
    # Optimization options
    compile_options.preprocess = False
    compile_options.swapRB = False
    compile_options.mean = [0, 0, 0]
    compile_options.std = [1, 1, 1]
    
    print("\nCompiling model for K230D...")
    
    try:
        # Initialize compiler
        compiler = nncase.Compiler(compile_options)
        
        # Import ONNX model
        print("Loading ONNX model...")
        with open(onnx_path, 'rb') as f:
            model_content = f.read()
        
        compiler.import_onnx(model_content, '')
        
        # Compile the model
        print("Compiling to kmodel...")
        compiler.compile()
        
        # Generate kmodel
        print("Generating kmodel binary...")
        kmodel = compiler.gencode_tobytes()
        
        # Save kmodel
        with open(output_path, 'wb') as f:
            f.write(kmodel)
        
        file_size = os.path.getsize(output_path) / (1024 * 1024)  # Size in MB
        print(f"\n✓ Conversion successful!")
        print(f"  Output: {output_path}")
        print(f"  Size: {file_size:.2f} MB")
        
    except Exception as e:
        print(f"\n✗ Conversion failed: {str(e)}")
        sys.exit(1)


def main():
    """Main conversion function with configurable parameters"""
    
    # Default paths
    onnx_model = "/workspace/best.onnx"
    output_model = "/workspace/yolov8n_k230.kmodel"
    
    # Check for command line arguments
    if len(sys.argv) > 1:
        onnx_model = sys.argv[1]
    if len(sys.argv) > 2:
        output_model = sys.argv[2]
    
    print("=" * 60)
    print("K230D Model Conversion Tool")
    print("=" * 60)
    print(f"Input ONNX:  {onnx_model}")
    print(f"Output kmodel: {output_model}")
    print("=" * 60)
    
    # Convert the model
    convert_onnx_to_kmodel(
        onnx_path=onnx_model,
        output_path=output_model,
        input_shape=(1, 3, 320, 320),  # YOLOv8n typical input size
        target='k230'
    )
    
    print("\n" + "=" * 60)
    print("Conversion complete! Deploy the .kmodel to your K230D device.")
    print("=" * 60)


if __name__ == "__main__":
    main()
