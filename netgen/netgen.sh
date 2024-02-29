#!/bin/bash
r=`realpath $0`
d=`dirname $r`
export CLASSPATH=$d/classes:$d/jacl141/lib/tcljava1.4.1/tcljava.jar:$d/jacl141/lib/tcljava1.4.1/jacl.jar
exec java -ea NetGen "$@"
