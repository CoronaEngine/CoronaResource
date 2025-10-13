#include "Bone.h"
#include <assimp/anim.h>

namespace Corona
{
    Bone::Bone(const std::string &name, const int ID, const aiNodeAnim *channel) : m_Name(name), m_ID(ID)
    {
        // 读取位置关键帧
        m_NumPositions = channel->mNumPositionKeys;
        for (int positionIndex = 0; positionIndex < m_NumPositions; ++positionIndex)
        {
            aiVector3D aiPosition = channel->mPositionKeys[positionIndex].mValue;
            double timeStamp = channel->mPositionKeys[positionIndex].mTime;
            KeyPosition data;
            data.position = ktm::fvec3(aiPosition[0], aiPosition[1], aiPosition[2]);
            data.timeStamp = static_cast<float>(timeStamp);
            m_Positions.push_back(data);
        }

        // 读取旋转关键帧
        m_NumRotations = channel->mNumRotationKeys;
        for (int rotationIndex = 0; rotationIndex < m_NumRotations; ++rotationIndex)
        {
            aiQuaternion aiOrientation = channel->mRotationKeys[rotationIndex].mValue;
            double timeStamp = channel->mRotationKeys[rotationIndex].mTime;
            KeyRotation data;
            data.orientation = ktm::fquat(aiOrientation.x, aiOrientation.y, aiOrientation.z, aiOrientation.w);
            data.timeStamp = static_cast<float>(timeStamp);
            m_Rotations.push_back(data);
        }

        // 读取缩放关键帧
        m_NumScales = channel->mNumScalingKeys;
        for (int keyIndex = 0; keyIndex < m_NumScales; ++keyIndex)
        {
            aiVector3D scale = channel->mScalingKeys[keyIndex].mValue;
            double timeStamp = channel->mScalingKeys[keyIndex].mTime;
            KeyScale data;
            data.scale = ktm::fvec3(scale[0], scale[1], scale[2]);
            data.timeStamp = static_cast<float>(timeStamp);
            m_Scales.push_back(data);
        }
    }
} // namespace Corona