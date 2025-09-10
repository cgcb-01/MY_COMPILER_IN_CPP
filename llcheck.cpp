#include <bits/stdc++.h>
using namespace std;

struct Production {
    int id;
    string lhs;
    vector<string> rhs;
};

string EPSILON = "#";

vector<Production> prods;
set<string> nonterms;
set<string> terms;
map<string,bool> nullable;
map<string,set<string>> first;
map<string,set<string>> follow;
map<string,map<string,int>> M;

vector<string> tokenizeRHS(const string &rhsRaw) {
    vector<string> tokens;
    for (size_t i = 0; i < rhsRaw.size(); i++) {
        if (rhsRaw[i] == '#') {
            tokens.push_back(EPSILON);
            continue;
        }
        char c = rhsRaw[i];
        if (isupper((unsigned char)c)) {
            string tok(1, c);
            if (i+1 < rhsRaw.size() && rhsRaw[i+1] == '\'') {
                tok.push_back('\'');
                i++;
            }
            tokens.push_back(tok);
            nonterms.insert(tok);
        } else {
            string tok(1, c);
            tokens.push_back(tok);
            terms.insert(tok);
        }
    }
    if (tokens.empty()) tokens.push_back(EPSILON);
    return tokens;
}

string joinSet(const set<string> &s) {
    string out;
    bool first = true;
    for (auto &e : s) {
        if (!first) out += ",";
        out += e;
        first = false;
    }
    if (out.empty()) out = "-";
    return out;
}

void computeNullable() {
    for (auto &nt : nonterms) nullable[nt] = false;
    bool changed = true;
    while (changed) {
        changed = false;
        for (auto &p : prods) {
            bool allNull = true;
            if (p.rhs.size() == 1 && p.rhs[0] == EPSILON) {
                if (!nullable[p.lhs]) { nullable[p.lhs] = true; changed = true; }
                continue;
            }
            for (auto &sym : p.rhs) {
                if (sym == EPSILON) continue;
                if (nonterms.count(sym)) {
                    if (!nullable[sym]) { allNull = false; break; }
                } else { allNull = false; break; }
            }
            if (allNull && !nullable[p.lhs]) {
                nullable[p.lhs] = true;
                changed = true;
            }
        }
    }
}

set<string> firstOfSequence(const vector<string> &seq) {
    set<string> res;
    if (seq.empty()) {
        res.insert(EPSILON);
        return res;
    }
    bool allNullSoFar = true;
    for (size_t i = 0; i < seq.size(); i++) {
        string sym = seq[i];
        if (sym == EPSILON) {
            res.insert(EPSILON);
            allNullSoFar = true;
            break;
        }
        if (!nonterms.count(sym)) {
            res.insert(sym);
            allNullSoFar = false;
            break;
        } else {
            auto it = first.find(sym);
            if (it != first.end()) {
                for (auto &x : it->second) if (x != EPSILON) res.insert(x);
                if (it->second.count(EPSILON)) {
                    allNullSoFar = true;
                    continue;
                } else { allNullSoFar = false; break; }
            } else {
                allNullSoFar = false;
                break;
            }
        }
    }
    if (allNullSoFar) res.insert(EPSILON);
    return res;
}

void computeFirst() {
    for (auto &t : terms) first[t].insert(t);
    for (auto &nt : nonterms) first[nt];
    bool changed = true;
    while (changed) {
        changed = false;
        for (auto &p : prods) {
            set<string> seqFirst = firstOfSequence(p.rhs);
            for (auto &x : seqFirst) {
                if (!first[p.lhs].count(x)) {
                    first[p.lhs].insert(x);
                    changed = true;
                }
            }
        }
    }
}

void computeFollow(string startSymbol) {
    for (auto &nt : nonterms) follow[nt];
    follow[startSymbol].insert("$");
    bool changed = true;
    while (changed) {
        changed = false;
        for (auto &p : prods) {
            string A = p.lhs;
            for (size_t i = 0; i < p.rhs.size(); i++) {
                string B = p.rhs[i];
                if (!nonterms.count(B)) continue;
                vector<string> beta;
                for (size_t j = i+1; j < p.rhs.size(); j++) beta.push_back(p.rhs[j]);
                set<string> firstBeta = firstOfSequence(beta);
                for (auto &t : firstBeta) {
                    if (t != EPSILON && !follow[B].count(t)) {
                        follow[B].insert(t);
                        changed = true;
                    }
                }
                if (firstBeta.count(EPSILON) || beta.empty()) {
                    for (auto &f : follow[A]) {
                        if (!follow[B].count(f)) {
                            follow[B].insert(f);
                            changed = true;
                        }
                    }
                }
            }
        }
    }
}

