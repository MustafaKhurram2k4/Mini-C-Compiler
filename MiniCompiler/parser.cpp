#include "parser.h"
#include <iostream>
#include <iomanip>

using namespace std;

Parser::Parser(const vector<Token>& tkns) : tokens(tkns), pos(0), temp_counter(1), label_counter(1), reg_counter(1) {}

Token Parser::currentToken() { 
    return (pos < tokens.size()) ? tokens[pos] : Token{END_OF_FILE, "", "EOF", 0}; 
}

// Looks one step backward to find where an error actually started
Token Parser::previousToken() {
    if (pos > 0) return tokens[pos - 1];
    return Token{UNKNOWN, "", "UNKNOWN", 1};
}

void Parser::advance() { 
    if (pos < tokens.size()) pos++; 
}

void Parser::logTrace(const string& msg) {
    compilation_trace.push_back(msg);
}

// UPDATED: Now uses previousToken() to blame the correct line for missing semicolons!
void Parser::expect(TokenType type, const string& err_msg) {
    if (currentToken().type == type) {
        advance();
    } else {
        Token prev = previousToken(); 
        cerr << "\n\033[1;31m[SYNTAX ERROR] Line " << prev.line 
             << ": " << err_msg << " (after '" << prev.value << "')\033[0m\n";
        exit(1);
    }
}

// Semantic Analysis: Checks if a variable is in the Symbol Table
void Parser::checkVariable(const string& id, int line) {
    if (symbol_table.find(id) == symbol_table.end()) {
        cerr << "\n\033[1;31m[SEMANTIC ERROR] Line " << line << ": Variable '" << id << "' is used before being declared.\033[0m\n";
        exit(1);
    }
}

string Parser::newTemp() { return "t" + to_string(temp_counter++); }
string Parser::newLabel() { return "L" + to_string(label_counter++); }
string Parser::newReg() { return "R" + to_string(reg_counter++); }

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
    else if (currentToken().type == KEYWORD && currentToken().value == "print") parsePrint();
    else if (currentToken().type == LBRACE) parseBlock();
    else {
        cerr << "\n\033[1;31m[SYNTAX ERROR] Line " << currentToken().line << ": Unexpected token '" << currentToken().value << "'\033[0m\n";
        exit(1);
    }
}

void Parser::parseDeclaration() {
    advance(); // Consume 'int'
    string id = currentToken().value;
    expect(IDENTIFIER, "Expected variable name after 'int'");
    expect(OPERATOR, "Expected '=' for initialization");
    
    string result = parseExpression(); 
    expect(SYMBOL, "Expected ';' at end of declaration");

    symbol_table[id] = "int"; // Add to Symbol Table
    logTrace("Declared Integer: " + id + " initialized to " + result);
    
    tac_instructions.push_back({id + " = " + result, "Allocate & assign value to " + id});
    asm_instructions.push_back({"MOV " + id + ", " + result, "Store computed result into memory for " + id});
}

void Parser::parseAssignment() {
    string dest = currentToken().value;
    checkVariable(dest, currentToken().line);
    advance();
    expect(OPERATOR, "Expected '=' in assignment");
    
    string result = parseExpression();
    expect(SYMBOL, "Expected ';' at end of statement");

    tac_instructions.push_back({dest + " = " + result, "Assign computed result to " + dest});
    asm_instructions.push_back({"MOV " + dest + ", " + result, "Save final value back to variable " + dest});
    logTrace("Processed assignment for variable: " + dest);
}

// RECURSIVE DESCENT MATH PARSER (BODMAS SUPPORT)
// 1. Expression handles + and - (Lowest Precedence)
string Parser::parseExpression() {
    string left = parseTerm(); 
    while (currentToken().value == "+" || currentToken().value == "-" || currentToken().type == REL_OP) {
        string op = currentToken().value;
        advance();
        string right = parseTerm(); 
        
        string temp = newTemp();
        tac_instructions.push_back({temp + " = " + left + " " + op + " " + right, "Evaluate expression"});
        
        string reg = newReg();
        asm_instructions.push_back({"MOV " + reg + ", " + left, "Load left operand to register"});
        string asm_op = (op == "+") ? "ADD " : (op == "-") ? "SUB " : "CMP ";
        asm_instructions.push_back({asm_op + reg + ", " + right, "Perform " + op + " operation"});
        
        left = temp; 
    }
    return left; 
}

// 2. Term handles * and / (Highest Math Precedence)
string Parser::parseTerm() {
    string left = parseFactor();
    while (currentToken().value == "*" || currentToken().value == "/") {
        string op = currentToken().value;
        advance();
        string right = parseFactor();

        string temp = newTemp();
        tac_instructions.push_back({temp + " = " + left + " " + op + " " + right, "Evaluate term"});
        
        string reg = newReg();
        asm_instructions.push_back({"MOV " + reg + ", " + left, "Load left operand to register"});
        string asm_op = (op == "*") ? "MUL " : "DIV ";
        asm_instructions.push_back({asm_op + reg + ", " + right, "Perform " + op + " operation"});
        
        left = temp;
    }
    return left;
}

