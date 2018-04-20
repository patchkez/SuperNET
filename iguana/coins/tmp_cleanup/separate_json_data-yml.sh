#!/bin/bash
path="../"
data="${path}/methods"

cd $path

IFS='
'

if [ "$1" != "list" ];then
  echo "USAGE:"
  echo -e "  - pass 'list' argument to script to display converted to YAML e.g.:\n"
  echo -e "      $0 list\n"
  echo -e "  - use '>' redirection to file if needed."
  exit 1
fi



echo "assetchains:"
for coin in `ls -1 *_7776`; do
    # echo  "Processing ==== $coin ===="
    coin_name=`echo ${coin} | sed 's/_7776//g'`   
    echo "  ${coin_name}:" | tr '[:lower:]' '[:upper:]'
    echo "    iguana_payload:" 
    cat $coin | \
	sed -n 's/^.*data \(.*\)$/\1/gp' | \
	sed -e 's/,/\n/g' | \
	sed -e :a -e '$!N;s/,/,\n/;ta' -e 'P;D' | \
	sed -e 's/\\"//g' \
	    -e 's/:/: /g' \
	    -e 's/"{//g' \
	    -e 's/}"//g' \
	    -e 's/"\/"/\//g' \
	    -e 's/^/      /g' | \
	sed '/genesisblock: /{s/genesisblock: \(.[^\n]*\)/genesisblock: >\n        \1/g; s/.\{90\}/&\n        /g}'
	
done
