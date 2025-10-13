#include "Animation.h"

#include <assimp/anim.h>
#include <assimp/scene.h>

namespace Corona
{
    Animation::Animation(const aiScene *scene, const aiAnimation *animation, std::map<std::string, std::shared_ptr<BoneInfo>> boneInfoMap, int &boneCount)
        : boneInfoMap(boneInfoMap)
    {
        m_Duration = animation->mDuration;
        m_TicksPerSecond = animation->mTicksPerSecond;

        // 读取动画中所有骨骼通道
        auto readMissingBones = [&](const aiAnimation *anim, std::map<std::string, std::shared_ptr<BoneInfo>> &boneInfoMap, int &boneCount) {
            for (uint32_t i = 0; i < anim->mNumChannels; i++)
            {
                auto channel = anim->mChannels[i];
                std::string boneName = channel->mNodeName.data;
                if (!boneInfoMap.contains(boneName))
                {
                    boneInfoMap[boneName] = std::make_shared<BoneInfo>();
                    boneInfoMap[boneName]->ID = boneCount;
                    boneCount++;
                }
                m_Bones.emplace_back(channel->mNodeName.data, boneInfoMap[channel->mNodeName.data]->ID, channel);
            }
        };

        readHeirarchyData(m_RootNode, scene->mRootNode);
        readMissingBones(animation, boneInfoMap, boneCount);
    }

    void Animation::readHeirarchyData(AssimpNodeData &dest, const aiNode *src)
    {
        auto ConvertMatrixToGLMFormat = [](const aiMatrix4x4 &from) -> ktm::fmat4x4 {
            ktm::fmat4x4 to;
            to[0][0] = from.a1;
            to[1][0] = from.a2;
            to[2][0] = from.a3;
            to[3][0] = from.a4;
            to[0][1] = from.b1;
            to[1][1] = from.b2;
            to[2][1] = from.b3;
            to[3][1] = from.b4;
            to[0][2] = from.c1;
            to[1][2] = from.c2;
            to[2][2] = from.c3;
            to[3][2] = from.c4;
            to[0][3] = from.d1;
            to[1][3] = from.d2;
            to[2][3] = from.d3;
            to[3][3] = from.d4;
            return to;
        };

        dest.name = src->mName.data;
        dest.transformation = ConvertMatrixToGLMFormat(src->mTransformation);
        dest.childrenCount = src->mNumChildren;

        for (uint32_t i = 0; i < src->mNumChildren; i++)
        {
            AssimpNodeData newData;
            readHeirarchyData(newData, src->mChildren[i]);
            dest.children.push_back(newData);
        }
    }
} // namespace Corona