#include "MetroMotion.h"

PACKED_STRUCT_BEGIN
struct MotionDataHeader_Exodus {   // size = 48 bytes
    Bitset256   bonesMask;
    uint16_t    numLocators;
    uint16_t    numXforms;
    uint32_t    totalSize;
    uint64_t    unknown_0;
} PACKED_STRUCT_END;

struct MotionDataHeader_Redux {   // size = 34 bytes
    Bitset128   bonesMask;
    uint16_t    numLocators;
    uint16_t    numXforms;
    uint32_t    totalSize;
    uint64_t    unknown_0;
} PACKED_STRUCT_END;

union MotionDataHeader {
    MotionDataHeader_Exodus *Exodus;
    MotionDataHeader_Redux *Redux;
};

enum MotionChunks {
    MC_HeaderChunk  = 0x00000000,
    MC_InfoChunk    = 0x00000001,
    MC_DataChunk    = 0x00000009,
};

MetroMotion::MetroMotion(const CharString& name)
    : mName(name)
    // header
    , mVersion(0)
    , mBonesCRC(0)
    , mNumBones(0)
    // info
    , mFlags(0)
    , mSpeed(0.0f)
    , mAccrue(0.0f)
    , mFalloff(0.0f)
    , mNumFrames(0)
    , mJumpFrame(0)
    , mLandFrame(0)
    , mAffectedBones()
    , mAffectedBones_Redux()
    , mMotionsDataSize(0)
    , mMotionsOffsetsSize(0)
    , mHighQualityBones()
{

}
MetroMotion::~MetroMotion() {

}

MotionDataHeader ReadMotionDataHeader(int version, const uint8_t *data) {
    if (version == 0xF) {
        MotionDataHeader hdr;
        hdr.Redux = new MotionDataHeader_Redux;
        std::memcpy(hdr.Redux, data, sizeof(MotionDataHeader_Redux));

        for (int i = 0; i < 4; i++) {
            hdr.Redux->bonesMask.dwords[i] = _byteswap_ulong(hdr.Redux->bonesMask.dwords[i]);
        }
        hdr.Redux->numLocators = _byteswap_ushort(hdr.Redux->numLocators);
        hdr.Redux->numXforms = _byteswap_ushort(hdr.Redux->numXforms);
        hdr.Redux->totalSize = _byteswap_ulong(hdr.Redux->totalSize);
        return hdr;
    }
    else {
        MotionDataHeader hdr;
        hdr.Exodus = new MotionDataHeader_Exodus;
        std::memcpy(hdr.Redux, data, sizeof(MotionDataHeader_Exodus));
        return hdr;
    }
}

bool MetroMotion::LoadHeader(MemStream& stream) {
    size_t chunksFound = 0;

    while (!stream.Ended()) {
        const size_t chunkId = stream.ReadTyped<uint32_t>();
        const size_t chunkSize = stream.ReadTyped<uint32_t>();
        const size_t chunkEnd = stream.GetCursor() + chunkSize;

        switch (chunkId) {
            case MC_HeaderChunk: {
                mVersion = stream.ReadTyped<uint32_t>();
                mBonesCRC = stream.ReadTyped<uint32_t>();
                mNumBones = stream.ReadTyped<uint16_t>();
                mNumLocators = stream.ReadTyped<uint16_t>();

                ++chunksFound;
            } break;

            case MC_InfoChunk: {
                mFlags = stream.ReadTyped<uint16_t>();

                mSpeed = stream.ReadTyped<float>();
                mAccrue = stream.ReadTyped<float>();
                mFalloff = stream.ReadTyped<float>();

                mNumFrames = stream.ReadTyped<uint32_t>();
                mJumpFrame = stream.ReadTyped<uint16_t>();
                mLandFrame = stream.ReadTyped<uint16_t>();

                if (mVersion == kMVersionRedux)
                    stream.ReadStruct(mAffectedBones_Redux);
                else
                    stream.ReadStruct(mAffectedBones);

                mMotionsDataSize = stream.ReadTyped<uint32_t>();
                mMotionsOffsetsSize = stream.ReadTyped<uint32_t>();

                if (mVersion == kMVersionRedux)
                    stream.ReadStruct(mHighQualityBones_Redux);
                else
                    stream.ReadStruct(mHighQualityBones);

                ++chunksFound;
            } break;
        }

        stream.SetCursor(chunkEnd);
    }

    return chunksFound == 2;
}

