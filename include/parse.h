#ifndef PARSE_H 
#define PARSE_H
#define MAX_BUFFER_SIZE 81
#include <sys/types.h>
#include <unistd.h>

typedef struct builtin_functions{
  const char *builtin_name;
  int (*func)(char **av);

}builtin_functions;

typedef enum{
  RUNNING,
  STOPPED,
  DONE
} job_state_t;

typedef struct{
  int job_id;
  pid_t pid;
  char command[MAX_BUFFER_SIZE];
  job_state_t state;
  int in_use;
} job_t;

int shell_cd(char** args);
int shell_help(char** args);
int shell_exit(char** args);
int shell_jobs(char** args);
int shell_bg(char** args);
int shell_fg(char** args);
int shell_history(char** args);
void shell_launch(char** args, int background);

void initialize_jobs();

//Signal stuff
void sigchild_handler(int sig);
// No longer used - commented out in shell.c
// void sigint_handler(int sig);
// void sigtstp_handler(int sig);

int is_background(char **args);


//job related
job_t* find_job_id(int job_id);
int add_job(pid_t pid, char *arg);
job_t* find_job_pid(pid_t pid);
void cleanup_jobs();


/*parese the input and tokenizes it as desired*/
char** parse(const char* input);

/*Frees the tokens*/
void freeTokens(char** tokens);

//composition functions
int has_operator(char** args, char* op);
int count_commands(char** args, char* op);
char*** split_operator(char** args,char* op, int numCommands);
void execute_sequential(char** args);
void execute_and(char** args);
void execute_or(char** args);
void execute_pipeline(char** args);

#endif