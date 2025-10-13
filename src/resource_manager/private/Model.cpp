#include "Model.h"

#include <iostream>
#include <algorithm>
#include <assimp/Importer.hpp>
#include <assimp/Vertex.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <cctype>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <regex>

namespace Corona
{

    bool ModelLoader::supports(const ResourceId &id) const
    {
        if (id.type == "model")
            return true;
        auto dot = id.path.find_last_of('.');
        if (dot == std::string::npos)
            return false;
        std::string ext = id.path.substr(dot + 1);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        static const char *kExts[] = {"obj", "fbx", "dae", "gltf", "glb"};
        for (auto e : kExts)
            if (ext == e)
                return true;
        return false;
    }

    std::shared_ptr<IResource> ModelLoader::load(const ResourceId &id)
    {
        const std::string &path = id.path;
        auto handle = std::make_shared<Model>();

        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            std::cout << "Assimp Error: " << importer.GetErrorString() << std::endl;
            return nullptr;
        }

        processNode(path, scene->mRootNode, scene, handle);

        handle->skeletalAnimations.reserve(scene->mNumAnimations);
        for (unsigned int i = 0; i < scene->mNumAnimations; i++)
        {
            handle->skeletalAnimations.emplace_back(scene, scene->mAnimations[i], handle->m_BoneInfoMap, handle->m_BoneCounter);
        }

        if (handle->meshes.size() > 0)
        {
            handle->minXYZ = handle->meshes[0].minXYZ;
            handle->maxXYZ = handle->meshes[0].maxXYZ;
        }
        for (size_t i = 0; i < handle->meshes.size(); i++)
        {
            handle->maxXYZ = ktm::fvec3(ktm::max(handle->meshes[i].maxXYZ.x, handle->maxXYZ.x),
                                        ktm::max(handle->meshes[i].maxXYZ.y, handle->maxXYZ.y),
                                        ktm::max(handle->meshes[i].maxXYZ.z, handle->maxXYZ.z));
            handle->minXYZ = ktm::fvec3(ktm::min(handle->meshes[i].minXYZ.x, handle->minXYZ.x),
                                        ktm::min(handle->meshes[i].minXYZ.y, handle->minXYZ.y),
                                        ktm::min(handle->meshes[i].minXYZ.z, handle->minXYZ.z));
        }

