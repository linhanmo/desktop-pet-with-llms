#include "audio/OfflineVoiceService.hpp"

#include "common/SettingsManager.hpp"
#include "common/Utils.hpp"

#include <QProcess>
#include <QTimer>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <QMessageBox>
#include <QDesktopServices>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QStandardPaths>
#include <QUrl>
#include <QPushButton>
#include <QCoreApplication>
#include <QAudioSource>
#include <QAudioDevice>
#include <QAudioFormat>
#include <QMediaDevices>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QDateTime>
#include <memory>
#include <functional>
#include <vector>
#include <cmath>

#if defined(Q_OS_WIN32)
#include <windows.h>
#include <mmdeviceapi.h>
#include <audiopolicy.h>
#include <endpointvolume.h>
#endif

static bool g_speakerHintShown = false;
static bool g_ttsHintShown = false;

namespace {

struct DebugServerConfig {
    QString serverUrl{QStringLiteral("http://127.0.0.1:7777/event")};
    QString sessionId{QStringLiteral("audio-output-unavailable")};
};

static QString clampForLog(const QString& value, int maxLen = 400)
{
    QString normalized = value;
    normalized.replace(QLatin1Char('\r'), QLatin1Char('\n'));
    if (normalized.size() > maxLen)
        normalized = normalized.left(maxLen) + QStringLiteral("...(truncated)");
    return normalized;
}

static QString audioDeviceSummary(const QAudioDevice& device)
{
    if (device.isNull())
        return QStringLiteral("<null>");

    return QStringLiteral("%1 [%2]")
        .arg(device.description(), QString::fromLatin1(device.id().toHex()));
}

static QString firstExistingPath(const QStringList& candidates)
{
    for (const QString& candidate : candidates) {
        if (!candidate.isEmpty() && QFileInfo::exists(candidate))
            return QDir::cleanPath(candidate);
    }
    return QString();
}

static QString detectQwenTtsModelMode(const QString& modelDir)
{
    if (modelDir.isEmpty())
        return QStringLiteral("base");

    const QString configPath = QDir(modelDir).filePath(QStringLiteral("config.json"));
    QFile configFile(configPath);
    if (configFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const QJsonDocument doc = QJsonDocument::fromJson(configFile.readAll());
        const QString mode = doc.object().value(QStringLiteral("tts_model_type")).toString().trimmed().toLower();
        if (!mode.isEmpty())
            return mode;
    }

    const QString dirName = QFileInfo(modelDir).fileName().trimmed().toLower();
    if (dirName.contains(QStringLiteral("customvoice")) || dirName.contains(QStringLiteral("custom_voice")))
        return QStringLiteral("customvoice");
    if (dirName.contains(QStringLiteral("voicedesign")) || dirName.contains(QStringLiteral("voice_design")))
        return QStringLiteral("voicedesign");
    return QStringLiteral("base");
}

static QString qwenTtsBackendScriptPath(const QString& modelDir)
{
    const QString appDir = QCoreApplication::applicationDirPath();
    return firstExistingPath({
        QDir(modelDir).filePath(QStringLiteral("backend/qwen3_tts_backend.py")),
        appResourcePath(QStringLiteral("voice_deps/qwen3-tts/backend/qwen3_tts_backend.py")),
        QDir(appDir).filePath(QStringLiteral("../res/voice_deps/qwen3-tts/backend/qwen3_tts_backend.py")),
        QStringLiteral("E:/desktoppet/Pet/res/voice_deps/qwen3-tts/backend/qwen3_tts_backend.py")
    });
}

static QString qwenTtsDefaultPromptAudioPath(const QString& modelDir)
{
    return firstExistingPath({
        QDir(modelDir).filePath(QStringLiteral("prompt/default_reference.wav")),
        QDir(modelDir).filePath(QStringLiteral("prompt/default_reference.mp3")),
        QDir(modelDir).filePath(QStringLiteral("prompt/default.wav")),
        QDir(modelDir).filePath(QStringLiteral("prompt/default.mp3"))
    });
}

static QString qwenTtsDefaultPromptTextPath(const QString& modelDir)
{
    return firstExistingPath({
        QDir(modelDir).filePath(QStringLiteral("prompt/default_reference.txt")),
        QDir(modelDir).filePath(QStringLiteral("prompt/default.txt"))
    });
}

static QString qwenTtsPythonExecutable()
{
    const QString appDir = QCoreApplication::applicationDirPath();
    const QString envOverride = qEnvironmentVariable("XIAOMO_QWEN_TTS_PYTHON").trimmed();
    const QString pathPython = QStandardPaths::findExecutable(QStringLiteral("python"));
    const QString pathPython3 = QStandardPaths::findExecutable(QStringLiteral("python3"));

#if defined(Q_OS_WIN32)
    return firstExistingPath({
        envOverride,
        QDir(appDir).filePath(QStringLiteral("../.venv-qwen3-tts/Scripts/python.exe")),
        QDir(appDir).filePath(QStringLiteral("../../.venv-qwen3-tts/Scripts/python.exe")),
        QStringLiteral("E:/desktoppet/Pet/.venv-qwen3-tts/Scripts/python.exe"),
        pathPython,
        pathPython3
    });
#else
    return firstExistingPath({
        envOverride,
        QDir(appDir).filePath(QStringLiteral("../.venv-qwen3-tts/bin/python3")),
        QDir(appDir).filePath(QStringLiteral("../../.venv-qwen3-tts/bin/python3")),
        pathPython3,
        pathPython
    });
#endif
}

static QString qwenTtsModelPath(const QString& voiceDepsDir)
{
    const QStringList candidates = {
        QStringLiteral("Qwen3-TTS-12Hz-1.7B-CustomVoice"),
        QStringLiteral("Qwen3-TTS-12Hz-0.6B-CustomVoice"),
        QStringLiteral("Qwen3-TTS-12Hz-1.7B-VoiceDesign"),
        QStringLiteral("Qwen3-TTS-12Hz-1.7B-Base"),
        QStringLiteral("Qwen3-TTS-12Hz-0.6B-Base"),
        QStringLiteral("qwen3-tts")
    };

    for (const QString& name : candidates) {
        const QString candidate = QDir(voiceDepsDir).filePath(name);
        if (QFileInfo::exists(candidate) && QFileInfo(candidate).isDir())
            return candidate;
    }
    return QDir(voiceDepsDir).filePath(QStringLiteral("qwen3-tts"));
}

static QString qwenTtsBackendSignature(const QString& pythonExe,
                                       const QString& backendScript,
                                       const QString& modelDir,
                                       const QString& modelMode)
{
    return QStringLiteral("%1\n%2\n%3\n%4")
        .arg(pythonExe, backendScript, modelDir, modelMode);
}

static QString mediaStatusName(QMediaPlayer::MediaStatus status)
{
    switch (status) {
    case QMediaPlayer::NoMedia: return QStringLiteral("NoMedia");
    case QMediaPlayer::LoadingMedia: return QStringLiteral("LoadingMedia");
    case QMediaPlayer::LoadedMedia: return QStringLiteral("LoadedMedia");
    case QMediaPlayer::StalledMedia: return QStringLiteral("StalledMedia");
    case QMediaPlayer::BufferingMedia: return QStringLiteral("BufferingMedia");
    case QMediaPlayer::BufferedMedia: return QStringLiteral("BufferedMedia");
    case QMediaPlayer::EndOfMedia: return QStringLiteral("EndOfMedia");
    case QMediaPlayer::InvalidMedia: return QStringLiteral("InvalidMedia");
    }
    return QStringLiteral("UnknownMediaStatus");
}

static QString playbackStateName(QMediaPlayer::PlaybackState state)
{
    switch (state) {
    case QMediaPlayer::StoppedState: return QStringLiteral("StoppedState");
    case QMediaPlayer::PlayingState: return QStringLiteral("PlayingState");
    case QMediaPlayer::PausedState: return QStringLiteral("PausedState");
    }
    return QStringLiteral("UnknownPlaybackState");
}

static QString debugEnvFilePath()
{
    const QString relativePath = QStringLiteral(".dbg/audio-output-unavailable.env");
    const QString appDir = QCoreApplication::applicationDirPath();
    const QString currentDir = QDir::currentPath();
    const QStringList candidates = {
        QDir(currentDir).filePath(relativePath),
        QDir(appDir).filePath(relativePath),
        QDir(appDir).filePath(QStringLiteral("../") + relativePath),
        QDir(appDir).filePath(QStringLiteral("../../") + relativePath),
        QStringLiteral("E:/desktoppet/Pet/.dbg/audio-output-unavailable.env")
    };

    for (const QString& candidate : candidates) {
        if (QFileInfo::exists(candidate))
            return QDir::cleanPath(candidate);
    }

    return QString();
}

static DebugServerConfig loadDebugServerConfig()
{
    static const DebugServerConfig config = [] {
        DebugServerConfig loaded;
        const QString envPath = debugEnvFilePath();
        if (envPath.isEmpty())
            return loaded;

        QFile envFile(envPath);
        if (!envFile.open(QIODevice::ReadOnly | QIODevice::Text))
            return loaded;

        const QString content = QString::fromUtf8(envFile.readAll());
        const QStringList lines = content.split(QLatin1Char('\n'));
        for (const QString& rawLine : lines) {
            const QString line = rawLine.trimmed();
            if (line.startsWith(QStringLiteral("DEBUG_SERVER_URL="))) {
                loaded.serverUrl = line.section(QLatin1Char('='), 1);
            } else if (line.startsWith(QStringLiteral("DEBUG_SESSION_ID="))) {
                loaded.sessionId = line.section(QLatin1Char('='), 1);
            }
        }
        return loaded;
    }();

    return config;
}

static QNetworkAccessManager* debugNetworkManager()
{
    static QNetworkAccessManager* manager = new QNetworkAccessManager(QCoreApplication::instance());
    return manager;
}

static void reportDebugEvent(const char* hypothesisId,
                             const char* location,
                             const QString& message,
                             const QJsonObject& data = QJsonObject())
{
    const DebugServerConfig config = loadDebugServerConfig();

    QJsonObject payload{
        {QStringLiteral("sessionId"), config.sessionId},
        {QStringLiteral("runId"), QStringLiteral("pre-fix")},
        {QStringLiteral("hypothesisId"), QString::fromLatin1(hypothesisId)},
        {QStringLiteral("location"), QString::fromLatin1(location)},
        {QStringLiteral("msg"), message},
        {QStringLiteral("ts"), QString::number(QDateTime::currentMSecsSinceEpoch())}
    };
    if (!data.isEmpty())
        payload.insert(QStringLiteral("data"), data);

    QNetworkRequest request(QUrl(config.serverUrl));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    debugNetworkManager()->post(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));
}

} // namespace

