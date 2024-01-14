#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h> 
#include <stdlib.h>

static int (*lazyInitialize)();
static int (*lazyRun)(char **argv);

//Data structure for storing the plugins, we will store the names of the plugins in an array and their handlers in another array at the
//same index (instance of perfect hashing).
static char plugin_names[10][21];
static void* plugin_handlers[10];
static int plugin_count = 0;

void get_user_input(char* cmd){
  //Put a ">" at the start of the new line to indicate shell is ready for a cmd.
  printf("> ");

  //Get cmd from the user. (Maximum: 200 chars + 1 NULL Terminator)
  fgets(cmd, 201, stdin);
  cmd[strlen(cmd) - 1] = '\0';
}

//Takes a string and a pointer to a string array, parses using " ", puts the values into the string array.
//Returns the number of arguments in the string.
int parse_cmd(char* cmd, char *arguments[21]){
  //Parse that input, split the string into a new array "arguments" with each element ending with a " ".
  char* ptr = strtok(cmd, " ");

  int cmd_argc = 0;
  while (ptr != NULL){
    arguments[cmd_argc++] = ptr;
		ptr = strtok(NULL, " ");
	}
 
  return cmd_argc;
}

void initialize_plugin(char* pname, void* handle){
  char* error = NULL;

  //check if plugin already exists in list
  int i; 
  for(i = 0; i < plugin_count; i++){
    if(strcmp(pname, plugin_names[i]) == 0){
      printf("Error: Plugin %s initialization failed!\n", pname);
      return;
    }
  }
 

  lazyInitialize = dlsym(handle, "initialize");
  error = dlerror();
  if(error){
    fprintf(stderr, "%s\n", error);
  }
  else{
    int initialize_result = lazyInitialize();
    if(initialize_result == 0){
      //Successfully loaded!
      strcpy(plugin_names[plugin_count], pname);
      plugin_handlers[plugin_count] = handle;
      plugin_count++;
      //plugin_names[plugin_count] is updated and plugin count is updated.
    }
    else {
     printf("Error: Plugin %s initialization failed!\n", pname);
    }
  }
}

void run_plugin(int i, char *arguments[21]){
  char* error = NULL;
  void* handle = plugin_handlers[i];

  lazyRun = dlsym(handle, "run");
  error = dlerror();
  if(error){
    fprintf(stderr, "%s\n", error);
  }
  else{
    int t = lazyRun(arguments);
  }
}

void load_plugin(char* pname){
  //get the path to the file from the plugin name (i.e. ./<pname>.so)
  char path_to_file[205] = "./";
  strcat(path_to_file, pname);
  strcat(path_to_file, ".so");

  //create a new handler
  void* handle = dlopen(path_to_file, RTLD_LAZY);
  if(!handle){
    printf("Error: Plugin %s initialization failed!\n", pname);
  }
  else{
    initialize_plugin(pname, handle);
  }
}

int check_for_built_ins(char *arguments[21]){
  //check for exit
  if(strcmp(arguments[0], "exit")  == 0){
    exit(0);
  }

  //check for load
  if(strcmp(arguments[0], "load") == 0 && arguments[1] != NULL){
    load_plugin(arguments[1]);
  }
  if(strcmp(arguments[0], "load") == 0 && arguments[1] == NULL){
    printf("Error: Plugin some_string initialization failed!\n");
  }
}

void check_for_plugins(char *arguments[21], int cmd_argc){
  int i; 
  for(i = 0; i < plugin_count; i++){
    if(strcmp(plugin_names[i], arguments[0]) == 0){
      //create an array of arguments following the plugin name, terminated by a NULL.
      char *plugin_argv[cmd_argc];
      //if there are no args
      if(cmd_argc == 1) plugin_argv[0] = NULL;
      else{
        int j;
        for(j = 0; j < cmd_argc; j++){
          plugin_argv[j] = arguments[j];
        }
        plugin_argv[j] = NULL;
      }
      
      run_plugin(i, plugin_argv);
    }
  }
}

void check_for_exec(char *arguments[21], int cmd_argc){
  //we have to create an array such that: "[path to bin, arg, arg, ..., NULL]"

  //path to binary
  char path_to_file[205] = "";
  //check for absolute or relative path
  if(strncmp(arguments[0], "/", 1) == 0 ||
    strncmp(arguments[0], ".", 1) == 0){
    strcpy(path_to_file, arguments[0]);
  }
  else{
    // char path_to_file[205] = "/usr/bin/";
    strcpy(path_to_file, "/usr/bin/");
    strcat(path_to_file, arguments[0]);
    arguments[0] = path_to_file;
  }
  
  pid_t pid = fork();

  if (pid == 0) {
    char *plugin_argv[cmd_argc];
    //if there are no args
    plugin_argv[0] = path_to_file;
    if(cmd_argc == 1) plugin_argv[1] = NULL;
    else{
      int j;
      for(j = 1; j < cmd_argc; j++){
        plugin_argv[j] = arguments[j];
      }
      plugin_argv[j] = NULL;
    }
   execv(path_to_file, plugin_argv);
   exit(EXIT_FAILURE);
  }
  //wait for exec to finish before doing whats next
  int status;
  pid_t ew = waitpid(pid, &status, 0);
}

int main(int argc, char* argv[]){
  while(1){  
    //get user input
    char cmd[201];
    get_user_input(cmd);

    //parse user input
    char *arguments[21]; 
    int cmd_argc = parse_cmd(cmd, arguments);

    //check first argument

    //Built in checker
    check_for_built_ins(arguments);
    
    //Plugin checker
    check_for_plugins(arguments, cmd_argc);

    //Attempt to exec
    check_for_exec(arguments, cmd_argc);
  }

  return 0;
}
