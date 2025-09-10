#include <iostream>
#include <vector>
#include <string>
#include <set>
#include <map>
#include <stack>
#include <iomanip>
#include <algorithm>
#include <cctype>

// Use '#' to represent epsilon
const char END_MARKER = '!';

// Represents a production rule: (1) A -> rule
struct Production {
    int number;
    char nonTerminal;
    std::string rule;
};

class LL1Parser {
private:
    std::vector<Production> grammar;
    std::set<char> nonTerminals;
    std::set<char> terminals;
    char originalStartSymbol;
    char augmentedStartSymbol = 'Z'; 

    std::map<char, bool> nullable;
    std::map<char, std::set<char>> firstSets;
    std::map<char, std::set<char>> followSets;
    std::map<char, std::map<char, int>> parseTable; // Stores production numbers

    void findAugmentedSymbol() {
        std::set<char> allSymbols;
        for(const auto& prod : grammar) {
            allSymbols.insert(prod.nonTerminal);
            for(char c : prod.rule) allSymbols.insert(c);
        }
        char temp = 'Z';
        while (temp >= 'A') {
            if (allSymbols.find(temp) == allSymbols.end()) {
                augmentedStartSymbol = temp;
                return;
            }
            temp--;
        }
        augmentedStartSymbol = '^'; 
    }

    bool isNonTerminal(char c) {
        return nonTerminals.count(c);
    }

    bool isStringNullable(const std::string& s) {
        if (s.empty() || s == "#") return true;
        for (char c : s) {
            if (isNonTerminal(c)) {
                if (nullable.find(c) == nullable.end() || !nullable.at(c)) {
                    return false;
                }
            } else { 
                return false;
            }
        }
        return true;
    }
    
    void printHorizontalLine(const std::vector<int>& widths) {
        std::cout << "+";
        for (int w : widths) {
            std::cout << std::string(w - 1, '-') << "+";
        }
        std::cout << std::endl;
    }

    void printRow(const std::vector<std::string>& cells, const std::vector<int>& widths) {
        std::cout << "|";
        for (size_t i = 0; i < cells.size(); ++i) {
            std::cout << " " << std::left << std::setw(widths[i] - 2) << cells[i] << "|";
        }
        std::cout << std::endl;
    }


public:
    LL1Parser() : originalStartSymbol('\0') {}

    void addProduction(const std::string& prodStr) {
        if (prodStr.length() < 3 || prodStr.substr(1, 2) != "->") return;
        char nt = prodStr[0];
        std::string rule = prodStr.substr(3);
        if (!isupper(nt)) return;
        grammar.push_back({(int)grammar.size() + 1, nt, rule});
        nonTerminals.insert(nt);
        for(char c : rule){
            if(isupper(c)) nonTerminals.insert(c);
        }
    }

    void setStartSymbol(char start) {
        bool found = false;
        for(const auto& prod : grammar){ if(prod.nonTerminal == start) found = true; }
        if (!found){ std::cerr << "Error: Start symbol '" << start << "' not found. Exiting." << std::endl; exit(1); }
        originalStartSymbol = start;
    }
    
    void initializeGrammar() {
        if (originalStartSymbol == '\0') { std::cerr << "Error: Start symbol not set. Exiting." << std::endl; exit(1); }
        findAugmentedSymbol();
        grammar.insert(grammar.begin(), {0, augmentedStartSymbol, std::string(1, originalStartSymbol)});
        nonTerminals.insert(augmentedStartSymbol);

        for (const auto& prod : grammar) {
            for (char c : prod.rule) {
                if (!isupper(c) && c != '#') terminals.insert(c);
            }
        }
    }

    void computeNullability() {
        for (char nt : nonTerminals) nullable[nt] = false;
        bool changed = true;
        while (changed) {
            changed = false;
            for (const auto& prod : grammar) {
                if (!nullable[prod.nonTerminal] && isStringNullable(prod.rule)) {
                    nullable[prod.nonTerminal] = true;
                    changed = true;
                }
            }
        }
    }

    void computeFirstSets() {
        for (char nt : nonTerminals) firstSets[nt] = std::set<char>();
        bool changed = true;
        while (changed) {
            changed = false;
            for (const auto& prod : grammar) {
                char nt = prod.nonTerminal;
                size_t original_size = firstSets[nt].size();
                for (char symbol : prod.rule) {
                    if (isNonTerminal(symbol)) {
                        for (char first_sym : firstSets[symbol]) {
                            firstSets[nt].insert(first_sym);
                        }
                        if (!nullable.at(symbol)) break;
                    } else if (symbol != '#') {
                        firstSets[nt].insert(symbol);
                        break;
                    }
                }
                if (firstSets[nt].size() != original_size) changed = true;
            }
        }
    }