        return std::static_pointer_cast<IResource>(handle);
    }

    void ModelLoader::processNode(const std::string &path, const aiNode *node, const aiScene *scene, const ModelPtr &model)
    {
        for (unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
            model->meshes.emplace_back();
            processMesh(path, mesh, scene, model, model->meshes.back());
        }
        for (unsigned int i = 0; i < node->mNumChildren; i++)
        {
            processNode(path, node->mChildren[i], scene, model);
        }
    }

    void ModelLoader::processMesh(const std::string &path, const aiMesh *mesh, const aiScene *scene, const ModelPtr &model, Mesh &resultMesh)
    {
        if (mesh->mNumVertices > 0)
        {
            resultMesh.minXYZ = resultMesh.maxXYZ = ktm::fvec3(mesh->mVertices[0].x, mesh->mVertices[0].y, mesh->mVertices[0].z);
        }

        for (unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            resultMesh.points.push_back(mesh->mVertices[i].x);
            ;
            resultMesh.points.push_back(mesh->mVertices[i].y);
            resultMesh.points.push_back(mesh->mVertices[i].z);

            resultMesh.minXYZ = ktm::fvec3(ktm::min(resultMesh.minXYZ.x, mesh->mVertices[i].x),
                                           ktm::min(resultMesh.minXYZ.y, mesh->mVertices[i].y),
                                           ktm::min(resultMesh.minXYZ.z, mesh->mVertices[i].z));
            resultMesh.maxXYZ = ktm::fvec3(ktm::max(resultMesh.maxXYZ.x, mesh->mVertices[i].x),
                                           ktm::max(resultMesh.maxXYZ.y, mesh->mVertices[i].y),
                                           ktm::max(resultMesh.maxXYZ.z, mesh->mVertices[i].z));

            if (mesh->HasNormals())
            {
                resultMesh.normals.push_back(mesh->mNormals[i].x);
                resultMesh.normals.push_back(mesh->mNormals[i].y);
                resultMesh.normals.push_back(mesh->mNormals[i].z);
            }

            if (mesh->HasTextureCoords(0))
            {
                resultMesh.texCoords.push_back(mesh->mTextureCoords[0][i].x);
                resultMesh.texCoords.push_back(mesh->mTextureCoords[0][i].y);
            }
            else
            {
                resultMesh.texCoords.push_back(0.0f);
                resultMesh.texCoords.push_back(0.0f);
            }

            resultMesh.boneIndices.push_back(-1);
            resultMesh.boneIndices.push_back(-1);
            resultMesh.boneIndices.push_back(-1);
            resultMesh.boneIndices.push_back(-1);

            resultMesh.boneWeights.push_back(0.0f);
            resultMesh.boneWeights.push_back(0.0f);
            resultMesh.boneWeights.push_back(0.0f);
            resultMesh.boneWeights.push_back(0.0f);
        }

        extractBoneWeightForVertices(resultMesh, mesh, scene, model);

        for (unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                resultMesh.Indices.push_back(face.mIndices[j]);
        }

        if (mesh->mMaterialIndex >= 0)
            loadMaterial(path, scene->mMaterials[mesh->mMaterialIndex], resultMesh);
    }

    void ModelLoader::extractBoneWeightForVertices(Mesh &resultMesh, const aiMesh *mesh, const aiScene *scene, const ModelPtr &model)
    {
        auto &boneInfoMap = model->m_BoneInfoMap;
        int &boneCount = model->m_BoneCounter;

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

        for (uint32_t boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex)
        {
            int boneID;
            if (std::string boneName = mesh->mBones[boneIndex]->mName.C_Str(); !boneInfoMap.contains(boneName))
            {
                auto boneInfo = std::make_shared<BoneInfo>();
                boneInfo->ID = boneCount;
                boneInfo->Name = boneName;
                boneInfo->OffsetMatrix = ConvertMatrixToGLMFormat(mesh->mBones[boneIndex]->mOffsetMatrix);
                boneInfoMap[boneName] = boneInfo;
                boneID = boneCount;
                boneCount++;
            }
            else
            {
                boneID = boneInfoMap[boneName]->ID;
            }
            auto weights = mesh->mBones[boneIndex]->mWeights;
            int numWeights = mesh->mBones[boneIndex]->mNumWeights;

            for (int weightIndex = 0; weightIndex < numWeights; ++weightIndex)
            {
                int vertexId = weights[weightIndex].mVertexId;
                float weight = weights[weightIndex].mWeight;

                for (int i = 0; i < 4; ++i)
                {
                    if (resultMesh.boneIndices[vertexId * 4 + i] == -1 && resultMesh.boneWeights[vertexId * 4 + i] <= 1e-3)
                    {
                        resultMesh.boneWeights[vertexId * 4 + i] = weight;
                        resultMesh.boneIndices[vertexId * 4 + i] = boneID;
                        break;
                    }
                }
            }
        }
    }

    void ModelLoader::loadMaterial(const std::string &path, const aiMaterial *material, Mesh &resultMesh)
    {
        std::string meshRootPath = "";
        std::regex rexPattern("(.*)((\\\\)|(/))");
        if (std::smatch matchPath; regex_search(path, matchPath, rexPattern))
        {
            meshRootPath = matchPath[1].str();
        }
        else
        {
            throw std::runtime_error("mesh path invalid in python");
        }
        std::string directory = meshRootPath;

        std::vector<aiTextureType> allTextureTypes = {
            aiTextureType_BASE_COLOR,
            aiTextureType_DIFFUSE,
            aiTextureType_SPECULAR,
            aiTextureType_EMISSIVE,
            aiTextureType_EMISSION_COLOR};

        for (size_t index = 0; index < allTextureTypes.size(); index++)
        {
            std::vector<std::shared_ptr<Texture>> textures;
            for (unsigned int i = 0; i < material->GetTextureCount(allTextureTypes[index]); i++)
            {
                aiString str;
                material->GetTexture(allTextureTypes[index], i, &str);

                std::string texturePath = directory + "\\" + str.C_Str();

                std::shared_ptr<Texture> texture;
                {
                    std::lock_guard<std::mutex> lk(textureMutex);
                    if (auto it = texturePathHash.find(texturePath); it != texturePathHash.end())
                    {
                        texture = it->second;
                    }
                    else
                    {
                        // 直接在锁内加载以保证并发安全（可优化为双检并在锁外加载）
                        texture = std::make_shared<Texture>();
                        texture->type = allTextureTypes[index];
                        texture->path = texturePath;
                        texture->data = stbi_load(texturePath.c_str(), &texture->width, &texture->height, &texture->nrChannels, 0);
                        texturePathHash[texturePath] = texture;
                    }
                }
                textures.push_back(texture);
            }

            resultMesh.textures.insert(resultMesh.textures.end(), textures.begin(), textures.end());
        }

        aiColor3D baseColor(0.f, 0.f, 0.f);
        if (material->Get(AI_MATKEY_BASE_COLOR, baseColor) == aiReturn_SUCCESS)
        {
            std::string tempTexturePath = directory + std::to_string(++attributeToImageIndex);

            std::lock_guard<std::mutex> lk(textureMutex);
            if (!texturePathHash.contains(tempTexturePath))
            {
                auto tempTexture = std::make_shared<Texture>();
                tempTexture->type = aiTextureType_BASE_COLOR;
                tempTexture->width = 1;
                tempTexture->height = 1;
                tempTexture->nrChannels = 4;
                tempTexture->data = static_cast<unsigned char *>(malloc(sizeof(unsigned char) * 4));
                tempTexture->data[0] = static_cast<unsigned char>(baseColor[0] * 255.0);
                tempTexture->data[1] = static_cast<unsigned char>(baseColor[1] * 255.0);
                tempTexture->data[2] = static_cast<unsigned char>(baseColor[2] * 255.0);
                tempTexture->data[3] = 255;

                resultMesh.textures.push_back(tempTexture);
                texturePathHash[tempTexturePath] = tempTexture;
            }
        }

        aiColor3D specColor(0.f, 0.f, 0.f);
        if (material->Get(AI_MATKEY_COLOR_SPECULAR, specColor) == aiReturn_SUCCESS)
        {
            std::string tempTexturePath = directory + std::to_string(++attributeToImageIndex);

            std::lock_guard<std::mutex> lk(textureMutex);
            if (!texturePathHash.contains(tempTexturePath))
            {
                auto tempTexture = std::make_shared<Texture>();
                tempTexture->type = aiTextureType_SPECULAR;
                tempTexture->width = 1;
                tempTexture->height = 1;
                tempTexture->nrChannels = 4;
                tempTexture->data = static_cast<unsigned char *>(malloc(sizeof(unsigned char) * 4));
                tempTexture->data[0] = static_cast<unsigned char>(specColor[0] * 255.0);
                tempTexture->data[1] = static_cast<unsigned char>(specColor[1] * 255.0);
                tempTexture->data[2] = static_cast<unsigned char>(specColor[2] * 255.0);
                tempTexture->data[3] = 255;

                resultMesh.textures.push_back(tempTexture);
                texturePathHash[tempTexturePath] = tempTexture;
            }
        }

        aiColor3D diffuseColor(0.f, 0.f, 0.f);
        if (material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor) == aiReturn_SUCCESS)
        {
            std::string tempTexturePath = directory + std::to_string(++attributeToImageIndex);

            std::lock_guard<std::mutex> lk(textureMutex);
            if (!texturePathHash.contains(tempTexturePath))
            {
                auto tempTexture = std::make_shared<Texture>();
                tempTexture->type = aiTextureType_DIFFUSE;
                tempTexture->width = 1;
                tempTexture->height = 1;
                tempTexture->nrChannels = 4;
                tempTexture->data = static_cast<unsigned char *>(malloc(sizeof(unsigned char) * 4));
                tempTexture->data[0] = static_cast<unsigned char>(diffuseColor[0] * 255.0);
                tempTexture->data[1] = static_cast<unsigned char>(diffuseColor[1] * 255.0);
                tempTexture->data[2] = static_cast<unsigned char>(diffuseColor[2] * 255.0);
                tempTexture->data[3] = 255;

                resultMesh.textures.push_back(tempTexture);
                texturePathHash[tempTexturePath] = tempTexture;
            }
        }

        aiColor3D emissiveColor(0.f, 0.f, 0.f);
        if (material->Get(AI_MATKEY_COLOR_EMISSIVE, emissiveColor) == aiReturn_SUCCESS)
        {
            std::string tempTexturePath = directory + std::to_string(++attributeToImageIndex);

            std::lock_guard<std::mutex> lk(textureMutex);
            if (!texturePathHash.contains(tempTexturePath))
            {
                auto tempTexture = std::make_shared<Texture>();
                tempTexture->type = aiTextureType_EMISSIVE;
                tempTexture->width = 1;
                tempTexture->height = 1;
                tempTexture->nrChannels = 4;
                tempTexture->data = static_cast<unsigned char *>(malloc(sizeof(unsigned char) * 4));
                tempTexture->data[0] = static_cast<unsigned char>(emissiveColor[0] * 255.0);
                tempTexture->data[1] = static_cast<unsigned char>(emissiveColor[1] * 255.0);
                tempTexture->data[2] = static_cast<unsigned char>(emissiveColor[2] * 255.0);
                tempTexture->data[3] = 255;

                resultMesh.textures.push_back(tempTexture);
                texturePathHash[tempTexturePath] = tempTexture;
            }
        }

        // 读取金属度、粗糙度等参数
        float Metallic = 0.0f;
        material->Get(AI_MATKEY_METALLIC_FACTOR, Metallic);

        float Roughness = 0.0f;
        material->Get(AI_MATKEY_ROUGHNESS_FACTOR, Roughness);

        float Specular = 0.0f;
        material->Get(AI_MATKEY_SPECULAR_FACTOR, Specular);

        float Glossiness = 0.0;
        material->Get(AI_MATKEY_GLOSSINESS_FACTOR, Glossiness);
    }

} // namespace Corona