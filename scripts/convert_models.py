#!/usr/bin/env python3
import os
import sys
import subprocess
import shutil

def run_command(cmd, cwd=None):
    """Run a command and return output"""
    print(f"Running: {' '.join(cmd)}")
    try:
        result = subprocess.run(cmd, cwd=cwd, capture_output=True, text=True)
        if result.returncode != 0:
            print(f"Error: {result.stderr}")
            return False, result.stderr
        print(f"Output: {result.stdout}")
        return True, result.stdout
    except Exception as e:
        print(f"Exception: {e}")
        return False, str(e)

def convert_sensevoice_to_gguf(model_dir, output_dir):
    """Convert SenseVoice model to GGUF format"""
    print("\n=== Converting SenseVoice to GGUF ===")
    
    model_pt = os.path.join(model_dir, 'model.pt')
    if not os.path.exists(model_pt):
        print(f"Error: model.pt not found in {model_dir}")
        return False
    
    os.makedirs(output_dir, exist_ok=True)
    
    cmd = [
        sys.executable, "-m", "llama.cpp.converter",
        "--model", model_pt,
        "--outfile", os.path.join(output_dir, "sense-voice-small-q4_0.gguf"),
        "--quantize", "q4_0"
    ]
    
    success, _ = run_command(cmd)
    return success

def convert_qwen3_tts_to_gguf(model_dir, output_dir):
    """Convert Qwen3-TTS model to GGUF format"""
    print("\n=== Converting Qwen3-TTS to GGUF ===")
    
    config_json = os.path.join(model_dir, 'config.json')
    if not os.path.exists(config_json):
        print(f"Error: config.json not found in {model_dir}")
        return False
    
    os.makedirs(output_dir, exist_ok=True)
    
    cmd = [
        sys.executable, "-m", "llama.cpp.converter",
        "--model", model_dir,
        "--outfile", os.path.join(output_dir, "qwen3-tts-0.5b-q4_0.gguf"),
        "--quantize", "q4_0"
    ]
    
    success, _ = run_command(cmd)
    return success

def copy_silero_model(src_dir, dest_dir):
    """Copy Silero VAD ONNX model to proper location"""
    print("\n=== Copying Silero VAD model ===")
    
    src_model = os.path.join(src_dir, 'src', 'silero_vad', 'data', 'silero_vad.onnx')
    if not os.path.exists(src_model):
        print(f"Error: silero_vad.onnx not found in {src_model}")
        return False
    
    os.makedirs(dest_dir, exist_ok=True)
    dest_model = os.path.join(dest_dir, 'silero_vad.onnx')
    
    shutil.copy2(src_model, dest_model)
    print(f"Copied {src_model} to {dest_model}")
    return True

def main():
    base_dir = os.path.join(os.path.dirname(os.path.dirname(__file__)), 'res', 'voice_deps')
    
    print(f"Base directory: {base_dir}")
    
    # 1. Copy Silero VAD model to root of silero-vad directory
    silero_src = os.path.join(base_dir, 'silero-vad')
    silero_dest = os.path.join(base_dir, 'silero-vad')
    copy_silero_model(silero_src, silero_dest)
    
    # 2. Convert SenseVoice to GGUF
    sensevoice_src = os.path.join(base_dir, 'sensevoice-small')
    sensevoice_dest = os.path.join(base_dir, 'sensevoice-small')
    convert_sensevoice_to_gguf(sensevoice_src, sensevoice_dest)
    
    # 3. Convert Qwen3-TTS to GGUF
    qwen3_src = os.path.join(base_dir, 'qwen3-tts')
    qwen3_dest = os.path.join(base_dir, 'qwen3-tts')
    convert_qwen3_tts_to_gguf(qwen3_src, qwen3_dest)
    
    print("\n=== Conversion Complete ===")
    print("Check the model directories for GGUF files.")
    print("If conversion failed, you may need to install llama.cpp Python package:")
    print("pip install llama-cpp-python")

if __name__ == "__main__":
    main()
