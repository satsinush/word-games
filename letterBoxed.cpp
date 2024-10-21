#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <conio.h> 
#include <set>
#include <cctype>

#include "profiler.cpp"

using namespace std;

Profiler::Profiler profiler;

struct ChainStruct
{
    string chainString;
    string printString;
    int length = 1;
};

struct SideStruct{
    vector<string> sides;
    string sideString;
    set<char> sideSet;
};

struct LetterIndexer{
    int start;
    int end;
};

//returns true if the word can be made with the sides given
//@param word string to check
//@param sides list of letters used to create the word
//@param minLength minimum length for each word
//@param lastSide used for recursion to skip the last used side
//@param depth keeps track of recursion depth
bool isValidWord(const string& word, const vector<string>& sides, const int minLength = 3, const int lastSide = -1, const int depth = 0){  
    //profiler.profileStart(__func__, depth!=0);

    //checks basic conditions before moving on to more complex recursive function
    if(word.length() < minLength && depth == 0){
        //profiler.profileEnd(__func__, depth!=0);
        return(false);
    }
    if(depth == word.size()){
        //profiler.profileEnd(__func__, depth!=0);
        return(true);
    }

    bool valid = false;
    for(int i=0; i<sides.size(); i++){
        if(i == lastSide){
            continue;
        }
        if(sides[i].find(word[depth]) != string::npos){
            valid = isValidWord(word, sides, minLength, i, depth+1);
        }
    }
    //profiler.profileEnd(__func__, depth!=0);
    return(valid);
}

//returns a new vector with only the words that can be made from the given sides
//@param words list of words to filter
//@param sides list of letters to create each word
//@param minLength minimum length for each word
vector<string> filterWords(vector<string>& words, const vector<string>& sides, const int minLength = 3){
    //profiler.profileStart(__func__);

    vector<string> newWords = {};
    newWords.reserve(words.size()/150);
    for(string& word: words){
        if(isValidWord(word,sides, minLength)){
            newWords.push_back(word);
        }
    }

    //profiler.profileEnd(__func__);
    return(newWords);
}

//sorts vector of words by length of each word
//@param words list of words to sort
//@param ascending whether to sort in ascending (true) or descending (false)
void sortStrings(vector<string>& words, const bool ascending=true){
    //profiler.profileStart(__func__);

    auto compAscending = [](string a, string b){
        return(a.size() < b.size());
    };

    auto compDescending = [](string a, string b){
        return(a.size() > b.size());
    };

    if(ascending){
        stable_sort(words.begin(),words.end(),compAscending);
    }else{
        stable_sort(words.begin(),words.end(),compDescending);
    }

    //profiler.profileEnd(__func__);
}

