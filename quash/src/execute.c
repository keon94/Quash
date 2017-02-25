/**
 * @file execute.c
 *
 * @brief Implements interface functions between Quash and the environment and
 * functions that interpret an execute commands.
 *
 * @note As you add things to this file you may want to change the method signature
 */

#include "execute.h"
#include "string.h"

#include <stdio.h>

#include "parsing_interface.h"

#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include "list.h"


// Remove this and all expansion calls to it
/**
 * @brief Note calls to any function that requires implementation
 */
#define IMPLEMENT_ME()                                                  \
  fprintf(stderr, "IMPLEMENT ME: %s(line %d): %s()\n", __FILE__, __LINE__, __FUNCTION__)

#define MAX_SIZE 1024

//the signal for killing a process. Its value is determined by run_kill
static int kill_signal = -2;

typedef struct Job{
  int job_id;  //id for the job
  List pid_list; //the process ids of the processes in the job
  char* cmd_input; //received cmd command for the job
  int process_pipe[2];
} Job;

List job_list = {NULL, NULL, 0};

//int process_pipe[2];

//removes a job whose processes have all terminated
void remove_job(Job* job){
  assert(job != NULL);
  if(job->cmd_input)
    free(job->cmd_input);
  free(job);
}

//removes a job and kills all of its ongoing processes
void kill_job_with_processes(Job* job){
  assert(job != NULL);
  pid_t pid;
  //remove all processes in the job
  while(!is_empty(&job->pid_list)){
    pid = *(pid_t*)peek_back(&job->pid_list);
    if(kill(pid, kill_signal) == -1){
      fprintf(stderr, "Failed to kill process %d running under job %d. Error no. %d\n", job->job_id, pid, errno );
    }
    remove_from_back(&job->pid_list, &free); //get rid of the pid from the pid list
  }
  //remove the job
  if(job->cmd_input)
    free(job->cmd_input);
  free(job);
}


/***************************************************************************
 * Interface Functions
 ***************************************************************************/

// Return a string containing the current working directory.
char* get_current_directory(bool* should_free) {
  char* current_dir = malloc(sizeof(char)*MAX_SIZE);
	return getcwd(current_dir,MAX_SIZE);

}

// Returns the value of an environment variable env_var
const char* lookup_env(const char* env_var) {
  return getenv(env_var);
}


// Check the status of background jobs
void check_jobs_bg_status() {
    
    int status;
    bool all_processes_completed;
    pid_t pid, back_pid, pid_state;
    Node *pid_node_next , *job_node_next;
    for(Node *job_node = job_list.back; job_node != NULL ;){ //traverse the job list
      all_processes_completed = true;
      back_pid = *(pid_t*)peek_back(&((Job*)peek(job_node))->pid_list); 
      for(Node* pid_node = ((Job*)peek(job_node))->pid_list.back; pid_node != NULL;){ //traverse the pid list (in each job)
        pid = *(pid_t*)peek(pid_node);
        pid_state = waitpid(pid, &status, WNOHANG);
        pid_node_next = pid_node->next_node;
        if(pid_state == pid){
          remove_node( &((Job*)peek(job_node))->pid_list , pid_node, &free); //child pid is done. remove from pid queue
        }
        else if(pid_state == -1){ //child encountered an error, job is complete, but with errors
          remove_node( &((Job*)peek(job_node))->pid_list , pid_node, &free);
          fprintf(stderr,"An error occured in Job %d. Child Process %d returned error code %d.\n", ((Job*)peek(job_node))->job_id, pid, errno); //error handling
          fflush(stderr);
        }
        else{
          //child process is still running, so job is incomplete
          all_processes_completed = false;
        }
        pid_node = pid_node_next;
      }

      job_node_next = job_node->next_node;
      if(all_processes_completed){ //all children are finished, so the job is done. remove it from the job list.
        print_job_bg_complete(((Job*)peek(job_node))->job_id, back_pid, ((Job*)peek(job_node))->cmd_input); 
        remove_node(&job_list, job_node, (void*)remove_job);   //frees the job 
      }
      job_node = job_node_next;
    }
    
}

// Prints the job id number, the process id of the first process belonging to
// the Job, and the command string associated with this job
void print_job(int job_id, pid_t pid, const char* cmd) {
  printf("[%d]\t%8d\t%s\n", job_id, pid, cmd);
  fflush(stdout);
}