bool MetroMotion::LoadFromData(MemStream& stream) {
    bool result = false;

    while (!stream.Ended()) {
        const size_t chunkId = stream.ReadTyped<uint32_t>();
        const size_t chunkSize = stream.ReadTyped<uint32_t>();
        const size_t chunkEnd = stream.GetCursor() + chunkSize;

        switch (chunkId) {
            case MC_HeaderChunk: {
                mVersion = stream.ReadTyped<uint32_t>();

                //#TODO_SK: check bones info against skeleton !
                mBonesCRC = stream.ReadTyped<uint32_t>();
                mNumBones = stream.ReadTyped<uint16_t>();
                mNumLocators = stream.ReadTyped<uint16_t>();
            } break;

            case MC_InfoChunk: {
                mFlags = stream.ReadTyped<uint16_t>();

                mSpeed = stream.ReadTyped<float>();
                mAccrue = stream.ReadTyped<float>();
                mFalloff = stream.ReadTyped<float>();

                mNumFrames = stream.ReadTyped<uint32_t>();
                mJumpFrame = stream.ReadTyped<uint16_t>();
                mLandFrame = stream.ReadTyped<uint16_t>();

                if (mVersion == kMVersionRedux)
                    stream.ReadStruct(mAffectedBones_Redux);
                else
                    stream.ReadStruct(mAffectedBones);

                mMotionsDataSize = stream.ReadTyped<uint32_t>();
                mMotionsOffsetsSize = stream.ReadTyped<uint32_t>();

                if (mVersion == kMVersionRedux)
                    stream.ReadStruct(mHighQualityBones_Redux);
                else
                    stream.ReadStruct(mHighQualityBones);
            } break;

            case MC_DataChunk: {
                assert(chunkSize == mMotionsDataSize);
                if (chunkSize != mMotionsDataSize) {
                    return false;
                }

                mMotionsData.resize(mMotionsDataSize);
                stream.ReadToBuffer(mMotionsData.data(), mMotionsData.size());
            } break;
        }

        stream.SetCursor(chunkEnd);
    }

    result = this->LoadInternal();

    mMotionsData.resize(0);

    return result;
}

const CharString& MetroMotion::GetName() const {
    return mName;
}

size_t MetroMotion::GetBonesCRC() const {
    return mBonesCRC;
}

size_t MetroMotion::GetNumBones() const {
    return mNumBones;
}

size_t MetroMotion::GetNumLocators() const {
    return mNumLocators;
}

size_t MetroMotion::GetNumFrames() const {
    return mNumFrames;
}

float MetroMotion::GetMotionTimeInSeconds() const {
    return scast<float>(mNumFrames) / scast<float>(kFrameRate);
}

bool MetroMotion::IsBoneAnimated(const size_t boneIdx) const {
    MotionDataHeader hdr = ReadMotionDataHeader(mVersion, mMotionsData.data());
    bool bonePresent;
    bool motionHasThisBone;

    if (mVersion == kMVersionRedux) {
        bonePresent = mAffectedBones_Redux.IsPresent(boneIdx);
        motionHasThisBone = hdr.Redux->bonesMask.IsPresent(boneIdx);
    }
    else {
        bonePresent = mAffectedBones.IsPresent(boneIdx);
        motionHasThisBone = hdr.Exodus->bonesMask.IsPresent(boneIdx);
    }

    return bonePresent && motionHasThisBone;
}

quat MetroMotion::GetBoneRotation(const size_t boneIdx, const size_t key) const {
    quat result(1.0f, 0.0f, 0.0f, 0.0f);

    const AttributeCurve& curve = mBonesRotations[boneIdx];
    if (!curve.points.empty()) {
        if (curve.points.size() == 1) { // constant value
            result = *rcast<const quat*>(&curve.points.front().value);
        } else {
            const float timing = scast<float>(key) / scast<float>(kFrameRate);

            const size_t numPoints = curve.points.size();
            size_t pointA = numPoints, pointB = numPoints;
            for (size_t i = 0; i < numPoints; ++i) {
                const auto& p = curve.points[i];
                if (p.time >= timing) {
                    pointB = i;
                    break;
                }
            }

            if (pointB == numPoints) {
                pointB--;
                pointA = pointB;
            } else if (pointB == 0) {
                pointA = 0;
            } else {
                pointA = pointB - 1;
            }

            const auto& pA = curve.points[pointA];

            if (pointA == pointB) {
                result = *rcast<const quat*>(&pA.value);
            } else {
                const auto& pB = curve.points[pointB];
                const float t = (timing - pA.time) / (pB.time - pA.time);

                result = QuatSlerp(*rcast<const quat*>(&pA.value), *rcast<const quat*>(&pB.value), t);
            }
        }
    }

    return result;
}

