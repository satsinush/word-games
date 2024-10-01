#include <iostream>
#include <fstream>
#include <list>
#include <string>
#include <vector>

using namespace std;

//returns true if the string s contains the char c
bool stringContains(string& s, char c){
    return(s.find(c) != string::npos);
}

bool wordUsesCharacters(string& word, string& characters){
    for(char c: word){
        if(!stringContains(characters, c)){
            return(false);
        }
    }
    return(true);
}

string combineLists(vector<string>& list){
    string output;
    for(string& w: list){
        for(char c: w){
            if(!stringContains(output, c)){
                output.append(w);
            }
        }
    }
    
    return(output);
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

int stringLength(string word){
    return(word.length());
}

vector<string> sortWords(vector<string>& words, int (*sorter)(string)){
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
    less = sortWords(less, sorter);
    greater = sortWords(greater, sorter);
    less.insert(less.begin(), equal.begin(), equal.end());
    less.insert(less.begin(), greater.begin(), greater.end());
    return(less);
}

bool matches(string a, string b){
    if(a == ""){
        return(true);
    }
    return(a[a.size()-1] == b[0]);
}

void printList(vector<string> v){
    for(string& s: v){
        cout << s << " ";
    }
    cout << "\n";
}

bool containsAllLetters(vector<string>& v, vector<string>& letters){
    for(char c: combineIntoString(letters)){
        string line = combineIntoString(v);
        if(!stringContains(line, c)){
            return(false);
        }
    }
    return(true);
}

vector<vector<string>> getValidString(vector<string>& words, string last, vector<string> sides, int depth = 0, int maxDepth = 5){    
    if(words.size() == 0 || depth+1 > maxDepth){
        return(vector<vector<string>>{{}});
    }
    
    vector<vector<string>> validStrings = {};
    for(int i = 0; i < words.size(); i++){
        string w = words[i];
        if(matches(last, w)){

            vector<string> newWords(words.begin(),words.begin()+i);
            newWords.insert(newWords.end(),words.begin()+i+1,words.end());
            bool longerList = false;
            vector<string> v;
            for(vector<string> s: getValidString(newWords, w, sides, depth+1, maxDepth)){
                longerList = true;
                v = {w};
                v.insert(v.end(),s.begin(),s.end());
                validStrings.push_back(v);
                
            }
            if(!longerList){
                v = {w};
                validStrings.push_back(v);
                
            }
        }
    }

    return(validStrings);
}

vector<vector<string>> filterLists(vector<vector<string>>& l, vector<string>& sides){
    vector<vector<string>> out = {};
    for(vector<string>& v: l){
        if(containsAllLetters(v, sides)){
            out.push_back(v);
        }
    }
    return(out);
}

int main(){
    ifstream file("words.txt");

    vector<string> words = {};
    string line;
    while (getline (file, line)) {
        // Output the text from the file
        words.push_back(line);
    }
    file.close();

    vector<string> testSides = {"abc","def","nop","lpr"}; 
    string testWord = "apple";


    string top = "ozu";
    string bottom = "nri";
    string left = "lfa";
    string right = "mbt";

    /*cout << "Top\n";
    cin >> top;
    cout << "\nBottom\n";
    cin >> bottom;
    cout << "\nLeft\n";
    cin >> left;
    cout << "\nRight\n";
    cin >> right;*/

    vector<string> allSides = {top, bottom, left, right};

    words = filterWords(words, allSides);
    words = sortWords(words, stringLength);

    cout << "\n\n";

    //words = vector<string>(words.begin(),words.begin()+50);

    for(string& word: words){
        cout << word << "\n";
    }cout << "\n\n";

    vector<vector<string>> allLists = getValidString(words, "", allSides, 0, 4);
    
    
    for(vector<string>& v: filterLists(allLists, allSides)){
        for(string& s: v){
            cout << s << " ";
        }
        cout << "\n";
    }

    cout << "\n\nDone";
    getchar();

    return(0);
}