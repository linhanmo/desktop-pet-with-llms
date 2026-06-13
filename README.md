# XiaoMo 桌面宠物（离线本地 LLM + Qwen3-TTS 语音合成）

一款基于 Qt + Live2D 的桌面宠物应用，支持本地离线 LLM 对话与离线 TTS（文字转语音），提供托盘菜单与全局快捷键，解压即用或从源码构建。

Linux 版本仓库（暂未开发完善）：[https://github.com/linhanmo/desktop-pet-with-llms-linux ](https://github.com/linhanmo/desktop-pet-with-llms-linux)

百度网盘下载（Windows 发行包镜像）：<https://pan.baidu.com/s/13QY8_rEd1pWLyFOL9JsFVQ?pwd=8888>

## 功能概览

- Live2D 看板娘：支持呼吸、眨眼、视线跟随、物理模拟（头发/衣物摆动），可设置屏幕显示、透明背景、全局置顶等。
- 本地离线 LLM 对话：内置 llama.cpp CLI 调用，支持 1.5B/7B 规模，风格可选 Original/Universal/Anime，System Prompt 可自定义。
- 离线 TTS 播报：接入本地 `Qwen3-TTS` Python 后端，Qt 侧负责流程与播放，Python 侧负责推理与 wav 生成。
- 语音交互链路：支持 `KWS` 唤醒词检测、`STT` 语音识别、`VAD` 语音活动检测；可在唤醒后直接把语音转成文字并发送给本地 LLM。
- 对话与气泡：角色旁显示对话气泡，支持不同气泡主题；聊天窗口支持清空历史、上下文条数与最大生成长度可调。
- 双对话模式：支持完整历史对话窗口，也支持更轻量的“单输入框”快速输入窗口，可在两种模式之间随时切换。
- 实时语音草稿：启用 STT 后，可在聊天窗口或单输入框中实时看到语音转文字草稿，便于边说边确认输入内容。
- 本地提醒：内置定时任务（每天固定时间或按间隔）弹出提示并可驱动动作组。
- 多语言与主题：支持简体中文、英文与主题切换；配置与对话历史持久化存储。

## 环境要求（运行）

- Windows 10/11（x64）
- 建议安装 7-Zip（用于解压 build-msvc 分卷）
- 若启用本地 Qwen3-TTS，建议准备 NVIDIA GPU；CPU 也可运行，但速度会明显更慢

## 环境要求（从源码构建）

- Git、CMake、Visual Studio 2022 Build Tools（含 VC 工具链）
- Miniconda（用于获取 Qt6）：建议创建 qt6env 并安装 qt6-main、ninja
- Windows PowerShell + winget（可选）：用于安装缺失依赖
- Python 3.10+（用于本地 Qwen3-TTS 后端）
- 建议额外准备独立虚拟环境 `.venv-qwen3-tts`，并安装 `qwen-tts`、`soundfile`

## 下载与运行（解压即用）

本项目采用 **build-msvc.zip 分卷**发布：下载后解压即可运行。

1. 下载

- 下载 Release 中的 `build-msvc.zip.001` \~ `build-msvc.zip.012`（必须全部下载到同一目录）

1. 解压

- 安装 7-Zip（推荐）后，在该目录执行：

```cmd
7z x build-msvc.zip.001
```

或在资源管理器里右键 `build-msvc.zip.001` → 7-Zip → 解压。

1. 运行

- 解压完成后，进入 `release/` 目录，运行 `XiaoMo.exe`

## 资源与目录约定

应用运行时查找资源的根目录为 `release/` 目录下的 `res/`。

`cubism.zip` 解压后的 **Cubism SDK 放在** **`sdk/`** **下**（用于从源码构建或资源更新场景），不放在 `res/` 下。

目录示例：

```
release/
  XiaoMo.exe
  res/
    bin/            # 包含 llama-cli.exe/llama.exe 等运行器（用于本地 LLM）
    llm/            # 本地 LLM 模型（*.gguf）
    models/         # Live2D 模型集合（每个子目录一个模型）
    voice_deps/     # 离线语音模型与依赖（Qwen3-TTS、sherpa-onnx STT/KWS/VAD 等）
    i18n/, icons/   # 语言与图标资源
sdk/
  cubism/         # Cubism SDK（由 cubism.zip 解压得到）
```

Live2D 模型（`res/models/`）：

- 每个模型放在独立文件夹，如 `res/models/<模型名>/`
- 模型文件优先匹配 `*.model3.json`，若无则尝试 `*.model.json`，否则回退 `model3.json/model.json/index.json`
- 默认模型根目录（设置中可改）：`文档\\XiaoMo\\Models`，首次运行会自动使用其中的第一个模型

本地 LLM 模型（`res/llm/`）：

- 将 gguf 文件放入 `res/llm/1.5B/` 或 `res/llm/7B/`（可不分文件夹，程序会自动匹配）
- 风格匹配规则（文件名包含以下关键字之一将优先选择）：
  - Anime：包含 `anime` 或 `anime.q`
  - Original：包含 `original` 或 `llama`
  - Universal：包含 `universal`
- 支持环境变量覆盖：
  - `LLAMA_RUNNER` 指定 llama 运行器路径
  - `LLM_MODEL` 指定 gguf 模型文件路径

离线 TTS（`res/voice_deps/`）：

- 当前默认 TTS 后端为 `res/voice_deps/qwen3-tts/`
- Python 后端脚本路径：`res/voice_deps/qwen3-tts/backend/qwen3_tts_backend.py`
- Base 模型默认参考音频：`res/voice_deps/qwen3-tts/prompt/default_reference.mp3`
- 程序会自动优先探测以下模型目录之一：
  - `Qwen3-TTS-12Hz-1.7B-CustomVoice`
  - `Qwen3-TTS-12Hz-0.6B-CustomVoice`
  - `Qwen3-TTS-12Hz-1.7B-VoiceDesign`
  - `Qwen3-TTS-12Hz-1.7B-Base`
  - `Qwen3-TTS-12Hz-0.6B-Base`
  - `qwen3-tts`
- 若当前加载的是 `Base` 模型，则必须提供参考音频；程序会优先使用 `prompt/default_reference.mp3`
- STT / 唤醒词 / VAD 仍使用 `res/voice_deps/sherpa-onnx-*` 与相关依赖
- 支持环境变量覆盖 Python 解释器：
  - `XIAOMO_QWEN_TTS_PYTHON`

## 快捷键与托盘

- Ctrl+T：显示/隐藏聊天窗口
- Ctrl+H：显示/隐藏桌宠窗口
- Ctrl+S：打开设置窗口
- 聊天窗口与单输入框支持一键切换，语音唤醒后会按当前“对话交互模式”自动弹出对应界面
- 托盘图标：右键菜单可快速打开聊天、设置、退出；切换显示/隐藏状态

## 使用指南（设置窗口）

设置窗口分“基础设置 / 模型设置 / AI 设置 / 高级设置”四个页签：

1. 基础设置

- 主题与语言：切换应用主题与显示语言
- 窗口行为：全局置顶、透明背景、鼠标穿透（穿透开启后需 Alt+Tab 切回再关闭）
- 输出气泡：选择角色旁边对话气泡样式（如 爱心/漫画）

1. 模型设置（Live2D）

- 模型选择：从默认模型根目录中选择不同模型；可一键打开当前模型所在目录
- 去除水印：为当前模型选择一个 `*.exp3.json` 表达式文件作为“水印表达式”，以便覆盖并去除水印效果（可随时取消）
- 动作与效果：开关“自动呼吸/自动眨眼/视线跟踪/物理模拟”

1. AI 设置（对话与语音）

- 角色名称：会替换 System Prompt 中的 `$name$` 变量
- 上下文条数：参与本地 LLM 对话的历史消息条数（建议 1.5B≈12，7B≈24）
- 最大生成长度：单次生成的最大 token 数（建议 1.5B≈192，7B≈384）
- 对话人设（System Prompt）：自定义角色语气与边界，支持 `$name$` 变量
- LLM 规模与风格：选择 1.5B/7B 以及 Original/Universal/Anime
- 对话交互模式：支持“历史对话窗口”和“单输入框”两种模式；后者更适合快速输入和语音唤醒场景
- 单输入框样式：支持 `floating / dock / hud` 等展示方式，可根据桌面布局选择更顺手的交互形态
- 离线 TTS：启用后自动朗读 AI 最终回复；当前默认接入本地 `Qwen3-TTS` 后端，可调节音量
- KWS：用于持续监听唤醒词，检测到后自动进入后续语音交互流程
- STT：用于把语音转成文字；可与 KWS 联动，也可单独开启
- VAD：用于检测说话起止，避免长时间静音也持续送入识别

1. 高级设置

- 抗锯齿 MSAA：调节渲染采样（2x/4x/8x）
- 模型显示屏幕：多屏环境可指定显示屏
- 定时任务：支持每天固定时间或按间隔触发的本地提醒

## 从源码构建（手动步骤概览）

参考以下手动流程（简版）：

1. 获取依赖

- 安装 Git、CMake、VS 2022 Build Tools（含 VC 工具链）
- 安装 Miniconda 并创建环境：`conda create -y -n qt6env -c conda-forge qt6-main ninja`

1. 拉取与配置

- 克隆仓库：`git clone https://github.com/linhanmo/desktop-pet-with-llms.git`
- 准备 Qt6：找到 `Qt6_DIR` 与 `CMAKE_PREFIX_PATH`（通常在 `envs/qt6env/Library/lib/cmake/Qt6` 与 `envs/qt6env/Library`）

1. CMake 构建

- 生成：`cmake -S Pet -B Pet/build -G "Visual Studio 17 2022" -A x64 -DQt6_DIR=... -DCMAKE_PREFIX_PATH=...`
- 编译：`cmake --build Pet/build --config Release -j 8`
- 部署：使用 `windeployqt6.exe` 收集运行所需 DLL 到可执行目录

1. 放置资源

- `cubism.zip` 解压到 `Pet/sdk/` 下，确保最终为 `Pet/sdk/cubism/`
- `models.zip`、`voice_deps.zip.*`、`llm.zip.*` 解压/放置到 `Pet/res/` 下（详见“资源与目录约定”）
- 若启用 Qwen3-TTS，本地 Python 环境建议位于 `Pet/.venv-qwen3-tts/`

1. 准备 Qwen3-TTS Python 环境（可选但推荐）

- 创建虚拟环境：`python -m venv .venv-qwen3-tts`
- 安装依赖：`.\.venv-qwen3-tts\Scripts\python.exe -m pip install qwen-tts soundfile`
- 如果 `python` 不是你想要的解释器，可通过环境变量 `XIAOMO_QWEN_TTS_PYTHON` 显式指定

## 常见问题

- 运行后无对话/回复很短
  - 确认 `res/bin` 中存在 `llama-cli.exe/llama.exe`，并在“AI 设置”中选择合适规模与风格
  - 放入匹配的 gguf 模型至 `res/llm/1.5B` 或 `res/llm/7B`；或设置环境变量 `LLM_MODEL` 指定路径
- 无法发声/离线 TTS 不可用
  - 确认 `res/voice_deps/qwen3-tts` 或 `Qwen3-TTS-*` 模型目录存在，且包含完整模型文件
  - 确认 `.venv-qwen3-tts` 已创建，并安装 `qwen-tts` 与 `soundfile`
  - Base 模型请确认 `res/voice_deps/qwen3-tts/prompt/default_reference.mp3` 存在
  - 如需强制指定解释器，可设置环境变量 `XIAOMO_QWEN_TTS_PYTHON`
- 唤醒词或语音识别没有反应
  - 确认“AI 设置”里已经开启 `KWS` 或 `STT`
  - 确认 `res/voice_deps/` 下的 `sherpa-onnx-*`、`silero-vad`、`sensevoice-small` 等语音依赖存在
  - 首次运行请确认系统已授予麦克风权限
  - `KWS + STT` 同时开启时，会先等待唤醒，再开始识别；若只开 `STT`，则会直接进入语音检测与识别
- TTS 可以生成和播放，但速度太慢
  - 当前版本已经改为常驻 `Qwen3-TTS` 后端，后续多次播报会比逐次重载模型更快
  - 首次播报仍可能受到模型冷启动影响，尤其是 `1.7B Base` 模型
  - 若追求更快速度，优先考虑使用 `0.6B` 版本、安装 `flash-attn`，或使用更强 GPU
- 找不到单输入框入口
  - 可在设置中的“对话交互模式”切换到单输入框
  - 也可在聊天窗口底部点击“切换到单输入框”，或在单输入框中切回“历史对话框”
- 语音识别结果没有自动进入输入框
  - 唤醒后程序会根据当前模式，把语音草稿同步到聊天窗口输入框或单输入框
  - 最终识别结果会直接触发一次本地提问；中途的 partial 文本仅用于草稿预览
- 合并/解压失败
  - 确认所有分卷已完整下载并位于同一目录；用 7-Zip 从 `.001` 开始解压

## 示例命令（速查）

- 解压 build-msvc 分卷

```cmd
7z x build-msvc.zip.001
```

- 解压可选资源（分卷从 .001 开始解压）

```cmd
7z x voice_deps.zip.001
7z x llm.zip.001
7z x models.zip
```

- 将资源解压到正确位置（示例：在仓库 Pet 目录）

```powershell
Expand-Archive -LiteralPath .\assets\cubism.zip -DestinationPath .\sdk -Force
Rename-Item -Path .\sdk\cubism* -NewName cubism -ErrorAction SilentlyContinue
Expand-Archive -LiteralPath .\assets\models.zip -DestinationPath .\res -Force
7z x .\assets\voice_deps.zip.001 -o.\res -y
New-Item -ItemType Directory -Force -Path .\res\llm | Out-Null
7z x .\assets\llm.zip.001 -o.\res\llm -y
```

- 创建并安装 Qwen3-TTS 虚拟环境

```powershell
python -m venv .\.venv-qwen3-tts
.\.venv-qwen3-tts\Scripts\python.exe -m pip install --upgrade pip
.\.venv-qwen3-tts\Scripts\python.exe -m pip install qwen-tts soundfile
```

- 指定 Qwen3-TTS Python 解释器

```powershell
$env:XIAOMO_QWEN_TTS_PYTHON = "E:\desktoppet\Pet\.venv-qwen3-tts\Scripts\python.exe"
```

- 直接测试本地 Qwen3-TTS 后端

```powershell
$env:KMP_DUPLICATE_LIB_OK = "TRUE"
.\.venv-qwen3-tts\Scripts\python.exe .\res\voice_deps\qwen3-tts\backend\qwen3_tts_backend.py `
  --model-dir .\res\voice_deps\qwen3-tts `
  --output-file .\build-msvc\qwen3_tts_test.wav `
  --text "你好，我是小墨。" `
  --language Chinese `
  --mode base `
  --ref-audio .\res\voice_deps\qwen3-tts\prompt\default_reference.mp3 `
  --x-vector-only `
  --max-new-tokens 256
```

- 指定 LLM 运行器与模型（环境变量覆盖）

```powershell
$env:LLAMA_RUNNER = "E:\XiaoMo\release\res\bin\llama-cli.exe"
$env:LLM_MODEL    = "E:\XiaoMo\release\res\llm\1.5B\your-model.gguf"
```

- llama-cli 快速自检（在 `release/` 目录执行）

```powershell
.\res\bin\llama-cli.exe -m .\res\llm\1.5B\your-model.gguf -p "你好" --simple-io -n 64
```

## 配置与日志路径

- 配置目录（Windows）：`%APPDATA%\IAIAYN\XiaoMo\Configs\config.json`
- 本地数据目录（Windows）：`%LOCALAPPDATA%\IAIAYN\XiaoMo`
- 启动日志：`%LOCALAPPDATA%\...\logs\startup.log`

## 许可证与致谢

- 本项目使用的第三方组件与模型版权归其各自所有者所有。Live2D 模型和语音模型请遵循其对应授权协议。

## 开发者信息与测试人员致谢

- 开发者：Mo
- 测试人员：guos7898-alt , xpresent-10

万分感谢所有测试人员的反馈与建议，他们帮助我全面优化了项目的核心性能与运行稳定性，打磨并提升了产品全链路的使用体验，更精准排查定位了多处潜在的程序缺陷与风险隐患，为项目的顺利落地与长期平稳运行筑牢了坚实根基。