vec3 MetroMotion::GetBonePosition(const size_t boneIdx, const size_t key) const {
    vec3 result(0.0f);

    const AttributeCurve& curve = mBonesPositions[boneIdx];
    if (!curve.points.empty()) {
        if (curve.points.size() == 1) { // constant value
            result = *rcast<const vec3*>(&curve.points.front().value);
        } else {
            const float timing = scast<float>(key) / scast<float>(kFrameRate);

            const size_t numPoints = curve.points.size();
            size_t pointA = numPoints, pointB = numPoints;
            for (size_t i = 0; i < numPoints; ++i) {
                const auto& p = curve.points[i];
                if (p.time >= timing) {
                    pointB = i;
                    break;
                }
            }

            if (pointB == numPoints) {
                pointB--;
                pointA = pointB;
            } else if (pointB == 0) {
                pointA = 0;
            } else {
                pointA = pointB - 1;
            }

            const auto& pA = curve.points[pointA];

            if (pointA == pointB) {
                result = pA.value;
            } else {
                const auto& pB = curve.points[pointB];
                const float t = (timing - pA.time) / (pB.time - pA.time);

                result = Lerp(*rcast<const vec3*>(&pA.value), *rcast<const vec3*>(&pB.value), t);
            }
        }
    }

    return result;
}

vec3 MetroMotion::GetBoneScale(const size_t boneIdx, const size_t key) const {
    vec3 result(0.0f);

    const AttributeCurve& curve = mBonesScales[boneIdx];
    if (!curve.points.empty()) {
        if (curve.points.size() == 1) { // constant value
            result = *rcast<const vec3*>(&curve.points.front().value);
        }
        else {
            const float timing = scast<float>(key) / scast<float>(kFrameRate);

            const size_t numPoints = curve.points.size();
            size_t pointA = numPoints, pointB = numPoints;
            for (size_t i = 0; i < numPoints; ++i) {
                const auto& p = curve.points[i];
                if (p.time >= timing) {
                    pointB = i;
                    break;
                }
            }

            if (pointB == numPoints) {
                pointB--;
                pointA = pointB;
            }
            else if (pointB == 0) {
                pointA = 0;
            }
            else {
                pointA = pointB - 1;
            }

            const auto& pA = curve.points[pointA];

            if (pointA == pointB) {
                result = pA.value;
            }
            else {
                const auto& pB = curve.points[pointB];
                const float t = (timing - pA.time) / (pB.time - pA.time);

                result = Lerp(*rcast<const vec3*>(&pA.value), *rcast<const vec3*>(&pB.value), t);
            }
        }
    }

    return result;
}



enum class AttribCurveType : uint8_t {
    Invalid         = 0,
    Uncompressed    = 1,    // raw float values
    OneValue        = 2,    // constant value, no curve
    Unknown_3       = 3,
    CompressedPos   = 4,    // quantized position, scale + offset + u16 values
    CompressedQuat  = 5,    // quantized quaternion (xyz, we restore w), s16_snorm values
    Unknown_6       = 6,
    Empty           = 7     // no curve, why not just filter it out with mask ???
};


