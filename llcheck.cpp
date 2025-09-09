#include<bits/stdc++.h>
using namespace std;
vector<char> terminals, non_terminals;
vector<pair<char, vector<string>>> productions;
int main()
{
    cout << "Insert the Terminals In Your Grammar:\n";
    while(true)
    {
        char c;
        cin >> c;
        if(c==NULL)
            break;
        else
            terminals.push_back(c);
    }
    cout << "Insert The Non-Terminals In your Grammar:\n";
    while(true)
    {
        char c;
        cin >> c;
        if(c==NULL)
            break;
        else
            non_terminals.push_back(c);
    }
    
}