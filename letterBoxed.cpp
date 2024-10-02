#include <iostream>
#include <fstream>
#include <list>
#include <string>
#include <vector>
#include <algorithm>
#include <conio.h> 


using namespace std;

//returns true if the string s contains the char c
bool stringContains(string& s, char c){
    return(s.find(c) != string::npos);
}

string combineIntoString(vector<string>& v){
    string out = "";
    for(string& s: v){
        out += s;
    }
    return(out);
}

bool isValidWord(string& word, vector<string>& sides, int lastSide = -1){
    string combined = combineIntoString(sides);
    
    if(word.length() < 3 && lastSide == -1){
        return(false);
    }
    if(word == ""){
        return(true);
    }
    bool valid = false;
    for(int i=0; i<sides.size(); i++){
        if(i == lastSide){
            continue;
        }
        if(stringContains(sides[i], word[0])){
            string newWord = word.substr(1,word.size());
            valid = isValidWord(newWord, sides, i);
        }
    }
    return(valid);
}

vector<string> filterWords(vector<string>& words, vector<string>& sides){
    vector<string> newWords = {};
    for(string& word: words){
        if(isValidWord(word,sides)){
            newWords.push_back(word);
        }
    }
    return(newWords);
}

int stringLength(string& word){
    return(word.length());
}

vector<string> sortWords(vector<string>& words, int (*sorter)(string&), bool ascending=true){
    if(words.size() <= 1){
        return(words);
    }

    int pivot = sorter(words[0]);
    vector<string> less = {};
    vector<string> equal = {words[0]};
    vector<string> greater = {};
    for(int i = 1; i < words.size(); i++){
        string w = words[i];
        if(sorter(w) == pivot){
            equal.push_back(w);
        }
        if(sorter(w) > pivot){
            greater.push_back(w);
        }
        if(sorter(w) < pivot){
            less.push_back(w);
        }
    }
    less = sortWords(less, sorter, ascending);
    greater = sortWords(greater, sorter, ascending);
    vector<string> sorted = {};
    if(ascending){
        sorted.insert(sorted.end(), less.begin(), less.end());
        sorted.insert(sorted.end(), equal.begin(), equal.end());
        sorted.insert(sorted.end(), greater.begin(), greater.end());
    }else{
        sorted.insert(sorted.end(), greater.begin(), greater.end());
        sorted.insert(sorted.end(), equal.begin(), equal.end());
        sorted.insert(sorted.end(), less.begin(), less.end());
    }
    return(sorted);
}

int listLength(vector<string>& list){
    return(list.size());
}

vector<vector<string>> sortList(vector<vector<string>>& list, int (*sorter)(vector<string>&), bool ascending=true){
    if(list.size() <= 1){
        return(list);
    }

    int pivot = sorter(list[0]);
    vector<vector<string>> less = {};
    vector<vector<string>> equal = {list[0]};
    vector<vector<string>> greater = {};
    for(int i = 1; i < list.size(); i++){
        vector<string> w = list[i];
        if(sorter(w) == pivot){
            equal.push_back(w);
        }
        if(sorter(w) > pivot){
            greater.push_back(w);
        }
        if(sorter(w) < pivot){
            less.push_back(w);
        }
    }
    less = sortList(less, sorter, ascending);
    greater = sortList(greater, sorter, ascending);
    vector<vector<string>> sorted = {};
    if(ascending){
        sorted.insert(sorted.end(), less.begin(), less.end());
        sorted.insert(sorted.end(), equal.begin(), equal.end());
        sorted.insert(sorted.end(), greater.begin(), greater.end());
    }else{
        sorted.insert(sorted.end(), greater.begin(), greater.end());
        sorted.insert(sorted.end(), equal.begin(), equal.end());
        sorted.insert(sorted.end(), less.begin(), less.end());
    }
    return(sorted);
}

bool matches(string& a, string& b){
    if(a == ""){
        return(true);
    }
    return(a[a.size()-1] == b[0]);
}

bool containsAllLetters(vector<string>& v, vector<string>& sides){
    for(char c: combineIntoString(sides)){
        string line = combineIntoString(v);
        if(!stringContains(line, c)){
            return(false);
        }
    }
    return(true);
}