static void showSpeakerHint(const QString& detail)
{
    if (g_speakerHintShown) return;
    g_speakerHintShown = true;
#if defined(Q_OS_MACOS)
    QString tip = QObject::tr("无法播放音频输出。\n\n请在“系统设置→声音”中检查输出设备与音量。");
#elif defined(Q_OS_WIN32)
    QString tip = QObject::tr("无法播放音频输出。\n\n请在“设置→系统→声音”中检查输出设备与音量。");
#else
    QString tip = QObject::tr("无法播放音频输出。\n\n请检查桌面环境的音频输出设备设置，并确保音量正常。");
#endif
    const QString d = detail.trimmed();
    if (!d.isEmpty())
        tip += QStringLiteral("\n\n") + (d.size() > 1200 ? d.right(1200) : d);

    QMessageBox box(QMessageBox::Warning, QObject::tr("音频输出不可用"), tip, QMessageBox::NoButton, nullptr);
    box.setWindowModality(Qt::ApplicationModal);
    box.setWindowFlag(Qt::WindowStaysOnTopHint, true);
#if defined(Q_OS_WIN32)
    QAbstractButton* openBtn = box.addButton(QObject::tr("打开设置"), QMessageBox::AcceptRole);
    box.addButton(QObject::tr("关闭"), QMessageBox::RejectRole);
    box.exec();
    if (box.clickedButton() == openBtn)
        QDesktopServices::openUrl(QUrl(QStringLiteral("ms-settings:sound")));
#else
    box.addButton(QObject::tr("关闭"), QMessageBox::RejectRole);
    box.exec();
#endif
}

static QString combineProcessOutput(QProcess* process)
{
    if (!process)
        return QString();

    const QString stdoutText = QString::fromLocal8Bit(process->readAllStandardOutput());
    const QString stderrText = QString::fromLocal8Bit(process->readAllStandardError());
    if (stdoutText.isEmpty())
        return stderrText;
    if (stderrText.isEmpty())
        return stdoutText;
    return stdoutText + QStringLiteral("\n") + stderrText;
}

static QString explainTtsFailure(const QString& detail)
{
    const QString normalized = detail.trimmed();
    if (normalized.contains(QStringLiteral("Invalid option --encoder-model"), Qt::CaseInsensitive)
        || normalized.contains(QStringLiteral("Invalid option --decoder-model"), Qt::CaseInsensitive)
        || normalized.contains(QStringLiteral("Invalid option --speech-tokenizer-dir"), Qt::CaseInsensitive))
    {
        return QObject::tr(
            "当前内置的 sherpa-onnx TTS 可执行程序不支持 `qwen3-tts` 这套参数，"
            "语音合成进程在生成音频前就已经退出。\n\n"
            "这不是系统输出设备不可用，而是 TTS 运行时与模型资源不兼容。"
            "\n\n原始错误：\n%1").arg(clampForLog(normalized, 1600));
    }

    if (normalized.contains(QStringLiteral("No module named 'qwen_tts'"), Qt::CaseInsensitive)
        || normalized.contains(QStringLiteral("No module named \"qwen_tts\""), Qt::CaseInsensitive))
    {
        return QObject::tr(
            "本地 Qwen3-TTS Python 环境缺少 `qwen-tts` 依赖，无法启动语音合成后端。"
            "\n\n请检查 `.venv-qwen3-tts` 是否创建完成，或重新安装运行时依赖。"
            "\n\n原始错误：\n%1").arg(clampForLog(normalized, 1600));
    }

    if (normalized.contains(QStringLiteral("requires --ref-audio"), Qt::CaseInsensitive)
        || normalized.contains(QStringLiteral("default prompt audio"), Qt::CaseInsensitive))
    {
        return QObject::tr(
            "当前加载的是 Qwen3-TTS Base 模型，它需要参考音频。"
            "\n\n请确保 `res/voice_deps/qwen3-tts/prompt/default_reference.mp3` 存在，"
            "或改为提供 `CustomVoice/VoiceDesign` 模型目录。"
            "\n\n原始错误：\n%1").arg(clampForLog(normalized, 1600));
    }

    if (normalized.isEmpty()) {
        return QObject::tr(
            "离线语音合成进程在生成音频前退出，未产生可播放的 wav 文件。"
            "\n\n这不是系统输出设备不可用，更可能是 TTS 运行时或模型资源异常。");
    }

    return normalized;
}

