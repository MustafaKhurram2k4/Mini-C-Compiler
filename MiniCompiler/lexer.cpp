#include "lexer.h"
#include <cctype>
using namespace std;

vector<Token> Lexer::tokenize(const string& source) {
    vector<Token> tokens;
    size_t i = 0;
    int current_line = 1;

    while (i < source.length()) {
        if (source[i] == '\n') { current_line++; i++; continue; }
        if (isspace(source[i])) { i++; continue; }

        // Ignore comments starting with //
        if (source[i] == '/' && i + 1 < source.length() && source[i+1] == '/') {
            while (i < source.length() && source[i] != '\n') i++;
            continue;
        }

        // Extract Identifiers (Variables) and Keywords
        if (isalpha(source[i])) {
            string val = "";
            while (i < source.length() && (isalnum(source[i]) || source[i] == '_')) { 
                val += source[i++]; 
            }
            if (val == "int" || val == "if" || val == "else" || val == "while" || val == "print") {
                tokens.push_back({KEYWORD, val, "KEYWORD", current_line});
            } else {
                tokens.push_back({IDENTIFIER, val, "IDENTIFIER", current_line});
            }
            continue;
        }

        // Extract Integer Numbers
        if (isdigit(source[i])) {
            string val = "";
            while (i < source.length() && isdigit(source[i])) val += source[i++];
            tokens.push_back({NUMBER, val, "NUMBER", current_line});
            continue;
        }

        // Extract Relational Operators (==, <=, >=, !=)
        if ((source[i] == '=' || source[i] == '<' || source[i] == '>' || source[i] == '!') && 
            i + 1 < source.length() && source[i+1] == '=') {
            tokens.push_back({REL_OP, string(1, source[i]) + "=", "REL_OP", current_line});
            i += 2; continue;
        }

        // Extract Single-Character Operators and Braces
        char c = source[i];
        if (c == '=' || c == '+' || c == '-' || c == '*' || c == '/') {
            tokens.push_back({OPERATOR, string(1, c), "OPERATOR", current_line});
        } else if (c == '<' || c == '>') { tokens.push_back({REL_OP, string(1, c), "REL_OP", current_line});
        } else if (c == '(') { tokens.push_back({LPAREN, "(", "LPAREN", current_line});
        } else if (c == ')') { tokens.push_back({RPAREN, ")", "RPAREN", current_line});
        } else if (c == '{') { tokens.push_back({LBRACE, "{", "LBRACE", current_line});
        } else if (c == '}') { tokens.push_back({RBRACE, "}", "RBRACE", current_line});
        } else if (c == ';') { tokens.push_back({SYMBOL, ";", "SYMBOL", current_line});
        } else {
            tokens.push_back({UNKNOWN, string(1, c), "UNKNOWN", current_line});
        }
        i++;
    }
    tokens.push_back({END_OF_FILE, "EOF", "EOF", current_line});
    return tokens;
}