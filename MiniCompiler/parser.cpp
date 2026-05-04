#include "parser.h"
#include <iostream>
#include <iomanip> //forcolumn alignment

Parser::Parser(const std::vector<Token>& tkns) : tokens(tkns), pos(0), temp_counter(1), label_counter(1), reg_counter(1) {}

Token Parser::currentToken() { return (pos < tokens.size()) ? tokens[pos] : Token{END_OF_FILE, "", "EOF", 0}; }
void Parser::advance() { if (pos < tokens.size()) pos++; }

void Parser::logTrace(const std::string& msg) {
    compilation_trace.push_back(msg);
}

void Parser::expect(TokenType type, const std::string& err_msg) {
    if (currentToken().type == type) advance();
    else {
        std::cerr << "\n\033[1;31m[SYNTAX ERROR] Line " << currentToken().line << ": " << err_msg << " near '" << currentToken().value << "'\033[0m\n";
        exit(1);
    }
}

void Parser::checkVariable(const std::string& id, int line) {
    if (symbol_table.find(id) == symbol_table.end()) {
        std::cerr << "\n\033[1;31m[SEMANTIC ERROR] Line " << line << ": Variable '" << id << "' is used before being declared.\033[0m\n";
        exit(1);
    }
}

std::string Parser::newTemp() { return "t" + std::to_string(temp_counter++); }
std::string Parser::newLabel() { return "L" + std::to_string(label_counter++); }
std::string Parser::newReg() { return "R" + std::to_string(reg_counter++); }

void Parser::parse() {
    logTrace("Starting compilation pipeline...");
    while (currentToken().type != END_OF_FILE) {
        parseStatement();
    }
    logTrace("Compilation pipeline finished successfully.");
}

void Parser::parseStatement() {
    if (currentToken().type == KEYWORD && currentToken().value == "int") parseDeclaration();
    else if (currentToken().type == IDENTIFIER) parseAssignment();
    else if (currentToken().type == KEYWORD && currentToken().value == "if") parseIf();
    else if (currentToken().type == KEYWORD && currentToken().value == "while") parseWhile();
    else if (currentToken().type == LBRACE) parseBlock();
    else {
        std::cerr << "\n\033[1;31m[SYNTAX ERROR] Line " << currentToken().line << ": Unexpected token '" << currentToken().value << "'\033[0m\n";
        exit(1);
    }
}

void Parser::parseBlock() {
    expect(LBRACE, "Expected '{' to open block");
    logTrace("Entered new scope block.");
    while (currentToken().type != RBRACE && currentToken().type != END_OF_FILE) {
        parseStatement();
    }
    expect(RBRACE, "Expected '}' to close block");
    logTrace("Exited scope block.");
}

void Parser::parseDeclaration() {
    advance(); // 'int'
    std::string id = currentToken().value;
    expect(IDENTIFIER, "Expected variable name after 'int'");
    expect(OPERATOR, "Expected '=' for initialization");
    std::string val = currentToken().value;
    expect(NUMBER, "Expected a numeric value");
    expect(SYMBOL, "Expected ';' at end of declaration");

    symbol_table[id] = "int";
    logTrace("Declared Integer: " + id + " initialized to " + val);
    
    tac_instructions.push_back({id + " = " + val, "Allocate & assign value to " + id});
    asm_instructions.push_back({"MOV " + id + ", " + val, "Store literal " + val + " directly into memory for " + id});
}

std::string Parser::parseExpression() {
    std::string left = currentToken().value;
    if (currentToken().type == IDENTIFIER) checkVariable(left, currentToken().line);
    advance();

    if (currentToken().type == OPERATOR || currentToken().type == REL_OP) {
        std::string op = currentToken().value;
        advance();
        std::string right = currentToken().value;
        if (currentToken().type == IDENTIFIER) checkVariable(right, currentToken().line);
        advance();

        std::string temp = newTemp();
        tac_instructions.push_back({temp + " = " + left + " " + op + " " + right, "Evaluate expression into temp variable " + temp});
        
        std::string reg = newReg();
        asm_instructions.push_back({"MOV " + reg + ", " + left, "Load left operand into CPU register " + reg});
        
        std::string asm_op = op == "+" ? "ADD " : op == "-" ? "SUB " : op == "*" ? "MUL " : "CMP ";
        asm_instructions.push_back({asm_op + reg + ", " + right, "Perform " + op + " operation with right operand"});
        
        return temp;
    }
    return left; // Single value
}

