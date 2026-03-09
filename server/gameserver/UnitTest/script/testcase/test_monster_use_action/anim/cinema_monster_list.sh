grep typeId *.txt | sed 's/.*"typeId": \([0-9]*\).*/\1,/g'  > cinema_monster.txt

