/* CMPSC 311 Project 7 starter kit, version 2
 *
 * 	Jonathan Swarts
 *	Nick Dyszel
 *
 * Usage:
 *   c99 -v -o pr7 pr7.2.c                        [Sun compiler]
 *   gcc -std=c99 -Wall -Wextra -o pr7 pr7.2.c    [GNU compiler]
 *
 *   pr7
 *   pr7%      [type a command and then return]
 *   pr7% exit
 *
 * This version is derived from the shellex.c program in CS:APP Sec. 8.4.
 */

/*----------------------------------------------------------------------------*/

#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "pr7_list.h"
#include "pr7_stack.h"
#define MAXLINE 128
#define MAXARGS 128
/* for use with getopt(3) */
extern char *optarg;
extern int optind;
extern int optopt;
extern int opterr;

int verbose = 0;
int interactive = 0;
int kill(pid_t pid, int sig);
int stat(const char *path, struct stat *buf);
int execve(const char *filename, char *const argv [], char *const envp[]);
int getopt(int argc, char * const argv[], const char *optstring);
int setenv(const char *envname, const char *envval, int overwrite);
int unsetenv(const char *name);
int open_shell_script(char const *filename);
int getarrow(char *com, struct pr7_stack *com_hist, FILE *fp);
  /* reads a command and brings up command history when arrow key is pressed */
int eval_line(char *cmdline);                 /* evaluate a command line */
int parse(char *buf, char *argv[]);           /* build the argv array */
int builtin(char *argv[]);                    /* if builtin command, run it */
int cleanup_terminated_children(void);        /* reap background processes */
char start_up_file[MAXLINE];
extern char **environ;
static pid_t foreground_pid = 0;
static struct pr7_list background_pid_table; // backgroud process ID table
/*----------------------------------------------------------------------------*/
void SIGINT_handler(int sig)
{
  if (sig == SIGINT)
  {
    if (foreground_pid == 0)
    {
      //Ignores the Call, we're in the shell.
    }
    else
    {
      kill(foreground_pid, SIGINT);
      foreground_pid = 0;
    }
    signal(SIGINT, SIGINT_handler);
  }
  else
  { fprintf(stderr, "signal %d received by SIGINT handler\n", sig); }
}
/*----------------------------------------------------------------------------*/
/* Displays information for program options */
static void usage(char *prog, int status)
{
  if (status == EXIT_SUCCESS)
  {
    printf("Usage: %s [-h] [-v] [-i] [-e] [-s f] [file]\n",
           prog);
    printf("    -h      help\n");
    printf("    -v      verbose mode\n");
	  printf("    -i      interactive mode\n");
	  printf("    -e      echo commands before execution\n");
	  printf("    -s f      use startup file f, default pr7.init\n");
	  printf("Shell commands:\n");
	  printf("    help\n");
  }
  else
  {
    fprintf(stderr, "%s: Try '%s -h' for usage information.\n", prog, prog);
  }
  
  exit(status);
}
/*----------------------------------------------------------------------------*/
/* Opens file, checks for NULL name, but don't send a NULL name to it.*/
int open_shell_script(char const *filename){
  int ret;
	FILE *oFile;
	if(filename != NULL){
		if((oFile = fopen(filename, "r")) != NULL){
		if(verbose > 0){
				printf("pr7: reading %s\n", filename);
			}
		char strBuf[MAXLINE];
		while(1){
			fgets(strBuf, MAXLINE, oFile);
			if(feof(oFile))
			break;
			ret = eval_line(strBuf);
		}
		return ret;
		}
	}
	return EXIT_FAILURE;
}
/*----------------------------------------------------------------------------*/
/* Compare to main() in CS:APP Fig. 8.22 */
int main(int argc, char *argv[])
{
  int ret = EXIT_SUCCESS;
  int ch, count = 1, temp = 1;
  char temp_start[MAXLINE];
  struct stat sb;
  char cmdline[MAXLINE];                /* command line */
  
  int flag = 0; //don't go to shell if 1
  list_init(&background_pid_table);
  background_pid_table.name = "Background Processes";
  signal(SIGINT, SIGINT_handler);	//install signal handler to start.
  while ((ch = getopt(argc, argv, ":hvies:")) != -1){
    switch (ch) {
      case 'h':
		count++; //to move up arguments
        usage(argv[0], EXIT_SUCCESS);
        flag = 1;
        break;
      case 'v':
		count++; //to move up arguments
        verbose++;
        break;
      case 'i':
		count++; //to move up arguments
        interactive++;
		flag = 0;
        break;
      case 'e':
		count++; //to move up arguments
			while(argv[temp] != NULL) {
				printf("%s ", argv[temp++]);
			}
			printf("\n");
        flag = 1;
        break;
      case 's':
		count++; //to move up arguments
		strcpy(temp_start,optarg);
		if(!stat(temp_start, &sb)){
			if(S_ISREG(sb.st_mode) && sb.st_mode & 0111){
				strcpy(start_up_file, temp_start);
			}
			else{
				strcpy(start_up_file, "pr7.init");
			}
		}
        flag = 1;
        break;
      case '?':
        flag = 1;
        printf("%s: invalid option '%c'\n", argv[0], optopt);
        usage(argv[0], EXIT_FAILURE);
        break;
      case ':':
        flag = 1;
        printf("%s: invalid option '%c' (missing argument)\n", argv[0], optopt);
        usage(argv[0], EXIT_FAILURE);
	  default:
	    usage(argv[0], EXIT_FAILURE);
        break;
    }
  }
  //check if it's a file, if not go to shell_script:
  if(!stat(argv[count], &sb)){
	if(S_ISREG(sb.st_mode) && sb.st_mode & 0111){
		if(argv[count] != NULL){
			if(execvp(start_up_file, argv) == -1){ //try to execute start_up_file.
			}
			else{
				if(execvp("pr7.init", argv) == -1){ //try default
				}
				else{
					printf("%s: start_up_file and pr7.init failed: %s\n", argv[0], strerror(errno));
					exit(EXIT_FAILURE); //Error on read
				}
			}
			if(execvp(argv[count], argv) == -1){ //try to execute file.
			}
			else{
				flag = 1;
			}
		}
	}
  }
  //read from shell script
  if(argv[count] != NULL && flag != 1){
    if((ch = open_shell_script(argv[count])) == EXIT_FAILURE){
      printf("%s: failed: %s\n", argv[0], strerror(errno));
      exit(EXIT_FAILURE); //Error on read
    }
    else{
		flag = 1;
	}
  }
  //You got the red shell on your tail!
  struct pr7_stack command_stack;
  stack_init(&command_stack);
  
  while (1 && flag != 1)
  {
    /* issue prompt and read command line */
    printf("%s%% ", argv[0]);
    if (getarrow(cmdline, &command_stack, stdin) == 0)
    {
      fgets(cmdline, MAXLINE, stdin);   /* cmdline includes trailing newline */
    }
    if (feof(stdin))                  /* end of file */
    { break; }
    stack_push(&command_stack, cmdline);
	  ret = eval_line(cmdline);
    cleanup_terminated_children();
  }
	
	return ret;
}

