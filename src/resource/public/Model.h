#pragma once

#include "Animation.h"
#include "Bone.h"
#include <IResource.h>
#include <IResourceLoader.h>
#include <ResourceTypes.h>

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <atomic>

// forward decls for Assimp types used in declarations
struct aiMesh;
struct aiScene;
struct aiNode;
struct aiMaterial;
#include "Mesh.h"

namespace Corona
{
    class Model final : public IResource
    {
      public:
        std::vector<Mesh> meshes;                                       ///< 鎵€鏈夌綉鏍?
        std::map<std::string, std::shared_ptr<BoneInfo>> m_BoneInfoMap; ///< 楠ㄩ淇℃伅鏄犲皠琛?
        int m_BoneCounter = 0;                                          ///< 楠ㄩ璁℃暟鍣?
        std::vector<Animation> skeletalAnimations;                      ///< 鎵€鏈夐楠煎姩鐢?
        ktm::fvec3 minXYZ{0.0f, 0.0f, 0.0f};               ///< 妯″瀷鍖呭洿鐩掓渶灏忕偣
        ktm::fvec3 maxXYZ{0.0f, 0.0f, 0.0f};               ///< 妯″瀷鍖呭洿鐩掓渶澶х偣
        ktm::fvec3 positon{0.0f, 0.0f, 0.0f};                           ///< 浣嶇疆
        ktm::fvec3 rotation{0.0f, 0.0f, 0.0f};                          ///< 鏃嬭浆
        ktm::fvec3 scale{1.0f, 1.0f, 1.0f};                            ///< 缂╂斁
        ktm::fmat4x4 modelMatrix = ktm::fmat4x4::from_eye();                          ///< 妯″瀷鐭╅樀

        void getModelMatrix()
        {
            modelMatrix = ktm::fmat4x4(ktm::translate3d(positon) * ktm::translate3d(rotation) * ktm::translate3d(scale));
        };
    };

    class ModelLoader : public IResourceLoader
    {
      public:
        // 鏀寔绫诲瀷锛歵ype=="model" 鎴?鎵╁睍鍚嶅父瑙佷笁缁存牸寮?
        bool supports(const ResourceId &id) const override;
        std::shared_ptr<IResource> load(const ResourceId &id) override;

      private:
        using ModelPtr = std::shared_ptr<Model>;
        void processMesh(const std::string &path, const aiMesh *mesh, const aiScene *scene, const ModelPtr &model, Mesh &resultMesh);
        void processNode(const std::string &path, const aiNode *node, const aiScene *scene, const ModelPtr &model);
        void loadMaterial(const std::string &path, const aiMaterial *material, Mesh &resultMesh);
        void extractBoneWeightForVertices(Mesh &resultMesh, const aiMesh *mesh, const aiScene *scene, const ModelPtr &model);
        std::mutex textureMutex;
        std::unordered_map<std::string, std::shared_ptr<Texture>> texturePathHash;
        std::atomic<uint32_t> attributeToImageIndex = 0;
    };
} // namespace Corona
