#pragma once

#include <assimp/material.h>
#include <ktm/type_vec.h>

#include <memory>

#include "IResource.h"


namespace Corona {

class Texture final : public IResource {
   public:
    aiTextureType type;             ///< 绾圭悊绫诲瀷 (aiTextureType)
    std::string path;               ///< 绾圭悊鏂囦欢璺緞
    unsigned char* data;            ///< 绾圭悊鏁版嵁鎸囬拡
    int width, height, nrChannels;  ///< 绾圭悊瀹姐€侀珮銆侀€氶亾鏁?
};

class Mesh final : public IResource {
   public:
    ktm::fvec3 minXYZ = ktm::fvec3(0.0f, 0.0f, 0.0f);  ///< 鍖呭洿鐩掓渶灏忕偣
    ktm::fvec3 maxXYZ = ktm::fvec3(0.0f, 0.0f, 0.0f);  ///< 鍖呭洿鐩掓渶澶х偣
    std::vector<uint32_t> Indices;                     ///< 涓夎闈㈢墖椤剁偣绱㈠紩
    std::vector<float> points;                         ///< 椤剁偣鍧愭爣锛堝缁堟寜绱㈠紩瀛樺偍锛?
    std::vector<float> normals;                        ///< 娉曠嚎锛堟寜椤剁偣鎴栨彃鍊兼柟寮忓瓨鍌級
    std::vector<float> texCoords;                      ///< 绾圭悊鍧愭爣锛堟寜椤剁偣鎴栨彃鍊兼柟寮忓瓨鍌級
    std::vector<uint32_t> boneIndices;                 ///< 楠ㄩ绱㈠紩锛堟瘡涓《鐐规渶澶?涓楠硷級
    std::vector<float> boneWeights;                    ///< 楠ㄩ鏉冮噸锛堜笌boneIndices涓€涓€瀵瑰簲锛?
    std::vector<std::shared_ptr<Texture>> textures;    ///< 缃戞牸浣跨敤鐨勬墍鏈夌汗鐞?
};

}  // namespace Corona