// 3. Factor handles Numbers, Variables, and Parentheses (e.g. (a+b) )
string Parser::parseFactor() {
    if (currentToken().type == NUMBER) {
        string val = currentToken().value;
        advance();
        return val;
    } else if (currentToken().type == IDENTIFIER) {
        string val = currentToken().value;
        checkVariable(val, currentToken().line);
        advance();
        return val;
    } else if (currentToken().type == LPAREN) {
        advance(); // Consume '('
        string val = parseExpression(); // Recursively parse inside
        expect(RPAREN, "Expected ')' to close math grouping");
        return val;
    }
    cerr << "\n\033[1;31m[SYNTAX ERROR] Line " << currentToken().line << ": Expected number or variable\033[0m\n";
    exit(1);
}

// CONTROL FLOW & UTILITIES
void Parser::parsePrint() {
    advance(); // Consume 'print'
    expect(LPAREN, "Expected '('");
    
    string var = currentToken().value;
    checkVariable(var, currentToken().line); 
    advance();
    
    expect(RPAREN, "Expected ')'");
    expect(SYMBOL, "Expected ';'");

    tac_instructions.push_back({"print " + var, "Output value to console"});
    asm_instructions.push_back({"PRINT " + var, "System call to output to screen"});
    logTrace("Generated PRINT instruction for " + var);
}

void Parser::parseBlock() {
    expect(LBRACE, "Expected '{'");
    logTrace("Entered new scope block.");
    while (currentToken().type != RBRACE && currentToken().type != END_OF_FILE) {
        parseStatement();
    }
    expect(RBRACE, "Expected '}'");
    logTrace("Exited scope block.");
}

void Parser::parseIf() {
    advance(); // 'if'
    expect(LPAREN, "Expected '('");
    string cond = parseExpression();
    expect(RPAREN, "Expected ')'");

    string L_False = newLabel();
    string L_End = newLabel();

    logTrace("Generated IF condition branching.");
    tac_instructions.push_back({"ifFalse " + cond + " goto " + L_False, "Jump if condition fails"});
    asm_instructions.push_back({"JMP_FALSE " + L_False, "Conditional jump"});

    parseBlock();

    tac_instructions.push_back({"goto " + L_End, "Skip ELSE block"});
    tac_instructions.push_back({L_False + ":", "--- Start of ELSE/False Block ---"});
    
    asm_instructions.push_back({"JMP " + L_End, "Unconditional jump"});
    asm_instructions.push_back({L_False + ":", "Label for False branch"});

    if (currentToken().type == KEYWORD && currentToken().value == "else") {
        advance();
        logTrace("Generated ELSE fallback block.");
        parseBlock();
    }
    
    tac_instructions.push_back({L_End + ":", "--- End IF ---"});
    asm_instructions.push_back({L_End + ":", "Label for End of IF"});
}

void Parser::parseWhile() {
    advance(); // 'while'
    string L_Start = newLabel();
    string L_End = newLabel();

    logTrace("Generated WHILE loop structure.");
    tac_instructions.push_back({L_Start + ":", "--- Loop Start ---"});
    asm_instructions.push_back({L_Start + ":", "Label for Loop Start"});

    expect(LPAREN, "Expected '('");
    string cond = parseExpression();
    expect(RPAREN, "Expected ')'");

    tac_instructions.push_back({"ifFalse " + cond + " goto " + L_End, "Break loop if false"});
    asm_instructions.push_back({"JMP_FALSE " + L_End, "Exit loop"});

    parseBlock();

    tac_instructions.push_back({"goto " + L_Start, "Loop back to start"});
    tac_instructions.push_back({L_End + ":", "--- Loop End ---"});
    
    asm_instructions.push_back({"JMP " + L_Start, "Jump to loop condition"});
    asm_instructions.push_back({L_End + ":", "Label for Loop Exit"});
}

void Parser::printUI() {
    cout << "\n\033[1;36m=======================================================================\033[0m\n";
    
    cout << "\n\033[1;33m[1] MEMORY MAP (SYMBOL TABLE)\033[0m\n";
    cout << left << setw(15) << "VARIABLE" << "TYPE" << "\n";
    cout << "---------------------------------\n";
    for (const auto& pair : symbol_table) {
        cout << left << setw(15) << ("\033[1;32m" + pair.first + "\033[0m") << pair.second << "\n";
    }

    cout << "\n\033[1;34m[2] INTERMEDIATE REPRESENTATION (TAC)\033[0m\n";
    cout << left << setw(25) << "INSTRUCTION" << "EXPLANATION\n";
    cout << "-----------------------------------------------------------------------\n";
    for (const auto& instr : tac_instructions) {
        if(instr.first.back() == ':') {
            cout << "\033[1;31m" << instr.first << "\033[0m\n"; 
        } else {
            cout << left << setw(25) << instr.first 
                 << "\033[1;30m// " << instr.second << "\033[0m\n";
        }
    }

    cout << "\n\033[1;36m[3] HARDWARE TARGET CODE (ASSEMBLY)\033[0m\n";
    cout << left << setw(25) << "INSTRUCTION" << "EXPLANATION\n";
    cout << "-----------------------------------------------------------------------\n";
    for (const auto& instr : asm_instructions) {
        if(instr.first.back() == ':') {
            cout << "\033[1;31m" << instr.first << "\033[0m\n";
        } else {
            cout << left << setw(25) << instr.first 
                 << "\033[1;30m; " << instr.second << "\033[0m\n";
        }
    }
    
    cout << "\n\033[1;32m[SYSTEM] Compilation finished. Zero errors detected.\033[0m\n";
}