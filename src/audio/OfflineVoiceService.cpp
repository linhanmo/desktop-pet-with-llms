#include "audio/OfflineVoiceService.hpp"

#include "common/SettingsManager.hpp"

#include <QProcess>
#include <QTimer>
#include <QDir>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <QMessageBox>
#include <QDesktopServices>
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
    if (m_settings.kwsEnabled || m_settings.sttEnabled) {
        initAudioDevices();
        startMicIfNeeded();
    }
}

void OfflineVoiceService::stop()
{
    stopMic();
    stopTts();
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
    if (m_mode == Mode::Idle || m_mode == Mode::Listening) {
        m_mode = Mode::Listening;
        m_kwsActive = true;
        m_vadDetected = false;
        startMicIfNeeded();
    }
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
    s.kwsEnabled = false;
    s.sttEnabled = false;
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
    s.ttsModelPath = QDir(voiceDepsDir).filePath(QStringLiteral("qwen3-tts"));
    s.ttsArgs = QStringLiteral("--encoder-model=\"%1/model.safetensors\" --decoder-model=\"%1/model.safetensors\" --speech-tokenizer-dir=\"%1/speech_tokenizer\"").arg(s.ttsModelPath);
    
    return s;
}

void OfflineVoiceService::applySettingsSnapshot(const SettingsSnapshot& next)
{
    if (m_settings.ttsArgs != next.ttsArgs || 
        m_settings.ttsVolumePercent != next.ttsVolumePercent ||
        m_settings.binDir != next.binDir) {
        stopTts();
    }
    
    m_settings = next;
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

    QObject::connect(m_mediaDevices, &QMediaDevices::audioInputsChanged,
                     this, [this]() {
        if (m_mode == Mode::Listening || m_mode == Mode::Recording) {
            stopMic();
            startMicIfNeeded();
        }
    }, Qt::UniqueConnection);
    
    QObject::connect(m_mediaDevices, &QMediaDevices::audioOutputsChanged,
                     this, [this]() {
        if (m_ttsAudio) {
            delete m_ttsAudio;
            m_ttsAudio = nullptr;
        }
    }, Qt::UniqueConnection);
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
            emit wakeWordDetected();
            m_kwsActive = false;
            m_mode = Mode::Recording;
            m_vadDetected = true;
            m_vadBuffer.clear();
            m_vadSilenceFrames = 0;
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
            emit wakeWordDetected();
            m_kwsActive = false;
            m_mode = Mode::Recording;
            m_vadDetected = true;
            m_vadBuffer.clear();
            m_vadSilenceFrames = 0;
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
        emit wakeWordDetected();
        m_kwsActive = false;
        m_mode = Mode::Recording;
        m_vadDetected = true;
        m_vadBuffer.clear();
        m_vadSilenceFrames = 0;
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
                m_vadDetected = false;
                m_vadBuffer.clear();
                m_vadSilenceFrames = 0;
                m_mode = Mode::Listening;
                m_kwsActive = true;
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
            m_vadDetected = false;
            m_vadBuffer.clear();
            m_vadSilenceFrames = 0;
            m_mode = Mode::Listening;
            m_kwsActive = true;
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
    
    connect(m_sttProcess, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), 
            this, [this, wavPath](int exitCode, QProcess::ExitStatus) {
        if (exitCode == 0) {
            QString result = QString::fromLocal8Bit(m_sttProcess->readAllStandardOutput());
            if (!result.isEmpty()) {
                emit sttResult(result.trimmed());
            }
        } else {
            QString error = QString::fromLocal8Bit(m_sttProcess->readAllStandardError());
            qWarning() << "STT error:" << error;
        }
        m_sttProcess->deleteLater();
        m_sttProcess = nullptr;
        QFile::remove(wavPath);
    });
    
    m_sttProcess->start();
}

