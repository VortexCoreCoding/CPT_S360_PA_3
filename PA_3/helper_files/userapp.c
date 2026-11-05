// The userspace test side of the module, runs a counter function for a maximum of argv[1] seconds
#include "userapp.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

// Register process with kernel module
void register_process(unsigned int pid)
{
    FILE* process_file = fopen("/proc/kmlab/status", "w");
    if (!process_file) {
        printf("Error: Register failed, process file missing\n");
        return;
    }
    fprintf(process_file, "%u\n", pid);
    fflush(process_file);
    fclose(process_file);
}

int main(int argc, char* argv[])
{
    int __expire = 10;
    time_t start_time = time(NULL);
    unsigned n1 = 0;

    if (argc == 2) {
        __expire = atoi(argv[1]);
    }

    register_process(getpid());

    // Terminate user application if the time has expired
    while (1) {
        if ((int)(time(NULL) - start_time) > __expire) {
            break;
        }
        n1++;
    }
    printf("\nPID %u reached value: %u\n==Processes==\n", getpid(), n1);

    FILE* process_file = fopen("/proc/kmlab/status", "r");
    if (!process_file) {
        return 1;
    }
    char buff[200] = "";
    while(fgets(buff, 200, process_file)) {
        printf("%s\n", buff);
    }

	return 0;
}
