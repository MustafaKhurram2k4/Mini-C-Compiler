#include "lexer.h"
#include <cctype>

std::vector<Token> Lexer::tokenize(const std::string& source) {
    std::vector<Token> tokens;
    size_t i = 0;
    int current_line = 1;

    while (i < source.length()) {
        if (source[i] == '\n') { current_line++; i++; continue; }
        if (std::isspace(source[i])) { i++; continue; }

        // Skip comments
        if (source[i] == '/' && i + 1 < source.length() && source[i+1] == '/') {
            while (i < source.length() && source[i] != '\n') i++;
            continue;
        }

        if (std::isalpha(source[i])) {
            std::string val = "";
            while (i < source.length() && (std::isalnum(source[i]) || source[i] == '_')) { 
                val += source[i++]; 
            }
            if (val == "int" || val == "if" || val == "else" || val == "while") {
                tokens.push_back({KEYWORD, val, "KEYWORD", current_line});
            } else {
                tokens.push_back({IDENTIFIER, val, "IDENTIFIER", current_line});
            }
            continue;
        }

        if (std::isdigit(source[i])) {
            std::string val = "";
            while (i < source.length() && std::isdigit(source[i])) val += source[i++];
            tokens.push_back({NUMBER, val, "NUMBER", current_line});
            continue;
        }

        // Relational Operators (==, <=, >=, !=)
        if ((source[i] == '=' || source[i] == '<' || source[i] == '>' || source[i] == '!') && 
            i + 1 < source.length() && source[i+1] == '=') {
            tokens.push_back({REL_OP, std::string(1, source[i]) + "=", "REL_OP", current_line});
            i += 2; continue;
        }

        // Single Operators & Braces
        char c = source[i];
        if (c == '=' || c == '+' || c == '-' || c == '*' || c == '/') {
            tokens.push_back({OPERATOR, std::string(1, c), "OPERATOR", current_line});
        } else if (c == '<' || c == '>') {
            tokens.push_back({REL_OP, std::string(1, c), "REL_OP", current_line});
        } else if (c == '(') { tokens.push_back({LPAREN, "(", "LPAREN", current_line});
        } else if (c == ')') { tokens.push_back({RPAREN, ")", "RPAREN", current_line});
        } else if (c == '{') { tokens.push_back({LBRACE, "{", "LBRACE", current_line});
        } else if (c == '}') { tokens.push_back({RBRACE, "}", "RBRACE", current_line});
        } else if (c == ';') { tokens.push_back({SYMBOL, ";", "SYMBOL", current_line});
        } else {
            tokens.push_back({UNKNOWN, std::string(1, c), "UNKNOWN", current_line});
        }
        i++;
    }
    tokens.push_back({END_OF_FILE, "", "EOF", current_line});
    return tokens;
}