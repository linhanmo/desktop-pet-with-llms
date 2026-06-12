#!/usr/bin/env python3
import os
import sys
import urllib.request
import zipfile
import tarfile
import hashlib

def download_file(url, dest_path):
    """Download a file from URL to destination path"""
    print(f"Downloading {url}...")
    os.makedirs(os.path.dirname(dest_path), exist_ok=True)
    
    try:
        urllib.request.urlretrieve(url, dest_path)
        print(f"Downloaded to {dest_path}")
        return True
    except Exception as e:
        print(f"Failed to download {url}: {e}")
        return False

def verify_checksum(file_path, expected_md5=None):
    """Verify file checksum"""
    if not expected_md5:
        return True
    md5_hash = hashlib.md5()
    with open(file_path, "rb") as f:
        for chunk in iter(lambda: f.read(4096), b""):
            md5_hash.update(chunk)
    return md5_hash.hexdigest() == expected_md5

def extract_zip(zip_path, dest_dir):
    """Extract ZIP file"""
    print(f"Extracting {zip_path} to {dest_dir}...")
    with zipfile.ZipFile(zip_path, 'r') as zip_ref:
        zip_ref.extractall(dest_dir)
    os.remove(zip_path)

def extract_tar(tar_path, dest_dir):
    """Extract TAR file"""
    print(f"Extracting {tar_path} to {dest_dir}...")
    with tarfile.open(tar_path, 'r:gz') as tar_ref:
        tar_ref.extractall(dest_dir)
    os.remove(tar_path)

def main():
    # Base directory for voice models
    base_dir = os.path.join(os.path.dirname(os.path.dirname(__file__)), 'res', 'voice_deps')
    os.makedirs(base_dir, exist_ok=True)
    
    print(f"Downloading voice models to {base_dir}")
    
    # 1. Silero VAD v5 ONNX Model
    print("\n=== Downloading Silero VAD v5 ONNX Model ===")
    silero_dir = os.path.join(base_dir, 'silero-vad')
    os.makedirs(silero_dir, exist_ok=True)
    
    silero_url = "https://github.com/snakers4/silero-vad/raw/master/files/silero_vad_v5.onnx"
    silero_path = os.path.join(silero_dir, 'silero_vad_v5.onnx')
    if not os.path.exists(silero_path):
        download_file(silero_url, silero_path)
    
    # 2. SenseVoice Small GGUF (STT)
    print("\n=== Downloading SenseVoice Small GGUF Model ===")
    sensevoice_dir = os.path.join(base_dir, 'sensevoice-small')
    os.makedirs(sensevoice_dir, exist_ok=True)
    
    sensevoice_url = "https://huggingface.co/csukuangfj/sense-voice-small-gguf/resolve/main/sense-voice-small-q4_0.gguf"
    sensevoice_path = os.path.join(sensevoice_dir, 'sense-voice-small-q4_0.gguf')
    if not os.path.exists(sensevoice_path):
        download_file(sensevoice_url, sensevoice_path)
    
    # 3. Sherpa-onnx for KWS (User should download manually)
    print("\n=== Sherpa-onnx for KWS ===")
    sherpa_dir = os.path.join(base_dir, 'sherpa-onnx-v1.12.10-win-x64-shared')
    os.makedirs(sherpa_dir, exist_ok=True)
    print("Please download sherpa-onnx from: https://github.com/k2-fsa/sherpa-onnx/releases")
    print(f"Place files in: {sherpa_dir}")
    
    # 4. Qwen3-TTS GGUF (User should download manually)
    print("\n=== Qwen3-TTS GGUF Model ===")
    qwen3_dir = os.path.join(base_dir, 'qwen3-tts')
    os.makedirs(qwen3_dir, exist_ok=True)
    print("Please download Qwen3-TTS model and convert to GGUF format")
    print(f"Place files in: {qwen3_dir}")
    
    print("\n=== Download Complete ===")
    print(f"Models downloaded to: {base_dir}")
    print("\nNext steps:")
    print("1. Download sherpa-onnx binaries and place in sherpa-onnx-v1.12.10-win-x64-shared/")
    print("2. Download Qwen3-TTS model and place in qwen3-tts/")
    print("3. Build the project with CMake")

if __name__ == "__main__":
    main()
