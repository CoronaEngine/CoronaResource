#include "PBRTImporter.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cctype>

namespace vision::pbrt {

static bool isDirective(const std::string& t) {
    // Minimal recognized set; extend later
    static const char* kDirs[] = {
        "Camera", "Film", "Sampler", "Integrator", "Accelerator",
        "WorldBegin", "WorldEnd", "Shape", "LightSource", "AreaLightSource",
        "Material", "MakeNamedMaterial", "NamedMaterial", "AttributeBegin", "AttributeEnd",
        "Transform", "ConcatTransform", "Scale", "Rotate", "Translate", "LookAt",
        "ObjectBegin", "ObjectEnd", "ObjectInstance", "Texture", "Include" };
    for (auto* d : kDirs) if (t == d) return true;
    return false;
}

SceneIR PBRTImporter::ParseFile(const std::filesystem::path& file, const ImportOptions& opt) {
    std::ifstream ifs(file, std::ios::binary);
    if (!ifs) {
        SceneIR scene; scene.errors.push_back({"ERR_OPEN_FILE", "Cannot open file: " + file.string(), ""});
        return scene;
    }
    std::ostringstream oss; oss << ifs.rdbuf();
    std::string content = oss.str();
    SimpleTokenizer tokenizer(content);
    return parse(tokenizer, file.string(), opt);
}

SceneIR PBRTImporter::ParseString(const std::string& content, const std::string& virtualName, const ImportOptions& opt) {
    SimpleTokenizer tokenizer(content);
    return parse(tokenizer, virtualName, opt);
}

SceneIR PBRTImporter::parse(SimpleTokenizer& tokenizer, const std::string& sourceName, const ImportOptions& /*opt*/) {
    SceneIR scene; scene.source = sourceName;
    bool inWorld = false;

    std::optional<Token> tok;
    while ((tok = tokenizer.next())) {
        if (tok->text.empty()) continue;
        const std::string& word = tok->text;
        if (word == "WorldBegin") { inWorld = true; continue; }
        if (word == "WorldEnd") { inWorld = false; continue; }
        if (word == "Camera") {
            // Next token assumed to be quoted type or plain type; collect params until next directive
            std::vector<ParamValue> params; // placeholder (enhance later)
            BlockNode cam; cam.type = "perspective"; // default placeholder
            cam.params = std::move(params);
            scene.cameras.push_back(std::move(cam));
            continue;
        }
        if (word == "Film") {
            BlockNode film; film.type = "image"; scene.films.push_back(std::move(film));
            continue;
        }
        if (word == "Shape") {
            ShapeIR shape; shape.type = "unknown";
            // Simple param scan: gather raw tokens until bracket-balanced or next directive
            std::optional<Token> next;
            while ((next = tokenizer.next())) {
                if (isDirective(next->text)) { /* push back? simplistic: cannot unget -> stop */ break; }
                if (next->text == "[") continue; // ignore arrays for now
                if (next->text == "]") continue;
                // Very naive: treat numeric as float param, others as string param
                ParamValue p;
                bool numeric = !next->text.empty() && (std::isdigit((unsigned char)next->text[0]) || next->text[0]=='-' || next->text[0]=='+');
                if (numeric) { p.kind = ParamValue::Kind::Float; p.name = "_val"; p.floats.push_back(std::strtof(next->text.c_str(), nullptr)); }
                else { p.kind = ParamValue::Kind::String; p.name = "_token"; p.strings.push_back(next->text); if (shape.type == "unknown") shape.type = next->text; }
                shape.params.push_back(std::move(p));
            }
            scene.shapes.push_back(std::move(shape));
            continue;
        }
        // Unhandled directive placeholder
    }

    if (inWorld) {
        scene.errors.push_back({"ERR_UNCLOSED_WORLD", "WorldBegin without WorldEnd", sourceName});
    }
    return scene;
}

} // namespace vision::pbrt