//sorts vector of words by length of each word
//@param chains list of chains to sort
//@param ascending whether to sort in ascending (true) or descending (false)
void sortChains(vector<ChainStruct>& chains, const bool ascending=true){
    //profiler.profileStart(__func__);

    auto compAscending = [](ChainStruct a, ChainStruct b){
        return(a.length < b.length);
    };

    auto compDescending = [](ChainStruct a, ChainStruct b){
        return(a.length > b.length);
    };

    if(ascending){
        stable_sort(chains.begin(),chains.end(),compAscending);
    }else{
        stable_sort(chains.begin(),chains.end(),compDescending);
    }

    //profiler.profileEnd(__func__);
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

//returns true if the string provided can use all letters on each side.
//Uses a recursive algorithm to check all possibilities if there are duplicate letters
//@param chainString string to check use of all letters
//@param sideStruct sideStruct that contains sides to create each word
//@param sidesLeft list of letters remaining to be used, same as sides on first call, updated during recursion
//@param lastSide used for recursion to skip the last used side
//@param depth keeps track of recursion depth
bool stringUsesAllLetters(const string& chainString, const SideStruct& sideStruct, vector<string>& sidesLeft, const int lastSide = -1, const int depth=0){
    //profiler.profileStart(__func__, depth!=0);

    if(depth == 0){
        //checks basic conditions before moving on to more complex recursive function

        if(chainString.size() < sideStruct.sideString.size()){
            //profiler.profileEnd(__func__, depth!=0);
            return(false);
        }

        set<char> chainSet = set<char>(chainString.begin(), chainString.end()); //faster than inline for some reason

        if(chainSet != sideStruct.sideSet){
            //profiler.profileEnd(__func__, depth!=0);
            return(false);
        }
    }

    if(isEmpty(sidesLeft)){
        //profiler.profileEnd(__func__, depth!=0);
        return(true);
    }
    
    bool valid = false;
    for(int i=0; i<sideStruct.sides.size(); i++){
        if(lastSide == i){
            continue;
        }

        for(int c=0; c<sideStruct.sides[i].size(); c++){
            if(sideStruct.sides[i][c] == chainString[depth]){

                int index =  sidesLeft[i].find(sideStruct.sides[i][c]);
                bool result;
                if(index != string::npos){
                    vector<string> newLeft(sidesLeft);
                    //newLeft[i].erase(newLeft[i].begin()+index); //slower than for loop
                    
                    newLeft[i].clear();
                    for(int x = 0; x < index; x++){
                        newLeft[i] += sidesLeft[i][x];
                    }
                    for(int x = index+1; x < sidesLeft[i].size(); x++){
                        newLeft[i] += sidesLeft[i][x];
                    }

                    result = stringUsesAllLetters(chainString, sideStruct, newLeft, i, depth+1);
                }else{
                    result = stringUsesAllLetters(chainString, sideStruct, sidesLeft, i, depth+1);
                }

                if(result){
                    valid = true;
                }
            }   
        }
    }
    //profiler.profileEnd(__func__, depth!=0);
    return(valid);
}


//Returns a new vector of chains that can be created from the given sides
//@param words words used in each chain
//@param sideStruct sideStruct that contains sides to create each word
//@param maxDepth maximum length of each chain
vector<ChainStruct> getAllChains(vector<string>& words, SideStruct& sideStruct, LetterIndexer letterIndexes[], const int maxDepth){
    //profiler.profileStart(__func__);
    
    vector<ChainStruct> allChains = {};
    allChains.reserve(pow(words.size(),(maxDepth-1)/2));
    
    vector<ChainStruct> validChains = {};
    allChains.reserve(words.size()*maxDepth);
    
    int lastIndex = 0;
    for(string& word: words){
        allChains.push_back({word, word});
        
        if(stringUsesAllLetters(word, sideStruct, sideStruct.sides)){
            validChains.push_back({word, word});
        }
    }
    int numChains;
    for(int i = 1; i<maxDepth-1; i++){
        numChains = allChains.size();
        for(int c = lastIndex; c < numChains; c++){
            int startIndex = letterIndexes[(int)allChains[c].chainString[allChains[c].chainString.size()-1]-97].start;
            int endIndex = letterIndexes[(int)allChains[c].chainString[allChains[c].chainString.size()-1]-97].end;
            for(int w = startIndex; w < endIndex; w++){
                allChains.push_back(allChains[c]);
                ChainStruct& chain = allChains[allChains.size()-1];
                chain.chainString.append(words[w].begin()+1,words[w].end());
                chain.printString += " "+words[w];

                if(stringUsesAllLetters(chain.chainString, sideStruct, sideStruct.sides)){
                    chain.length = i+1;
                    validChains.push_back(chain);
                }
            }
        }
        lastIndex = numChains;
    }

    numChains = allChains.size();
    for(int c = lastIndex; c < numChains; c++){
        int startIndex = letterIndexes[(int)allChains[c].chainString[allChains[c].chainString.size()-1]-97].start;
        int endIndex = letterIndexes[(int)allChains[c].chainString[allChains[c].chainString.size()-1]-97].end;
        for(int w = startIndex; w < endIndex; w++){
            string s = allChains[c].chainString+string(words[w].begin()+1,words[w].end());
            if(stringUsesAllLetters(s, sideStruct, sideStruct.sides)){
                validChains.push_back(ChainStruct{"",allChains[c].printString+" "+words[w],maxDepth});
            }
        }
    }

    //profiler.profileEnd(__func__);
    return validChains;
}

//converts each uppercase char in the string to lower case
//@param d string to update
string stringToLower(string& d){
    string data;
    for(char c: d){
        data += tolower(c);
    }
    return(data);
}

int getInt(double min = -INFINITY, double max = INFINITY){
    while(true){
        string s;
        if(cin >> s){
            bool digit = true;
            for(char c: s){
                if(!isdigit(c)){
                    digit = false;
                }
            }
            if(!digit){
                continue;
            }

            int i = stoi(s);

            if(i < min){
                continue;
            }
            if(i > max){
                continue;
            }
            return(i);
        }else{
            cin.clear();
            cin.ignore();
            continue;
        }
    }
}

string getSide(){
    while(true){
        string s;
        if(cin >> s){
            if(s.size() != 3){
                continue;
            }

            bool alpha = true;
            for(char c: s){
                if(!isalpha(c)){
                    alpha = false;
                }
            }
            if(!alpha){
                continue;
            }

            return(stringToLower(s));
        }else{
            cin.clear();
            cin.ignore();
            continue;
        }
    }
}


int main(){
    cout << "Reading word file\n\n";

    bool allowInput = true;
    if(!allowInput){
        profiler.start();
    }
    ifstream file("words.txt");

    vector<string> allWords = {};
    string line;
    while (getline (file, line)) {
        // Output the text from the file
        allWords.push_back(line);
    }
    file.close();

    int loops = 0;
    while(allowInput || loops == 0)
    {
        vector<string> words = allWords;

        string side1, side2, side3, side4;

        int minWordLength = 3;
        if(allowInput){
            cout << "Side 1: ";
            side1 = getSide();
            cout << "Side 2: ";
            side2 = getSide();
            cout << "Side 3: ";
            side3 = getSide();
            cout << "Side 4: ";
            side4 = getSide();

            cout << "\nMinimum word length (min = 3): ";
            minWordLength = getInt(3);
            cout << "\n";
        }else{
            side1 = "uvj";
            side2 = "swi";
            side3 = "tge";
            side4 = "bac";
        }

        SideStruct sideStruct;
        sideStruct.sideString = side1;
        sideStruct.sideString.append(side2);
        sideStruct.sideString.append(side3);
        sideStruct.sideString.append(side4);

        cout << "Minimum word length = " << minWordLength << "\n\n";

        sideStruct.sides = {side1,side2,side3,side4};
        sideStruct.sideSet = set<char>(sideStruct.sideString.begin(), sideStruct.sideString.end());

        cout << "Filtering word list for valid words\n\n";
        words = filterWords(words, sideStruct.sides, minWordLength);

        LetterIndexer letterIndexers[26]{};
        char currentChar = words[0][0];
        LetterIndexer currentIndexer{0,0};
        for(int i = 0; i < words.size(); i++){
            if(currentChar != words[i][0]){
                currentIndexer.end = i;
                letterIndexers[(int)words[i-1][0]-97] = LetterIndexer{currentIndexer};

                currentIndexer.start = i;
            }
            currentChar = words[i][0];
        }
        letterIndexers[(int)currentChar-97] = LetterIndexer{currentIndexer.start, (int)words.size()-1};
        cout << "Sorting words\n\n";

        vector<string> lengthSorted = words;
        sortStrings(lengthSorted, true);

        for(string& w: lengthSorted){
            cout << w << "\n";
        }if(lengthSorted.size() > 0){cout << "\n";}

        cout << words.size() << " valid word(s) found\n\n";

        int maxDepth = 2;

        if(allowInput){
            cout << "Max depth (min = 1, max = 4): ";
            maxDepth = getInt(1, 4);
            cout << "\n";
        }
        cout << "Max depth = " << maxDepth << "\n\n";

        cout << "Searching for all possible word chains\n\n";
        vector<ChainStruct> chains = getAllChains(words, sideStruct, letterIndexers, maxDepth);
        cout << "Sorting word chains\n\n";
        sortChains(chains, false);

        if(loops == 0 && !allowInput){
            profiler.end();
            profiler.logProfilerData();
        }
        
        int lastLength = 0;
        for(ChainStruct& c: chains){
            if(lastLength != c.length && lastLength != 0){
                cout << "\n";
            }
            cout << c.printString << "\n";
            lastLength = c.length;
        }if(chains.size() > 0){cout << "\n";}

        cout << chains.size() << " solution(s) found";

        getch();

        cout << "\n\n\n\n";
        loops++;
    }
    
    return(0);
}