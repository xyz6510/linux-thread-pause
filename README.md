# linux-thread-pause
<b>pause linux thread</b>
<pre>

#define _GNU_SOURCE

#include &ltsched.h&gt
#include &ltsignal.h&gt
#include &ltsys/mman.h&gt
#include &ltunistd.h&gt

int somefunction(void *data)
{
  //do not use malloc, use mmap like this
  //mmap(NULL,num_bytes_to_allocate,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
  //using mmap like this will zero memory , so it is slower then malloc
  
  //do not use printf,fprintf use write
  //char msg[64];
  //int len=sprintf(msg,"my name is\n");
  //write(STDOUT_FILENO,msg,len);
  
  for(;;) {
  //do something
    sleep(1);
  }
  return 0;
}

int main()
{
  //ignore SIGCHLD
  signal(SIGCHLD,SIG_IGN);
  //alocate stack
  int size=64*1024;
  char *stack=mmap(NULL,size,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
  //use clone
  int pid=clone(somefunction,stack+size,CLONE_VM|SIGCHLD,NULL);

  for(;;) {
    //send stop cont signals to pid
    kill(pid,SIGSTOP);
    sleep(1);
    kill(pid,SIGCONT);
  }

  return 0;
}

</pre>
<pre>
example tp.c
compile with gcc -o tp tp.c -lncurses
</pre>