/*----------------------------------------------------------------------------*/

/* reads a command from the input and references command history if up/down 
 * arrow key is pressed */
int getarrow(char *com, struct pr7_stack *com_hist, FILE *fp)
{
  struct pr7_command *cur = NULL;
  
  int c0 = -1;
  int c1 = -1;
  int c2 = -1;
  
  while (((c0 = fgetc(fp)) == 27) && ((c1 = fgetc(fp)) == 91) && \
         (((c2 = fgetc(fp)) == 65) || (c2 == 66)))
  {
    if (c2 == 65)
    {
      if (cur == NULL) cur = com_hist->top;
      else if (cur->next != NULL) cur = cur->next;
    }
    if ((c2 == 66) && (cur != NULL))
    {
      cur = cur->prev;
    }
    c0 = c1 = c2 = -1;
  }
  
  if (c2 != -1) ungetc(c2, fp);
  if (c1 != -1) ungetc(c1, fp);
  if (c0 != -1) ungetc(c0, fp);
  
  if (cur == NULL) return 0;
  
  com = cur->command;
  return 1;
}

/*----------------------------------------------------------------------------*/
/* evaluate a command line
 * Compare to eval() in CS:APP Fig. 8.23.
 */

int eval_line(char *cmdline)
{
  char *argv[MAXARGS];  /* argv for execve() */
  char buf[MAXLINE];    /* holds modified command line */
  int background;       /* should the job run in background or foreground? */
  pid_t pid;            /* process id */
  int ret = EXIT_SUCCESS;
  int flag = 0;
  
  strcpy(buf, cmdline);                 /* buf[] will be modified by parse() */
  background = parse(buf, argv);        /* build the argv array */
  
  if (argv[0] == NULL)          /* ignore empty lines */
  { return ret; }
	
  if (strcmp(argv[0], "#") == 0){		/* ignores comment lines **EXTRA CREDIT** */
    strcpy(argv[0], "\0");
    return ret;
  }
	
  if (builtin(argv) == 1)       /* the work is done */
  { return ret; }
  
  //check if it's a file, skip the shell_script if so
  struct stat sb;
	if(!stat(argv[0], &sb)){
		if(S_ISREG(sb.st_mode) && sb.st_mode & 0111){
			flag = 1;
		}
	}

  //read from shell script
  if(flag == 0){
	if((open_shell_script(argv[0]) == EXIT_FAILURE)){ /*if it's a shell_script, get out of here and do that!*/
	}
	else{return ret;}
  }
  if ((pid = fork()) == 0)      /* child runs user job */
  {
	  foreground_pid = getpid(); //it's running in the foreground
	  
	if (execvp(argv[0], argv) == -1){
      fprintf(stderr, "%s: failed: %s\n", argv[0], strerror(errno));
      _exit(EXIT_FAILURE);
    }
	
  }
  if (background)            /* parent waits for foreground job to terminate */
  {
    if (list_add_once(&background_pid_table, pid, 1) == NULL)
    { fprintf(stderr, "%s: could not add process to list: %d\n", argv[0], \
              pid); }
    if (verbose) list_print(&background_pid_table);
    foreground_pid = 0; //It's running in the background
    printf("background process %d: %s\n", (int) pid, cmdline);
  }
  else
  {
    while (waitpid(pid, &ret, 0) == (pid_t) -1)
    {
      if (errno == ECHILD)
      {
        fprintf(stderr, "%s: failed: %s\n", argv[0], strerror(errno));
        exit(EXIT_FAILURE);
      }
    }
  }
  
  return ret;
}

