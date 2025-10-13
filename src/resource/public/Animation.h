#pragma once
#include "Bone.h"
#include "IResource.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

// 鍓嶇疆澹版槑锛氶伩鍏嶅湪鍏紑澶翠腑寮曞叆閲嶉噺绾уご鏂囦欢
struct aiScene;
struct aiAnimation;
struct aiNode;

#include <map>
#include <string>

namespace Corona
{

    class Animation final : public IResource
    {
      public:
        struct AssimpNodeData
        {
            ktm::fmat4x4 transformation;          // 鑺傜偣鍙樻崲鐭╅樀
            std::string name;                     // 鑺傜偣鍚嶇О
            int childrenCount;                    // 瀛愯妭鐐规暟閲?
            std::vector<AssimpNodeData> children; // 瀛愯妭鐐规暟鎹?
        };

        Animation() = default;
        ~Animation() = default;

        Animation(const aiScene *scene, const aiAnimation *animation, std::map<std::string, std::shared_ptr<BoneInfo>> boneInfoMap, int &boneCount);
        void readHeirarchyData(AssimpNodeData &dest, const aiNode *src);

        double m_Duration;                                            // 鍔ㄧ敾鎸佺画鏃堕棿
        double m_TicksPerSecond;                                      // 姣忕閲囨牱鏁?
        std::vector<Bone> m_Bones;                                    // 鍔ㄧ敾娑夊強鐨勬墍鏈夐楠?
        AssimpNodeData m_RootNode;                                    // 鏍硅妭鐐规暟鎹?
        std::map<std::string, std::shared_ptr<BoneInfo>> boneInfoMap; // 楠ㄩ淇℃伅鏄犲皠琛?
    };

} // namespace Corona
