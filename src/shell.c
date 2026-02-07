// Implement your shell in this source file.
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include "dynamicstring.h"
#include "parse.h"
#include <signal.h>


#define MAX_BUFFER_SIZE 81
#define NUM_BUILTIN_FUNC 7
#define MAX_JOBS 5
#define MAX_HISTORY 100

int status = 0;
job_t job_table[MAX_JOBS];
int next_job_id = 1;
pid_t foreground_pid = 0;
pid_t foreground_pgid = 0;
char* command_history[MAX_HISTORY];
int history_count = 0;

/*
Required Built In Functions
cd, help, exit, jobs, bg, ONE COMMAND OF MY CHOICE
*/


struct builtin_functions array_builtin[] = 
{
  {.builtin_name = "exit",. func = shell_exit},
  {.builtin_name = "cd", .func = shell_cd},
  {.builtin_name = "help",. func = shell_help},
  {.builtin_name = "jobs",. func = shell_jobs},
  {.builtin_name = "bg",. func = shell_bg},
  {.builtin_name = "fg",. func = shell_fg},
  {.builtin_name = "history",. func = shell_history},
  {.builtin_name = NULL,. func = NULL}

};

int Getcwd(char* buf, size_t size);

void shell_exec(char** args);

void shell_execvp(const char * file, char* const argv[]);

void addHistory(const char* command);


void sigtstp_handler(int sig) {
  (void)sig;
}

void sigint_handler(int sig);

int main(int argc, char** argv){
 
  alarm(120); //useful in the case that you accidently create a 'fork bomb'
  signal(SIGCHLD, sigchild_handler);
  signal(SIGTSTP, sigtstp_handler);  
  signal(SIGINT, sigint_handler);     
  signal(SIGTTOU, SIG_IGN);
  signal(SIGTTIN, SIG_IGN);


  setpgid(0, 0);

  char line[MAX_BUFFER_SIZE];
  initialize_jobs();


    while(1){
      printf("mini-shell>");
        // Read in 1 line of text
        // The line is coming from 'stdin' standard input
        if (fgets(line,MAX_BUFFER_SIZE,stdin) == NULL){
          continue;
        }

        size_t len = strlen(line);
        if (len == MAX_BUFFER_SIZE - 1 && line[len - 1] != '\n'){
          printf("mini-shell> Error: Input exceeds %d char limit\n", MAX_BUFFER_SIZE - 1);
          int c;
          while ((c = getchar()) != '\n' && c != EOF);
          continue;
        }

        addHistory(line);
        char** tokens = parse(line);

        if (tokens == NULL){
          printf("something bad happened");
        }
        
        /*for (int i = 0; tokens[i] != NULL; i++){
          printf("%s\n", tokens[i]);
        }*/

        shell_exec(tokens);


        /*if(strcmp(line,"help")==0){
            printf("You typed in help\n");
        }
        if(strcmp(line,"help\n")==0){
            printf("You typed in help with an endline\n");
        }*/
         freeTokens(tokens);
         
    }

  return 0;

}

int Getcwd(char* buf, size_t size){
  if (NULL == getcwd(buf, size)){
    perror("getcwd error");
    return 1;
  }
  return 0;
}

/*int shell_pwd(char** args){
  (void) args;
  char pwd[MAX_BUFFER_SIZE];
  int temp  = Getcwd(pwd, sizeof(pwd));
  if (temp == 1){
    perror("getcwd went wrong");
    return 1;
  }
  printf("%s\n", pwd);
  return 0;
}*/

