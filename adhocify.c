//The author disclaims copyright to this source code. 
#define  _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <libgen.h>
#include <inttypes.h>
#include <time.h>
#include <stdbool.h>
#include <stdarg.h>
#include <errno.h>
#define BUF_SIZE (sizeof(struct inotify_event) * 1024) + 255


struct ifdLookup
{
	int ifd;
	char *path;
	bool isdir;
	struct ifdLookup *next;
};

struct ifdLookup *lkp_first = NULL;
struct ifdLookup **lookup = &lkp_first;


void *xmalloc(size_t size)
{
	void *m = malloc(size);
	if(m == NULL) 
	{
		perror("malloc"); 
		exit(EXIT_FAILURE);
	}
	return m;
}

char *xstrdup(const char *s)
{
	char *tmp = strdup(s);
	if(tmp == NULL)
	{
		perror("strdup");
		exit(EXIT_FAILURE);
	}
	return tmp;
}

char *xrealpath(const char *path, char *resolved_path)
{
	char *tmp = realpath(path, resolved_path);
	if(tmp == NULL)
	{
		perror("realpath");
		exit(EXIT_FAILURE);
	}
	return tmp;
}


char *ndirname(const char *path)
{
	if(path == NULL) return xstrdup("."); 
	char *c = strdupa(path);
	return xstrdup(dirname(c));
}

char *find_ifd_path(int ifd)
{
	for(struct ifdLookup *lkp = lkp_first; lkp != NULL; lkp = lkp->next)
		if(lkp->ifd == ifd) return lkp->path;
	return NULL;
}




void logwrite(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	fflush(stderr);
	va_end(args);
}

void logerror(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	char *prefix = "Error: "; 
	char *tmp = alloca(strlen(format) + strlen(prefix) + 1);
	strcpy(tmp, prefix);
	strcat(tmp, format);
	vfprintf(stderr, tmp, args);
	fflush(stderr);
	va_end(args);
	
}

void queue_watch(char *pathname)
{
	struct stat sb;
	int s = stat(pathname, &sb);
	if(s == -1)
	{
		perror("stat");
		exit(EXIT_FAILURE);
	}
	*lookup = xmalloc(sizeof(struct ifdLookup));
	struct ifdLookup *lkp = *lookup;
	lkp->ifd = 0;
	char *path = xrealpath(pathname, NULL);
	lkp->path = path;
	lkp->isdir = S_ISDIR(sb.st_mode);
	lkp->next = NULL;
	lookup = &lkp->next;
}

void start_watches(int fd, uint32_t mask)
{

	struct ifdLookup *lkp = lkp_first;
	for(; lkp != NULL; lkp = lkp->next)
	{
		int ret = inotify_add_watch(fd, lkp->path, mask);
		if(ret == -1) 
		{
			perror("inotify_add_watch"); 
			exit(EXIT_FAILURE); 
		}

		lkp->ifd = ret;
	}	
}

bool redirect_stdout(const char *outfile)
{
	int fd = open(outfile, O_CREAT | O_WRONLY | O_APPEND, S_IRUSR | S_IWUSR);
	if(fd == -1)
	{
		perror("open");
		return false;
	}

	if(dup2(fd, 1) == -1 || dup2(fd, 2) == -1)
	{
		perror("dup2");
		return false;
	}

	return true;
}

//path: file to execute
//eventfile: path to the file the event occured on 
//outfile: to log output of the program to be executed
//mask: occured adhocify event
//noappend: supply the adhocify event as an environment variable to the program to be executed
bool run( char *path, char *eventfile, const char *outfile, uint32_t mask, bool noappend)
{
	char *envvar = NULL;
	struct stat sb;
	if(stat(path, &sb) == -1) 
	{ 
		perror("stat"); 
		return false; 
	} 

	pid_t pid = fork();
	if(pid == 0)
	{
		if(outfile)
		{
			if(! redirect_stdout(outfile)) 
				return false;	
		}
		
		char *argv0 = memrchr(path, '/', strlen(path));
		if(argv0==NULL) 
			argv0=path; 
		else
			argv0+=1;

		envvar = xmalloc(19 * sizeof(char));
		sprintf(envvar, "adhocifyevent=%"PRIu32, mask); 
		putenv(envvar);
		
		execl(path, argv0, (! noappend) ? eventfile : NULL, NULL);
		perror("execlp");
		return false;
	}
	if(pid == -1)
		perror("fork");

	return true;

}

uint32_t nameToMask(char *name)
{ 
	if(!strcmp(name, "IN_CLOSE_WRITE"))
		return IN_CLOSE_WRITE;
	else if(!strcmp(name, "IN_OPEN"))
		return IN_OPEN;
	else if(!strcmp(name, "IN_MODIFY"))
		return IN_MODIFY;
	else if(!strcmp(name, "IN_DELETE"))
		return IN_DELETE;
	else if(!strcmp(name, "IN_ATTRIB"))
		return IN_ATTRIB;
	else if(!strcmp(name, "IN_CLOSE_NOWRITE"))
		return IN_CLOSE_NOWRITE;
	else if(!strcmp(name, "IN_MOVED_FROM"))
		return IN_MOVED_FROM;
	else if(!strcmp(name, "IN_MOVED_TO"))
		return IN_MOVED_TO;
	else if(!strcmp(name, "IN_CREATE"))
		return IN_CREATE;
	else if(!strcmp(name, "IN_DELETE_SELF"))
		return IN_DELETE_SELF;
	else if(!strcmp(name, "IN_MOVE_SELF"))
		return IN_DELETE_SELF;
	else if(!strcmp(name, "IN_ALL_EVENTS"))
		return IN_ALL_EVENTS;
	else if(!strcmp(name, "IN_CLOSE"))
		return IN_CLOSE;
	else if(!strcmp(name, "IN_MOVE"))
		return IN_MOVE;
	else
		return 0;

}

