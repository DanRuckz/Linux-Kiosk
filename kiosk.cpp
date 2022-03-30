#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <cstring>

void sizeof_string(const char*, unsigned int *);
void copy_argv(std::vector<char*> *command, int * argc, char** argv);
pid_t pid;
void handle_signal(int sig);
std::vector<char*>command;


int main(int argc, char *argv[]){
    
    char fail_arguments[] = "No arguments given, aborting.\n";
    //This functions takes the arguments from the program and coppies it to the command vector. This is needed because argv[0] is always the current program, and we cannot send that to execv
    copy_argv(&command, &argc, argv);
    command.push_back(NULL);
    int child_status_val;
    //signal handlers for SIGINT and SIGTERM, I think we can make a bitmask for the rest of them and use that.
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
        
        //Checks if the number of arguments is sufficient, to avoid sending a vector without arguments to execv. One permanent element is always a NULL pointer, because the execv requires the argument vector to be NULL
        //terminated for each argument, and the last argument must be a null pointer. Writing the error message to stdout and quitting.
        if(command.size() <2){
        write(STDOUT_FILENO, fail_arguments, sizeof(fail_arguments));
        return 0;
        }

        //This is the loop where the kiosk is happening. A fork is happening here, the child is running and the parent is waiting
        //as soon as the child has died, the parent would print "finished waiting, closing." and get back to the beginning of the loop and open a new child process with the same command.
        while(true){
            pid = fork();
            if(pid == -1){
                perror("");
            }

            if (pid == 0){
                execv(command[0],command.data());
                perror("Failed");
                puts("Please use absolute path for your program");
            }
                waitpid(pid, &child_status_val, 0);
                puts("finished waiting, closing.");
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
    }
    else if (sig = 15){
        signal = (char*)"SIGTERM";
    }
    printf("Signal %s received, closing\n", signal);
    int size = command.size();
    for (int i=0;i<size-1;i++){
        free(command[i]);
        }
        kill(pid, SIGTERM);
        exit(0);
    }

    //strcat(argument, "\n");
    //sizeof_string(argument, &size);
    //write(STDOUT_FILENO, argument, size);    