string stringListToString(vector<string> list){
    string out = "";
    int i = 0;
    for(string& s: list){
        if(i == 0){
            out.append(s);
        }else{
            out.append(s.substr(1,s.size()));
        }
        i++;
    }
    return(out);
}

bool stringVectorIsEmpty(vector<string> v){
    for(string s: v){
        if(s != ""){
            return(false);
        }
    }
    return(true);
}

bool stringContainsAllLetters(string& totalString, vector<string>& letters, vector<string>& lettersLeft, int lastSide = -1){
    if(stringVectorIsEmpty(lettersLeft)){
        return(true);
    }
    
    bool valid = false;
    for(int i=0; i<letters.size(); i++){
        if(lastSide == i){
            continue;
        }

        for(int c=0; c<letters[i].size(); c++){
            char letter = letters[i][c];
            char firstLetter = totalString[0];
            if(letter==firstLetter){
                vector<string> newLeft = lettersLeft;
                int index =  newLeft[i].find(letter);
                if(index != string::npos){
                    newLeft[i] = newLeft[i].substr(0,index)+newLeft[i].substr(index+1,newLeft[i].size());
                }

                string newString = totalString.substr(1,totalString.size());
                if(stringContainsAllLetters(newString, letters, newLeft, i)){
                    valid = true;
                }
            }   
        }
    }
    return(valid);
}

vector<vector<string>> getValidString(vector<string>& words, string& last, vector<string>& sides, int depth = 0, int maxDepth = 5){    
    if(words.size() == 0 || depth+1 > maxDepth){
        return(vector<vector<string>>{});
    }
    
    vector<vector<string>> validStrings = {};
    for(int i = 0; i < words.size(); i++){
        string w = words[i];
        if(matches(last, w)){
            vector<string> newWords(words.begin(),words.begin()+i);
            newWords.insert(newWords.end(),words.begin()+i+1,words.end());
            
            validStrings.push_back({w}); 
            vector<string> v;
            for(vector<string> s: getValidString(newWords, w, sides, depth+1, maxDepth)){
                v = {w};
                v.insert(v.end(),s.begin(),s.end());
                validStrings.push_back(v);
            }
        }
    }

    return(validStrings);
}

vector<vector<string>> filterLists(vector<vector<string>>& l, vector<string>& sides){
    vector<vector<string>> out = {};
    for(vector<string>& v: l){
        string s = stringListToString(v);
        if(stringContainsAllLetters(s, sides, sides)){
            out.push_back(v);
        }
    }
    return(out);
}

string stringToLower(string d){
    string data = d;
    std::transform(data.begin(), data.end(), data.begin(),
    [](unsigned char c){ return std::tolower(c); });
    return(data);
}

int main(){

    cout << "Reading word file\n\n";

    ifstream file("words.txt");

    vector<string> words = {};
    string line;
    while (getline (file, line)) {
        // Output the text from the file
        words.push_back(line);
    }
    file.close();

    string top;
    string bottom;
    string left;
    string right;

    cout << "Side 1: ";
    cin >> top;
    cout << "Side 2: ";
    cin >> bottom;
    cout << "Side 3: ";
    cin >> left;
    cout << "Side 4: ";
    cin >> right;

    vector<string> allSides = {stringToLower(top), stringToLower(bottom), stringToLower(left), stringToLower(right)};

    cout << "\nFiltering word list for valid words\n\n";
    words = filterWords(words, allSides);
    cout << "Sorting word list\n\n";
    words = sortWords(words, stringLength, false);

    cout << "\n";
    for(string& w: words){
        cout << w << "\n";
    }cout << "\n";

    cout << words.size() << " valid word(s) found\n\n";

    int maxDepth;

    cout << "Max depth: ";
    cin >> maxDepth;

    cout << "\nSearching for all possible word strings (Max depth = " << maxDepth << ")\n\n";
    string last = "";
    vector<vector<string>> allLists = getValidString(words, last, allSides, 0, maxDepth);
    cout << "Filtering string list\n\n";
    allLists = filterLists(allLists, allSides);
    cout << "Sorting string list\n\n";
    allLists = sortList(allLists, listLength, false);
    
    
    for(vector<string>& v: allLists){
        for(string& s: v){
            cout << s << " ";
        }
        cout << "\n";
    }cout << "\n";

    cout << allLists.size() << " solution(s) found";
    
    getch();

    return(0);
}