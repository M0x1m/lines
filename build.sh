exech(){
	OUTPUT=$("$@")
	echo $@
	if [ "$?" != "0" ] ; then
		exec echo Compilation failed
	fi
}

CFLAGS=${CFLAGS:="-O3 -pipe -Wall"}
CC=${CC:="cc"}
SRC=${SRC:=lines}
SRC=(${SRC[*]})

exech $CC $CFLAGS $SRC.c -o $SRC