// Prints a start up message for background processes
void print_job_bg_start(int job_id, pid_t pid, const char* cmd) {
  printf("Background job started: ");
  print_job(job_id, pid, cmd);
}

// Prints a completion message followed by the print job
void print_job_bg_complete(int job_id, pid_t pid, const char* cmd) {
  printf("Completed: \t");
  print_job(job_id, pid, cmd);
}

/***************************************************************************
 * Functions to process commands
 ***************************************************************************/
// Run a program reachable by the path environment variable, relative path, or
// absolute path
void run_generic(GenericCommand cmd) {
  // Execute a program with a list of arguments. The `args` array is a NULL
  // terminated (last string is always NULL) list of strings. The first element
  // in the array is the executable
  char* exec = cmd.args[0];
  char** args = cmd.args;
  //printf("process %d about to execvp on %s\n", getpid(), cmd.args[0]);
  execvp(exec, args);
  fprintf(stderr, "ERROR: Failed to execute %s. Error #%d\n", exec, errno);
  exit(EXIT_FAILURE);
}

// Print strings
void run_echo(EchoCommand cmd) {
  // Print an array of strings. The args array is a NULL terminated (last
  // string is always NULL) list of strings.
  char** str = cmd.args;

  (void) str; // Silence unused variable warning

  for(; *str != '\0'; ++str ){
    printf("%s ", *str);
  }
  printf("\n");
  // Flush the buffer before returning
  fflush(stdout);
}

// Sets an environment variable
void run_export(ExportCommand cmd) {
  // Write an environment variable
  const char* env_var = cmd.env_var;
  const char* val = cmd.val;

  if(setenv(env_var,val,1) == -1){
    fprintf(stderr,"Error: Failed to update the %s environment variable to %s. Error #%d\n",env_var,val,errno);
    exit(EXIT_FAILURE);  
  }
}

// Changes the current working directory
void run_cd(CDCommand cmd) {
  // Get the directory name
  const char* dir = cmd.dir;

  // Check if the directory is valid
  if (dir == NULL) {
    fprintf(stderr,"Error: Failed to resolve path. Error #%d\n",errno);
    exit(EXIT_FAILURE);  
  }
  char* cwd = get_current_directory(NULL);

  setenv("PREV_PWD", cwd ,1);

  if(chdir(cmd.dir) == -1){
    fprintf(stderr,"Error: Failed to go to %s. Error #%d\n",cmd.dir,errno);
    exit(EXIT_FAILURE);
  }
  free(cwd);
  cwd = get_current_directory(NULL);
  if(setenv("PWD",cwd,1) == -1){
    fprintf(stderr,"Error: Failed to update the PWD environment variable to %s. Error #%d\n",cwd,errno);
    exit(EXIT_FAILURE);
  }
  free(cwd);    
}

// Sends a signal to all processes contained in a job
void run_kill(KillCommand cmd) {
  int signal = cmd.sig;
  int job_id = cmd.job;

  //check if job is running
  for(Node* job_node = job_list.back; job_node != NULL; job_node = job_node->next_node){   
    if(((Job*)peek(job_node))->job_id == job_id){ //find the job in the job list
      kill_signal = signal;
      print_job_bg_complete(job_id, *(pid_t*)peek(((Job*)peek(job_node))->pid_list.back), (char*)((Job*)peek(job_node))->cmd_input);
      remove_node(&job_list, job_node, (void*)kill_job_with_processes);
      break;
    }
  }
}


// Prints the current working directory to stdout
void run_pwd() {
  char* cwd = get_current_directory(NULL);
	printf("%s \n", cwd);
  // Flush the buffer before returning
  fflush(stdout);
  free(cwd);
}

// Prints all background jobs currently in the job list to stdout
void run_jobs() {
  for(Node* job_node = job_list.back; job_node != NULL; job_node = job_node->next_node){
    print_job(((Job*)peek(job_node))->job_id, *(pid_t*)peek_back(&((Job*)peek(job_node))->pid_list), ((Job*)peek(job_node))->cmd_input);
  }
  fflush(stdout);
}

/***************************************************************************
 * Functions for command resolution and process setup
 ***************************************************************************/

/**
 * @brief A dispatch function to resolve the correct @a Command variant
 * function for child processes.
 *
 * This version of the function is tailored to commands that should be run in
 * the child process of a fork.
 *
 * @param cmd The Command to try to run
 *
 * @sa Command
 */