void buildPredictionTable(vector<pair<string,string>> &conflicts) {
    for (auto &p : prods) {
        set<string> seqFirst = firstOfSequence(p.rhs);
        for (auto &t : seqFirst) {
            if (t == EPSILON) continue;
            if (M[p.lhs].count(t) && M[p.lhs][t] != p.id) {
                conflicts.push_back({p.lhs, t});
            }
            M[p.lhs][t] = p.id;
        }
        if (seqFirst.count(EPSILON)) {
            for (auto &b : follow[p.lhs]) {
                if (M[p.lhs].count(b) && M[p.lhs][b] != p.id) {
                    conflicts.push_back({p.lhs, b});
                }
                M[p.lhs][b] = p.id;
            }
        }
    }
}

void printNullableFirstFollowTable() {
    size_t wNT = 10, wNullable = 10, wFirst = 30, wFollow = 30;
    cout << "\n+" << string(wNT,'-') << "+" << string(wNullable,'-') << "+"
         << string(wFirst,'-') << "+" << string(wFollow,'-') << "+\n";
    cout << "|" << setw(wNT) << left << " NonTerm"
         << "|" << setw(wNullable) << left << " Nullable"
         << "|" << setw(wFirst) << left << " First"
         << "|" << setw(wFollow) << left << " Follow" << "|\n";
    cout << "+" << string(wNT,'-') << "+" << string(wNullable,'-') << "+"
         << string(wFirst,'-') << "+" << string(wFollow,'-') << "+\n";
    for (auto &nt : nonterms) {
        string fn = joinSet(first[nt]);
        string fo = joinSet(follow[nt]);
        cout << "|" << setw(wNT) << left << (" " + nt)
             << "|" << setw(wNullable) << left << (nullable[nt] ? "True" : "False")
             << "|" << setw(wFirst) << left << (" " + fn)
             << "|" << setw(wFollow) << left << (" " + fo) << "|\n";
    }
    cout << "+" << string(wNT,'-') << "+" << string(wNullable,'-') << "+"
         << string(wFirst,'-') << "+" << string(wFollow,'-') << "+\n";
}

void printPredictionTable() {
    set<string> termSet = terms;
    termSet.insert("$");
    vector<string> termsVec(termSet.begin(), termSet.end());
    sort(termsVec.begin(), termsVec.end(), [](const string &a, const string &b){
        if (a == "$") return false;
        if (b == "$") return true;
        return a < b;
    });
    int wNT = 8, wCol = 7;
    cout << "\nPrediction Table (entries = production id)\n";
    cout << "+" << string(wNT,'-') << "+";
    for (size_t i=0;i<termsVec.size();i++) cout << string(wCol,'-') << "+";
    cout << "\n";
    cout << "|" << setw(wNT) << left << "NT";
    for (auto &t : termsVec) cout << "|" << setw(wCol) << left << t;
    cout << "|\n";
    cout << "+" << string(wNT,'-') << "+";
    for (size_t i=0;i<termsVec.size();i++) cout << string(wCol,'-') << "+";
    cout << "\n";
    for (auto &nt : nonterms) {
        cout << "|" << setw(wNT) << left << nt;
        for (auto &t : termsVec) {
            string out = "";
            auto itRow = M.find(nt);
            if (itRow != M.end()) {
                auto itCell = itRow->second.find(t);
                if (itCell != itRow->second.end()) out = to_string(itCell->second);
            }
            cout << "|" << setw(wCol) << left << out;
        }
        cout << "|\n";
    }
    cout << "+" << string(wNT,'-') << "+";
    for (size_t i=0;i<termsVec.size();i++) cout << string(wCol,'-') << "+";
    cout << "\n";
}

void printStepRow(int step, const string &stackStr, const string &inputRem, const string &action) {
    int wStep = 6, wStack = 40, wInput = 20, wAction = 40;
    cout << "| " << setw(wStep-1) << left << step
         << "| " << setw(wStack-1) << left << stackStr
         << "| " << setw(wInput-1) << left << inputRem
         << "| " << setw(wAction-1) << left << action << "|\n";
}

