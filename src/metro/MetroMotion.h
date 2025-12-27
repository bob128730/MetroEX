#pragma once
#include "MetroTypes.h"
#include "MetroSkeleton.h"

struct AttributeCurve {
    struct AttribPoint {
        float   time;
        vec4    value;
    };

    MyArray<AttribPoint> points;
};

class MetroMotion {
public:
    static const size_t kFrameRate = 30;
    static const size_t kMVersionRedux = 0xF;
public:
    MetroMotion(const CharString& name = "", MetroSkeleton *skeleton = nullptr);
    ~MetroMotion();

    bool                    LoadHeader(MemStream& stream);
    bool                    LoadFromData(MemStream& stream);

    const CharString&       GetName() const;

    size_t                  GetBonesCRC() const;
    size_t                  GetNumBones() const;
    size_t                  GetNumLocators() const;
    size_t                  GetNumFrames() const;
    float                   GetMotionTimeInSeconds() const;

    bool                    IsBoneAnimated(const size_t boneIdx) const;
    quat                    GetBoneRotation(const size_t boneIdx, const size_t key) const;
    vec3                    GetBonePosition(const size_t boneIdx, const size_t key) const;
    vec3                    GetBoneScale(const size_t boneIdx, const size_t key) const;

//private:
    bool                    LoadInternal();
    void                    ReadAttributeCurve(const uint8_t* curveData, AttributeCurve& curve, const size_t attribSize, bool disableBSwap, bool smooth);

//private:
    CharString              mName;

    // header
    size_t                  mVersion;
    size_t                  mBonesCRC;
    size_t                  mNumBones;
    size_t                  mNumLocators;
    // info
    size_t                  mFlags;
    float                   mSpeed;
    float                   mAccrue;
    float                   mFalloff;
    size_t                  mNumFrames;
    size_t                  mJumpFrame;
    size_t                  mLandFrame;
    Bitset256               mAffectedBones;
    Bitset128               mAffectedBones_Redux;
    size_t                  mMotionsDataSize;
    size_t                  mMotionsOffsetsSize;
    Bitset256               mHighQualityBones;
    Bitset128               mHighQualityBones_Redux;
    MetroSkeleton           *mSkeleton;
    // data
    BytesArray              mMotionsData;
    // curves
    MyArray<AttributeCurve> mBonesRotations;
    MyArray<AttributeCurve> mBonesPositions;
    MyArray<AttributeCurve> mBonesScales;
};
