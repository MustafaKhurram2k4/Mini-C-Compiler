#ifndef LEXER_H
#define LEXER_H

#include <iostream>
#include <vector>
#include <string>

enum TokenType { 
    KEYWORD, IDENTIFIER, NUMBER, OPERATOR, REL_OP, 
    LPAREN, RPAREN, LBRACE, RBRACE, SYMBOL, UNKNOWN, END_OF_FILE 
};

struct Token {
    TokenType type;
    std::string value;
    std::string typeName;
    int line;
};

class Lexer {
public:
    std::vector<Token> tokenize(const std::string& source);
};

#endif