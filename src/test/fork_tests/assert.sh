#!/bin/bash

function ASSERT_EQUALS {
    if [[ "$(echo "$1" | tr -d '\n\r')" != "$(echo "$2" | tr -d '\n\r')" ]]
    then
	echo "ASSERT EQUALS FAILS: $1 != $2"
	return 1
    fi
}

function ASSERT_LESSTHAN {
    a="$(echo "$1" | tr -d '\n\r')"
    b="$(echo "$2" | tr -d '\n\r')"
    res="$(echo "$a"'<'"$b" | bc -l | tr -d '\n\r')"
    if [[ "$res" == "0" ]]
    then
	echo "ASSERT LESSTHAN FAILS: $1 >= $2"
	return 1
    fi
}

function ASSERT_GREATERTHAN {
    a="$(echo "$1" | tr -d '\n\r')"
    b="$(echo "$2" | tr -d '\n\r')"
    res="$(echo "$a"'>'"$b" | bc -l | tr -d '\n\r')"
    if [[ "$res" == "0" ]]
    then
	echo "ASSERT GREATERTHAN FAILS: $1 <= $2"
	return 1
    fi
}