void simulatePDA(const string &startSymbol, const string &userInput) {
    string inp = userInput + "$";
    vector<string> stack;
    stack.push_back("$");
    stack.push_back(startSymbol);
    cout << "\n+" << string(6,'-') << "+" << string(40,'-') << "+"
         << string(20,'-') << "+" << string(40,'-') << "+\n";
    cout << "| " << setw(5) << left << "Step"
         << "| " << setw(39) << left << "Stack"
         << "| " << setw(19) << left << "Input"
         << "| " << setw(39) << left << "Action" << "|\n";
    cout << "+" << string(6,'-') << "+" << string(40,'-') << "+"
         << string(20,'-') << "+" << string(40,'-') << "+\n";
    int pos = 0, step = 1;
    while (!stack.empty()) {
        string stackStr;
        for (int i = (int)stack.size()-1; i >= 0; i--) stackStr += "[" + stack[i] + "]";
        string inputRem = inp.substr(pos);
        string action = "-";
        string top = stack.back();
        if (!nonterms.count(top)) {
            string look(1, inp[pos]);
            if (top == look) {
                stack.pop_back();
                action = "Match '" + look + "'";
                pos++;
            } else {
                action = "Mismatch top=" + top + " input=" + look;
                printStepRow(step++, stackStr, inputRem, action);
                cout << "+" << string(6,'-') << "+" << string(40,'-') << "+"
                     << string(20,'-') << "+" << string(40,'-') << "+\n";
                cout << "\nRejected ❌\n";
                return;
            }
            printStepRow(step++, stackStr, inputRem, action);
            continue;
        }
        string look(1, inp[pos]);
        if (M[top].count(look)) {
            int pid = M[top][look];
            auto prod = prods[pid];
            string rhsdisp;
            for (auto &t : prod.rhs) {
                if (rhsdisp.size()) rhsdisp += " ";
                rhsdisp += t;
            }
            action = "Apply (" + to_string(pid) + "): " + top + "->" + rhsdisp;
            stack.pop_back();
            if (!(prod.rhs.size() == 1 && prod.rhs[0] == EPSILON)) {
                for (int j = (int)prod.rhs.size()-1; j >= 0; j--) stack.push_back(prod.rhs[j]);
            }
            printStepRow(step++, stackStr, inputRem, action);
            continue;
        } else {
            action = "No table entry for " + top + "," + look;
            printStepRow(step++, stackStr, inputRem, action);
            cout << "+" << string(6,'-') << "+" << string(40,'-') << "+"
                 << string(20,'-') << "+" << string(40,'-') << "+\n";
            cout << "\nRejected ❌\n";
            return;
        }
    }
    cout << "+" << string(6,'-') << "+" << string(40,'-') << "+"
         << string(20,'-') << "+" << string(40,'-') << "+\n";
    if (pos == (int)inp.size()) cout << "\nAccepted ✅\n";
    else cout << "\nRejected ❌\n";
}

int main() {
    int n;
    cout << "Enter number of productions: ";
    cin >> n;
    vector<pair<string,string>> userProds;
    for (int i = 0; i < n; i++) {
        string line; cin >> line;
        auto pos = line.find("->");
        string lhs = line.substr(0,pos);
        string rhs = line.substr(pos+2);
        nonterms.insert(lhs);
        userProds.push_back({lhs,rhs});
    }
    for (int i = 0; i < (int)userProds.size(); i++) {
        Production p;
        p.id = i+1;
        p.lhs = userProds[i].first;
        p.rhs = tokenizeRHS(userProds[i].second);
        prods.push_back(p);
    }
    string startSym = prods[0].lhs;
    terms.insert("$");
    computeNullable();
    computeFirst();
    computeFollow(startSym);
    vector<pair<string,string>> conflicts;
    buildPredictionTable(conflicts);
    printNullableFirstFollowTable();
    cout << "\nProductions:\n";
    for (auto &p : prods) {
        cout << "(" << p.id << ") " << p.lhs << " -> ";
        for (int j=0;j<p.rhs.size();j++) {
            if (j) cout << " ";
            cout << p.rhs[j];
        }
        cout << "\n";
    }
    printPredictionTable();
    while (true) {
        cout << "\nEnter input string or 'exit': ";
        string in; cin >> in;
        if (in == "exit") break;
        simulatePDA(startSym,in);
    }
}