void child_run_command(Command cmd) {
  CommandType type = get_command_type(cmd);

  switch (type) {
  case GENERIC:
    run_generic(cmd.generic);
    break;
  case ECHO:
    run_echo(cmd.echo);
    break;
  case PWD:
    run_pwd();
    break;
  case JOBS:
    run_jobs();
    break;
  case EXPORT:
  case CD:
  case KILL:
  case EXIT:
  case EOC:
    break;

  default:
    fprintf(stderr, "Unknown command type: %d\n", type);
  }
}

/**
 * @brief A dispatch function to resolve the correct @a Command variant
 * function for the quash process.
 *
 * This version of the function is tailored to commands that should be run in
 * the parent process (quash).
 *
 * @param cmd The Command to try to run
 *
 * @sa Command
 */
void parent_run_command(Command cmd) {
  CommandType type = get_command_type(cmd);

  switch (type) {
  case EXPORT:
    run_export(cmd.export);
    break;
  case CD:
    run_cd(cmd.cd);
    break;
  case KILL:
    run_kill(cmd.kill);
    break;
  case GENERIC:
  case ECHO:
  case PWD:
  case JOBS:
  case EXIT:
  case EOC:
    break;
  default:
    fprintf(stderr, "Unknown command type: %d\n", type);
  }
}

void printList(List *l){
    for(Node* n = l->back;; n = n->next_node){
        printf("%d ;", *(int*)n->data);
        //printf("{%p , %p}\n", n->prev_node, n->next_node);
        if(n == l->front) break;
    }
    printf("\n");
}
/**
 * @brief Creates one new process centered around the @a Command in the @a
 * CommandHolder setting up redirects and pipes where needed
 *
 * @note Processes are not the same as jobs. A single job can have multiple
 * processes running under it. This function creates a process that is part of a
 * larger job->
 *
 * @note Not all commands should be run in the child process. A few need to
 * change the quash process in some way
 *
 * @param holder The CommandHolder to try to run
 *
 * @sa Command CommandHolder
 */
void create_process(CommandHolder holder, Job* job) {
  // Read the flags field from the parser
  bool p_in  = holder.flags & PIPE_IN;
  bool p_out = holder.flags & PIPE_OUT;
  bool r_in  = holder.flags & REDIRECT_IN;
  bool r_out = holder.flags & REDIRECT_OUT;
  bool r_app = holder.flags & REDIRECT_APPEND; // This can only be true if r_out
                                               // is true

  // TODO: Remove warning silencers
  (void) p_in;  // Silence unused variable warning
  (void) p_out; // Silence unused variable warning
  (void) r_in;  // Silence unused variable warning
  (void) r_out; // Silence unused variable warning
  (void) r_app; // Silence unused variable warning

  #define P_READ 0
  #define P_WRITE 1
  //printf("Proc: %d , Pipes: p_in %d, p_out %d\n", getpid(), p_in, p_out);

  // TODO: Setup pipes, redirects, and new process
  //IMPLEMENT_ME();

  //check if piping is necessary
  

  

	pid_t *m_pid = malloc(sizeof(pid_t));
	*m_pid = fork();

	if(*m_pid == 0){
    //printf("child pid %d now executing %s.\n", getpid(), holder.cmd.generic.args[0]);
    if(p_out == true) { //the writer child to the pipe
      //printf("writer pid %d from parent %d: Pipes: p_in %d, p_out %d\n", getpid(), getppid(), p_in, p_out);
      if(close(job->process_pipe[P_READ]) < 0){ 
        fprintf(stderr, "Child Error: Failure in closing the Write-end of process_pipe. Error no. %d\n", errno); 
        return;
      }
      dup2(job->process_pipe[P_WRITE], STDOUT_FILENO);
      //printf("child pid %d has dup2d.\n", getpid());
      //return;
    } //find test-cases -type f -name '*'.txt | grep valgrind
       
    if(p_in == true){ //the reader child from the pipe
      //printf("reader pid %d from parent pid %d Pipes: p_in %d, p_out %d\n", getpid(), getppid(), p_in, p_out);
      if(close(job->process_pipe[P_WRITE]) < 0){ 
        fprintf(stderr, "Child Error: Failure in closing the Write-end of process_pipe. Error no. %d\n", errno); 
        return;
      }
      dup2(job->process_pipe[P_READ], STDIN_FILENO);
    }

    if(r_out==true && r_app==true){
        FILE* out_file;
        out_file=fopen(holder.redirect_out,"a"); //a indicated append
        
        dup2(fileno(out_file),STDOUT_FILENO); //http://stackoverflow.com/questions/14543443/in-c-how-do-you-redirect-stdin-stdout-stderr-to-files-when-making-an-execvp-or-- was throwing a really weird warning because it wasn't cast
        fclose(out_file);
    }
    if(r_out == true){
        FILE* out_file;
        out_file=fopen(holder.redirect_out,"w"); //w indicates write
        dup2(fileno(out_file),STDOUT_FILENO);
        fclose(out_file);
    }
    
    if(r_in == true){
        FILE* in_file;
        in_file = fopen(holder.redirect_in, "r");
        dup2(fileno(in_file), STDIN_FILENO);
        fclose(in_file);
    }
    
    child_run_command(holder.cmd); // This should be done in the child branch of a fork;  
    exit(EXIT_SUCCESS);
	}
	else{  
    //dup2(process_pipe[P_READ], STDIN_FILENO);
    /*
    if(p_out){
      //close(process_pipe[P_WRITE]);
      if(close(process_pipe[P_READ]) < 0)
      { 
        fprintf(stderr, "Error: Failure in closing the Write-end of process_pipe. Error no. %d\n", errno); 
      } 
    }*/

    if(p_in){ //parent must close the end of the pipe
      if(close(job->process_pipe[P_READ]) < 0)
      { 
        fprintf(stderr, "Error: Failure in closing the Write-end of process_pipe. Error no. %d\n", errno); 
      } 

      if(close(job->process_pipe[P_WRITE]) < 0)
      { 
        fprintf(stderr, "Error: Failure in closing the Read-end of process_pipe. Error no. %d\n", errno); 
      } 
    }

    if(get_command_type(holder.cmd) == CD || get_command_type(holder.cmd) == EXPORT || get_command_type(holder.cmd) == KILL)
		  parent_run_command(holder.cmd); // This should be done in the parent branch of // a fork   

    //printf("parent pid: %d generated child pid: %d\n", getpid(), *m_pid)  ;        
    add_to_front(&job->pid_list, m_pid); 
    //printList(pid_list);                              
	}
}
//find test-cases -type f -name '*'.txt | grep valgrind
void init_job(Job* job){
  job->job_id = 1;
  init_list(&job->pid_list);
  job->cmd_input = get_command_string();
  if(pipe(job->process_pipe) < 0){
    fprintf(stderr, "Piping failed for the command %s. Error no. %d", get_command_string(), errno);
  }
}