void Parser::parseAssignment() {
    std::string dest = currentToken().value;
    checkVariable(dest, currentToken().line);
    advance();
    expect(OPERATOR, "Expected '=' in assignment");
    
    std::string result = parseExpression();
    expect(SYMBOL, "Expected ';' at end of statement");

    tac_instructions.push_back({dest + " = " + result, "Assign computed result to " + dest});
    asm_instructions.push_back({"MOV " + dest + ", " + result, "Save final value back to variable " + dest});
    logTrace("Processed assignment for variable: " + dest);
}

void Parser::parseIf() {
    advance(); // 'if'
    expect(LPAREN, "Expected '(' for condition");
    std::string cond = parseExpression();
    expect(RPAREN, "Expected ')' after condition");

    std::string L_False = newLabel();
    std::string L_End = newLabel();

    logTrace("Generated IF condition branching.");
    tac_instructions.push_back({"ifFalse " + cond + " goto " + L_False, "If condition fails, jump to False block"});
    asm_instructions.push_back({"JMP_FALSE " + L_False, "Conditional CPU jump to label " + L_False});

    parseBlock();

    tac_instructions.push_back({"goto " + L_End, "Skip ELSE block by jumping to End"});
    tac_instructions.push_back({L_False + ":", "--- Start of ELSE/False Block ---"});
    
    asm_instructions.push_back({"JMP " + L_End, "Unconditional jump to end of statement"});
    asm_instructions.push_back({L_False + ":", "Label definition for False branch"});

    if (currentToken().type == KEYWORD && currentToken().value == "else") {
        advance();
        logTrace("Generated ELSE fallback block.");
        parseBlock();
    }
    
    tac_instructions.push_back({L_End + ":", "--- End of IF Statement ---"});
    asm_instructions.push_back({L_End + ":", "Label definition for End of IF"});
}

void Parser::parseWhile() {
    advance(); // 'while'
    std::string L_Start = newLabel();
    std::string L_End = newLabel();

    logTrace("Generated WHILE loop structure.");
    tac_instructions.push_back({L_Start + ":", "--- Loop Start Point ---"});
    asm_instructions.push_back({L_Start + ":", "Label definition for Loop Start"});

    expect(LPAREN, "Expected '(' for loop condition");
    std::string cond = parseExpression();
    expect(RPAREN, "Expected ')' after loop condition");

    tac_instructions.push_back({"ifFalse " + cond + " goto " + L_End, "If loop condition fails, break loop"});
    asm_instructions.push_back({"JMP_FALSE " + L_End, "Exit loop if condition is false"});

    parseBlock();

    tac_instructions.push_back({"goto " + L_Start, "Loop back to start condition"});
    tac_instructions.push_back({L_End + ":", "--- Loop End Point ---"});
    
    asm_instructions.push_back({"JMP " + L_Start, "Jump back to evaluate loop condition again"});
    asm_instructions.push_back({L_End + ":", "Label definition for Loop Exit"});
}

void Parser::printUI() {
    std::cout << "\n\033[1;36m=======================================================================\033[0m\n";
    
    std::cout << "\n\033[1;33m[1] MEMORY MAP (SYMBOL TABLE)\033[0m\n";
    std::cout << std::left << std::setw(15) << "VARIABLE" << "TYPE" << "\n";
    std::cout << "---------------------------------\n";
    for (const auto& pair : symbol_table) {
        std::cout << std::left << std::setw(15) << ("\033[1;32m" + pair.first + "\033[0m") << pair.second << "\n";
    }

    std::cout << "\n\033[1;34m[2] INTERMEDIATE REPRESENTATION (TAC)\033[0m\n";
    std::cout << std::left << std::setw(25) << "INSTRUCTION" << "EXPLANATION\n";
    std::cout << "-----------------------------------------------------------------------\n";
    for (const auto& instr : tac_instructions) {
        if(instr.first.back() == ':') {
            std::cout << "\033[1;31m" << instr.first << "\033[0m\n"; // Just print labels
        } else {
            std::cout << std::left << std::setw(25) << instr.first 
                      << "\033[1;30m// " << instr.second << "\033[0m\n";
        }
    }

    std::cout << "\n\033[1;36m[3] HARDWARE TARGET CODE (ASSEMBLY)\033[0m\n";
    std::cout << std::left << std::setw(25) << "INSTRUCTION" << "EXPLANATION\n";
    std::cout << "-----------------------------------------------------------------------\n";
    for (const auto& instr : asm_instructions) {
        if(instr.first.back() == ':') {
            std::cout << "\033[1;31m" << instr.first << "\033[0m\n";
        } else {
            std::cout << std::left << std::setw(25) << instr.first 
                      << "\033[1;30m; " << instr.second << "\033[0m\n";
        }
    }
    
    std::cout << "\n\033[1;32m[SYSTEM] Compilation finished. Zero errors detected.\033[0m\n\n";
}