bool MetroMotion::LoadInternal() {
    bool result = false;

    if (!mMotionsData.empty() && mMotionsData.size() > mMotionsOffsetsSize) {
        uint8_t* ptr = mMotionsData.data();

        MotionDataHeader hdr = ReadMotionDataHeader(mVersion, mMotionsData.data());
        uint32_t* offsetsTable;

        if (mVersion == kMVersionRedux) {
            mBonesScales.resize(mNumBones);
            offsetsTable = rcast<uint32_t*>(ptr + sizeof(MotionDataHeader_Redux));
        }
        else {
            offsetsTable = rcast<uint32_t*>(ptr + sizeof(MotionDataHeader_Exodus));
        }

        mBonesRotations.resize(mNumBones);
        mBonesPositions.resize(mNumBones);

        int stride = (mVersion == kMVersionRedux) ? 3 : 2;

        for (size_t boneIdx = 0, flatIdx = 0; boneIdx < mNumBones; ++boneIdx) {
            bool bonePresent;
            bool motionHasThisBone;
            if (mVersion == kMVersionRedux) {
                bonePresent = mAffectedBones_Redux.IsPresent(boneIdx);
                motionHasThisBone = hdr.Redux->bonesMask.IsPresent(boneIdx);
            }
            else {
                bonePresent = mAffectedBones.IsPresent(boneIdx);
                motionHasThisBone = hdr.Exodus->bonesMask.IsPresent(boneIdx);
            }

            if (bonePresent && motionHasThisBone) {
                size_t offsetQ = offsetsTable[flatIdx * stride + 0];
                size_t offsetT = offsetsTable[flatIdx * stride + 1];

                size_t offsetS = offsetsTable[flatIdx * stride + 2]; //Redux only

                bool disableBSwap = true;

                if (offsetQ > mMotionsData.size()){
                    offsetQ = _byteswap_ulong(offsetQ); disableBSwap = false;
                }
                if (offsetT > mMotionsData.size()) {
                    offsetT = _byteswap_ulong(offsetT); disableBSwap = false;
                }
                if (offsetS > mMotionsData.size()) {
                    offsetS = _byteswap_ulong(offsetS); disableBSwap = false;
                }

                this->ReadAttributeCurve(ptr + offsetQ, mBonesRotations[boneIdx], 4, disableBSwap);
                this->ReadAttributeCurve(ptr + offsetT, mBonesPositions[boneIdx], 3, disableBSwap);

                if(mVersion == kMVersionRedux)
                    this->ReadAttributeCurve(ptr + offsetS, mBonesScales[boneIdx], 3, disableBSwap);

                ++flatIdx;
            }
        }

        result = true;
    }

    return result;
}