/*----------------------------------------------------------------------------*/
/* parse the command line and build the argv array
 *
 * Compare to parseline() in CS:APP Fig. 8.24.
 */

int parse(char *buf, char *argv[])
{
  char *delim;          /* points to first whitespace delimiter */
  int argc = 0;         /* number of args */
  int bg;               /* background job? */
  
  char whsp[] = " \t\n\v\f\r";          /* whitespace characters */
  
  /* Note - the trailing '\n' in buf is whitespace, and we need it as a delimiter. */
  
  while (1)                             /* build the argv list */
  {
    buf += strspn(buf, whsp);         /* skip leading whitespace */
    delim = strpbrk(buf, whsp);       /* next whitespace char or NULL */
    if (delim == NULL)                /* end of line */
    { break; }
    argv[argc++] = buf;               /* start argv[i] */
    *delim = '\0';                    /* terminate argv[i] */
    buf = delim + 1;                  /* start argv[i+1]? */
  }
  argv[argc] = NULL;
  
  if (argc == 0)                        /* blank line */
  { return 0; }
  
  /* should the job run in the background? */
  if ((bg = (strcmp(argv[argc-1], "&") == 0)))
  { argv[--argc] = NULL; }
  
  return bg;
}

/*----------------------------------------------------------------------------*/
/* if first arg is a builtin command, run it and return true
 *
 * Compare to builtin_command() in CS:APP Fig. 8.23.
 */