static void showTtsHint(const QString& detail)
{
    if (g_ttsHintShown) return;
    g_ttsHintShown = true;

    QString tip = QObject::tr(
        "离线语音合成不可用。\n\n当前问题发生在 TTS 引擎/模型阶段，不是系统音频输出设备。");
    const QString d = explainTtsFailure(detail);
    if (!d.isEmpty())
        tip += QStringLiteral("\n\n") + d;

    QMessageBox box(QMessageBox::Warning, QObject::tr("离线语音合成不可用"), tip, QMessageBox::NoButton, nullptr);
    box.setWindowModality(Qt::ApplicationModal);
    box.setWindowFlag(Qt::WindowStaysOnTopHint, true);
    box.addButton(QObject::tr("关闭"), QMessageBox::RejectRole);
    box.exec();
}

static bool g_micHintShown = false;

static void showMicHint(const QString& detail)
{
    if (g_micHintShown) return;
    g_micHintShown = true;
#if defined(Q_OS_MACOS)
    QString tip = QObject::tr("无法访问麦克风。\n\n请在“系统设置→隐私与安全性→麦克风”中允许本应用访问麦克风。");
#elif defined(Q_OS_WIN32)
    QString tip = QObject::tr("无法访问麦克风。\n\n请在“设置→隐私和安全性→麦克风”中允许本应用访问麦克风。");
#else
    QString tip = QObject::tr("无法访问麦克风。\n\n请检查系统麦克风权限设置。");
#endif
    const QString d = detail.trimmed();
    if (!d.isEmpty())
        tip += QStringLiteral("\n\n") + (d.size() > 1200 ? d.right(1200) : d);

    QMessageBox box(QMessageBox::Warning, QObject::tr("麦克风不可用"), tip, QMessageBox::NoButton, nullptr);
    box.setWindowModality(Qt::ApplicationModal);
    box.setWindowFlag(Qt::WindowStaysOnTopHint, true);
#if defined(Q_OS_WIN32)
    QAbstractButton* openBtn = box.addButton(QObject::tr("打开设置"), QMessageBox::AcceptRole);
    box.addButton(QObject::tr("关闭"), QMessageBox::RejectRole);
    box.exec();
    if (box.clickedButton() == openBtn)
        QDesktopServices::openUrl(QUrl(QStringLiteral("ms-settings:privacy-microphone")));
#else
    box.addButton(QObject::tr("关闭"), QMessageBox::RejectRole);
    box.exec();
#endif
}

OfflineVoiceService::OfflineVoiceService(QObject* parent)
    : QObject(parent)
{
    reloadFromSettings();
}

OfflineVoiceService::~OfflineVoiceService()
{
    stop();
    if (m_ttsPlayer) delete m_ttsPlayer;
    if (m_ttsAudio) delete m_ttsAudio;
    if (m_audioSource) delete m_audioSource;
}

void OfflineVoiceService::reloadFromSettings()
{
    applySettingsSnapshot(readSettings());
}

void OfflineVoiceService::start()
{
    syncCaptureState();
    warmTtsBackendIfNeeded();
}

void OfflineVoiceService::stop()
{
    stopMic();
    stopTts();
    stopTtsBackend();
    if (m_sttProcess) {
        m_sttProcess->kill();
        m_sttProcess->deleteLater();
        m_sttProcess = nullptr;
    }
    m_mode = Mode::Idle;
}

void OfflineVoiceService::speakText(const QString& text)
{
    if (!m_settings.ttsEnabled) return;
    startTts(text);
}

void OfflineVoiceService::startListening()
{
    if (!shouldCaptureMic())
        return;
    resetListeningState();
    syncCaptureState();
}

void OfflineVoiceService::stopListening()
{
    if (m_mode == Mode::Listening || m_mode == Mode::Recording) {
        m_mode = Mode::Idle;
        m_kwsActive = false;
        m_vadDetected = false;
        m_vadBuffer.clear();
        m_vadSilenceFrames = 0;
        stopMic();
    }
}

OfflineVoiceService::SettingsSnapshot OfflineVoiceService::readSettings() const
{
    SettingsSnapshot s;
    const auto& sm = SettingsManager::instance();
    
    s.ttsEnabled = sm.offlineTtsEnabled();
    s.kwsEnabled = sm.kwsEnabled();
    s.sttEnabled = sm.sttEnabled();
    s.binDir = sm.sherpaOnnxBinDir();
    s.ttsArgs = sm.sherpaTtsArgs();
    s.ttsVolumePercent = sm.ttsVolumePercent();
    
    QString voiceDepsDir = QCoreApplication::applicationDirPath() + QStringLiteral("/../res/voice_deps");
    QDir appDir(QCoreApplication::applicationDirPath());
    if (!QDir(voiceDepsDir).exists()) {
        voiceDepsDir = appDir.filePath(QStringLiteral("res/voice_deps"));
    }
    if (!QDir(voiceDepsDir).exists()) {
        voiceDepsDir = QStringLiteral("res/voice_deps");
    }
    
    QString sherpaDir = QDir(voiceDepsDir).filePath(QStringLiteral("sherpa-onnx-v1.12.10-win-x64-shared"));
    s.kwsModelPath = sherpaDir + QStringLiteral("/bin");
    s.vadModelPath = QDir(voiceDepsDir).filePath(QStringLiteral("silero-vad/src/silero_vad/data/silero_vad.onnx"));
    s.sttModelPath = QDir(voiceDepsDir).filePath(QStringLiteral("sensevoice-small/model.pt"));
    s.ttsModelPath = qwenTtsModelPath(voiceDepsDir);
    s.ttsArgs = QStringLiteral("qwen3-local-backend:%1").arg(s.ttsModelPath);
    
    return s;
}

void OfflineVoiceService::applySettingsSnapshot(const SettingsSnapshot& next)
{
    const bool audioPipelineChanged =
        m_settings.kwsEnabled != next.kwsEnabled ||
        m_settings.sttEnabled != next.sttEnabled ||
        m_settings.kwsModelPath != next.kwsModelPath ||
        m_settings.vadModelPath != next.vadModelPath ||
        m_settings.sttModelPath != next.sttModelPath;
    if (m_settings.ttsArgs != next.ttsArgs || 
        m_settings.ttsVolumePercent != next.ttsVolumePercent ||
        m_settings.binDir != next.binDir ||
        m_settings.ttsModelPath != next.ttsModelPath) {
        stopTts();
        stopTtsBackend();
    }
    
    m_settings = next;
    if (audioPipelineChanged)
        resetListeningState();
    syncCaptureState();
    warmTtsBackendIfNeeded();
}

bool OfflineVoiceService::shouldCaptureMic() const
{
    return m_settings.kwsEnabled || m_settings.sttEnabled;
}

void OfflineVoiceService::resetListeningState()
{
    m_vadBuffer.clear();
    m_vadSilenceFrames = 0;
    if (!m_sttPartialResult.isEmpty()) {
        m_sttPartialResult.clear();
        emit sttPartialResult(QString());
    }

    if (m_settings.kwsEnabled) {
        m_mode = Mode::Listening;
        m_kwsActive = true;
        m_vadDetected = false;
        return;
    }

    if (m_settings.sttEnabled) {
        m_mode = Mode::Recording;
        m_kwsActive = false;
        m_vadDetected = true;
        return;
    }

    m_mode = Mode::Idle;
    m_kwsActive = false;
    m_vadDetected = false;
}

