#!/usr/bin/env bash
cppcheck --enable=all --inconclusive --std=posix \
	--inline-suppr --quiet \
	--template='{file}:{line},{severity},{id},{message}' -I../include \
	--suppress="unusedFunction" \
	../include ../src ../tests \
	2>&1 \
	| grep -v "syntaxError,syntax error" \
	| grep -v "information,missingIncludeSystem"

