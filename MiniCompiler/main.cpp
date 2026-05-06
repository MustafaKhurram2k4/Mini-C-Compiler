#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include "lexer.h"
#include "parser.h"
using namespace std;

// THE VIRTUAL MACHINE (EXECUTION ENGINE)
class VirtualMachine {
private:
    unordered_map<string, int> memory;
    unordered_map<string, int> labels;

    // Resolves whether a string is a raw number or a variable from memory
    int getValue(const string& s) {
        if (isdigit(s[0]) || (s[0] == '-' && isdigit(s[1]))) return stoi(s);
        return memory[s]; 
    }

public:
    void execute(const vector<pair<string, string>>& tac) {
        // Step 1: 1st Pass - Record all Jump Labels
        for (int i = 0; i < tac.size(); i++) {
            string instr = tac[i].first;
            if (instr.back() == ':') {
                labels[instr.substr(0, instr.size() - 1)] = i;
            }
        }

        cout << "\n\033[1;35m[4] PROGRAM EXECUTION OUTPUT (VIRTUAL MACHINE)\033[0m\n";
        cout << "-----------------------------------------------------------------------\n";
        
        int pc = 0; // Program Counter (Current Line)
        
        // Step 2: Execute Code
        while (pc < tac.size()) {
            string instr = tac[pc].first;
            stringstream ss(instr);
            string token1, token2, token3, token4, token5;
            
            ss >> token1;

            if (token1 == "print") {
                ss >> token2;
                cout << ">> Output: \033[1;32m" << getValue(token2) << "\033[0m\n";
            } 
            else if (token1 == "goto") {
                ss >> token2;
                pc = labels[token2]; 
                continue; 
            } 
            else if (token1 == "ifFalse") {
                ss >> token2 >> token3 >> token4; 
                if (getValue(token2) == 0) { 
                    pc = labels[token4];
                    continue;
                }
            } 
            else if (instr.back() == ':') {
                // Ignore labels on execution pass
            } 
            else {
                // Assignment or Math execution
                ss >> token2; // '='
                if (ss >> token3) {
                    if (ss >> token4) { 
                        ss >> token5; 
                        int leftVal = getValue(token3);
                        int rightVal = getValue(token5);
                        int res = 0;
                        
                        // Execute Math and Relational logic
                        if (token4 == "+") res = leftVal + rightVal;
                        else if (token4 == "-") res = leftVal - rightVal;
                        else if (token4 == "*") res = leftVal * rightVal;
                        else if (token4 == "/") {
                            if (rightVal == 0) { // Safety Guard!
                                cerr << "\033[1;31m[VM EXCEPTION] Division by zero detected!\033[0m\n";
                                return;
                            }
                            res = leftVal / rightVal;
                        }
                        else if (token4 == "<") res = leftVal < rightVal;
                        else if (token4 == ">") res = leftVal > rightVal;
                        else if (token4 == "==") res = leftVal == rightVal;
                        else if (token4 == "!=") res = leftVal != rightVal;
                        
                        memory[token1] = res; 
                    } else {
                        memory[token1] = getValue(token3); // Simple assignment
                    }
                }
            }
            pc++; 
        }
    }
};

int main() {
    ifstream file("input.txt");
    if (!file.is_open()) {
        cerr << "\033[1;31m[FATAL] Missing 'input.txt' in directory.\033[0m\n";
        return 1;
    }

    string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    file.close();

    // 1. Lexical Analysis
    Lexer lexer;
    vector<Token> tokens = lexer.tokenize(content);
    
    // 2. Syntax & Semantic Analysis + TAC Generation
    Parser parser(tokens);
    parser.parse();   
    parser.printUI(); 

    // 3. Execution
    VirtualMachine vm;
    vm.execute(parser.getTAC());

    return 0;
}