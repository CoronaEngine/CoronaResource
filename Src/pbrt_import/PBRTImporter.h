#pragma once
#include "SceneIR.h"
#include "Tokenizer.h"
#include <string>
#include <filesystem>

namespace vision::pbrt {

struct ImportOptions {
    bool strict = true;
    bool enableInclude = true;
    size_t maxIncludeDepth = 32;
};

class PBRTImporter {
public:
    SceneIR ParseFile(const std::filesystem::path& file, const ImportOptions& opt = {});
    SceneIR ParseString(const std::string& content, const std::string& virtualName = "<memory>", const ImportOptions& opt = {});
private:
    SceneIR parse(SimpleTokenizer& tokenizer, const std::string& sourceName, const ImportOptions& opt);
};

} // namespace vision::pbrt