void OfflineVoiceService::syncCaptureState()
{
    if (!shouldCaptureMic()) {
        stopMic();
        if (m_sttProcess) {
            m_sttProcess->kill();
            m_sttProcess->deleteLater();
            m_sttProcess = nullptr;
        }
        resetListeningState();
        return;
    }

    initAudioDevices();
    startMicIfNeeded();
}

void OfflineVoiceService::pauseCaptureForTts()
{
    if (!shouldCaptureMic()) {
        m_resumeCaptureAfterTts = false;
        return;
    }

    if (m_sttProcess) {
        m_sttProcess->kill();
        m_sttProcess->deleteLater();
        m_sttProcess = nullptr;
    }

    stopMic();
    m_resumeCaptureAfterTts = true;
}

void OfflineVoiceService::resumeCaptureAfterTtsIfNeeded()
{
    if (!m_resumeCaptureAfterTts)
        return;

    m_resumeCaptureAfterTts = false;
    syncCaptureState();
}

QString OfflineVoiceService::exePath(const QString& baseName) const
{
    QString path = m_settings.binDir + QStringLiteral("/") + baseName;
#if defined(Q_OS_WIN32)
    if (!path.endsWith(QStringLiteral(".exe")))
        path += QStringLiteral(".exe");
#endif
    if (QFileInfo::exists(path))
        return path;
    
    path = QCoreApplication::applicationDirPath() + QStringLiteral("/") + baseName;
#if defined(Q_OS_WIN32)
    if (!path.endsWith(QStringLiteral(".exe")))
        path += QStringLiteral(".exe");
#endif
    return QFileInfo::exists(path) ? path : QString();
}

QStringList OfflineVoiceService::splitArgs(const QString& args) const
{
    QStringList result;
    QString current;
    bool inQuote = false;
    
    for (int i = 0; i < args.size(); ++i) {
        const QChar c = args[i];
        if (c == QLatin1Char('"')) {
            inQuote = !inQuote;
        } else if (c.isSpace() && !inQuote) {
            if (!current.isEmpty()) {
                result.append(current);
                current.clear();
            }
        } else {
            current.append(c);
        }
    }
    if (!current.isEmpty()) {
        result.append(current);
    }
    return result;
}

void OfflineVoiceService::initAudioDevices()
{
    if (!m_mediaDevices) {
        m_mediaDevices = new QMediaDevices(this);
    }

    if (m_mediaDeviceSignalsConnected)
        return;

    QObject::connect(m_mediaDevices, &QMediaDevices::audioInputsChanged,
                     this, [this]() {
        if (m_mode == Mode::Listening || m_mode == Mode::Recording) {
            stopMic();
            startMicIfNeeded();
        }
    });

    QObject::connect(m_mediaDevices, &QMediaDevices::audioOutputsChanged,
                     this, [this]() {
        if (m_ttsAudio) {
            delete m_ttsAudio;
            m_ttsAudio = nullptr;
        }
    });

    m_mediaDeviceSignalsConnected = true;
}

void OfflineVoiceService::startMicIfNeeded()
{
    if (m_audioSource) {
        return;
    }

    QAudioDevice inputDevice = QMediaDevices::defaultAudioInput();
    if (inputDevice.isNull()) {
        auto availableDevices = QMediaDevices::audioInputs();
        if (!availableDevices.isEmpty()) {
            for (const QAudioDevice &dev : availableDevices) {
                if (!dev.isNull()) {
                    inputDevice = dev;
                    break;
                }
            }
        }
    }

    if (inputDevice.isNull()) {
        showMicHint(QStringLiteral("No available audio input device found."));
        emit audioError(QStringLiteral("No available audio input device found."));
        return;
    }

    QAudioFormat fmt;
    fmt.setSampleRate(16000);
    fmt.setChannelCount(1);
    fmt.setSampleFormat(QAudioFormat::Int16);

    if (!inputDevice.isFormatSupported(fmt)) {
        fmt = inputDevice.preferredFormat();
    }

    if (!fmt.isValid()) {
        showMicHint(QStringLiteral("Audio format not supported by device."));
        emit audioError(QStringLiteral("Audio format not supported by device."));
        return;
    }

    m_inputFormat = fmt;
    m_resampleStep = double(fmt.sampleRate()) / 16000.0;
    m_resamplePos = 0.0;
    m_audioBuffer.clear();
    m_resampledBuffer.clear();

    m_audioSource = new QAudioSource(inputDevice, fmt, this);
    m_audioDevice = m_audioSource->start();
    
    if (!m_audioDevice) {
        showMicHint(QStringLiteral("Failed to start audio input."));
        emit audioError(QStringLiteral("Failed to start audio input."));
        delete m_audioSource;
        m_audioSource = nullptr;
        return;
    }

    QObject::connect(m_audioDevice, &QIODevice::readyRead, this, [this]() {
        const QByteArray data = m_audioDevice->readAll();
        if (!data.isEmpty()) {
            processAudioBuffer(data);
        }
    });

    QObject::connect(m_audioSource, &QAudioSource::stateChanged, this, [this](QAudio::State state) {
        if (state == QAudio::StoppedState && m_audioSource && m_audioSource->error() != QAudio::NoError) {
            const QString detail = QStringLiteral("Audio input stopped with error code %1.")
                                       .arg(int(m_audioSource->error()));
            showMicHint(detail);
            emit audioError(detail);
        }
    });
}

bool OfflineVoiceService::ensureTtsAudioOutput(int ttsVolumePercent, QString* errorDetail)
{
    const QAudioDevice defaultOutput = QMediaDevices::defaultAudioOutput();
    const auto outputs = QMediaDevices::audioOutputs();
    QJsonArray availableOutputs;
    for (const QAudioDevice& dev : outputs) {
        availableOutputs.append(audioDeviceSummary(dev));
    }

    if (m_ttsAudio) {
        m_ttsAudio->setVolume(qBound(0.0, double(ttsVolumePercent) / 100.0, 1.0));
        // #region debug-point D:reuse-existing-output
        reportDebugEvent("D",
                         "OfflineVoiceService::ensureTtsAudioOutput/reuse",
                         QStringLiteral("[DEBUG] Reusing existing QAudioOutput"),
                         QJsonObject{
                             {QStringLiteral("defaultOutput"), audioDeviceSummary(defaultOutput)},
                             {QStringLiteral("availableOutputs"), availableOutputs},
                             {QStringLiteral("currentVolume"), m_ttsAudio->volume()}
                         });
        // #endregion
        return true;
    }

    QAudioDevice outputDevice = defaultOutput;
    if (outputDevice.isNull()) {
        for (const QAudioDevice& dev : outputs) {
            if (!dev.isNull()) {
                outputDevice = dev;
                break;
            }
        }
    }

    if (outputDevice.isNull()) {
        // #region debug-point D:no-output-device
        reportDebugEvent("D",
                         "OfflineVoiceService::ensureTtsAudioOutput/no-device",
                         QStringLiteral("[DEBUG] Failed to select any audio output device"),
                         QJsonObject{
                             {QStringLiteral("defaultOutput"), audioDeviceSummary(defaultOutput)},
                             {QStringLiteral("availableOutputs"), availableOutputs},
                             {QStringLiteral("requestedVolumePercent"), ttsVolumePercent}
                         });
        // #endregion
        if (errorDetail)
            *errorDetail = QStringLiteral("No available audio output device found.");
        return false;
    }

    m_ttsAudio = new QAudioOutput(outputDevice, this);
    m_ttsAudio->setVolume(qBound(0.0, double(ttsVolumePercent) / 100.0, 1.0));
    // #region debug-point D:create-output
    reportDebugEvent("D",
                     "OfflineVoiceService::ensureTtsAudioOutput/create",
                     QStringLiteral("[DEBUG] Created QAudioOutput for TTS"),
                     QJsonObject{
                         {QStringLiteral("defaultOutput"), audioDeviceSummary(defaultOutput)},
                         {QStringLiteral("selectedOutput"), audioDeviceSummary(outputDevice)},
                         {QStringLiteral("availableOutputs"), availableOutputs},
                         {QStringLiteral("requestedVolumePercent"), ttsVolumePercent},
                         {QStringLiteral("actualVolume"), m_ttsAudio->volume()}
                     });
    // #endregion
    return true;
}

