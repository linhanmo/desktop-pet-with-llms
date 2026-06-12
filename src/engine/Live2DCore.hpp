#pragma once
#include <QString>
#include <QByteArray>
#include <cstdint>
#include "common/Utils.hpp"

#ifdef AMAIGIRL_USE_MINGW

struct csmMoc {};
struct csmModel {};
struct csmVector2 { float X, Y; };
typedef uint8_t csmFlags;

constexpr csmFlags csmIsVisible = 1;
constexpr csmFlags csmIsInvertedMask = 2;
constexpr csmFlags csmBlendAdditive = 4;
constexpr csmFlags csmBlendMultiplicative = 8;

inline uint32_t csmGetParameterCount(csmModel*) { return 0; }
inline const float* csmGetParameterValues(csmModel*) { return nullptr; }
inline const float* csmGetParameterMinimumValues(csmModel*) { return nullptr; }
inline const float* csmGetParameterMaximumValues(csmModel*) { return nullptr; }
inline const float* csmGetParameterDefaultValues(csmModel*) { return nullptr; }
inline const char** csmGetParameterIds(csmModel*) { return nullptr; }
inline uint32_t csmGetPartCount(csmModel*) { return 0; }
inline const char** csmGetPartIds(csmModel*) { return nullptr; }
inline float* csmGetPartOpacityValues(csmModel*) { return nullptr; }
inline void csmReadCanvasInfo(csmModel*, csmVector2*, csmVector2*, float*) {}
inline void csmResetDrawableDynamicFlags(csmModel*) {}
inline void csmUpdateModel(csmModel*) {}
inline void csmSetParameterValue(csmModel*, const char*, float) {}
inline void csmSetPartOpacity(csmModel*, const char*, float) {}
inline const csmFlags* csmGetDrawableDynamicFlags(csmModel*) { return nullptr; }
inline float* csmGetPartOpacities(csmModel*) { return nullptr; }
inline uint32_t csmGetDrawableCount(csmModel*) { return 0; }
inline const int32_t* csmGetDrawableParentPartIndices(csmModel*) { return nullptr; }
inline const int32_t* csmGetDrawableRenderOrders(csmModel*) { return nullptr; }
inline const float* csmGetDrawableOpacities(csmModel*) { return nullptr; }
inline const csmVector2** csmGetDrawableVertexPositions(csmModel*) { return nullptr; }
inline const int32_t* csmGetDrawableVertexCounts(csmModel*) { return nullptr; }

struct MocHolder {
    csmMoc* moc{nullptr};
    QByteArray mocStorage;
    void* mocAlignedPtr{nullptr};
    csmModel* model{nullptr};
    QByteArray modelStorage;
    void* modelAlignedPtr{nullptr};
};

class Live2DCore {
public:
    static QString versionString() { return QString(); }
    static uint32_t latestMocVersion() { return 0; }
    static uint32_t mocVersion(const QByteArray& data) { return 0; }
    static MocHolder loadMoc(const QString& path) { return {}; }
};

#else

#include <Live2DCubismCore.h>

struct MocHolder {
    csmMoc* moc{nullptr};
    QByteArray mocStorage;
    void* mocAlignedPtr{nullptr};
    csmModel* model{nullptr};
    QByteArray modelStorage;
    void* modelAlignedPtr{nullptr};
};

class Live2DCore {
public:
    static QString versionString();
    static uint32_t latestMocVersion();
    static uint32_t mocVersion(const QByteArray& data);
    static MocHolder loadMoc(const QString& path);
};

#endif