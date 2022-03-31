#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <cstring>
#include <ctime>

//GLOBAL DECLARATIONS
void sizeof_string(const char*, unsigned int *);
void copy_argv(std::vector<char*> *command, int * argc, char** argv);
pid_t pid;
void handle_signal(int sig);
std::vector<char*>command;
std::string execCommand(const char* cmd);

int main(int argc, char *argv[]){
    
    char fail_arguments[] = "No arguments given, aborting.\n";
    //This functions takes the arguments from the program and coppies it to the command vector. This is needed because argv[0] is always the current program, and we cannot send that to execv
    //As an additional explanation to this, execv accepts char* and char**, each c string has to be null terminated and the last pointer in the char** array has to be a null pointer.
    //Each string in argv is null terminated but the last element is not a null pointer. Also, I am unaware of a way to send a partial char** argument.
    //I chose to use a vector because of it's ease of use and management and also easy to free the memory at the end.
    copy_argv(&command, &argc, argv);
    command.push_back(NULL);
    //Those signals are the de-facto ways to finish the program, the memory allocated above is cleaned within the signal handlers.
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    signal(SIGUSR1, handle_signal);
        
        //Checks if the number of arguments is sufficient, to avoid sending a vector without arguments to execv. One permanent element is always a NULL pointer, because the execv requires the argument vector to be NULL
        //terminated for each argument, and the last argument must be a null pointer. Writing the error message to stdout and quitting.
        if(command.size() <2){
        write(STDOUT_FILENO, fail_arguments, sizeof(fail_arguments));
        return 0;
        }
        //This is where we prepare the strings for the command to look up the process name and preemtively kill it, so the process the kiosk spawns should be the only one.
        //All of it is done by just simply running shell commands and getting their stdout back to a variable.
        std::string current_program_name = "echo " + std::string(command[0]) + "|awk -F/ '{print $NF}'"; //getting the program name using shell commands
        current_program_name = execCommand(current_program_name.c_str());
        current_program_name[strlen(current_program_name.c_str())-1] = ' '; //The command returned has a \n in the end, the current line replaces it with a ' '.
        std::string kill_command = "ps aux |grep -i " + current_program_name + "|grep -v 'grep'|grep -v "+ argv[0] + "|awk '{print $2}' |xargs kill 2>/dev/null";
        std::string output = execCommand((char *)kill_command.c_str()); //this executes the following ps aux |grep -i <kiosk_program_name> |grep -v 'grep'|grep -v <current_program_name> |awk '{print $2}' |xargs kill 2>/dev/null"

        //Initializing needed variables for the exis status of the child process and the timer with the fork count
        int child_status_val;
        time_t start_time = time(0);
        unsigned int number_of_forks = 0;
        time_t running_time;
        
        //Creating a pipe here for the processes to communicate
        int fd[2];
        if(pipe(fd) == -1){
            perror("Failed");
            return -1;
        }
        //This is the loop where the kiosk is happening. A fork is happening here, the child is running and the parent is waiting
        //as soon as the child has died, the parent would print "finished waiting, closing." and get back to the beginning of the loop and open a new child process with the same command.
        //The aim is to keep the target window open
        while(true){
            
            //The following 2 if statements are responsible to make sure that the program doesn't get into an infinite fork loop
            //Sometimes the target programs return 0 after opening and they don't close - this creates an infinite loop of opening target programs and avoids a total meltdown
            //It's based on a timer and will stop execution is the child process spawns fast enough, this is good enough for a browser.
            running_time = time(0);
            if(running_time - start_time <= 2 && number_of_forks >= 2){
                handle_signal(SIGUSR1);
            }
            if(running_time - start_time > 2 && number_of_forks <2){
                start_time = time(0);
                number_of_forks = 0;
            }
            //forking the process here
            pid = fork();
            if(pid == -1){
                perror("");
                exit(0);
            }
            //Child process execution 'number_of_forks' is added +1 after each successful fork, this is transferred back to the parent process to make sure the fork
            //is happening with compliance with the timer (not too fast)
            if (pid == 0){
                number_of_forks+=1;
                if(write(fd[1], &number_of_forks, sizeof(int)) == -1){
                    puts("Can't write to pipe");
                }
                execv(command[0],command.data());
                //if execution fails due to some error, throw a message and kill the parent process with SIGTERM - which should kill the child too
                perror("Failed");
                puts("Please use absolute path for your program");
                kill(getppid(),SIGTERM);
            }
            else{
                //parent reading from the child and puts it into the 'number_of_forks' variable
                if(read(fd[0], &number_of_forks, sizeof(int)) == -1){
                    puts("Can't read from pipe");
                }
                //waiting for the child to end and throwing a message. if signal not sent, after the child is done the loop starts over
                waitpid(pid, &child_status_val, 0);
                puts("finished waiting, closing.");
            }
        }
        //If for some reason the loop breaks, memory is freed here
        for(int i=0;i<command.size()-1;i++){
            free(command[i]);
        }
        command.clear();
    return 0;
}

//allocating memory in this function for each new string and then pushing that pointer with the allocation to the char* vector.
void copy_argv(std::vector<char*> *command, int* argc, char** argv){
    
    unsigned int ssize;
    char* string_allocator;

    for(int i=1; i < *argc; i++){
        sizeof_string(argv[i], &ssize);
        string_allocator = (char*)malloc(ssize);
            if(string_allocator == NULL){
                puts("Error Happened, could not allocate memory");
            }
        strcpy(string_allocator, argv[i]);
        command->push_back(string_allocator);
    }

}
//This function takes the string and a pointer to an unsigned int which will serve as the destination variable to store the size of the string given before.
void sizeof_string(const char * string, unsigned int * dest_size){
    unsigned int string_index = 0;
    unsigned int size =0;
    while(string[string_index]){
        string_index +=1;
        size +=1;
    }
    *(dest_size) = size;
}

//simple signal handler function, upon receiving the signal, it will free the memory allocated earlier and will kill the child process that it spawned and then finish itself with exit code of 0;
void handle_signal(int sig)
{   
    char* signal;
    if (sig = 2){
        signal = (char*)"SIGINT";
        printf("Signal %s received, closing by sending SIGTERM\n", signal);

    }
    else if (sig = 15){
        signal = (char*)"SIGTERM";
        printf("Signal %s received, closing by sending SIGTERM\n", signal);
    }
    else if (sig = 10){
        signal = (char*)"SIGUSR1";
        pid = getpid();
        printf("Signal %s received, closing to prevent a bug by sending SIGTERM\n", signal);
    }
        int size = command.size();
        for (int i=0;i<size-1;i++){
            free(command[i]);
            }
            kill(pid, SIGTERM);
            exit(0);
    }

//This function takes a const char* and returns a string, executes a shell command and returns the output. Usually ends with '\n'
std::string execCommand(const char* cmd){
    char buffer[128];
    std::string result = "";
    FILE* pipe = popen(cmd, "r");
    if (!pipe) throw std::runtime_error("popen() failed!");
    try {
        while (fgets(buffer, sizeof buffer, pipe) != NULL) {
            result += buffer;
        }
    } catch (...) {
        pclose(pipe);
        throw;
    }
    pclose(pipe);
    return result;
}