    void computeFollowSets() {
        for (char nt : nonTerminals) followSets[nt] = std::set<char>();
        followSets[augmentedStartSymbol].insert(END_MARKER);
        bool changed = true;
        while (changed) {
            changed = false;
            for (const auto& prod : grammar) {
                for (size_t i = 0; i < prod.rule.length(); ++i) {
                    char B = prod.rule[i];
                    if (isNonTerminal(B)) {
                        size_t original_size = followSets[B].size();
                        std::string beta = prod.rule.substr(i + 1);
                        
                        bool betaIsNullable = true;
                        for (char sym_beta : beta) {
                           if(isNonTerminal(sym_beta)) {
                               for(char t : firstSets[sym_beta]) followSets[B].insert(t);
                               if(!nullable.at(sym_beta)){ betaIsNullable = false; break; }
                           } else { 
                               followSets[B].insert(sym_beta);
                               betaIsNullable = false; break;
                           }
                        }

                        if (betaIsNullable) {
                            for (char t : followSets[prod.nonTerminal]) followSets[B].insert(t);
                        }

                        if (followSets[B].size() != original_size) changed = true;
                    }
                }
            }
        }
    }

    void createParseTable() {
        for (char nt : nonTerminals) {
            for (char t : terminals) parseTable[nt][t] = -1;
            parseTable[nt][END_MARKER] = -1;
        }

        for (const auto& prod : grammar) {
            char A = prod.nonTerminal;
            std::string alpha = prod.rule;
            std::set<char> first_of_alpha;
            
            bool is_alpha_nullable = true;
            for(char sym : alpha){
                if (sym == '#') continue;
                if(isNonTerminal(sym)){
                    first_of_alpha.insert(firstSets[sym].begin(), firstSets[sym].end());
                    if(!nullable.at(sym)){ is_alpha_nullable = false; break; }
                } else {
                    first_of_alpha.insert(sym);
                    is_alpha_nullable = false; break;
                }
            }
            if (alpha == "#") is_alpha_nullable = true;

            for (char t : first_of_alpha) {
                if (parseTable[A][t] != -1) std::cerr << "Conflict at Table[" << A << ", " << t << "]! Grammar not LL(1).\n";
                parseTable[A][t] = prod.number;
            }

            if (is_alpha_nullable) {
                for (char t : followSets[A]) {
                    if (parseTable[A][t] != -1) std::cerr << "Conflict at Table[" << A << ", " << t << "] for nullable rule! Grammar not LL(1).\n";
                    parseTable[A][t] = prod.number;
                }
            }
        }
    }

    void parseString(const std::string& inputStr) {
        std::string input = inputStr + END_MARKER;
        std::stack<char> pdaStack;
        pdaStack.push(END_MARKER);
        pdaStack.push(augmentedStartSymbol);

        int ip = 0;
        
        std::cout << "\n--- PDA Simulation for input: \"" << inputStr << "\" ---\n";
        std::vector<int> widths = {25, 25, 25};
        printHorizontalLine(widths);
        printRow({"Stack", "Input", "Action"}, widths);
        printHorizontalLine(widths);

        std::string lastAction = "";

        while (!pdaStack.empty()) {
            std::string stackStr;
            std::stack<char> tempStack = pdaStack;
            while(!tempStack.empty()) { stackStr = tempStack.top() + stackStr; tempStack.pop(); }
            
            printRow({stackStr, input.substr(ip), lastAction}, widths);

            char top = pdaStack.top();
            char currentInput = input[ip];

            if (isNonTerminal(top)) {
                if (parseTable.find(top) == parseTable.end() || parseTable.at(top).find(currentInput) == parseTable.at(top).end() || parseTable.at(top).at(currentInput) == -1) {
                    lastAction = "ERROR: No rule!";
                    printRow({stackStr, input.substr(ip), lastAction}, widths);
                    break;
                }
                int ruleNum = parseTable.at(top).at(currentInput);
                
                Production prod_to_apply;
                for(const auto& p : grammar) { if (p.number == ruleNum) { prod_to_apply = p; break; } }
                
                lastAction = "Apply " + std::string(1, top) + " -> " + prod_to_apply.rule;
                
                pdaStack.pop();
                if (prod_to_apply.rule != "#") {
                    for (int i = prod_to_apply.rule.length() - 1; i >= 0; --i) pdaStack.push(prod_to_apply.rule[i]);
                }
            } else { 
                if (top == currentInput) {
                    if (top == END_MARKER) {
                        lastAction = "Success!";
                        printRow({stackStr, input.substr(ip), lastAction}, widths);
                        printHorizontalLine(widths);
                        std::cout << "\n>>> String \"" << inputStr << "\" ACCEPTED by empty stack! <<<\n";
                        return;
                    }
                    lastAction = "Match '" + std::string(1, top) + "'";
                    pdaStack.pop();
                    ip++;
                } else {
                    lastAction = "ERROR: Mismatch!";
                    printRow({stackStr, input.substr(ip), lastAction}, widths);
                    break;
                }
            }
        }
        printHorizontalLine(widths);
        std::cout << "\n>>> String \"" << inputStr << "\" REJECTED. <<<\n";
    }