void MetroMotion::ReadAttributeCurve(const uint8_t* curveData, AttributeCurve& curve, const size_t attribSize, bool disableBSwap) {
    uint32_t curveHeader = *rcast<const uint32_t*>(curveData);

    if (mVersion == kMVersionRedux && !disableBSwap)
        curveHeader = _byteswap_ulong(curveHeader);

    const size_t numPoints = scast<size_t>(curveHeader & 0xFFFF);
    const AttribCurveType ctype = scast<AttribCurveType>((curveHeader >> 16) & 0xF);

    assert(attribSize <= 4);

    curveData += 4;

    if (ctype == AttribCurveType::Empty) {
        curve.points.clear();
    } else if (ctype == AttribCurveType::OneValue) {
        curve.points.resize(1);
        auto& p = curve.points.back();
        p.time = 0.0f;
        memcpy(&p.value, curveData, attribSize * sizeof(float));

        if (mVersion == kMVersionRedux && !disableBSwap) {
            p.value.x = FloatByteSwap(p.value.x);
            p.value.y = FloatByteSwap(p.value.y);
            p.value.z = FloatByteSwap(p.value.z);
            p.value.w = FloatByteSwap(p.value.w);
        }

        p.value = MetroSwizzle(p.value);

    } else if (ctype == AttribCurveType::Unknown_3 || ctype == AttribCurveType::Unknown_6) {
        assert(false);
    } else {
        curve.points.resize(numPoints);

        switch (ctype) {
            case AttribCurveType::Uncompressed: {
                const float* timingsPtr = rcast<const float*>(curveData);
                const float* valuesPtr = rcast<const float*>(curveData + (numPoints * sizeof(float)));

                for (auto& p : curve.points) {
                    p.time = *timingsPtr;

                    if (mVersion == kMVersionRedux && !disableBSwap)
                        p.time = FloatByteSwap(p.time);

                    memcpy(&p.value, valuesPtr, attribSize * sizeof(float));

                    if (mVersion == kMVersionRedux) {
                        p.value.x = FloatByteSwap(p.value.x);
                        p.value.y = FloatByteSwap(p.value.y);
                        p.value.z = FloatByteSwap(p.value.z);
                        p.value.w = FloatByteSwap(p.value.w);
                    }

                    p.value = MetroSwizzle(p.value);

                    timingsPtr++;
                    valuesPtr += attribSize;
                }
            } break;

            case AttribCurveType::CompressedPos: {

                float timingScale = *rcast<const float*>(curveData);
                if (mVersion == kMVersionRedux && !disableBSwap)
                    timingScale = FloatByteSwap(timingScale);

                timingScale = 1.0f / timingScale;

                curveData += 4;

                vec3 scale = rcast<const vec3*>(curveData)[0];
                vec3 offset = rcast<const vec3*>(curveData)[1];

                if (mVersion == kMVersionRedux && !disableBSwap) {
                    scale.x = FloatByteSwap(scale.x);
                    scale.y = FloatByteSwap(scale.y);
                    scale.z = FloatByteSwap(scale.z);
                    offset.x = FloatByteSwap(offset.x);
                    offset.y = FloatByteSwap(offset.y);
                    offset.z = FloatByteSwap(offset.z);
                }
                curveData += sizeof(vec3[2]);

                const uint16_t* timingsPtr = rcast<const uint16_t*>(curveData);
                const uint16_t* valuesPtr = rcast<const uint16_t*>(curveData + (numPoints * sizeof(uint16_t)));

                for (auto& p : curve.points) {
                    uint16_t time = *timingsPtr;

                    uint16_t x = valuesPtr[0];
                    uint16_t y = valuesPtr[1];
                    uint16_t z = valuesPtr[2];

                    if (mVersion == kMVersionRedux && !disableBSwap) {
                        time = _byteswap_ushort(time);
                        x = _byteswap_ushort(x);
                        y = _byteswap_ushort(y);
                        z = _byteswap_ushort(z);
                    }
                    
                    p.time = time * timingScale;
                    p.value.x = x * scale.x + offset.x;
                    p.value.y = y * scale.y + offset.y;
                    p.value.z = z * scale.z + offset.z;

                    p.value = MetroSwizzle(p.value);

                    timingsPtr++;
                    valuesPtr += 3;
                }
            } break;

            case AttribCurveType::CompressedQuat: {
                const float normFactor = 0.0000215805f;
                //const float normFactor = 1.0f / (65535.0f / sqrt(2.0f));

                float timingScale = *rcast<const float*>(curveData);
                if (mVersion == kMVersionRedux && !disableBSwap)
                    timingScale = FloatByteSwap(timingScale);

                timingScale = 1.0f / timingScale;

                curveData += 4;

                const uint16_t* timingsPtr = rcast<const uint16_t*>(curveData);
                const int16_t* valuesPtr = rcast<const int16_t*>(curveData + (numPoints * sizeof(uint16_t)));

                for (auto& p : curve.points) {
                    uint16_t time = *timingsPtr;

                    int16_t qx_val = valuesPtr[0];
                    int16_t qy_val = valuesPtr[1];
                    int16_t qz_val = valuesPtr[2];


                    if (mVersion == kMVersionRedux && !disableBSwap) {
                        time = _byteswap_ushort(time);
                        qx_val = _byteswap_ushort(qx_val);
                        qy_val = _byteswap_ushort(qy_val);
                        qz_val = _byteswap_ushort(qz_val);
                    }

                    const int permutation = (qy_val & 1) | (2 * (qx_val & 1));
                    const int wsign = (qz_val & 1);

                    p.time = time * timingScale;
                    float qx = qx_val * normFactor;
                    float qy = qy_val * normFactor;
                    float qz = qz_val * normFactor;

                    const float t = 1.0f - (qx * qx) - (qy * qy) - (qz * qz);
                    const float qw = (t < 0.0f) ? 0.0f : (wsign ? -std::sqrtf(t) : std::sqrtf(t));

                    switch (permutation) {
                        case 0: p.value = vec4(qw, qx, qy, qz); break;
                        case 1: p.value = vec4(qx, qw, qy, qz); break;
                        case 2: p.value = vec4(qx, qy, qw, qz); break;
                        case 3: p.value = vec4(qx, qy, qz, qw); break;
                    }

                    p.value = MetroSwizzle(p.value);

                    *rcast<quat*>(&p.value) = Normalize(*rcast<quat*>(&p.value));

                    timingsPtr++;
                    valuesPtr += 3;
                }
            } break;
        }
    }
}
