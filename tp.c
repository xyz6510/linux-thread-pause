/*
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sched.h>
#include <signal.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <ncurses.h>

typedef struct {
	int id;
	int pid;
	char *s;
	char *m;
	int cnt;
} THP;

int glb=0;

void sigrt(int s,siginfo_t *info,void *u)
{
	THP *th=info->si_value.sival_ptr;
	th->cnt++;
}

int proc(void *arg)
{
	THP *th=arg;
	int pid=getpid();
	sprintf(th->m,"%2i:%6i:%6i",th->id,pid,th->cnt);

	struct sigaction sa;
    memset(&sa,0,sizeof(sa));
    sa.sa_sigaction=sigrt;
    sa.sa_flags=SA_SIGINFO;
    sigfillset(&sa.sa_mask);

	sigaction(SIGRTMIN+0,&sa,NULL);

	kill(pid,SIGSTOP);

	for (;;) {
		sprintf(th->m,"%2i:%6i:%6i",th->id,pid,th->cnt);
		th->cnt++;
		glb++;
		usleep(500*1000);
	}

	return 0;
}

int main()
{
	signal(SIGCHLD,SIG_IGN);

	int stack_size=sizeof(void *)*1024;
	int mem_size=64*1024;
	int thpx=4;
	int thpy=8;
	int thp_num=thpx*thpy;


	THP *thp=malloc(thp_num*sizeof(THP));
	memset(thp,0,sizeof(thp_num*sizeof(THP)));

	int flags=CLONE_VM | SIGCHLD;

	srand(time(NULL));
	int i;
	for (i=0;i<thp_num;i++) {
		THP *th=&thp[i];
		th->id=i;
		th->cnt=rand()%1000;
		th->s=mmap(NULL,stack_size,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
		th->m=mmap(NULL,mem_size,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
		th->pid=clone(proc,th->s+stack_size,flags,th);
	}
//	for (i=0;i<thp_num;i++) {
//		THP *th=&thp[i];
//		kill(th->pid,SIGCONT);
//	}

	initscr();
	cbreak();
	noecho();
	clear();
	nodelay(stdscr,TRUE);
	curs_set(0);

	char str[128];
	int idx=0;
	mvhline(2,0,'-',80);
	for (;;) {
		sprintf(str,"id:%2i ESC: exit",idx);
		mvaddstr(0,0,str);
		sprintf(str,"p:pause-all c:cont-all q:pause-id w:cont-id ,:dec-id .:inc-id s:signal-id");
		mvaddstr(1,0,str);
		sprintf(str,"global:%5i",glb);
		mvaddstr(4,0,str);
		int j;
		for (i=0;i<thpx;i++) {
			for (j=0;j<thpy;j++) {
				THP *th=&thp[(i*thpy)+j];
				mvaddnstr(j+6,i*20,th->m,20);
			}
		}
		refresh();
		int ch=getch();
		if (ch!=ERR) {
			int ex=0;
			union sigval val;
			switch(ch) {
				case 'p':
					for (i=0;i<thp_num;i++) {THP *th=&thp[i];kill(th->pid,SIGSTOP);}
				break;
				case 'c':
					for (i=0;i<thp_num;i++) {THP *th=&thp[i];kill(th->pid,SIGCONT);}
				break;
				case '.':
					idx++;if (idx>=thp_num) idx=thp_num-1;
				break;
				case ',':
					idx--;if (idx<0) idx=0;
				break;
				case 'q':
					kill(thp[idx].pid,SIGSTOP);
				break;
				case 'w':
					kill(thp[idx].pid,SIGCONT);
				break;
				case 's':
					val.sival_ptr=&thp[idx];sigqueue(thp[idx].pid,SIGRTMIN+0,val);
				break;
				case 27://esc
					ex=1;
				break;
			}
			if (ex) break;
		}
		usleep(16000);
	}
	endwin();

	for (i=0;i<thp_num;i++) {
		THP *th=&thp[i];
		int ret=kill(th->pid,SIGKILL);
		fprintf(stderr,"sigkill:%i returns:%i\n",th->pid,ret);
		munmap(th->s,stack_size);
		munmap(th->m,mem_size);
	}

	return 42;
}
