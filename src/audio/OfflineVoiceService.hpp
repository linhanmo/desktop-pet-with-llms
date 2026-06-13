#pragma once

#include <QObject>
#include <QAudioFormat>
#include <QString>
#include <QStringList>
#include <memory>
#include <functional>

class QProcess;
class QTimer;
class QAudioSource;
class QIODevice;
class QAudioOutput;
class QMediaPlayer;
class QMediaDevices;

class OfflineVoiceService : public QObject
{
    Q_OBJECT
public:
    explicit OfflineVoiceService(QObject* parent = nullptr);
    ~OfflineVoiceService() override;

    void reloadFromSettings();
    void start();
    void stop();

    void speakText(const QString& text);

    void startListening();
    void stopListening();

signals:
    void wakeWordDetected();
    void sttResult(const QString& text);
    void sttPartialResult(const QString& text);
    void audioError(const QString& error);

private:
    struct SettingsSnapshot {
        bool ttsEnabled{false};
        bool kwsEnabled{false};
        bool sttEnabled{false};
        QString binDir;
        QString ttsArgs;
        int ttsVolumePercent{80};
        QString kwsModelPath;
        QString vadModelPath;
        QString sttModelPath;
        QString ttsModelPath;
    };

    void applySettingsSnapshot(const SettingsSnapshot& next);
    SettingsSnapshot readSettings() const;

    QString exePath(const QString& baseName) const;
    QStringList splitArgs(const QString& args) const;

    void initAudioDevices();
    void startMicIfNeeded();
    void stopMic();
    bool ensureTtsAudioOutput(int ttsVolumePercent, QString* errorDetail = nullptr);
    bool shouldCaptureMic() const;
    void resetListeningState();
    void syncCaptureState();
    void pauseCaptureForTts();
    void resumeCaptureAfterTtsIfNeeded();

    void processAudioBuffer(const QByteArray& data);
    void processKWS(const float* samples, size_t count);
    void processVAD(const float* samples, size_t count);
    void processSTT(const float* samples, size_t count);

    void startTts(const QString& text);
    void stopTts();
    void stopTtsBackend();
    bool ensureTtsBackendProcess(const QString& pythonExe,
                                 const QString& backendScript,
                                 const QString& modelDir,
                                 const QString& modelMode,
                                 QString* errorDetail = nullptr);
    void handleTtsBackendStdout();
    void warmTtsBackendIfNeeded();

private:
    SettingsSnapshot m_settings;

    QProcess* m_tts{nullptr};
    QProcess* m_sttProcess{nullptr};

    QAudioSource* m_audioSource{nullptr};
    QIODevice* m_audioDevice{nullptr};
    QAudioOutput* m_ttsAudio{nullptr};
    QMediaPlayer* m_ttsPlayer{nullptr};
    QMediaDevices* m_mediaDevices{nullptr};
    QString m_ttsWavPath;
    QString m_ttsBackendSignature;
    QByteArray m_ttsBackendStdoutBuffer;
    bool m_resumeCaptureAfterTts{false};
    bool m_mediaDeviceSignalsConnected{false};
    qint64 m_ttsRequestCounter{0};
    qint64 m_activeTtsRequestId{0};
    bool m_ttsRequestActive{false};

    std::vector<float> m_audioBuffer;
    std::vector<float> m_resampledBuffer;
    QAudioFormat m_inputFormat;
    double m_resampleStep{1.0};
    double m_resamplePos{0.0};

    enum class Mode {
        Idle,
        Listening,
        Recording,
        Processing
    };
    Mode m_mode{Mode::Idle};

    bool m_kwsActive{false};
    bool m_vadDetected{false};
    std::vector<float> m_vadBuffer;
    size_t m_vadSilenceFrames{0};
    const size_t m_vadSilenceThreshold{50};

    QString m_sttPartialResult;
};