void check_forkbomb(char *dir_log, char *dir_prog)
{
	dir_log = ndirname(dir_log);
	dir_prog = ndirname(dir_prog);	

	struct ifdLookup *lkp = lkp_first;
	while(lkp)
	{
		if(lkp->isdir)
		{

			char *dir_lkpPath = lkp->path;
			if( !strcmp(dir_lkpPath, dir_log)
					|| !strcmp(dir_lkpPath, dir_prog))
			{
				logerror("Don't place your logfiles or prog in a directory you are watching for events\n");
				exit(EXIT_FAILURE);
			}
		}

		lkp = lkp->next;
	}

	free(dir_log);
	free(dir_prog);
}

void watches_from_stdin()
{
	char *line = NULL;
	size_t n = 0;
	ssize_t r;
	while((r = getline(&line, &n, stdin)) != -1)
	{
		if(line[r-1] == '\n') line[r-1] = 0;
		queue_watch(line);
	}
}

char *get_eventfile_abspath(struct inotify_event *event)
{
	char *wdpath = find_ifd_path(event->wd);
	if(wdpath == NULL) 
		return NULL;
		
	size_t nameLen = strlen(event->name);
	char *abspath = xmalloc((strlen(wdpath) + nameLen + 2) * sizeof(char));
	strcpy(abspath, wdpath);
	if(nameLen > 0) 
	{
		strcat(abspath, "/");
		strcat(abspath, event->name);
	}

	return abspath;
}

int main(int argc, char **argv)
{
	uint32_t mask = 0;
	uint32_t optmask = 0; 
	bool noappend=false;
	bool fromstdin=false;
	bool forkbombcheck=true;
	int option;
	int ifd;
	char *logfile = NULL;
	char *watchpath = NULL;
	
	if(argc < 2) 
	{ 
		logwrite("Insert usage text here\n"); 
		exit(EXIT_FAILURE);
	}

	signal(SIGCHLD, SIG_IGN);

	while((option = getopt(argc, argv, "absdo:w:m:l:")) != -1)
	{
		switch(option)
		{
			case 'd':
				if(daemon(0,0) == -1)
				{
					perror("daemon");
					exit(EXIT_FAILURE);
				}
				break;
			case 'o':
				logfile = optarg;
				break;
			case 'm':
				optmask = nameToMask(optarg);
				if(optmask == 0) { 
					logerror("Not supported inotify event: %s\n", optmask); 
					exit(EXIT_FAILURE); 
				}
				mask |= optmask;
				break;
			case 'w':
				watchpath = optarg;
				queue_watch(watchpath);
				break;
			case 'a':
				noappend=true;
				break;
			case 's':
				fromstdin=true;
				break;
			case 'b':
				forkbombcheck=false;
				break;
		}	
	}

	if(lkp_first == NULL)
	{ 
		watchpath = getcwd(NULL,0); 
		if(watchpath == NULL) 
		{ 
			perror("getcwd"); 
			exit(EXIT_FAILURE);
		} 
		queue_watch(watchpath);
	}
	
	if(fromstdin)
		watches_from_stdin();
	
	if(mask == 0) mask |= IN_CLOSE_WRITE;
	if(optind >= argc) 
	{ 
		logerror("missing prog/script path\n");
		exit(EXIT_FAILURE); 
	}

	char *prog = argv[optind];

	if(logfile)
		logfile = xrealpath(logfile, NULL);
	

	if(forkbombcheck)
	{
		char *path_prog = xrealpath(prog, NULL);
		check_forkbomb(logfile, path_prog);
	}
	
	ifd = inotify_init();
	start_watches(ifd, mask);
	
	while(1) 
	{
		int len;
		int i =0;
		char buf[BUF_SIZE];
		len = read(ifd, buf, BUF_SIZE);
		if(len == -1) 
		{
			if(errno == EINTR) continue;
			perror("read");
			exit(EXIT_FAILURE);
		}
		
		while(i < len)
		{

			struct inotify_event *event = (struct inotify_event *)&buf[i];
			if(event->mask & mask) 
			{	
				char *eventfile_abspath = get_eventfile_abspath(event);
				if(eventfile_abspath == NULL)
				{
					logerror("Could not get absoulte path for event. Watch descriptor %i\n", event->wd);
					break;
				}
				
				logwrite("Starting execution of child %s\n", prog);
				int r = run(prog, eventfile_abspath, logfile, event->mask, noappend);
				if(!r) 
					logerror("Execution of child %s failed\n", prog);
				fflush(stdout);
				fflush(stderr);
				free (eventfile_abspath);
			}



			i+=sizeof(struct inotify_event) + event->len;
		}
	}              
}