void OfflineVoiceService::startTts(const QString& text)
{
    stopTts();
    
    QString programGen = m_settings.kwsModelPath + QStringLiteral("/sherpa-onnx-offline-tts");
    QString programPlay = m_settings.kwsModelPath + QStringLiteral("/sherpa-onnx-offline-tts-play");
#if defined(Q_OS_WIN32)
    if (!programGen.endsWith(QStringLiteral(".exe")))
        programGen += QStringLiteral(".exe");
    if (!programPlay.endsWith(QStringLiteral(".exe")))
        programPlay += QStringLiteral(".exe");
#endif
    
    if (!QFileInfo::exists(programGen)) {
        programGen = exePath(QStringLiteral("sherpa-onnx-offline-tts"));
    }
    if (!QFileInfo::exists(programPlay)) {
        programPlay = exePath(QStringLiteral("sherpa-onnx-offline-tts-play"));
    }
    
    bool canGen = !programGen.isEmpty() && QFileInfo::exists(programGen);
    bool canPlay = !programPlay.isEmpty() && QFileInfo::exists(programPlay);
    QString program = canPlay ? programPlay : programGen;
    
    if (!canGen && !canPlay) {
        return;
    }
    
    QString wavPath;
    bool useWavGen = (program == programGen);
    if (useWavGen) {
        QDir cache(SettingsManager::instance().cacheDir());
        if (!cache.exists()) cache.mkpath(QStringLiteral("."));
        wavPath = cache.filePath(QStringLiteral("tts_%1.wav").arg(QDateTime::currentMSecsSinceEpoch()));
        m_ttsWavPath = wavPath;
    }
    
    QString args = QStringLiteral("--encoder-model=\"%1/model.safetensors\" --decoder-model=\"%1/model.safetensors\" --speech-tokenizer-dir=\"%1/speech_tokenizer\"").arg(m_settings.ttsModelPath);
    
    if (useWavGen && !args.contains(QStringLiteral("--output-filename"))) {
        QString outArg = QStringLiteral("--output-filename=\"%1\"").arg(wavPath);
        args = outArg + QStringLiteral(" ") + args;
    }
    
    QString escaped = text;
    escaped.replace('"', QStringLiteral("\\\""));
    QString quoted = QStringLiteral("\"") + escaped + QStringLiteral("\"");
    args = args + QStringLiteral(" ") + quoted;
    
    m_tts = new QProcess(this);
    m_tts->setProgram(program);
    m_tts->setArguments(splitArgs(args));
    
    connect(m_tts, &QProcess::errorOccurred, this, [this](QProcess::ProcessError) {
        if (!m_tts) return;
        QString out = QString::fromLocal8Bit(m_tts->readAll());
        showSpeakerHint(m_tts->errorString() + QStringLiteral("\n") + out);
    });
    
    const int ttsVolPercent = m_settings.ttsVolumePercent;
    connect(m_tts, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), 
            this, [this, wavPath, ttsVolPercent](int exitCode, QProcess::ExitStatus) {
        if (!m_tts) return;
        QString out = QString::fromLocal8Bit(m_tts->readAll());
        if (exitCode != 0) {
            showSpeakerHint(out);
        }
        m_tts->deleteLater();
        m_tts = nullptr;
        
        if (exitCode == 0 && !wavPath.isEmpty() && QFileInfo::exists(wavPath)) {
            if (m_ttsAudio) {
                delete m_ttsAudio;
                m_ttsAudio = nullptr;
            }
            
            m_ttsAudio = new QAudioOutput(this);
            m_ttsAudio->setVolume(qBound(0.0, double(ttsVolPercent) / 100.0, 1.0));
            
            if (!m_ttsPlayer) {
                m_ttsPlayer = new QMediaPlayer(this);
            } else {
                m_ttsPlayer->stop();
            }
            
            m_ttsPlayer->setAudioOutput(m_ttsAudio);
            m_ttsPlayer->setSource(QUrl::fromLocalFile(wavPath));
            
            connect(m_ttsPlayer, &QMediaPlayer::errorOccurred, this, 
                    [this](QMediaPlayer::Error, const QString &errorString) {
                qWarning() << "TTS playback error:" << errorString;
            }, Qt::SingleShotConnection);
            
            m_ttsPlayer->play();
        }
    });
    
    m_tts->start();
    
#if defined(Q_OS_WIN32)
    if (program == programPlay) {
        float vol01 = qBound(0.0f, float(m_settings.ttsVolumePercent) / 100.0f, 1.0f);
        auto tries = std::make_shared<int>(0);
        auto loop = std::make_shared<std::function<void()>>();
        *loop = [this, vol01, tries, loop]{
            if (!m_tts) return;
            qint64 pid64 = m_tts->processId();
            if (pid64 <= 0) return;
            if (++*tries >= 20) return;
            QTimer::singleShot(120, this, *loop);
        };
        QTimer::singleShot(60, this, *loop);
    }
#endif
}

void OfflineVoiceService::stopTts()
{
    if (m_tts) {
        m_tts->kill();
        m_tts->deleteLater();
        m_tts = nullptr;
    }
    if (m_ttsPlayer) {
        m_ttsPlayer->stop();
    }
    if (!m_ttsWavPath.isEmpty()) {
        QFile::remove(m_ttsWavPath);
        m_ttsWavPath.clear();
    }
}
