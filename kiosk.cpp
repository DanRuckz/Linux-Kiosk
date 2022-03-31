#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <cstring>
#include <ctime>


void sizeof_string(const char*, unsigned int *);
void copy_argv(std::vector<char*> *command, int * argc, char** argv);
pid_t pid;
void handle_signal(int sig);
std::vector<char*>command;
std::string execCommand(const char* cmd);

int main(int argc, char *argv[]){
    
    char fail_arguments[] = "No arguments given, aborting.\n";
    //This functions takes the arguments from the program and coppies it to the command vector. This is needed because argv[0] is always the current program, and we cannot send that to execv
    copy_argv(&command, &argc, argv);
    command.push_back(NULL);
    int child_status_val;
    //signal handlers for SIGINT and SIGTERM, I think we can make a bitmask for the rest of them and use that.
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
        std::string current_program_name = "echo " + std::string(command[0]) + "|awk -F/ '{print $NF}'";
        current_program_name = execCommand(current_program_name.c_str());
        current_program_name[strlen(current_program_name.c_str())-1] = ' ';
        std::string kill_command = "ps aux |grep -i " + current_program_name + "|grep -v 'grep'|grep -v "+ argv[0] + "|awk '{print $2}' |xargs kill 2>/dev/null";
        std::string output = execCommand((char *)kill_command.c_str());
        time_t start_time = time(0);
        unsigned int number_of_forks = 0;
        time_t running_time;
        
        int fd[2];
        if(pipe(fd) == -1){
            perror("Failed");
            return -1;
        }
        //This is the loop where the kiosk is happening. A fork is happening here, the child is running and the parent is waiting
        //as soon as the child has died, the parent would print "finished waiting, closing." and get back to the beginning of the loop and open a new child process with the same command.
        while(true){

            running_time = time(0);
            //printf("number of forks: %d\n", number_of_forks);
            if(running_time - start_time <= 5 && number_of_forks >= 5){
                //printf("running time %lu:\n", running_time-start_time);
                handle_signal(SIGUSR1);
            }
            if(running_time - start_time > 5 && number_of_forks <5){
                start_time = time(0);
                number_of_forks = 0;
            }

            pid = fork();
            if(pid == -1){
                perror("");
                exit(0);
            }

            if (pid == 0){
                number_of_forks+=1;
                if(write(fd[1], &number_of_forks, sizeof(int)) == -1){
                    puts("Can't write to pipe");
                }
                execv(command[0],command.data());
                perror("Failed");
                puts("Please use absolute path for your program");
                kill(getppid(),SIGTERM);
            }
            else{
                if(read(fd[0], &number_of_forks, sizeof(int)) == -1){
                    puts("Can't read from pipe");
                }
                waitpid(pid, &child_status_val, 0);
                puts("finished waiting, closing.");
            }
        }

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
                puts("Error Happened, could not allocate memory\n");
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


    //strcat(argument, "\n");
    //sizeof_string(argument, &size);
    //write(STDOUT_FILENO, argument, size);    