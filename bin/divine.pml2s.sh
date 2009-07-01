#!/bin/sh

FILE=$1

DIR="/usr/local/share/divine-cluster/jar"

# parse into AST
if ! echo $FILE | grep '\.ast\.xml$' >/dev/null
then
  java -cp $DIR"/pml2s.jar" xml.XmlSerializer $FILE || exit
  FILE=$FILE".ast.xml"
fi

# generate code
java -cp $DIR"/pml2s.jar:"$DIR"/jdom.jar" CodeGen.CodeGen $FILE