void shell_exec(char** args){
  int index = 0;
  const char *temp;
  int background = 0;

  if (args == NULL || args[0] == NULL){
    return;
  }
  background = is_background(args);

  if (background != 0 && (has_operator(args, ";") || has_operator(args, "||") || has_operator(args, "&&") || has_operator(args, "|"))){
    pid_t pid = fork();
    if (pid == 0){
      setpgid(0,0);
      signal(SIGINT, SIG_DFL);
      signal(SIGTSTP, SIG_DFL);
      signal(SIGCHLD, SIG_DFL);
      signal(SIGTTOU, SIG_DFL);
      signal(SIGTTIN, SIG_DFL);

      if (has_operator(args, ";") != 0){
        execute_sequential(args);
      }
      else if (has_operator(args, "||")){
        execute_or(args);
      }
      else if (has_operator(args, "&&")){
        execute_and(args);
      }
      else if (has_operator(args, "|")){
        execute_pipeline(args);
      }
      exit(0);
    }
    else if (pid>0){
      setpgid(pid, pid);
      char commandStr[MAX_BUFFER_SIZE];
      commandStr[0] = '\0';
      for (int i = 0; args[i] != NULL; i++){
        strcat(commandStr, args[i]);
        if (args[i+1] != NULL){
          strcat(commandStr, " ");
        }
      }
      int job_id = add_job(pid, commandStr);
      printf("[%d] %d\n", job_id, pid);
    }
    else{
      perror("fork");
    }
    return;
  }

  if (has_operator(args, ";") != 0){
    execute_sequential(args);
    return;
  }
  if (has_operator(args, "||") != 0){
    execute_or(args);
    return;
  }
  if (has_operator(args, "&&") != 0){
    execute_and(args);
    return;
  }
  if (has_operator(args, "|") != 0){
    execute_pipeline(args);
    return;
  }

  while (index < NUM_BUILTIN_FUNC){
    temp = array_builtin[index].builtin_name;
    if (strcmp(args[0], temp) == 0){
      status = (array_builtin[index].func(args));
      if (status != 0){
        printf("Error, failed");
      }
      return;
    }
    index++;
  }
  //first check if its built into execve
  // if its not, we  we will find another way to exec it
  //check for builtins here, then we run execve if not

  shell_launch(args, background);
}

void shell_launch(char** args, int background){

  pid_t pid;
  int status;
  int job_id;
  char commandStr[MAX_BUFFER_SIZE];
  //here is where we handle the forking and stuff
  commandStr[0] = '\0';
  for (int i = 0; args[i] != NULL; i++){
    strcat(commandStr, args[i]);
    if (args[i+1] != NULL){
      strcat(commandStr, " ");
    }
  }

  pid = fork();

  if (pid == 0){
    setpgid(0, 0);

    signal(SIGINT, SIG_DFL);
    signal(SIGTSTP, SIG_DFL);
    signal(SIGCHLD, SIG_DFL);
    signal(SIGTTOU, SIG_DFL);
    signal(SIGTTIN, SIG_DFL);

    shell_execvp(args[0], args); //we want to edit this to tell us if the thing sucesfully updates
    printf("Child: should never get here");
    exit(1);
  }
  else if (pid > 0){
    setpgid(pid, pid);

    if (background != 0){
      job_id = add_job(pid, commandStr);
      printf("[%d] %d\n", job_id, pid);
    }

    else{
      foreground_pid = pid;
      if (tcsetpgrp(STDIN_FILENO, pid) < 0) {
        perror("tcsetpgrp");
      }

      sigset_t mask, oldmask;
      sigemptyset(&mask);
      sigaddset(&mask, SIGCHLD);
      sigprocmask(SIG_BLOCK, &mask, &oldmask);

      waitpid(pid, &status, WUNTRACED);

      sigprocmask(SIG_SETMASK, &oldmask, NULL);

      if (tcsetpgrp(STDIN_FILENO, getpgrp()) < 0) {
        perror("tcsetpgrp");
      }

      if (WIFSTOPPED(status)){
        job_id = add_job(pid, commandStr);
        job_t *job = find_job_pid(pid);
        if (job != NULL){
          job->state = STOPPED;
          printf("\n[%d] + %-12s%s\n", job->job_id, "stopped", job->command);
        }
      }
      foreground_pid = 0;
    }
  }
  else{
    perror("fork");
  }
}