void OfflineVoiceService::stopMic()
{
    if (m_audioSource) {
        m_audioSource->stop();
        delete m_audioSource;
        m_audioSource = nullptr;
        m_audioDevice = nullptr;
    }
    m_audioBuffer.clear();
    m_resampledBuffer.clear();
}

void OfflineVoiceService::processAudioBuffer(const QByteArray& data)
{
    const int channels = qMax(1, m_inputFormat.channelCount());
    const int bytesPerSample = m_inputFormat.bytesPerSample();
    const int bytesPerFrame = bytesPerSample * channels;
    
    if (bytesPerFrame <= 0 || data.size() < bytesPerFrame)
        return;

    const int frames = data.size() / bytesPerFrame;
    m_audioBuffer.reserve(m_audioBuffer.size() + frames);

    const char* raw = data.constData();
    for (int i = 0; i < frames; ++i) {
        double acc = 0.0;
        for (int ch = 0; ch < channels; ++ch) {
            const char* s = raw + (i * bytesPerFrame + ch * bytesPerSample);
            float v = 0.0f;
            if (m_inputFormat.sampleFormat() == QAudioFormat::Int16) {
                int16_t x = 0;
                memcpy(&x, s, sizeof(int16_t));
                v = float(x) / 32768.0f;
            } else if (m_inputFormat.sampleFormat() == QAudioFormat::Int32) {
                int32_t x = 0;
                memcpy(&x, s, sizeof(int32_t));
                v = float(double(x) / 2147483648.0);
            } else if (m_inputFormat.sampleFormat() == QAudioFormat::Float) {
                float x = 0.0f;
                memcpy(&x, s, sizeof(float));
                v = x;
            }
            acc += v;
        }
        m_audioBuffer.push_back(float(acc / double(channels)));
    }

    while (m_resamplePos + 1.0 < double(m_audioBuffer.size())) {
        const int i = int(m_resamplePos);
        const double frac = m_resamplePos - double(i);
        const float a = m_audioBuffer[size_t(i)];
        const float b = m_audioBuffer[size_t(i + 1)];
        const float y = float((1.0 - frac) * double(a) + frac * double(b));
        m_resampledBuffer.push_back(y);
        m_resamplePos += m_resampleStep;
    }

    const int drop = qMax(0, int(m_resamplePos) - 1);
    if (drop > 0) {
        m_audioBuffer.erase(m_audioBuffer.begin(), m_audioBuffer.begin() + drop);
        m_resamplePos -= double(drop);
    }

    if (m_resampledBuffer.size() >= 512) {
        size_t n = m_resampledBuffer.size() - (m_resampledBuffer.size() % 256);
        if (m_kwsActive) {
            processKWS(m_resampledBuffer.data(), n);
        }
        if (m_vadDetected || (m_mode == Mode::Recording)) {
            processVAD(m_resampledBuffer.data(), n);
        }
        m_resampledBuffer.erase(m_resampledBuffer.begin(), m_resampledBuffer.begin() + n);
    }
}

void OfflineVoiceService::processKWS(const float* samples, size_t count)
{
    if (!m_settings.kwsEnabled)
        return;

    auto handleWakeWordDetected = [this]() {
        emit wakeWordDetected();
        if (!m_settings.sttEnabled) {
            m_mode = Mode::Listening;
            m_kwsActive = true;
            m_vadDetected = false;
            return;
        }
        m_kwsActive = false;
        m_mode = Mode::Recording;
        m_vadDetected = true;
        m_vadBuffer.clear();
        m_vadSilenceFrames = 0;
    };

    if (!QDir(m_settings.kwsModelPath).exists()) {
        static bool warned = false;
        if (!warned) {
            qWarning() << "KWS sherpa-onnx directory not found:" << m_settings.kwsModelPath;
            warned = true;
        }
        
        float energy = 0.0f;
        for (size_t i = 0; i < count; ++i) {
            energy += samples[i] * samples[i];
        }
        energy = sqrt(energy / float(count));
        
        if (energy > 0.05f) {
            handleWakeWordDetected();
            qDebug() << "Wake word detected via energy threshold";
        }
        return;
    }
    
    QString kwsExe = m_settings.kwsModelPath + QStringLiteral("/sherpa-onnx-keyword-spotter-microphone");
#if defined(Q_OS_WIN32)
    if (!kwsExe.endsWith(QStringLiteral(".exe")))
        kwsExe += QStringLiteral(".exe");
#endif
    
    if (!QFileInfo::exists(kwsExe)) {
        static bool warned = false;
        if (!warned) {
            qWarning() << "KWS executable not found:" << kwsExe;
            warned = true;
        }
        
        float energy = 0.0f;
        for (size_t i = 0; i < count; ++i) {
            energy += samples[i] * samples[i];
        }
        energy = sqrt(energy / float(count));
        
        if (energy > 0.05f) {
            handleWakeWordDetected();
            qDebug() << "Wake word detected via energy threshold";
        }
        return;
    }
    
    float energy = 0.0f;
    for (size_t i = 0; i < count; ++i) {
        energy += samples[i] * samples[i];
    }
    energy = sqrt(energy / float(count));
    
    if (energy > 0.05f) {
        handleWakeWordDetected();
        qDebug() << "Wake word detected, starting recording";
    }
}

void OfflineVoiceService::processVAD(const float* samples, size_t count)
{
    if (!QFileInfo::exists(m_settings.vadModelPath)) {
        static bool warned = false;
        if (!warned) {
            qWarning() << "VAD model not found:" << m_settings.vadModelPath;
            warned = true;
        }
        
        float energy = 0.0f;
        for (size_t i = 0; i < count; ++i) {
            energy += samples[i] * samples[i];
        }
        energy = sqrt(energy / float(count));
        
        if (energy < 0.01f) {
            m_vadSilenceFrames++;
            if (m_vadSilenceFrames > m_vadSilenceThreshold) {
                if (m_vadBuffer.size() > 16000) {
                    processSTT(m_vadBuffer.data(), m_vadBuffer.size());
                }
                resetListeningState();
            }
        } else {
            m_vadSilenceFrames = 0;
        }
        m_vadBuffer.insert(m_vadBuffer.end(), samples, samples + count);
        return;
    }
    
    float energy = 0.0f;
    for (size_t i = 0; i < count; ++i) {
        energy += samples[i] * samples[i];
    }
    energy = sqrt(energy / float(count));
    
    if (energy < 0.01f) {
        m_vadSilenceFrames++;
        if (m_vadSilenceFrames > m_vadSilenceThreshold) {
            if (m_vadBuffer.size() > 16000) {
                processSTT(m_vadBuffer.data(), m_vadBuffer.size());
            }
            resetListeningState();
            qDebug() << "VAD silence detected, stopping recording";
        }
    } else {
        m_vadSilenceFrames = 0;
    }
    
    m_vadBuffer.insert(m_vadBuffer.end(), samples, samples + count);
}

