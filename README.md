What is adhocify?
=================

adhocify uses inotify to watch for file system events. Once an event 
occurs it can execute a command. The path of the file and the event
will be passed to that command.
 
Requirements
============
adhocify only runs on Linux. You need a kernel and libc with inotify 
support - pretty much all satisfy this condition.

Options
=======
See adhocify --help.

Examples:
=========
```
adhocify -w /tmp/ /home/user/myscript.sh
```
Watches for IN\_CLOSE\_WRITE events in /tmp/ and  launches script /home/user/myscript.sh

```
adhocify -w /tmp/ -w /var/run /home/user/myscript.sh
```
Same as above, but also watches /var/run

```
adhocify /home/user/myscript.sh
```
Watches for IN\_CLOSE\_WRITE events in the current directory, launches script /home/user/myscript.

```
adhocify -m IN_OPEN -w /tmp /home/user/myscript.sh
```
Watches for IN\_OPEN events in /tmp/, launches script /home/user/myscript.sh

```
adhocify -w /tmp -i *.txt /home/user/myscript.sh
```
Watches for IN\_CLOSE\_WRITE events in /tmp/ but will not pass any events for *.txt files to the script

```
find . -type d | adhocify -s /home/user/myscript.sh
```
Starts monitoring every subdirectory of the current path for IN\_CLOSE\_WRITE events. However, inotify has  limits, thus it may not always work, e. g. when inotify descriptors are being used by other programs or the tree is too large. Therefore adhocify will exit
if it cannot setup a watch for all supplied directories. See inotify(7), /proc/sys/fs/inotify/.

### Child exit codes
```
adhocify -m IN_CREATE --exit-with-child=0 -- /usr/bin/test -f awaited_file
```
Keep running until the file named "awaited_file" is created in the current directory.

`--exit-with-child` also supports negation, so e. g. with `--exit-with-child='!0'` adhocify would keep running as long as the child commands exits with 0.

### Passing event data to the command
```
adhocify -w /tmp/ -w /var/run /home/user/myscript.sh {}
```
Passes the full path of the file an event occured on to the specified command. It can be retreived from argv[1] in the called command.

```
adhocify -w /tmp/ /bin/echo the file {} was written to
```
Running echo "Test" > /tmp/test will print in the shell adhocify was launched in: "the file /tmp/test was written to"

```
adhocify -m IN_CREATE -m IN_CLOSE_WRITE -w /path -- /bin/env
```

adhocify passes the inotify event to the command as an environment variable. The variable is called ```ADHOCIFYEVENT``` and contains the value of inotify_event->mask as set by inotify.

Other tools
===========
If adhocify does not suit your needs, take a look at:
  * inotify-tools: https://github.com/rvoicilas/inotify-tools/wiki