void shell_execvp(const char * file, char* const argv[]){
  if (file == NULL || argv == NULL){
    printf("Invalid Arguments");
  }
  if (execvp(file, argv) == -1){
    printf("mini-shell>Command not found--Did you mean something else?\n");
    exit(EXIT_FAILURE);
  }
  return;
}

int shell_exit(char** args){
  cleanup_jobs();
  for (int i = 0; i < history_count; i++){
    free(command_history[i]);
  }
  exit(0);
}

int shell_history(char** args){
  for (int i = 0; i < history_count; i ++){
    printf("%4d  %s\n",i+1, command_history[i]);
  }
  return 0;
}

void addHistory(const char* command){
  if (command == NULL || command[0] == '\n' || command[0] == '\0'){
    return;
  }

  if (strncmp(command, "history", 7) == 0){
    return;
  }

  if (history_count >= MAX_HISTORY){
    free(command_history[0]);
    for (int i = 0; i < MAX_HISTORY - 1; i++){
      command_history[i] = command_history[i+1];
    }
    history_count = MAX_HISTORY - 1;
  }

  char* commandCopy = strdup(command);
  size_t len = strlen(commandCopy);
  if (len > 0 && commandCopy[len - 1] == '\n'){
    commandCopy[len - 1] = '\0';
  }

  command_history[history_count] = commandCopy;
  history_count++;
}

int shell_cd(char** args){
  char cwd[MAX_BUFFER_SIZE];
  char oldcwd[MAX_BUFFER_SIZE];
  char* path = NULL;
  char * expanded = NULL;

  int temp = Getcwd(oldcwd, sizeof(cwd));
  if (temp == 1){
    perror("getcwd");
    return 1;
  }

  
  if (args[1] == NULL || strcmp(args[1],"~") == 0){
    path = getenv("HOME");
  }
  else if (strcmp(args[1], "-") == 0){
    path = getenv("OLDPWD");
    if (path == NULL || strcmp(path,getenv("HOME")) == 0){
      printf("~\n");
      path = getenv("HOME");
    }
    else{
      printf("%s\n",path);
    }
    
  }
  else if (args[2] != NULL){
    printf("Too many arguments");
    return 1;
  }
  else if (args[1][0] == '~'){
    const char* home = getenv("HOME");
    if (home == NULL){
      printf("mini-shell> HOME not set");
      return 1;
    }
    size_t len = strlen(home + strlen(args[1]));
    expanded = malloc(len+1);
    if (expanded == NULL){
      perror("malloc failed");
      return 1;
    }
    strcpy(expanded,home);
    strcat(expanded, args[1] + 1);
    path = expanded;
  }
  else{
    path = args[1];
  }


  if (chdir(path) != 0){
    perror("mini-shell> cd");
    if (expanded != NULL){
      free(expanded);
    }
    return 1;
  }

  setenv("OLDPWD", oldcwd, 1);
  if (getcwd(cwd, sizeof(cwd)) == NULL){
    perror("getcwd");
    if (expanded != NULL){
      free(expanded);
    }
    return 1;
  }
  setenv("PWD", cwd, 1);

  if (expanded != NULL){
    free(expanded);
  }

  return 0;

}

void initialize_jobs(){
  for (int i = 0; i < MAX_JOBS; i++){
    job_table[i].in_use = 0;
    job_table[i].job_id = 0;
    job_table[i].pid = 0;
  }
}

int add_job(pid_t pid, char *arg){
  for (int i = 0; i < MAX_JOBS; i++){
    if (job_table[i].in_use == 0){
      job_table[i].in_use = 1;
      job_table[i].job_id = next_job_id;
      next_job_id++;
      job_table[i].pid = pid;
      strncpy(job_table[i].command, arg, 80);
      job_table[i].command[79] = '\0';
      job_table[i].state = RUNNING;
      return job_table[i].job_id;
    }
  }
  perror("Jobs Full\n");
  return -1;
}