void OfflineVoiceService::processSTT(const float* samples, size_t count)
{
    if (!m_settings.sttEnabled || !QFileInfo::exists(m_settings.sttModelPath)) {
        return;
    }

    if (!m_sttPartialResult.isEmpty()) {
        m_sttPartialResult.clear();
        emit sttPartialResult(QString());
    }
    
    QString wavPath = SettingsManager::instance().cacheDir() + QStringLiteral("/stt_temp.wav");
    
    QFile wavFile(wavPath);
    if (wavFile.open(QIODevice::WriteOnly)) {
        QDataStream out(&wavFile);
        out.setByteOrder(QDataStream::LittleEndian);
        
        out.writeRawData("RIFF", 4);
        quint32 fileSize = 36 + quint32(count * 2);
        out << fileSize;
        out.writeRawData("WAVE", 4);
        out.writeRawData("fmt ", 4);
        out << quint32(16);
        out << quint16(1);
        out << quint16(1);
        out << quint32(16000);
        out << quint32(32000);
        out << quint16(2);
        out << quint16(16);
        out.writeRawData("data", 4);
        out << quint32(count * 2);
        
        for (size_t i = 0; i < count; ++i) {
            int16_t sample = int16_t(samples[i] * 32767.0f);
            out << sample;
        }
        
        wavFile.close();
    }
    
    if (m_sttProcess) {
        m_sttProcess->kill();
        m_sttProcess->deleteLater();
        m_sttProcess = nullptr;
    }
    
    QString sttExe = m_settings.kwsModelPath + QStringLiteral("/sherpa-onnx-offline");
#if defined(Q_OS_WIN32)
    if (!sttExe.endsWith(QStringLiteral(".exe")))
        sttExe += QStringLiteral(".exe");
#endif
    
    if (!QFileInfo::exists(sttExe)) {
        sttExe = exePath(QStringLiteral("sherpa-onnx-offline"));
    }
    
    if (!QFileInfo::exists(sttExe)) {
        qWarning() << "STT executable not found";
        QFile::remove(wavPath);
        return;
    }
    
    m_sttProcess = new QProcess(this);
    QString args = QStringLiteral("--nn-model=\"%1\" --wav-filename=\"%2\"").arg(
        m_settings.sttModelPath, wavPath);
    m_sttProcess->setProgram(sttExe);
    m_sttProcess->setArguments(splitArgs(args));

    connect(m_sttProcess, &QProcess::readyReadStandardOutput, this, [this]{
        if (!m_sttProcess)
            return;
        const QString chunk = QString::fromLocal8Bit(m_sttProcess->readAllStandardOutput());
        if (chunk.isEmpty())
            return;

        m_sttPartialResult += chunk;
        QString preview = m_sttPartialResult;
        preview.replace(QLatin1Char('\r'), QLatin1Char('\n'));

        QString partial;
        const QStringList lines = preview.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
        if (!lines.isEmpty())
            partial = lines.back().trimmed();
        else
            partial = preview.trimmed();

        if (!partial.isEmpty())
            emit sttPartialResult(partial);
    });
    
    connect(m_sttProcess, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), 
            this, [this, wavPath](int exitCode, QProcess::ExitStatus) {
        QString result = m_sttPartialResult;
        if (m_sttProcess)
            result += QString::fromLocal8Bit(m_sttProcess->readAllStandardOutput());
        if (exitCode == 0) {
            if (!result.isEmpty()) {
                emit sttPartialResult(QString());
                emit sttResult(result.trimmed());
            }
        } else {
            QString error = QString::fromLocal8Bit(m_sttProcess->readAllStandardError());
            qWarning() << "STT error:" << error;
        }
        m_sttPartialResult.clear();
        m_sttProcess->deleteLater();
        m_sttProcess = nullptr;
        QFile::remove(wavPath);
    });
    
    m_sttProcess->start();
}

bool OfflineVoiceService::ensureTtsBackendProcess(const QString& pythonExe,
                                                  const QString& backendScript,
                                                  const QString& modelDir,
                                                  const QString& modelMode,
                                                  QString* errorDetail)
{
    const QString signature = qwenTtsBackendSignature(pythonExe, backendScript, modelDir, modelMode);
    if (m_tts && m_tts->state() != QProcess::NotRunning && m_ttsBackendSignature == signature)
        return true;

    stopTtsBackend();

    m_tts = new QProcess(this);
    m_ttsBackendSignature = signature;
    m_ttsBackendStdoutBuffer.clear();
    m_tts->setProgram(pythonExe);
    m_tts->setArguments(QStringList{
        backendScript,
        QStringLiteral("--model-dir"), modelDir,
        QStringLiteral("--language"), QStringLiteral("Chinese"),
        QStringLiteral("--mode"), modelMode,
        QStringLiteral("--max-new-tokens"), QStringLiteral("512"),
        QStringLiteral("--serve-stdio")
    });

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("KMP_DUPLICATE_LIB_OK"), QStringLiteral("TRUE"));
    env.insert(QStringLiteral("PYTHONUTF8"), QStringLiteral("1"));
    m_tts->setProcessEnvironment(env);

    connect(m_tts, &QProcess::readyReadStandardOutput,
            this, &OfflineVoiceService::handleTtsBackendStdout);
    connect(m_tts, &QProcess::errorOccurred, this, [this](QProcess::ProcessError) {
        if (!m_tts)
            return;

        const QString detail = m_tts->errorString() + QStringLiteral("\n") + combineProcessOutput(m_tts);
        reportDebugEvent("A",
                         "OfflineVoiceService::ensureTtsBackendProcess/error",
                         QStringLiteral("[DEBUG] Qwen3-TTS backend process error"),
                         QJsonObject{
                             {QStringLiteral("errorString"), m_tts->errorString()},
                             {QStringLiteral("detail"), clampForLog(detail)},
                             {QStringLiteral("requestActive"), m_ttsRequestActive}
                         });

        if (m_ttsRequestActive) {
            m_ttsRequestActive = false;
            showTtsHint(detail);
            resumeCaptureAfterTtsIfNeeded();
        }
    });
    connect(m_tts, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this, [this](int exitCode, QProcess::ExitStatus) {
        if (!m_tts)
            return;

        const QString detail = combineProcessOutput(m_tts);
        const bool hadActiveRequest = m_ttsRequestActive;
        reportDebugEvent(exitCode == 0 ? "B" : "A",
                         "OfflineVoiceService::ensureTtsBackendProcess/finished",
                         QStringLiteral("[DEBUG] Qwen3-TTS backend process finished"),
                         QJsonObject{
                             {QStringLiteral("exitCode"), exitCode},
                             {QStringLiteral("detail"), clampForLog(detail)},
                             {QStringLiteral("requestActive"), hadActiveRequest}
                         });

        m_ttsRequestActive = false;
        m_tts->deleteLater();
        m_tts = nullptr;
        m_ttsBackendSignature.clear();
        m_ttsBackendStdoutBuffer.clear();

        if (exitCode != 0 && hadActiveRequest) {
            showTtsHint(detail);
            resumeCaptureAfterTtsIfNeeded();
        }
    });

    m_tts->start();
    if (!m_tts->waitForStarted(2000)) {
        const QString detail = m_tts->errorString() + QStringLiteral("\n") + combineProcessOutput(m_tts);
        if (errorDetail)
            *errorDetail = detail.trimmed();
        stopTtsBackend();
        return false;
    }

    return true;
}

