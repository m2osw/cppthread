#!/bin/sh
#
# Sample script to run make without having to retype the long path each time
# This will work if you built the environment using our ~/bin/build-snap script

PROCESSORS=4

case $1 in
"-l")
	make -C ../../../BUILD/contrib/cppthread 2>&1 | less -SR
	;;

"-d")
	rm -rf ../../../BUILD/contrib/cppthread/doc/cppthread-doc-1.0.tar.gz
	make -C ../../../BUILD/contrib/cppthread
	;;

"-i")
	make -j${PROCESSORS} -C ../../../BUILD/contrib/cppthread install
	;;

"-t")
	(
		if make -j${PROCESSORS} -C ../../../BUILD/contrib/cppthread
		then
			shift
			../../../BUILD/contrib/cppthread/tests/unittest --progress $*
		fi
	) 2>&1 | less -SR
	;;

"")
	make -j${PROCESSORS} -C ../../../BUILD/contrib/cppthread
	;;

*)
	echo "error: unknown command line option \"$1\""
	;;

esac