void remove_job(int job_id){
  for (int i = 0; i < MAX_JOBS;i++){
    if (job_table[i].in_use != 0 && job_table[i].job_id == job_id){
      job_table[i].in_use = 0;
      return;
    }
  }
}

job_t* find_job_id(int job_id){
  for (int i = 0; i < MAX_JOBS; i++){
    if (job_table[i].in_use != 0 && job_table[i].job_id == job_id){
      return &job_table[i];
    }
  }
  return NULL;
}

job_t* find_job_pid(pid_t pid){
  for (int i = 0; i < MAX_JOBS; i++){
    if (job_table[i].in_use != 0 && job_table[i].pid == pid){
      return &job_table[i];
    }
  }
  return NULL;
}

int get_curr_job(){
  int maxID = -1;
  for (int i = 0; i < MAX_JOBS; i++){
    if (job_table[i].in_use != 0 && job_table[i].job_id > maxID){
      maxID = job_table[i].job_id;
    }
  }
  return maxID;
}

void update_job_states(){
  int status;
  pid_t res;

  for (int i = 0; i < MAX_JOBS; i++){
    if (job_table[i].in_use){
      res = waitpid(job_table[i].pid, &status, WNOHANG | WUNTRACED);
      if (res == job_table[i].pid){
        if (WIFEXITED(status) || WIFSIGNALED(status)){
          job_table[i].state = DONE;
        }
        else if (WIFSTOPPED(status)){
          job_table[i].state = STOPPED;
        }
      }
    }
  }
}

int shell_jobs(char** args){
  update_job_states();
  int curr_job = get_curr_job();

  for (int i = 0; i < MAX_JOBS; i++){
    if (job_table[i].in_use){
      char *CurrState;
      char *indicator;

      if (job_table[i].job_id == curr_job){
        indicator = " + ";
      }
      else{
        indicator = "   ";
      }

      switch(job_table[i].state){
        case RUNNING:
          CurrState = "running";
          break;
        case STOPPED:
          CurrState = "stopped";
          break;
        case DONE:
          CurrState = "done";
          break;
        default:
          CurrState = "unknown";
      }

      printf("[%d] %s%-12s%s\n", job_table[i].job_id, indicator, CurrState, job_table[i].command);

      if (job_table[i].state == DONE){
        remove_job(job_table[i].job_id);
      }
    }
  }
  return 0;
}

int shell_fg(char** args){
  int job_id = 0;
  job_t *job = NULL;
  int status = 0;

  if (args[1] == NULL){
    job_id = get_curr_job();
    if (job_id == -1){
      printf("fg: no current job\n");
      return 1;
    }
    job = find_job_id(job_id);
  }
  else{
    if (args[1][0] == '%'){
      job_id = atoi(args[1] + 1);
    }
    else{
      job_id = atoi(args[1]);
    }
    job = find_job_id(job_id);
  }

  if(job == NULL){
    printf("fg: job not found\n");
    return 1;
  }

  if (job->state == STOPPED){
    if (kill(job->pid, SIGCONT) < 0){
      perror("killed");
      return 1;
    }
  }

  job->state = RUNNING;
  foreground_pid = job->pid;
  printf("%s\n", job->command);

  if (tcsetpgrp(STDIN_FILENO, job->pid) < 0) {
    perror("tcsetpgrp");
  }

  sigset_t mask, oldmask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGCHLD);
  sigprocmask(SIG_BLOCK, &mask, &oldmask);

  waitpid(job->pid, &status, WUNTRACED);

  sigprocmask(SIG_SETMASK, &oldmask, NULL);

  if (tcsetpgrp(STDIN_FILENO, getpgrp()) < 0) {
    perror("tcsetpgrp");
  }

  if (WIFEXITED(status) || WIFSIGNALED(status)){
    remove_job(job_id);
  }

  else if (WIFSTOPPED(status)){
    job->state = STOPPED;
    printf("\n[%d] + %-12s%s\n", job->job_id, "stopped", job->command);
  }
  foreground_pid = 0;
  return 0;
}

