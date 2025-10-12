#pragma once
#include <string>
#include <vector>
#include <array>
#include <unordered_map>

// Minimal IR structures for PBRT importer skeleton.
// Extended design documented in pbrt_porting_plan.txt
namespace vision::pbrt {

struct ParamValue {
    enum class Kind { Float, Int, Bool, String, Unknown } kind = Kind::Unknown;
    std::string name;
    std::vector<float> floats;
    std::vector<int> ints;
    std::vector<bool> bools;
    std::vector<std::string> strings;
};

struct BlockNode { std::string type; std::vector<ParamValue> params; };
struct MaterialIR : BlockNode { std::string name; };
struct LightIR : BlockNode { bool area = false; };

struct ShapeIR {
    std::string type;
    std::vector<ParamValue> params;
    std::array<float, 16> transform{ {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1} };
    std::string materialRef;
};

struct InstanceIR { std::string prototype; std::array<float,16> transform{}; };
struct PrototypeIR { std::string name; std::vector<ShapeIR> shapes; };

struct ParseError { std::string code; std::string message; std::string loc; };

struct SceneIR {
    std::string version = "0.1";
    std::string source;
    std::vector<BlockNode> cameras;
    std::vector<BlockNode> films;
    BlockNode sampler; bool hasSampler = false;
    BlockNode integrator; bool hasIntegrator = false;
    BlockNode accel; bool hasAccel = false;
    std::vector<MaterialIR> materials;
    std::vector<LightIR> lights;
    std::vector<ShapeIR> shapes;
    std::vector<PrototypeIR> prototypes;
    std::vector<InstanceIR> instances;
    std::vector<ParseError> errors;
};

} // namespace vision::pbrt

