# Conversion Environment for K230D (nncase)
FROM ubuntu:22.04

# Prevent interactive prompts
ENV DEBIAN_FRONTEND=noninteractive

# Set working directory
WORKDIR /workspace

# Install system dependencies
RUN apt-get update && apt-get install -y \
    wget \
    curl \
    git \
    vim \
    python3.10 \
    python3-pip \
    libgomp1 \
    libgl1-mesa-glx \
    libglib2.0-0 \
    && rm -rf /var/lib/apt/lists/*

# Install .NET SDK 7.0 (required for nncase 2.x)
RUN wget https://dot.net/v1/dotnet-install.sh -O dotnet-install.sh && \
    chmod +x dotnet-install.sh && \
    ./dotnet-install.sh --channel 7.0 --install-dir /usr/share/dotnet && \
    ln -s /usr/share/dotnet/dotnet /usr/bin/dotnet && \
    rm dotnet-install.sh

# Set .NET environment variables
ENV DOTNET_ROOT=/usr/share/dotnet
ENV PATH="${DOTNET_ROOT}:${PATH}"

# Install Python packages for conversion
RUN pip3 install --no-cache-dir \
    onnx==1.15.0 \
    onnxruntime==1.16.3 \
    opencv-python==4.8.1.78 \
    numpy==1.24.3 \
    pillow==10.1.0 \
    pyyaml==6.0.1 \
    tqdm==4.66.1

# Install nncase v2.x for K230D
# Note: Adjust version based on official K230D SDK recommendations
RUN pip3 install --no-cache-dir \
    nncase==2.10.0 \
    nncase-kpu==2.10.0

# Create necessary directories
RUN mkdir -p /models /output /test_images /workspace

# Verify installations
RUN python3 --version && \
    dotnet --version && \
    python3 -c "import nncase; print('nncase imported successfully')"

CMD ["/bin/bash"]
