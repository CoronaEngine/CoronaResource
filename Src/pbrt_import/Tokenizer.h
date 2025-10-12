#pragma once
#include <string>
#include <string_view>
#include <optional>
#include <vector>

namespace vision::pbrt {

struct Token {
    std::string text;
    int line = 1;
    int column = 1;
};

// A deliberately minimal tokenizer for initial port scaffolding.
class SimpleTokenizer {
public:
    explicit SimpleTokenizer(std::string src) : source_(std::move(src)) {}

    std::optional<Token> next();

private:
    char peek() const { return pos_ < source_.size() ? source_[pos_] : '\0'; }
    char get();
    void skipWhitespace();

    std::string source_;
    size_t pos_ = 0;
    int line_ = 1;
    int col_ = 1;
};

} // namespace vision::pbrt

