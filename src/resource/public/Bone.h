#pragma once
#include <IResource.h>
// forward declare to avoid heavy include in header
struct aiNodeAnim;
// 闇€瑕佷娇鐢?ktm 鐨勬暟瀛︾被鍨嬪畾涔夛紙fvec3/fquat/fmat4x4锛?
#include "ktm/type_mat.h"
#include "ktm/type_quat.h"
#include "ktm/type_vec.h"
#include <vector>

#include <string>

namespace Corona
{
    class Bone final : public IResource
    {
      public:
        Bone() = default;
        ~Bone() = default;

        Bone(const std::string &name, const int ID, const aiNodeAnim *channel);

        struct KeyPosition
        {
            ktm::fvec3 position; // 鍏抽敭甯т綅缃?
            float timeStamp;     // 鏃堕棿鎴?
        };
        struct KeyRotation
        {
            ktm::fquat orientation; // 鍏抽敭甯ф棆杞紙鍥涘厓鏁帮級
            float timeStamp;        // 鏃堕棿鎴?
        };
        struct KeyScale
        {
            ktm::fvec3 scale; // 鍏抽敭甯х缉鏀?
            float timeStamp;  // 鏃堕棿鎴?
        };

        std::vector<KeyPosition> m_Positions; // 鎵€鏈変綅缃叧閿抚
        std::vector<KeyRotation> m_Rotations; // 鎵€鏈夋棆杞叧閿抚
        std::vector<KeyScale> m_Scales;       // 鎵€鏈夌缉鏀惧叧閿抚
        int m_NumPositions;                   // 浣嶇疆鍏抽敭甯ф暟閲?
        int m_NumRotations;                   // 鏃嬭浆鍏抽敭甯ф暟閲?
        int m_NumScales;                      // 缂╂斁鍏抽敭甯ф暟閲?
        std::string m_Name;                   // 楠ㄩ鍚嶇О
        int m_ID;                             // 楠ㄩID
    };

    class BoneInfo final : public IResource
    {
      public:
        int ID;                    // 楠ㄩID
        std::string Name;          // 楠ㄩ鍚嶇О
        ktm::fmat4x4 OffsetMatrix; // 缁戝畾濮挎€佸埌楠ㄩ绌洪棿鐨勫亸绉荤煩闃?
    };

} // namespace Corona
