#!/bin/bash
path="../"
data="${path}/methods"


URL="https://raw.githubusercontent.com/jl777/komodo/beta/src/assetchains"

cd $path

if [ "$1" != "list" ];then
  echo "USAGE:"
  echo -e "  - pass 'list' argument to script to display converted to YAML e.g.:\n"
  echo -e "      $0 list\n"
  echo -e "  - use '>' redirection to file if needed."
  exit 1
fi


IFS='
'

SUPPLIES=(`curl $URL -s | grep "^komodo_asset" | awk '{print $2 " " $3 }'`)

echo "assetchains:"
for coin in `ls -1 *_7776`; do
    # echo  "Processing ==== $coin ===="
    coin_name=`echo ${coin} | sed 's/_7776//g' | tr '[:lower:]' '[:upper:]'`   
    echo "  ${coin_name}:"
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

    LIST="" 
    for SUPP in `echo "${SUPPLIES[*]}"`;do
	ASE=`echo ${SUPP} | awk '{print $1}'`
	SUPPLY=`echo ${SUPP} | awk '{print $2}'`
	if [ "${ASE}" = "${coin_name}" ];then
	    LIST="${SUPPLY}"
	fi
    done
    if [ -n "$LIST" ];then
	echo "    supply: $LIST" 
    else
	echo "    supply:"
    fi 	
done
