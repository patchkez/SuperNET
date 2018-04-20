#!/bin/bash
path="../"
data="${path}/methods"

cd $path

IFS='
'

echo "production =["
for coin in `ls -1 *_7776`; do
    # echo  "Processing ==== $coin ===="
    coin_name=`echo ${coin} | sed 's/_7776//g'`   
    
    X=$(cat $coin | sed -n 's/^.*data \(.*\)$/\1/gp' | sed \
	-e 's/\\//g' \
	-e 's/,/,\n/g' \
	-e 's/}"/\n}/g' \
	-e 's/":"/" : "/g' \
	-e 's/}//g' )

    echo $X | sed \
    -e 's/, /,\n/g' \
    -e 's/^/    /g' \
    -e "s/^    \"{/${coin_name} = {\n/g" \
    -e :a -e '$!N;s/\n=/ /;ta' -e 'P;D' 
    # echo $Z # | sed  -e "s/\"{/${coin_name} = {\n/g"
echo "},"
done
echo "]"
