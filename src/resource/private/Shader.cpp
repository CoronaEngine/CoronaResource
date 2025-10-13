#include "Shader.h"

#include <fstream>
#include <sstream>
#include <string>

namespace Corona
{

    bool ShaderLoader::supports(const ResourceId &id) const
    {
        if (id.type == "shader")
            return true;
        // 简单判断：路径包含"/shaders"目录
        return id.path.find("/shaders") != std::string::npos || id.path.find("\\shaders") != std::string::npos;
    }

    std::shared_ptr<IResource> ShaderLoader::load(const ResourceId &id)
    {
        auto shader = std::make_shared<Shader>();
        const std::string &root = id.path;
        try
        {
            shader->vertCode = readStringFile(root + "/shaders/test.vert.glsl");
            shader->fragCode = readStringFile(root + "/shaders/test.frag.glsl");
            shader->computeCode = readStringFile(root + "/shaders/test.comp.glsl");
        }
        catch (...)
        {
            return nullptr;
        }
        return shader;
    }

    std::string ShaderLoader::readStringFile(const std::string_view &path)
    {
        std::ifstream file(path.data());
        if (!file.is_open())
        {
            throw std::runtime_error("Could not open the file.");
        }

        std::stringstream buffer;
        buffer << file.rdbuf();

        file.close();
        return buffer.str();
    }
} // namespace Corona