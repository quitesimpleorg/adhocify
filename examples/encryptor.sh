#!/bin/sh
#example: encrypt files once they get written to a directory and remove them
#launch with: adhocify -w /path/encryptin /path/to/this/script.sh {}
#This is a simple example and has security flaws:
#-no secure delete (better to use e. g. ramfs or tmpfs...)
#-then still not necessarily secure against people who can dump the content of the memory
set -e
DESTINATION="/tmp/store"
if [ -z "$1" ] ; then
echo "Need path to encrypt" >&2
exit 1
fi
sleep 2 #some clients may want to set permissions and so on after writing
FILEPATH="$1"
gpg -e -r mail@example.com -o $DESTINATION/$(basename $FILEPATH) $FILEPATH 
rm $FILEPATH



