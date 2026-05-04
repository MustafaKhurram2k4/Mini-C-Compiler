#include <fstream>
#include <iostream>
#include "lexer.h"
#include "parser.h"

int main() {
    std::ifstream file("input.txt");
    if (!file.is_open()) {
        std::cerr << "\033[1;31m[FATAL] Missing 'input.txt' in kernel directory.\033[0m\n";
        return 1;
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    Lexer lexer;
    std::vector<Token> tokens = lexer.tokenize(content);
    
    Parser parser(tokens);
    parser.parse();
    parser.printUI();

    return 0;
}