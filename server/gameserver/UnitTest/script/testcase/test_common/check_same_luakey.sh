luac -l $1 | grep SETTABLE.* | awk '{ map[$8] = map[$8]+1  }END {for( k in map) {if (map[k] > 1)  print k, map[k];fi}}'
