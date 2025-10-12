#include "Tokenizer.h"
#include <cctype>
#include <string>

namespace vision { namespace pbrt {

void SimpleTokenizer::skipWhitespace() {
    while (true) {
        char c = peek();
        if (c == '\0') return;
        if (c == ' ' || c == '\t' || c == '\r') { get(); continue; }
        if (c == '\n') { get(); line_++; col_ = 1; continue; }
        if (c == '#') { // comment until end line
            while (c != '\0' && c != '\n') { c = get(); }
            continue;
        }
        break;
    }
}

char SimpleTokenizer::get() {
    if (pos_ >= source_.size()) return '\0';
    char c = source_[pos_++];
    col_++;
    return c;
}

std::optional<Token> SimpleTokenizer::next() {
    skipWhitespace();
    char c = peek();
    if (c == '\0') return std::nullopt;
    Token t; t.line = line_; t.column = col_;
    if (c == '"') {
        get(); // consume opening quote
        std::string accum;
        while (true) {
            char k = get();
            if (k == '\0' || k == '\n') break; // unterminated -> return what we have
            if (k == '"') break;
            if (k == '\\') { // escape
                char esc = get();
                if (esc == 'n') accum.push_back('\n');
                else if (esc == 't') accum.push_back('\t');
                else if (esc == '\\') accum.push_back('\\');
                else accum.push_back(esc);
            } else accum.push_back(k);
        }
        t.text = accum;
        return t;
    }
    if (c == '[' || c == ']') { t.text = std::string(1, get()); return t; }

    std::string accum;
    while (true) {
        c = peek();
        if (c == '\0' || std::isspace(static_cast<unsigned char>(c)) || c=='"' || c=='[' || c==']' || c=='#') break;
        accum.push_back(get());
    }
    t.text = accum;
    return t;
}

} } // namespace vision::pbrt
