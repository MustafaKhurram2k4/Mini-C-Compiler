#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include <string>
#include <unordered_map>
#include "lexer.h"

class Parser {
private:
    std::vector<Token> tokens;
    size_t pos;
    
    // Symbol table now stores Type
    std::unordered_map<std::string, std::string> symbol_table;
    
    // Instructions now store pairs: {Code, Explanation}
    std::vector<std::pair<std::string, std::string>> tac_instructions;
    std::vector<std::pair<std::string, std::string>> asm_instructions;
    std::vector<std::string> compilation_trace;
    
    int temp_counter;
    int label_counter;
    int reg_counter;

    Token currentToken();
    void advance();
    void expect(TokenType type, const std::string& err_msg);
    void checkVariable(const std::string& id, int line);
    void logTrace(const std::string& msg);
    
    std::string newTemp();
    std::string newLabel();
    std::string newReg();

    void parseStatement();
    void parseBlock();
    void parseDeclaration();
    void parseAssignment();
    void parseIf();
    void parseWhile();
    std::string parseExpression();

public:
    Parser(const std::vector<Token>& tkns);
    void parse();
    void printUI();
};

#endif