scp -r -P 2222 ./src aneedham@10.10.10.10:/home/aneedham/word-games/

g++ -std=c++20 src/*.cpp -o word_games

--mode letterboxed --letters uvjswitgebac --maxDepth 2 --minWordLength 3 --minUniqueLetters 2 --pruneRedundantPaths 1 --pruneDominatedClasses 0