int shell_bg(char** args){
  int job_id = 0;
  job_t *job = NULL;

  if (args[1] == NULL){
    job_id = -1;
    int index = 0;
    int max_id = -1;

    for (index = 0; index < MAX_JOBS; index++){
      if (job_table[index].in_use != 0 && job_table[index].state == STOPPED && job_table[index].job_id > max_id){
        max_id = job_table[index].job_id;
        job_id = job_table[index].job_id;
      }
    }

    if (job_id == -1){
      printf("bg: no current job\n");
      return 1;
    }
  }
  else{
    if (args[1][0] == '%'){
      job_id = atoi(args[1] + 1);
    }
    else{
      job_id = atoi(args[1]);
    }
  }
  job = find_job_id(job_id);
  if(job == NULL){
    printf("bg: job %d not found\n", job_id);
    return 1;
  }

  if (job->state != STOPPED){
    printf("bg: job %d already running\n", job_id);
    return 1;
  }

  if (kill(job->pid, SIGCONT)< 0){
    perror("bg: kill");
    return 1;
  }

  job->state = RUNNING;
  printf("[%d]+ %s\n", job->job_id, job->command);
  return 0;
}



int is_background(char **args){
  int i = 0;
  while(args[i] != NULL){
    i++;
  }

  if (i > 0 && strcmp(args[i-1], "&") == 0){
    free(args[i-1]);
    args[i-1] = NULL;
    return 1;
  }

  return 0;
}

void sigchild_handler(int sig){
  (void) sig;
  int status;
  pid_t pid;
  job_t *job;

  while((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0){
    job = find_job_pid(pid);
    if (WIFEXITED(status) || WIFSIGNALED(status)){
      if (job != NULL){
        printf("\n[%d]  Done\t\t%s\n", job->job_id, job->command);
        remove_job(job->job_id);
        printf("mini-shell>");
        fflush(stdout);
      }
    }
    else if (WIFSTOPPED(status)){
      if (job != NULL){
        job->state = STOPPED;
      }
    }
  }
}

void cleanup_jobs(){
  for (int i = 0; i < MAX_JOBS; i++){
    if (job_table[i].in_use != 0){
      kill(job_table[i].pid, SIGTERM);
      usleep(100000);
      kill(job_table[i].pid, SIGKILL);
      waitpid(job_table[i].pid, NULL, 0);
    }
  }
}

int shell_help(char** args){
  (void) args;
  printf("Type a program name along with it's arguments\n");
  printf("Here is a list of all the built in commands\n");
  printf("bg: Run jobs in the background\n");
  printf("cd: Change directories\n");
  printf("exit: Exits the shell\n");
  printf("fg: Moves jobs from background to foreground\n");
  printf("help: Displays a list of builtin commands\n");
  printf("history: Displays previous user commands\n");
  printf("jobs: Displays status of jobs in the current session\n");
  return 0;
}


void sigint_handler(int sig){
  sigset_t mask, oldmask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGCHLD);
  sigprocmask(SIG_BLOCK, &mask, &oldmask);

  if (foreground_pid > 0){
    kill(foreground_pid, SIGKILL);
  }

  for (int i = 0; i < MAX_JOBS; i++){
    if (job_table[i].in_use != 0){
      kill(job_table[i].pid, SIGTERM);
      usleep(100000);
      kill(job_table[i].pid, SIGKILL);
      waitpid(job_table[i].pid, NULL, 0);
    }
  }

  for (int i = 0; i < history_count; i++){
    free(command_history[i]);
  }
  printf("\nmini-shell terminated\n");
  fflush(stdout);

  exit(0);
}

