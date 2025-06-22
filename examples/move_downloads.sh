#!/bin/bash
#Hardlinks all incoming files (e. g. downloads) to another directory.
#There, they will be put into subdirectories which are named after the current date (YYYYMMDD) to get some minimal automatic "organization".  
#adhocify -d -m IN_CLOSE_WRITE -m IN_MOVED_TO -w /home/user/Downloads -w /home/user/other_dir /path/to/move_downloads.sh

INCOMING="$1"
stat -c"%s" "$INCOMING" | grep -q "^0$" && exit
#ignore partial downloads (.part in firefox, .crdownload in chrome)
echo "$INCOMING" | grep -q .part$ && exit 
echo "$INCOMING" | grep -q .crdownload$ && exit
today=$(date +%Y%m%d)
TARGET_DIR="/target/dir/path"
TODAY_DIR="$TARGET_DIR"/$today
if [ ! -d "$TODAY_DIR" ] ; then
	mkdir "$TODAY_DIR"
	rm -f "$TARGET_DIR"/today
	ln -s "$TODAY_DIR" "$TARGET_DIR"/today
fi
# Nowadays, some browsers don't like it when files are moved away immediately and might report download failure. Use hardlinks so they don't complain.
# Alternatively, a sufficiently long enough "sleep" followed by "mv" might work
ln "$INCOMING" "$TARGET_DIR"/$today/