void OfflineVoiceService::handleTtsBackendStdout()
{
    if (!m_tts)
        return;

    m_ttsBackendStdoutBuffer += m_tts->readAllStandardOutput();
    while (true) {
        const int newlineIndex = m_ttsBackendStdoutBuffer.indexOf('\n');
        if (newlineIndex < 0)
            break;

        const QByteArray rawLine = m_ttsBackendStdoutBuffer.left(newlineIndex).trimmed();
        m_ttsBackendStdoutBuffer.remove(0, newlineIndex + 1);
        if (rawLine.isEmpty())
            continue;

        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(rawLine, &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            reportDebugEvent("A",
                             "OfflineVoiceService::handleTtsBackendStdout/parse",
                             QStringLiteral("[DEBUG] Ignored non-JSON backend output"),
                             QJsonObject{
                                 {QStringLiteral("line"), clampForLog(QString::fromUtf8(rawLine))}
                             });
            continue;
        }

        const QJsonObject obj = doc.object();
        if (obj.value(QStringLiteral("event")).toString() == QStringLiteral("ready")) {
            reportDebugEvent("B",
                             "OfflineVoiceService::handleTtsBackendStdout/ready",
                             QStringLiteral("[DEBUG] Qwen3-TTS backend ready"),
                             obj);
            continue;
        }

        const qint64 responseId = obj.value(QStringLiteral("id")).toVariant().toLongLong();
        const bool ok = obj.value(QStringLiteral("ok")).toBool();
        const QString outputFile = obj.value(QStringLiteral("output_file")).toString();
        if (responseId != m_activeTtsRequestId) {
            if (ok && !outputFile.isEmpty())
                QFile::remove(outputFile);
            continue;
        }

        m_ttsRequestActive = false;
        if (!ok) {
            const QString detail = obj.value(QStringLiteral("error")).toString().trimmed()
                + QStringLiteral("\n")
                + obj.value(QStringLiteral("traceback")).toString().trimmed();
            showTtsHint(detail.trimmed());
            resumeCaptureAfterTtsIfNeeded();
            continue;
        }

        const QFileInfo wavInfo(outputFile);
        reportDebugEvent("B",
                         "OfflineVoiceService::handleTtsBackendStdout/response",
                         QStringLiteral("[DEBUG] Qwen3-TTS backend synthesized audio"),
                         QJsonObject{
                             {QStringLiteral("requestId"), QString::number(responseId)},
                             {QStringLiteral("outputFile"), outputFile},
                             {QStringLiteral("wavExists"), wavInfo.exists()},
                             {QStringLiteral("wavSize"), QString::number(wavInfo.exists() ? wavInfo.size() : -1)},
                             {QStringLiteral("sampleRate"), QString::number(obj.value(QStringLiteral("sample_rate")).toInt())},
                             {QStringLiteral("frames"), QString::number(obj.value(QStringLiteral("frames")).toInt())}
                         });

        if (!wavInfo.exists()) {
            showTtsHint(QObject::tr("Qwen3-TTS 已返回成功，但未找到生成的 wav 文件。"));
            resumeCaptureAfterTtsIfNeeded();
            continue;
        }

        QString outputError;
        if (!ensureTtsAudioOutput(m_settings.ttsVolumePercent, &outputError)) {
            showSpeakerHint(outputError);
            resumeCaptureAfterTtsIfNeeded();
            continue;
        }

        if (!m_ttsPlayer) {
            m_ttsPlayer = new QMediaPlayer(this);
            connect(m_ttsPlayer, &QMediaPlayer::errorOccurred, this,
                    [this](QMediaPlayer::Error error, const QString& errorString) {
                QJsonObject data{
                    {QStringLiteral("errorCode"), int(error)},
                    {QStringLiteral("errorString"), errorString},
                    {QStringLiteral("source"), m_ttsPlayer ? m_ttsPlayer->source().toString() : QString()}
                };
                if (m_ttsAudio)
                    data.insert(QStringLiteral("audioOutputDevice"), audioDeviceSummary(m_ttsAudio->device()));
                reportDebugEvent("A",
                                 "OfflineVoiceService::startTts/player-error",
                                 QStringLiteral("[DEBUG] QMediaPlayer emitted error"),
                                 data);
            });
            connect(m_ttsPlayer, &QMediaPlayer::mediaStatusChanged, this,
                    [this](QMediaPlayer::MediaStatus status) {
                QJsonObject data{
                    {QStringLiteral("status"), mediaStatusName(status)},
                    {QStringLiteral("errorString"), m_ttsPlayer ? m_ttsPlayer->errorString() : QString()},
                    {QStringLiteral("source"), m_ttsPlayer ? m_ttsPlayer->source().toString() : QString()}
                };
                if (m_ttsAudio)
                    data.insert(QStringLiteral("audioOutputDevice"), audioDeviceSummary(m_ttsAudio->device()));
                reportDebugEvent(status == QMediaPlayer::InvalidMedia ? "C" : "A",
                                 "OfflineVoiceService::startTts/player-status",
                                 QStringLiteral("[DEBUG] QMediaPlayer media status changed"),
                                 data);
            });
            connect(m_ttsPlayer, &QMediaPlayer::playbackStateChanged, this,
                    [this](QMediaPlayer::PlaybackState state) {
                QJsonObject data{
                    {QStringLiteral("playbackState"), playbackStateName(state)},
                    {QStringLiteral("errorString"), m_ttsPlayer ? m_ttsPlayer->errorString() : QString()},
                    {QStringLiteral("source"), m_ttsPlayer ? m_ttsPlayer->source().toString() : QString()}
                };
                if (m_ttsAudio)
                    data.insert(QStringLiteral("audioOutputDevice"), audioDeviceSummary(m_ttsAudio->device()));
                reportDebugEvent("A",
                                 "OfflineVoiceService::startTts/player-playback",
                                 QStringLiteral("[DEBUG] QMediaPlayer playback state changed"),
                                 data);
            });
        } else {
            m_ttsPlayer->stop();
        }

        m_ttsPlayer->setAudioOutput(m_ttsAudio);
        m_ttsPlayer->setSource(QUrl::fromLocalFile(outputFile));
        reportDebugEvent("B",
                         "OfflineVoiceService::startTts/player-source",
                         QStringLiteral("[DEBUG] Prepared QMediaPlayer source for generated wav"),
                         QJsonObject{
                             {QStringLiteral("wavPath"), outputFile},
                             {QStringLiteral("wavSize"), QString::number(wavInfo.size())},
                             {QStringLiteral("sourceUrl"), QUrl::fromLocalFile(outputFile).toString()},
                             {QStringLiteral("audioOutputDevice"), m_ttsAudio ? audioDeviceSummary(m_ttsAudio->device()) : QStringLiteral("<null>")}
                         });

        connect(m_ttsPlayer, &QMediaPlayer::errorOccurred, this,
                [this](QMediaPlayer::Error, const QString &errorString) {
            qWarning() << "TTS playback error:" << errorString;
            showSpeakerHint(errorString);
            resumeCaptureAfterTtsIfNeeded();
        }, Qt::SingleShotConnection);
        connect(m_ttsPlayer, &QMediaPlayer::mediaStatusChanged, this,
                [this](QMediaPlayer::MediaStatus status) {
            if (status == QMediaPlayer::EndOfMedia
                || status == QMediaPlayer::InvalidMedia
                || status == QMediaPlayer::NoMedia)
            {
                resumeCaptureAfterTtsIfNeeded();
            }
        }, Qt::SingleShotConnection);

        m_ttsPlayer->play();
    }
}

