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
    
    // Symbol table tracks variables
    std::unordered_map<std::string, std::string> symbol_table;
    
    // Instructions format: {Code, Explanation}
    std::vector<std::pair<std::string, std::string>> tac_instructions;
    std::vector<std::pair<std::string, std::string>> asm_instructions;
    std::vector<std::string> compilation_trace;
    
    int temp_counter;
    int label_counter;
    int reg_counter;

    // Core Helpers
    Token currentToken();
    Token previousToken(); // Tracks footsteps for better error reporting!
    void advance();
    void expect(TokenType type, const std::string& err_msg);
    void checkVariable(const std::string& id, int line);
    void logTrace(const std::string& msg);
    
    std::string newTemp();
    std::string newLabel();
    std::string newReg();

    // Statement Parsers
    void parseStatement();
    void parseBlock();
    void parseDeclaration();
    void parseAssignment();
    void parseIf();
    void parseWhile();
    void parsePrint();

    // Recursive Descent Math Parsers (BODMAS/PEMDAS)
    std::string parseExpression(); 
    std::string parseTerm();       
    std::string parseFactor();     

public:
    Parser(const std::vector<Token>& tkns);
    void parse();
    void printUI();
    
    // Fetches the code so the Virtual Machine can run it!
    std::vector<std::pair<std::string, std::string>> getTAC() const { return tac_instructions; } 
};

#endif