    void printNumberedProductions() {
        std::cout << "\n--- Grammar Productions ---\n";
        for (const auto& prod : grammar) {
            // Start from 1 for user-facing productions
            if (prod.number == 0) continue;
            std::cout << "(" << prod.number << ") " << prod.nonTerminal << " -> " << prod.rule << std::endl;
        }
    }

    void printSets() {
        std::cout << "\n--- Nullability, FIRST, and FOLLOW Sets ---\n";
        std::vector<char> sortedNTs;
        for(char nt : nonTerminals) sortedNTs.push_back(nt);
        std::sort(sortedNTs.begin(), sortedNTs.end());

        std::vector<int> widths = {18, 15, 25, 25};
        printHorizontalLine(widths);
        printRow({"Non-Terminal", "Nullable", "FIRST Set", "FOLLOW Set"}, widths);
        printHorizontalLine(widths);

        for (char nt : sortedNTs) {
            std::string firstStr = "{ ";
            for (char c : firstSets[nt]) { firstStr += c; firstStr += ", "; }
            if (!firstSets[nt].empty()) firstStr.resize(firstStr.length() - 2);
            firstStr += " }";

            std::string followStr = "{ ";
            for (char c : followSets[nt]) { followStr += c; followStr += ", "; }
            if (!followSets[nt].empty()) followStr.resize(followStr.length() - 2);
            followStr += " }";
            
            printRow({std::string(1, nt), (nullable[nt] ? "True" : "False"), firstStr, followStr}, widths);
        }
        printHorizontalLine(widths);
    }

    void printParseTable() {
        std::cout << "\n--- LL(1) Prediction Table ---\n";
        std::vector<char> tableTerminals;
        for(char t : terminals) tableTerminals.push_back(t);
        std::sort(tableTerminals.begin(), tableTerminals.end());
        tableTerminals.push_back(END_MARKER);
        
        std::vector<int> widths = {5};
        for(size_t i = 0; i < tableTerminals.size(); ++i) widths.push_back(5);

        std::vector<std::string> header = {" "};
        for(char t : tableTerminals) header.push_back(std::string(1,t));

        printHorizontalLine(widths);
        printRow(header, widths);
        printHorizontalLine(widths);
        
        std::vector<char> sortedNTs;
        for(char nt : nonTerminals) sortedNTs.push_back(nt);
        std::sort(sortedNTs.begin(), sortedNTs.end());

        for (char nt : sortedNTs) {
            std::vector<std::string> row;
            row.push_back(std::string(1, nt));
            for (char t : tableTerminals) {
                 int ruleNum = parseTable.count(nt) && parseTable.at(nt).count(t) ? parseTable.at(nt).at(t) : -1;
                 if(ruleNum != -1) {
                     row.push_back("(" + std::to_string(ruleNum) + ")");
                 } else {
                     row.push_back(" ");
                 }
            }
            printRow(row, widths);
        }
        printHorizontalLine(widths);
    }
};

int main() {
    LL1Parser parser;
    std::cout << "Enter grammar productions (e.g., S->aB, C->#), one per line.\n";
    std::cout << "Type 'done' when finished:\n";
    std::cout << "----------------------------------------------------------------\n";
    std::string line;
    while (std::getline(std::cin, line) && line != "done") {
        if (!line.empty()) parser.addProduction(line);
    }

    char startSymbol;
    std::cout << "Enter the start symbol: ";
    std::cin >> startSymbol;
    parser.setStartSymbol(startSymbol);

    parser.initializeGrammar();
    parser.computeNullability();
    parser.computeFirstSets();
    parser.computeFollowSets();
    parser.createParseTable();

    parser.printNumberedProductions();
    parser.printSets();
    parser.printParseTable();

    std::string inputStr;
    std::cout << "\nEnter a string to parse (or type 'quit' to exit): ";
    while (std::cin >> inputStr && inputStr != "quit") {
        parser.parseString(inputStr);
        std::cout << "\nEnter another string to parse (or type 'quit' to exit): ";
    }

    return 0;
}
