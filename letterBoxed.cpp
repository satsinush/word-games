#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <conio.h> 

#include "profiler.cpp"


using namespace std;

Profiler::Profiler profiler;

//returns true if the string contains the given char
//@param s string to search in
//@param c char to search for
bool stringContainsChar(string& s, char c){
    return(s.find(c) != string::npos);
}

//returns true if the word can be made with the sides given
//@param word string to check
//@param sides list of letters used to create the word
//@param lastSide used for recursion to skip the last used side
bool isValidWord(string& word, vector<string>& sides, int lastSide = -1, int depth = 0){  
    profiler.startProfile(__func__, depth!=0);
    if(word.length() < 3 && lastSide == -1){
        profiler.endProfile(__func__, depth!=0);
        return(false);
    }
    if(word == ""){
        profiler.endProfile(__func__, depth!=0);
        return(true);
    }
    bool valid = false;
    for(int i=0; i<sides.size(); i++){
        if(i == lastSide){
            continue;
        }
        if(stringContainsChar(sides[i], word[0])){
            string newWord = word.substr(1,word.size());
            valid = isValidWord(newWord, sides, i, depth+1);
        }
    }
    profiler.endProfile(__func__, depth!=0);
    return(valid);
}

//returns a new vector with only the words that can be made from the given sides
//@param words list of words to filter
//@param sides list of letters to create each word
vector<string> filterWords(vector<string>& words, vector<string>& sides){
    profiler.startProfile(__func__);

    vector<string> newWords = {};
    for(string& word: words){
        if(isValidWord(word,sides)){
            newWords.push_back(word);
        }
    }

    profiler.endProfile(__func__);
    return(newWords);
}


//sorts vector of words by length of each word
//@param words list of words to sort
//@param ascending whether to sort in ascending (true) or descending (false)
void sortWords(vector<string>& words, bool ascending=true){
    profiler.startProfile(__func__);

    auto compAscending = [](string& a, string& b){
        return(a.size() < b.size());
    };

    auto compDescending = [](string& a, string& b){
        return(a.size() > b.size());
    };

    if(ascending){
        sort(words.begin(),words.end(),compAscending);
    }else{
        sort(words.begin(),words.end(),compAscending);
    }

    profiler.endProfile(__func__);
}

//sorts vector of words by length of each word
//@param chains list of chains to sort
//@param ascending whether to sort in ascending (true) or descending (false)
void sortChains(vector<vector<string>>& chains, bool ascending=true){
    profiler.startProfile(__func__);

    auto compAscending = [](vector<string>& a, vector<string>& b){
        return(a.size() < b.size());
    };

    auto compDescending = [](vector<string>& a, vector<string>& b){
        return(a.size() > b.size());
    };

    if(ascending){
        sort(chains.begin(),chains.end(),compAscending);
    }else{
        sort(chains.begin(),chains.end(),compAscending);
    }

    profiler.endProfile(__func__);
}

//returns true if the last letter of the first word is the same as the first letter as the last word or if word1 is blank
bool lastMatchesFirst(string& word1, string& word2){
    profiler.startProfile(__func__);
    if(word1 == ""){
        profiler.endProfile(__func__);
        return(true);
    }
    profiler.endProfile(__func__);
    return(word1[word1.size()-1] == word2[0]);
}

//appends each word in the chain to a string but omits the first letter of each word after the first
//@param chain list of words to turn into a string
string chainToString(vector<string>& chain){
    profiler.startProfile(__func__);
    string out = "";
    int i = 0;
    for(string& s: chain){
        if(i == 0){
            out.append(s);
        }else{
            out.append(s.substr(1,s.size()));
        }
        i++;
    }
    profiler.endProfile(__func__);
    return(out);
}

//returns true if the vector only contains empty strings
//@param chain chain to check if empty
bool isEmpty(vector<string>& chain){
    for(string& s: chain){
        if(s != ""){
            return(false);
        }
    }
    return(true);
}

//returns true if the string provided can use all letters on each side
//Uses a recursive algorithm to check all possibilities if there are duplicate letters
//@param chainString string to check use of all letters
//@param sides list of letters to the chain
//@param sidesLeft list of letters remaining to be used, same as sides on first call, updated during recursion
//@param lastSide used for recursion to skip the last used side
bool stringUsesAllLetters(string& chainString, vector<string>& sides, vector<string>& sidesLeft, int lastSide = -1, int depth=0){
    profiler.startProfile(__func__, depth!=0);
    if(lastSide == -1){
        sidesLeft = sides;
    }
    if(isEmpty(sidesLeft)){
        profiler.endProfile(__func__, depth!=0);
        return(true);
    }
    
    bool valid = false;
    for(int i=0; i<sides.size(); i++){
        if(lastSide == i){
            continue;
        }

        for(int c=0; c<sides[i].size(); c++){
            char letter = sides[i][c];
            char firstLetter = chainString[0];
            if(letter==firstLetter){
                string newString = chainString.substr(1,chainString.size());

                int index =  sidesLeft[i].find(letter);
                bool result;
                if(index != string::npos){
                    vector<string> newLeft;

                    newLeft = sidesLeft;
                    newLeft[i] = newLeft[i].substr(0,index)+newLeft[i].substr(index+1,newLeft[i].size());

                    result = stringUsesAllLetters(newString, sides, newLeft, i, depth+1);
                }else{
                    result = stringUsesAllLetters(newString, sides, sidesLeft, i, depth+1);
                }

                if(result){
                    valid = true;
                }
            }   
        }
    }
    profiler.endProfile(__func__, depth!=0);
    return(valid);
}

