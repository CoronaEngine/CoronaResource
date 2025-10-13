#pragma once
#include <IResource.h>
#include <IResourceLoader.h>
#include <ResourceTypes.h>

namespace Corona
{

    class Shader final : public IResource
    {
      public:
        std::string vertCode;
        std::string fragCode;
        std::string computeCode;
    };

    class ShaderLoader final : public IResourceLoader
    {
      public:
        bool supports(const ResourceId &id) const override;
        std::shared_ptr<IResource> load(const ResourceId &id) override;

      private:
        std::string readStringFile(const std::string_view &path);
    };

} // namespace Corona
