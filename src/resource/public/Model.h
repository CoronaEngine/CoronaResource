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
        std::vector<Mesh> meshes;
        std::map<std::string, std::shared_ptr<BoneInfo>> m_BoneInfoMap;
        int m_BoneCounter = 0;
        std::vector<Animation> skeletalAnimations;
        ktm::fvec3 minXYZ{0.0f, 0.0f, 0.0f};
        ktm::fvec3 maxXYZ{0.0f, 0.0f, 0.0f};
        ktm::fvec3 positon{0.0f, 0.0f, 0.0f};
        ktm::fvec3 rotation{0.0f, 0.0f, 0.0f};
        ktm::fvec3 scale{1.0f, 1.0f, 1.0f};
        ktm::fmat4x4 modelMatrix = ktm::fmat4x4::from_eye();

        void getModelMatrix()
        {
            modelMatrix = ktm::fmat4x4(ktm::translate3d(positon) * ktm::translate3d(rotation) * ktm::translate3d(scale));
        };
    };

    class ModelLoader : public IResourceLoader
    {
      public:
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