int builtin(char *argv[]){
  if (strcmp(argv[0], "exit") == 0) {     /* exit command */
		if(background_pid_table.length > 0){
			printf("There are %d background jobs running. \n",background_pid_table.length);
			return 1;
		}
		else{
			exit(0);
		}
  }
	
  if (strcmp(argv[0], "echo") == 0) {	/* echo command */
    int c = 1;
    while(argv[c] != NULL) {
      printf("%s ", argv[c++]);
    }
    printf("\n");
    return 1;
  }
  if (strcmp(argv[0], "dir") == 0){	/* dir command */
    //char *getcwd(char *buf, size_t size);
    char *buf;
    char *ptr;
    size_t size;
    
    size = pathconf(".", _PC_PATH_MAX);
    if ((buf = (char *)malloc((size_t)size)) != NULL)
      ptr = getcwd(buf, (size_t)size);
    
    printf("%s\n", ptr);
    free(ptr);
    return 1;
  }
  
  if (strcmp(argv[0], "cdir") == 0){	/* cdir command */
    int check;
    if(argv[1] == NULL){ //no directory to change to, so default HOME.
      check = chdir(getenv("HOME"));
      setenv("PWD", getenv("HOME"), 1);	//Change PWD environment variable.
                                        //printf("%s \n", getenv("PWD"));  //debugging purposes
      return 1;
    }
    //Get current directory
    char *buf;
    char *ptr;
    size_t size;
    
    size = pathconf(".", _PC_PATH_MAX);
    if ((buf = (char *)malloc((size_t)size)) != NULL)
      ptr = getcwd(buf, (size_t)size);
    
    if(chdir(argv[1]) == -1){	//Change directory.
      printf("%s: failed: %s\n", argv[0], strerror(errno));
      chdir(ptr); // Stay where you were.
    }
    size = pathconf(".", _PC_PATH_MAX);
    if ((buf = (char *)malloc((size_t)size)) != NULL)
      ptr = getcwd(buf, (size_t)size);
	  
    if(setenv("PWD", ptr, 1) == -1){	//Change PWD environment variable.
    }
    
    //printf("%s \n", getenv("PWD"));  //debugging purposes
    free(ptr);
    return 1;
  }
  
  if (strcmp(argv[0], "penv") == 0){	/* penv command */
    char *str;
    char **environmentHolder = environ;
    if(argv[1] == NULL){	//Print entire list of env variables.
      while(*environmentHolder != NULL){
        printf("%s \n", *environmentHolder);
        environmentHolder++;
      }
      return 1;
    }
    if((str = getenv(argv[1])) == NULL){
      //Do not print anything.
      return 1;
    }
    printf("%s \n", str);
    return 1;
  }
  
  if (strcmp(argv[0], "senv") == 0){	/* senv command */
    if(argv[1] == NULL){ //Nothing to set
      printf("%s: failed: No environment variable chosen \n", argv[0]);
      return 1;
    }
    
    if(argv[2] == NULL){ // Environment value not chosen
      if((setenv(argv[1], "", 1)) == -1) //set it to empty
        printf("%s: failed: %s\n", argv[0], strerror(errno));
      return 1;
    }
    
    if((setenv(argv[1], argv[2], 1)) == -1) //set it to argument 2
			printf("%s: failed: %s\n", argv[0], strerror(errno));
    return 1;
  }
  
  if (strcmp(argv[0], "unsenv") == 0){	/* unsenv command */
    if(argv[1] == NULL){ //Nothing to unset
      printf("%s: failed: No environment variable chosen \n", argv[0]);
      return 1;
    }
    
    if(unsetenv(argv[1]) == -1)
			printf("%s: failed: %s\n", argv[0], strerror(errno));
    
    return 1;
  }
  
  if (strcmp(argv[0], "help") == 0){	/* help command */
    printf("Commands: \n  help\n  exit [n]\n  echo\n  dir\n  cdir [directory]\n  penv\n  penv variable_name\n  senv variable_name value\n");
    printf("unsetenv variable_name\n  limits\n  pjobs\n");
    return 1;
  }
  
  if (strcmp(argv[0], "&") == 0)        /* ignore singleton & */
  { return 1; }
  
  if (strcmp(argv[0], "limits") == 0){	/* print all limits that affect use of shell **EXTRA CREDIT** */
    printf("MAXARGS: %d\nMAXLINE: %d\n", MAXARGS, MAXLINE);
    return 1;
	}
	
  if (strcmp(argv[0], "pjobs") == 0){	/* print all jobs */
    list_print(&background_pid_table);
    return 1;
	}
	
  return 0;                             /* not a builtin command */
}

/*----------------------------------------------------------------------------*/

/* Find all the child processes that have terminated, without waiting.
 *
 * This code is adapted from the GNU info page on waitpid() and the Solaris
 * man page for waitpid(2).
 */

int cleanup_terminated_children(void)
{
  pid_t pid;
  int status;
  int count = 0;
  struct pr7_process *entry;
  
  while ((pid = waitpid(-1, &status, WNOHANG)) != 0)
  {
    if (pid == -1)            /* returns -1 if there was an error */
    {
      /* errno will have been set by waitpid() */
      if (errno == ECHILD)  /* no children */
      { break; }
      if (errno == EINTR)   /* waitpid() was interrupted by a signal */
      { continue; }       /* try again */
      else
      {
        printf("unexpected error in cleanup_terminated_children(): %s\n", \
               strerror(errno));
        break;
      }
    }
    if(interactive > 0)
		{ printf("process %d terminated with status %d\n", pid, status); }
    entry = list_update_entry(&background_pid_table, pid, status);
    if (interactive) list_print(&background_pid_table);
    if (entry != NULL) list_remove(&background_pid_table, entry);
    if (interactive) list_print(&background_pid_table);
    count++;
  }
  
  return count;
}