int has_operator(char** args, char* op){
  for (int i = 0; args[i] != NULL; i++){
    if (strcmp(args[i], op) == 0){
      return 1;
    }
  }
  return 0;
}

int count_commands(char** args, char* op){
  int count = 1;
  for (int i = 0; args[i] != NULL; i++){
    if (strcmp(args[i], op) == 0){
      count++;
    }
  }
  return count;
}

char*** split_operator(char** args, char* op, int numCommands){
  char*** commands = malloc(numCommands * sizeof(char**));
  if (commands == NULL){
    return NULL;
  }

  int index = 0;
  int arg_start = 0;

  for (int i = 0; args[i] != NULL || index < numCommands; i++){
    if (args[i] == NULL || strcmp(args[i], op) == 0){
      int arg_count = i - arg_start;
      commands[index] = malloc((arg_count + 1) * sizeof(char*));
      if (commands[index] == NULL){
        for (int j = 0; j < index; j++){
            free(commands[j]);
          }
          free(commands);
          return NULL;
      }

      for (int j = 0; j < arg_count; j++){
        commands[index][j] = args[arg_start + j];
      }
      commands[index][arg_count] = NULL;
      index++;
      arg_start = i + 1;
    }

    if (args[i] == NULL){
      break;
    }
  }
  return commands;
}

void execute_pipeline(char** args){
  int numCommands = count_commands(args, "|");
  char*** commands = split_operator(args, "|", numCommands);

  if (commands == NULL){
    perror("malloc");
    return;
  }

  int numPipes = numCommands - 1;
  int (*pipes)[2] = malloc(numPipes * sizeof(int[2]));
  if (pipes == NULL){
    perror("malloc");
    for (int i = 0; i < numCommands; i++){
      free(commands[i]);
    }
    free(commands);
    return;
  }

  for (int i = 0; i < numPipes; i++){
    if (pipe(pipes[i]) < 0){
      perror("pipe");
      for (int j = 0; j < i; j++){
        close(pipes[j][0]);
        close(pipes[j][1]);
      }
      free(pipes);
      for (int j = 0; j < numCommands; j++){
        free(commands[j]);
      }
      free(commands);
      return;
    }
  }

  pid_t pids[numCommands];
  for (int i = 0; i < numCommands; i++){
    pids[i] = fork();

    if (pids[i] == 0){
      setpgid(0,0);
      signal(SIGINT, SIG_DFL);
      signal(SIGTSTP, SIG_DFL);
      signal(SIGCHLD, SIG_DFL);
      signal(SIGTTOU, SIG_DFL);
      signal(SIGTTIN, SIG_DFL);

      if (i > 0){
        dup2(pipes[i-1][0], STDIN_FILENO);
      }
      
      if (i < numCommands - 1){
        dup2(pipes[i][1], STDOUT_FILENO);
      }

      for (int j = 0; j < numPipes; j++){
        close(pipes[j][0]);
        close(pipes[j][1]);
      }

      execvp(commands[i][0], commands[i]);
      printf("mini-shell>Command not found--Did you mean something else?\n");
      exit(EXIT_FAILURE); //might have to free stuff here
    }
    else if (pids[i] < 0){
      perror("fork");
    }
  }

  for (int i = 0; i < numPipes; i++){
    close(pipes[i][0]);
    close(pipes[i][1]);
  }

  for (int i = 0; i < numCommands; i++){
    waitpid(pids[i], NULL, 0);
  }
  free(pipes);
  for (int i = 0; i < numCommands; i++){
    free(commands[i]);
  }
  free(commands);
}

void execute_sequential(char** args){
  int numCommands = count_commands(args, ";");
  char*** commands = split_operator(args, ";", numCommands);

  if (commands == NULL){
    perror("malloc");
    return;
  }

  for (int i = 0; i < numCommands; i++){
    shell_exec(commands[i]);
  }
  for (int i = 0; i < numCommands; i++){
    free(commands[i]);
  }
  free(commands);
}