// Run a list of commands
void run_script(CommandHolder* holders) {
  if (holders == NULL)
    return;

  if(!is_empty(&job_list))
    check_jobs_bg_status();

  if (get_command_holder_type(holders[0]) == EXIT &&
      get_command_holder_type(holders[1]) == EOC) {
    end_main_loop();
    return;
  }
 
  Job* job = malloc(sizeof(Job));
  init_job(job);
  
  CommandType type;
  // Run all commands in the `holder` array. This is every process's cmd per job'
  /*
  if(pipe(process_pipe) < 0){
    fprintf(stderr, "Piping Failed. ERROR no. %d\n", errno);
    return;
  }
*/

  for (int i = 0; (type = get_command_holder_type(holders[i])) != EOC; ++i){
		create_process(holders[i], job); 
	} 
  if (!(holders[0].flags & BACKGROUND)) {
    // Not a background Job
    // Wait for all processes under the job to complete
      int status;
      for(pid_t* child_process = (pid_t*)peek_back(&job->pid_list); !is_empty(&job->pid_list) ;remove_from_back(&job->pid_list, &free)){
        //printf("List: ");printList(&job->pid_list);
        //printf("checking proc %d\n", *child_process);
        if ((waitpid(*child_process, &status, 0)) == -1) {
          fprintf(stderr, "Job %d, Process %d encountered an error. ERROR %d\n", job->job_id, *child_process, errno);
          //exit(EXIT_FAILURE);
        }      //find test-cases -type f -name '*'.txt | grep valgrind 
        /*else{
          printf("process %d successfully finished.\n", *child_process);
        }*/
      }
      remove_job(job);
  }
  else {
    // A background job->
    
    if(!is_empty(&job_list)){
      job->job_id = ((Job*)peek_front(&job_list))->job_id + 1; //otherwise assign a new job id
    }
    add_to_front(&job_list, job); //push to the queue the new job
    print_job_bg_start(job->job_id, *(pid_t*)peek_back(&job->pid_list), job->cmd_input);

  }
}
