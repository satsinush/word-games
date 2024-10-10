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
bool isValidWord(string& word, vector<string>& sides, int minLength = 3, int lastSide = -1, int depth = 0){  
    profiler.startProfile(__func__, depth!=0);

    //checks basic conditions before moving on to more complex recursive function
    if(word.length() < minLength && depth == 0){
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
            valid = isValidWord(newWord, sides, minLength, i, depth+1);
        }
    }
    profiler.endProfile(__func__, depth!=0);
    return(valid);
}

//returns a new vector with only the words that can be made from the given sides
//@param words list of words to filter
//@param sides list of letters to create each word
vector<string> filterWords(vector<string>& words, vector<string>& sides, int minLength = 3){
    profiler.startProfile(__func__);

    vector<string> newWords = {};
    for(string& word: words){
        if(isValidWord(word,sides, minLength)){
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

    profiler.endProfile(__func__);
}

//sorts vector of words by length of each word
//@param chains list of chains to sort
//@param ascending whether to sort in ascending (true) or descending (false)
void sortChains(vector<vector<string>>& chains, bool ascending=true){
    profiler.startProfile(__func__);

    auto compAscending = [](vector<string> a, vector<string> b){
        return(a.size() < b.size());
    };

    auto compDescending = [](vector<string> a, vector<string> b){
        return(a.size() > b.size());
    };

    if(ascending){
        stable_sort(chains.begin(),chains.end(),compAscending);
    }else{
        stable_sort(chains.begin(),chains.end(),compDescending);
    }

    profiler.endProfile(__func__);
}

//returns true if the last letter of the first word is the same as the first letter as the last word or if word1 is blank
bool lastMatchesFirst(string& word1, string& word2){
    profiler.startProfile(__func__);
    profiler.endProfile(__func__);
    return(word1[word1.size()-1] == word2[0] || word1 == "");
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

    if(depth == 0){
        sidesLeft = sides;

        //checks basic conditions before moving on to more complex recursive function

        //TODO cache sideString to not run loop every call
        string sidesString = sides[0];
        for(int i = 1; i<sides.size(); i++){
            sidesString.append(sides[i]);
        }

        if(chainString.size() < sidesString.size()){
            profiler.endProfile(__func__, depth!=0);
            return(false);
        }

        set<char> chainSet = set<char>(chainString.begin(), chainString.end());

        //TODO cache sideSet
        set<char> sidesSet  = set<char>(sidesString.begin(), sidesString.end());

        if(chainSet != sidesSet){
            profiler.endProfile(__func__, depth!=0);
            return(false);
        }
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
vector<vector<string>> getValidChains(vector<string>& words, string& lastWord, int maxDepth, int depth = 0){
    profiler.startProfile(__func__, depth!=0);

    if(words.size() == 0 || depth+1 > maxDepth){
        profiler.endProfile(__func__, depth!=0);

        //TODO return {lastWord}, see line 269

        return(vector<vector<string>>{});
    }

    if(depth == 0){
        lastWord = "";
    }
    
    vector<vector<string>> validStrings = {};

    //TODO only reserve what will be needed

    validStrings.reserve(words.size());
    bool hasMatched = false;
    for(int i = 0; i < words.size(); i++){

        //TODO skip words that use the same letters as the last word?

        string& w = words[i];
        if(lastMatchesFirst(lastWord, w)){
            hasMatched = true;

            //TODO remove creating new vector for new words, not necessary
            //running the loop one more time for each word will be more efficient than creating a new vector in each loop

            vector<string> newWords(words.begin(),words.begin()+i);
            newWords.insert(newWords.end(),words.begin()+i+1,words.end());

            validStrings.push_back({w});
            vector<string> v;
            for(vector<string>& s: getValidChains(newWords, w, maxDepth, depth+1)){
                //TODO return {w} in getValidChains to prevent additional insert statement
                //also consolidate to using one pushback statement per function
                
                v = {w};
                v.insert(v.end(),s.begin(),s.end());
                validStrings.push_back(v);
            }
        }
        else if(hasMatched){
            break; //breaks out of loop since all words are sorted alphabetically from left to right
        }
    }
    profiler.endProfile(__func__, depth!=0);
    return(validStrings);
}

//TODO combine filter chains and get chains to minimize memory usage

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

    ifstream file("words.txt");

    vector<string> allWords = {};
    string line;
    while (getline (file, line)) {
        // Output the text from the file
        allWords.push_back(line);
    }
    file.close();

    while(true)
    {
        profiler.start();

        vector<string> words = allWords;

        string side1, side2, side3, side4;

        bool allowInput = true;
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

        cout << "Minimum word length = " << minWordLength << "\n\n";

        vector<string> sides = {side1,side2,side3,side4};

        cout << "Filtering word list for valid words\n\n";
        words = filterWords(words, sides, minWordLength);
        cout << "Sorting words\n\n";

        vector<string> lengthSorted = words;
        sortWords(lengthSorted, true);

        for(string& w: lengthSorted){
            cout << w << "\n";
        }if(lengthSorted.size() > 0){cout << "\n";}

        cout << words.size() << " valid word(s) found\n\n";

        int maxDepth;

        if(allowInput){
            cout << "Max depth (min = 1, max = 4): ";
            maxDepth = getInt(1, 4);
            cout << "\n";
        }else{
            maxDepth = 2;
        }
        cout << "Max depth = " << maxDepth << "\n\n";

        cout << "Searching for all possible word chains\n\n";
        string blank = "";
        vector<vector<string>> chains = getValidChains(words, blank, maxDepth);
        cout << "Filtering word chains\n\n";
        chains = filterChains(chains, sides);
        cout << "Sorting word chains\n\n";
        sortChains(chains, false);
        
        for(vector<string>& v: chains){
            for(string& s: v){
                cout << s << " ";
            }
            cout << "\n";
        }if(chains.size() > 0){cout << "\n";}

        cout << chains.size() << " solution(s) found";

        profiler.end();
        //profiler.logProfilerData();
        
        getch();

        cout << "\n\n\n\n";
    }
    
    return(0);
}