void execute_and(char** args){
  int numCommands = count_commands(args, "&&");
  char*** commands = split_operator(args, "&&", numCommands);

  if (commands == NULL){
    perror("malloc");
    return;
  }

  for (int i = 0; i < numCommands; i++){
    int commandStatus = 0;
    int is_builtin = 0;

    if (has_operator(commands[i], "|")){
      pid_t pid = fork();
      if (pid == 0){
        execute_pipeline(commands[i]);
        exit(0);
      }
      else if (pid > 0){
        waitpid(pid, &commandStatus, 0);
      }
      else{
        perror("fork");
        break;
      }
    }
    else{
      for (int j = 0; j < NUM_BUILTIN_FUNC; j++){
        if (strcmp(commands[i][0], array_builtin[j].builtin_name) == 0){
          commandStatus = array_builtin[j].func(commands[i]);
          is_builtin = 1;
          break;
        }
      }

      if (!is_builtin){
        pid_t pid = fork();
        if (pid == 0){
          setpgid(0, 0);
          signal(SIGINT, SIG_DFL);
          signal(SIGTSTP, SIG_DFL);
          signal(SIGCHLD, SIG_DFL);
          signal(SIGTTOU, SIG_DFL);
          signal(SIGTTIN, SIG_DFL);

          execvp(commands[i][0], commands[i]);
          printf("mini-shell>Command not found--Did you mean something else?\n");
          exit(EXIT_FAILURE);
        }
        else if (pid > 0){
          waitpid(pid, &commandStatus, 0);
        }
        else{
          perror("fork");
          break;
        }
      }
    }

    if (is_builtin){
      if (commandStatus != 0){
        break;  
      }
    }
    else{
      if (WIFEXITED(commandStatus) && WEXITSTATUS(commandStatus) != 0){
        break; 
      }
      if (WIFEXITED(commandStatus) == 0){
        break;  
      }
    }
  }
  for (int i = 0; i < numCommands; i++){
    free(commands[i]);
  }
  free(commands);
}

void execute_or(char** args){
  int numCommands = count_commands(args, "||");
  char*** commands = split_operator(args, "||", numCommands);

  if (commands == NULL){
    perror("malloc");
    return;
  }

  for (int i = 0; i < numCommands; i++){
    int commandStatus = 0;
    int is_builtin = 0;

    if (has_operator(commands[i], "|")){
      pid_t pid = fork();
      if (pid == 0){
        execute_pipeline(commands[i]);
        exit(0);
      }
      else if (pid > 0){
        waitpid(pid, &commandStatus, 0);
      }
      else{
        perror("fork");
        break;
      }
    }
    else{
      for (int j = 0; j < NUM_BUILTIN_FUNC; j++){
        if (strcmp(commands[i][0], array_builtin[j].builtin_name) == 0){
          commandStatus = array_builtin[j].func(commands[i]);
          is_builtin = 1;
          break;
        }
      }
      if (is_builtin == 0){
        pid_t pid = fork();
        if (pid == 0){
          setpgid(0,0);
          signal(SIGINT, SIG_DFL);
          signal(SIGTSTP, SIG_DFL);
          signal(SIGCHLD, SIG_DFL);
          signal(SIGTTOU, SIG_DFL);
          signal(SIGTTIN, SIG_DFL);

          execvp(commands[i][0], commands[i]);
          printf("mini-shell>Command not found--Did you mean something else?\n");
          exit(EXIT_FAILURE);

        }

        else if (pid > 0){
          waitpid(pid, &commandStatus, 0);
        }
        else{
          perror("fork");
          break;
        }
      }
    }

    if (is_builtin){
      if (commandStatus == 0){
        break;
      }
    }
    else{
      if (WIFEXITED(commandStatus) && WEXITSTATUS(commandStatus) == 0){
        break;
      }
    }
  }
  for (int i = 0; i < numCommands; i++){
    free(commands[i]);
  }
  free(commands);
}