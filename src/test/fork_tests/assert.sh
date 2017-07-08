#!/bin/bash

function ASSERT_EQUALS {
    if [[ "$1" != "$2" ]]
    then
	echo "ASSERT EQUALS FAILS: $1 != $2"
	return 1
    fi
}

function ASSERT_LESSTHAN {
    res=$(echo $1'<'$2 | bc -l)
    if [[ "$res" == "0" ]]
    then
	echo "ASSERT LESSTHAN FAILS: $1 >= $2"
	return 1
    fi
}

function ASSERT_GREATERTHAN {
    res=$(echo $1'>'$2 | bc -l)
    if [[ "$res" == "0" ]]
    then
	echo "ASSERT GREATERTHAN FAILS: $1 <= $2"
	return 1
    fi
}
