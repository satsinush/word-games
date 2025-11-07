## Wordle
```
wordle --word-length 5 --max-depth 1 --exclude-uncommon-words true --guesses "STEAL 20100;CRANE 01002" -o results/wordle.txt
```

## Mastermind
```
mastermind --guesses "RGBC 1 2;MYRC 1 2" --pegs 4 --colors "RGBCMY" --allow-duplicates true --max-depth 1 -o results/mastermind.txt
```

## Spelling Bee
```
spellingbee --letters nhmkace --exclude-uncommon-words false --must-include-first-letter true --reuse-letters true -o results/spellingbee.txt
```

## Letter Boxed
```
letterboxed --letters uvjswitgebac --preset 0 --max-depth 3 --min-word-length 4 --min-unique-letters 3 --prune-paths true --prune-classes false -o results/letterboxed.txt
```

## Dungleon
```
dungleon --guesses "ar kn bo ne fr 00010" --solutions "vi zo ne sk bt" --max-depth 0 --exclude-impossible false -o results/dungleon.txt
```
