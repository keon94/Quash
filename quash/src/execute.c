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

#include "quash.h"
#include "parsing_interface.h"

#include <sys/wait.h>
#include <errno.h>



// Remove this and all expansion calls to it
/**
 * @brief Note calls to any function that requires implementation
 */
#define IMPLEMENT_ME()                                                  \
  fprintf(stderr, "IMPLEMENT ME: %s(line %d): %s()\n", __FILE__, __LINE__, __FUNCTION__)

#define MAX_SIZE 1024


typedef struct Job{
  int job_id;  //id for the job
  pid_t pid; //the process id of the first process in the job
  char* cmd_input; //received cmd command for the job
} Job;


IMPLEMENT_DEQUE_STRUCT(JobQueue, Job); // struct JobQueue - A list structure for all the jobs running in quash
PROTOTYPE_DEQUE(JobQueue, Job);
IMPLEMENT_DEQUE(JobQueue, Job);

JobQueue job_queue = {NULL,0,0,0};


/***************************************************************************
 * Interface Functions
 ***************************************************************************/

// Return a string containing the current working directory.
char* get_current_directory(bool* should_free) {
  // TODO: Get the current working directory. This will fix the prompt path.
  // HINT: This should be pretty simple
  char* current_dir = malloc(sizeof(char)*MAX_SIZE);
	return getcwd(current_dir,MAX_SIZE);

}

// Returns the value of an environment variable env_var
const char* lookup_env(const char* env_var) {
  // TODO: Lookup environment variables. This is required for parser to be able
  // to interpret variables from the command line and display the prompt
  // correctly
  // HINT: This should be pretty simple
  //IMPLEMENT_ME();

  // TODO: Remove warning silencers
  (void) env_var; // Silence unused variable warning

  return getenv(env_var);
}

// Check the status of background jobs
void check_jobs_bg_status() {
  // TODO: Check on the statuses of all processes belonging to all background
  // jobs. This function should remove jobs from the jobs queue once all
  // processes belonging to a job have completed.
  //IMPLEMENT_ME();

  // TODO: Once jobs are implemented, uncomment and fill the following line
  // print_job_bg_complete(job_id, pid, cmd);
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

  // TODO: Remove warning silencers
  (void) exec; // Silence unused variable warning
  (void) args; // Silence unused variable warning

  // TODO: Implement run generic
  //IMPLEMENT_ME();
  execvp(exec, args);
  fprintf(stderr, "ERROR: Failed to execute %s. Error #%d\n", exec, errno);
  exit(EXIT_FAILURE);
}

// Print strings
void run_echo(EchoCommand cmd) {
  // Print an array of strings. The args array is a NULL terminated (last
  // string is always NULL) list of strings.
  char** str = cmd.args;

  // TODO: Remove warning silencers
  (void) str; // Silence unused variable warning

  // TODO: Implement echo
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

  // TODO: Remove warning silencers
  (void) env_var; // Silence unused variable warning
  (void) val;     // Silence unused variable warning

  // TODO: Implement export.
  // HINT: This should be quite simple.
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

  // TODO: Remove warning silencers
  (void) signal; // Silence unused variable warning
  (void) job_id; // Silence unused variable warning

  // TODO: Kill all processes associated with a background job
  IMPLEMENT_ME();
}


// Prints the current working directory to stdout
void run_pwd() {
  // TODO: Print the current working directory
  //IMPLEMENT_ME();
  char* cwd = get_current_directory(NULL);
	printf("%s \n", cwd);
  // Flush the buffer before returning
  fflush(stdout);
  free(cwd);
}

// Prints all background jobs currently in the job list to stdout
void run_jobs() {
  // TODO: Print background jobs
  IMPLEMENT_ME();

  // Flush the buffer before returning
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
    break;
  case ECHO:
    break;
  case PWD:
    break;
  case JOBS:
    break;
  case EXIT:
    break;
  case EOC:
    break;

  default:
    fprintf(stderr, "Unknown command type: %d\n", type);
  }
}


/**
 * @brief Creates one new process centered around the @a Command in the @a
 * CommandHolder setting up redirects and pipes where needed
 *
 * @note Processes are not the same as jobs. A single job can have multiple
 * processes running under it. This function creates a process that is part of a
 * larger job.
 *
 * @note Not all commands should be run in the child process. A few need to
 * change the quash process in some way
 *
 * @param holder The CommandHolder to try to run
 *
 * @sa Command CommandHolder
 */
void create_process(CommandHolder holder, pid_t *first_child_pid) {
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

  // TODO: Setup pipes, redirects, and new process
  //IMPLEMENT_ME();
	//printf("%s\n%s\n%c\n", holder.redirect_in, holder.redirect_out, holder.flags);

	pid_t m_pid;

	m_pid = fork();
  if(first_child_pid)
    *first_child_pid = m_pid;
	if(m_pid == 0){   
		child_run_command(holder.cmd); // This should be done in the child branch of a fork;
    exit(EXIT_SUCCESS);
	}
	else{
    
    if(get_command_type(holder.cmd) == CD || get_command_type(holder.cmd) == EXPORT || get_command_type(holder.cmd) == KILL)
		  parent_run_command(holder.cmd); // This should be done in the parent branch of // a fork
      int status;                            
      if ((waitpid(m_pid, &status, 0)) == -1) {
        fprintf(stderr, "Process encountered an error. ERROR %d\n", errno);
        exit(EXIT_FAILURE);
      }     
	}
}



// Run a list of commands
void run_script(CommandHolder* holders) {
  if (holders == NULL)
    return;

  check_jobs_bg_status();

  if (get_command_holder_type(holders[0]) == EXIT &&
      get_command_holder_type(holders[1]) == EOC) {
    end_main_loop();
    return;
  }

  Job job = {.job_id = 1, .pid = 0 , .cmd_input = get_command_string()};
  
  CommandType type;
  // Run all commands in the `holder` array
	create_process(holders[0], &job.pid); //pass in pid for first process (child)
  for (int i = 1; (type = get_command_holder_type(holders[i])) != EOC; ++i){
		create_process(holders[i], NULL); //NULL because these are the remaining processes (after the first child)
	}
  
  if (!(holders[0].flags & BACKGROUND)) {
    // Not a background Job
    // TODO: Wait for all processes under the job to complete

     //first command   
    //IMPLEMENT_ME();
  }
  else {
    // A background job.
    // TODO: Push the new job to the job queue
    //IMPLEMENT_ME();
    
    if(job_queue.cap == 0){
      job_queue = new_JobQueue(1); //if the queue is empty, initialise it (to size 1)
    }
    else
      job.job_id = peek_back_JobQueue(&job_queue).job_id + 1; //otherwise assign a new job id
    
    push_front_JobQueue(&job_queue, job); //push to the queue the new job
    //TODO: need to take care of deallocations of the jobs upon completion
    // TODO: Once jobs are implemented, uncomment and fill the following line
    print_job_bg_start(job.job_id, job.pid, job.cmd_input);
  }
}
