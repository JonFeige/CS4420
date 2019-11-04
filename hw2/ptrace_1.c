/* The follwing program is based on an example 
   from the Linux Journal Article "Playing with ptrace, Part 1
   It was modified to run on a modern x86_64 processor (like pu1, pu2, pu3, etc.) 
*/

#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/reg.h>
#include <x86_64-linux-gnu/asm/unistd_64.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/*
__NR_write is the number of the write() system call
  see x86_64-linux-gnu/asm/unistd_64.h
*/


//Data.val = Ptrace(PTRACE_PEEKDATA,child,addr+I *, NULL);

struct fd {
   int fd_read;
   int fd_write;
};

struct fd fds[50];

int read_write(int fd, int read, int write)
{
	fds[fd].fd_read += read;
	fds[fd].fd_write += write;
}

int main(int argc, char **argv)
{   
	FILE *fptr;
	fptr = fopen(argv[1],"w");

	for(int i = 0; i < 50; i++)
	{
		fds[i].fd_read = 0;
		fds[i].fd_write = 0;
	}
	
	pid_t child;
    long orig_rax, rax;
    unsigned long params[3];
    int status;
    int insyscall = 0;
    child = fork();
	
	char * args[argc-2];
	memcpy(args,argv+2, ((argc-2) * sizeof(*argv)));
	args[argc-2] = NULL;
	
    if(child == 0) {
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        execvp(argv[2], args);
    }
    else {
		while(1) {
			wait(&status);
			if(WIFEXITED(status))
				break;
			orig_rax = ptrace(PTRACE_PEEKUSER,
                     child, 8 * ORIG_RAX, NULL);

			/*Write*/
			if(orig_rax == __NR_write) {
				if(insyscall == 0) {
					insyscall = 1;
					params[0] = ptrace(PTRACE_PEEKUSER,child, 8 * RDI,NULL);
					params[1] = ptrace(PTRACE_PEEKUSER,child, 8 * RSI,NULL);
					params[2] = ptrace(PTRACE_PEEKUSER,child, 8 * RDX,NULL);
						
					fprintf(fptr,"Write (%ld , %lx , %ld) --> ",params[0], params[1],params[2]);
					read_write(params[0],0,params[2]);
				}
				else {
					rax = ptrace(PTRACE_PEEKUSER,child, 8 * ORIG_RAX, NULL);
                    fprintf(fptr,"%ld\n\n", rax);
                    insyscall = 0;
                }
            }
			
			/*Read*/
			else if(orig_rax == __NR_read){
				if(insyscall == 0) {
					insyscall = 1;
					params[0] = ptrace(PTRACE_PEEKUSER,child, 8 * RDI,NULL);
					params[1] = ptrace(PTRACE_PEEKUSER,child, 8 * RSI,NULL);
					params[2] = ptrace(PTRACE_PEEKUSER,child, 8 * RDX,NULL);
								   
					fprintf(fptr,"Read (%ld , %lx , %ld) --> ",params[0], params[1],params[2]);
					read_write(params[0],params[2],0);
				}
				else {
					rax = ptrace(PTRACE_PEEKUSER,child, 8 * ORIG_RAX, NULL);
                    fprintf(fptr,"%ld\n\n", rax);
                    insyscall = 0;
                }
			}
			
			/*Open*/
			else if(orig_rax == __NR_open)
				if(insyscall == 0) {
					insyscall = 1;
					params[0] = ptrace(PTRACE_PEEKUSER,child, 8 * RDI,NULL);
					params[1] = ptrace(PTRACE_PEEKUSER,child, 8 * RSI,NULL);
								   
					fprintf(fptr,"Open (%ls , %ld) --> ",params[0], params[1]);
				}
				else {
					rax = ptrace(PTRACE_PEEKUSER,child, 8 * ORIG_RAX, NULL);
                    fprintf(fptr,"%ld\n\n", rax);
                    insyscall = 0;
                }
			
			/*Close*/
			else if(orig_rax == __NR_close)
				if(insyscall == 0) {
					insyscall = 1;
					params[0] = ptrace(PTRACE_PEEKUSER,child, 8 * RDI,NULL);
								   
					fprintf(fptr,"Close (%ld) --> ",params[0]);
				}
				else {
					rax = ptrace(PTRACE_PEEKUSER,child, 8 * ORIG_RAX, NULL);
                    fprintf(fptr,"%ld\n\n", rax);
                    insyscall = 0;
                }
			
			/*Exit*/			
			else if(orig_rax == __NR_exit)
				if(insyscall == 0) {
					insyscall = 1;
					params[0] = ptrace(PTRACE_PEEKUSER,child, 8 * RDI,NULL);
								   
					fprintf(fptr,"Exit (%ld) --> ",params[0]);
				}
				else {
					rax = ptrace(PTRACE_PEEKUSER,child, 8 * ORIG_RAX, NULL);
                    fprintf(fptr,"%ld\n\n", rax);
                    insyscall = 0;
                }
				
            ptrace(PTRACE_SYSCALL,
                   child, NULL, NULL);
        }
    }
	
	for(int i =0; i < 50; i++)
	{
		if((fds[i].fd_read != 0) || (fds[i].fd_write != 0))
		{
			fprintf(fptr,"file descriptor: %-5d, read volume: %-10d bytes,	write volume: %-10d bytes\n",i,fds[i].fd_read,fds[i].fd_write);
		}
	}
	fclose(fptr);
    return 0;
}