void OfflineVoiceService::warmTtsBackendIfNeeded()
{
    if (!m_settings.ttsEnabled)
        return;

    const QString pythonExe = qwenTtsPythonExecutable();
    const QString backendScript = qwenTtsBackendScriptPath(m_settings.ttsModelPath);
    const QString modelMode = detectQwenTtsModelMode(m_settings.ttsModelPath);
    const bool canRunBackend =
        !pythonExe.isEmpty() && QFileInfo::exists(pythonExe) &&
        !backendScript.isEmpty() && QFileInfo::exists(backendScript) &&
        !m_settings.ttsModelPath.isEmpty() && QFileInfo::exists(m_settings.ttsModelPath);
    if (!canRunBackend)
        return;

    QString backendError;
    if (!ensureTtsBackendProcess(pythonExe, backendScript, m_settings.ttsModelPath, modelMode, &backendError)
        && !backendError.isEmpty())
    {
        reportDebugEvent("A",
                         "OfflineVoiceService::warmTtsBackendIfNeeded/error",
                         QStringLiteral("[DEBUG] Failed to prewarm Qwen3-TTS backend"),
                         QJsonObject{
                             {QStringLiteral("detail"), clampForLog(backendError)}
                         });
    }
}

void OfflineVoiceService::startTts(const QString& text)
{
    stopTts();

    if (text.trimmed().isEmpty())
        return;

    const QString pythonExe = qwenTtsPythonExecutable();
    const QString backendScript = qwenTtsBackendScriptPath(m_settings.ttsModelPath);
    const QString modelMode = detectQwenTtsModelMode(m_settings.ttsModelPath);
    const QString defaultPromptAudio = qwenTtsDefaultPromptAudioPath(m_settings.ttsModelPath);
    const QString defaultPromptText = qwenTtsDefaultPromptTextPath(m_settings.ttsModelPath);
    const bool canRunBackend =
        !pythonExe.isEmpty() && QFileInfo::exists(pythonExe) &&
        !backendScript.isEmpty() && QFileInfo::exists(backendScript) &&
        !m_settings.ttsModelPath.isEmpty() && QFileInfo::exists(m_settings.ttsModelPath);

    // #region debug-point A:tts-program-selection
    reportDebugEvent("A",
                     "OfflineVoiceService::startTts/select",
                     QStringLiteral("[DEBUG] Selected Qwen3-TTS backend"),
                     QJsonObject{
                         {QStringLiteral("pythonExecutable"), pythonExe},
                         {QStringLiteral("backendScript"), backendScript},
                         {QStringLiteral("canRunBackend"), canRunBackend},
                         {QStringLiteral("modelMode"), modelMode},
                         {QStringLiteral("defaultPromptAudio"), defaultPromptAudio},
                         {QStringLiteral("defaultPromptText"), defaultPromptText},
                         {QStringLiteral("textLength"), text.size()},
                         {QStringLiteral("ttsModelPath"), m_settings.ttsModelPath},
                         {QStringLiteral("ttsModelPathExists"), QFileInfo::exists(m_settings.ttsModelPath)}
                     });
    // #endregion

    if (!canRunBackend) {
        QStringList missing;
        if (pythonExe.isEmpty() || !QFileInfo::exists(pythonExe))
            missing << QStringLiteral("Python");
        if (backendScript.isEmpty() || !QFileInfo::exists(backendScript))
            missing << QStringLiteral("backend script");
        if (m_settings.ttsModelPath.isEmpty() || !QFileInfo::exists(m_settings.ttsModelPath))
            missing << QStringLiteral("Qwen3-TTS model directory");
        showTtsHint(QObject::tr("无法启动 Qwen3-TTS 本地后端，缺少：%1").arg(missing.join(QStringLiteral(", "))));
        return;
    }

    pauseCaptureForTts();

    QDir cache(SettingsManager::instance().cacheDir());
    if (!cache.exists()) cache.mkpath(QStringLiteral("."));
    const QString wavPath = cache.filePath(QStringLiteral("tts_%1.wav").arg(QDateTime::currentMSecsSinceEpoch()));
    m_ttsWavPath = wavPath;
    QJsonObject request{
        {QStringLiteral("id"), ++m_ttsRequestCounter},
        {QStringLiteral("command"), QStringLiteral("synthesize")},
        {QStringLiteral("output_file"), wavPath},
        {QStringLiteral("text"), text},
        {QStringLiteral("language"), QStringLiteral("Chinese")},
        {QStringLiteral("mode"), modelMode},
        {QStringLiteral("max_new_tokens"), 512}
    };
    m_activeTtsRequestId = m_ttsRequestCounter;

    if (modelMode == QStringLiteral("base")) {
        if (defaultPromptAudio.isEmpty()) {
            showTtsHint(QObject::tr(
                "当前 Qwen3-TTS Base 模型缺少默认参考音频。\n\n"
                "请检查 `res/voice_deps/qwen3-tts/prompt/default_reference.mp3` 是否存在。"));
            resumeCaptureAfterTtsIfNeeded();
            return;
        }
        request.insert(QStringLiteral("ref_audio"), defaultPromptAudio);
        request.insert(QStringLiteral("x_vector_only"), true);
        if (!defaultPromptText.isEmpty())
            request.insert(QStringLiteral("ref_text"), defaultPromptText);
    }

    QString backendError;
    if (!ensureTtsBackendProcess(pythonExe, backendScript, m_settings.ttsModelPath, modelMode, &backendError)) {
        showTtsHint(backendError);
        resumeCaptureAfterTtsIfNeeded();
        return;
    }

    m_ttsRequestActive = true;
    const QByteArray payload = QJsonDocument(request).toJson(QJsonDocument::Compact) + '\n';
    if (m_tts->write(payload) < 0) {
        m_ttsRequestActive = false;
        showTtsHint(m_tts->errorString());
        resumeCaptureAfterTtsIfNeeded();
        return;
    }
    m_tts->waitForBytesWritten(1000);
    reportDebugEvent("B",
                     "OfflineVoiceService::startTts/request-sent",
                     QStringLiteral("[DEBUG] Sent TTS request to warm backend"),
                     QJsonObject{
                         {QStringLiteral("requestId"), QString::number(m_activeTtsRequestId)},
                         {QStringLiteral("wavPath"), wavPath},
                         {QStringLiteral("textLength"), text.size()},
                         {QStringLiteral("modelMode"), modelMode}
                     });
}

void OfflineVoiceService::stopTts()
{
    m_ttsRequestActive = false;
    m_activeTtsRequestId = ++m_ttsRequestCounter;
    if (m_ttsPlayer) {
        m_ttsPlayer->stop();
    }
    if (!m_ttsWavPath.isEmpty()) {
        QFile::remove(m_ttsWavPath);
        m_ttsWavPath.clear();
    }
    resumeCaptureAfterTtsIfNeeded();
}

void OfflineVoiceService::stopTtsBackend()
{
    if (m_tts) {
        m_tts->kill();
        m_tts->deleteLater();
        m_tts = nullptr;
    }
    m_ttsBackendSignature.clear();
    m_ttsBackendStdoutBuffer.clear();
}
