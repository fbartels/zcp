#!/usr/bin/env bash

testrun() {
	if [ -z "`which $1`" ]; then
		echo $1 is needed to build
		false
	else
		echo "Running $@"
		"$@"
	fi
	if [ $? -ne 0 ]; then exit; fi
}


if ! pkg-config --version >/dev/null 2>&1 ; then
	echo 'Please install pkg-config >= 0.18 before running '$0
	exit 1
fi
if test `pkg-config --version` \< "0.18"; then
	echo 'Please install pkg-config >= 0.18 before running '$0
	exit 1
fi

testrun ${AUTOHEADER:-autoheader}
testrun ${ACLOCAL:-aclocal} -Iautoconf
testrun ${AUTOCONF:-autoconf}
testrun ${LIBTOOLIZE:-libtoolize} --copy --automake --force
testrun ${AUTOMAKE:-automake} --add-missing --copy --foreign --force

echo
echo 'Please run ./configure && make'
#./configure
#make
