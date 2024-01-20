#!/bin/bash

echo 'Installing fmtlib on your system...'

vcpkg install fmt

read -n 1
if [ $? = 0 ]; then
	exit ;
fi