//returns a list of all valid chains that can be made from the given words
//@param words list of words to turn into chains
//@param maxDepth specifies the max number of words in each chain
//@param lastWord used for recurion to determine the last word in the chain
//@param depth used to determine the depth of recursion in the function
vector<vector<string>> getValidChains(vector<string>& words, int maxDepth = 5, string lastWord = "", int depth = 0){
    profiler.startProfile(__func__, depth!=0);
    if(words.size() == 0 || depth+1 > maxDepth){
        profiler.endProfile(__func__, depth!=0);
        return(vector<vector<string>>{});
    }
    
    vector<vector<string>> validStrings = {};
    validStrings.reserve(words.size());
    for(int i = 0; i < words.size(); i++){
        string& w = words[i];
        if(lastMatchesFirst(lastWord, w)){
            vector<string> newWords(words.begin(),words.begin()+i);
            newWords.insert(newWords.end(),words.begin()+i+1,words.end());
            
            validStrings.push_back({w}); 
            vector<string> v;
            for(vector<string>& s: getValidChains(newWords,maxDepth, w, depth+1)){
                v = {w};
                v.insert(v.end(),s.begin(),s.end());
                validStrings.push_back(v);
            }
        }
    }
    profiler.endProfile(__func__, depth!=0);
    return(validStrings);
}

//returns a new list of chains that all contain every letter from the given the sides
//@param chains list of chains to filter
//@param sides list of letters to create each chain
vector<vector<string>> filterChains(vector<vector<string>>& chains, vector<string>& sides){
    profiler.startProfile(__func__);
    vector<vector<string>> out = {};
    out.reserve(chains.size());
    for(vector<string>& v: chains){
        string s = chainToString(v);
        if(stringUsesAllLetters(s, sides, sides)){
            out.push_back(v);
        }
    }
    profiler.endProfile(__func__);
    return(out);
}

//converts each uppercase char in the string to lower case
//@param d string to update
void stringToLower(string& d){
    std::transform(d.begin(), d.end(), d.begin(),
    [](unsigned char c){ return std::tolower(c); });
}

int main(){
    profiler.start();

    cout << "Reading word file\n\n";

    ifstream file("words.txt");

    vector<string> words = {};
    string line;
    while (getline (file, line)) {
        // Output the text from the file
        words.push_back(line);
    }
    file.close();

    string side1, side2, side3, side4;

    bool allowInput = true;

    if(allowInput){
        cout << "Side 1: ";
        cin >> side1;
        cout << "Side 2: ";
        cin >> side2;
        cout << "Side 3: ";
        cin >> side3;
        cout << "Side 4: ";
        cin >> side4;
    }else{
        side1 = "uvj";
        side2 = "swi";
        side3 = "tge";
        side4 = "bac";
    }

    stringToLower(side1);
    stringToLower(side2);
    stringToLower(side3);
    stringToLower(side4);

    vector<string> sides = {side1,side2,side3,side4};

    cout << "\nFiltering word chain for valid words\n\n";
    words = filterWords(words, sides);
    cout << "Sorting word chain\n\n";
    sortWords(words, true);

    cout << "\n";
    for(string& w: words){
        cout << w << "\n";
    }cout << "\n";

    cout << words.size() << " valid word(s) found\n\n";

    int maxDepth;

    if(allowInput){
        cout << "Max depth: ";
        cin >> maxDepth;
    }else{
        maxDepth = 2;
    }

    cout << "\nSearching for all possible word strings (Max depth = " << maxDepth << ")\n\n";
    vector<vector<string>> chains = getValidChains(words, maxDepth);
    cout << "Filtering string chain\n\n";
    chains = filterChains(chains, sides);
    cout << "Sorting string chain\n\n";
    sortChains(chains, false);
    
    
    for(vector<string>& v: chains){
        for(string& s: v){
            cout << s << " ";
        }
        cout << "\n";
    }cout << "\n";

    cout << chains.size() << " solution(s) found";

    profiler.end();
    profiler.logProfilerData();
    
    getch();

    return(0);
}