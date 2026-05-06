#ifndef LEXER_H
#define LEXER_H

#include <iostream>
#include <vector>
#include <string>

// The vocabulary of our programming language
enum TokenType { 
    KEYWORD, IDENTIFIER, NUMBER, OPERATOR, REL_OP, 
    LPAREN, RPAREN, LBRACE, RBRACE, SYMBOL, UNKNOWN, END_OF_FILE 
};

// Represents a single "word" or symbol chopped from the source code
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