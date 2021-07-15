#!/bin/bash

# $1 Path to the new version.
# $2 Path to the old version.

cp -a -n $2/config.h $1/
cp -a -n $2/.gitignore $1/.gitignore
cp -a -n $2/include/libxml/xmlversion.h $1/include/libxml/
