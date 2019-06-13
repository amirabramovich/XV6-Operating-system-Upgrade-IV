#include "types.h"
#include "stat.h"
#include "defs.h"
#include "param.h"
#include "traps.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"

#define PNFUNCS 5
#define NFUNCS 4  
#define NAME 300
#define STATUS 400
#define INODEINF 500

extern struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

extern void fd_counters(int array[6]);
extern void ide_counters(int array[3], int devices[64], int blocks[64]);
extern int inode_counters(int indexes[NINODE]);
extern struct inode * get_inode(int index);

// int to string:
int itoa(int n, char *str){
	int temp, len;
	temp = n;
	len = 1;
	while (temp/10!=0){
		len++;
		temp /= 10;
	}
	for (temp = len; temp > 0; temp--){
		str[temp-1] = (n%10)+48;
		n/=10;
	}
	str[len]='\0';
  return len;
}


int 
procfsisdir(struct inode *ip) {
  return ip->inum == namei("proc")->inum||ip->minor == T_DIR;
}

void 
procfsiread(struct inode* dp, struct inode *ip) {
	ip->valid = 1;
	ip->type = T_DEV;
	ip->major = PROCFS;
	if (ip->inum >= 1000)
		ip->minor = T_DIR;
	else
		ip->minor = T_FILE;
}

int
procfsread(struct inode *ip, char *dst, int off, int n) {
	struct proc *p;
	struct dirent de;
  char buf[1024]={0};

  //cprintf("ip is %d\n",ip->inum);

	if (ip == namei("proc")){
    char *pnames[PNFUNCS] = { ".", "..", "ideinfo", "filestat","inodeinfo"};
    ushort pnames_lengths[PNFUNCS] = {1,2,7,8,9};
		int index=PNFUNCS;

    de.inum = ip->inum;
    memmove(de.name, pnames[0], pnames_lengths[0]+1);
    memmove(buf, (char*)&de, sizeof(de));

    de.inum = ROOTINO;
    memmove(de.name, pnames[1], pnames_lengths[1]+1);
    memmove(buf+ sizeof(de), (char*)&de, sizeof(de));

    for(int i=2;i<PNFUNCS-1;i++){
      de.inum=i+996;
      memmove(de.name, pnames[i], pnames_lengths[i]+1);
      memmove(buf + i*sizeof(de), (char*)&de, sizeof(de));
    }

    de.inum=1000;
    memmove(de.name, pnames[PNFUNCS-1], pnames_lengths[PNFUNCS-1]+1);
    memmove(buf + (PNFUNCS-1)*sizeof(de), (char*)&de, sizeof(de));

		acquire(&ptable.lock);
		for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
			if(p->state != UNUSED && p->state != ZOMBIE){
        char numb[4];
				int numb_len = itoa(p->pid, numb);
				de.inum = p->pid+1000;
				memmove(de.name, numb, numb_len+1);
        memmove(buf + index * sizeof(de), (char*)&de, sizeof(de));
        index++;
			}
		}
		release(&ptable.lock);
    if (off < index * sizeof(de)) {
      int size = index * sizeof(de) - off;
      size = size < n ? size : n;
      memmove(dst, buf + off, size);
      return size;
    }
  }else if(ip->inum == 998){
      memset(buf,0,1024);
      int array[3];
      int devices[64];
      int blocks[64];
      char str[4];
      ide_counters(array, devices, blocks);
      memmove(buf, "waiting operations: ", 20);
      int len = itoa(array[0], str);
      

      memmove(buf+20, str, len);
      len += 20;
      

      memmove(buf+len, "\nread waiting: ", 15);
      len += 15;
      int tmp = itoa(array[1], str);
      memmove(buf+len, str, tmp);
      len += tmp;

      memmove(buf+len, "\nwrite waiting: ", 16);
      len += 16;
      tmp = itoa(array[2], str);
      memmove(buf+len, str, tmp);
      len += tmp;

      memmove(buf+len, "\nworking blocks: ", 17);
      len += 17;

      for(int i=0;i<array[0];i++){
        memmove(buf+len, "(", 1);
        len++;
        tmp = itoa(devices[i],str);
        memmove(buf+len, str, tmp);
        len += tmp;
        memmove(buf+len, ",", 1);
        len++;
        tmp = itoa(blocks[i],str);
        memmove(buf+len, str, tmp);
        len += tmp;
        memmove(buf+len, ");", 2);
        len+=2;
      }
      memmove(buf+len, "\n", 1);
      len++;
      if (off < len) {
        int size = len - off;
        size = size < n ? size : n;
        memmove(dst, (char *)((uint)buf+(uint)off), size);
        return size;
      }
    }else if(ip->inum == 999){
    memset(buf,0,1024);
    int array[6];
    char str[4];
    fd_counters(array);
    int len = itoa(array[0], str);
    memmove(buf, "free fds: ", 10);

    memmove(buf+10, str, len);
    len += 10;
    

    memmove(buf+len, "\nunique fds: ", 13);
    len += 13;
    int tmp = itoa(array[1], str);
    memmove(buf+len, str, tmp);
    len += tmp;

    memmove(buf+len, "\nwritable fds: ", 15);
    len += 15;
    tmp = itoa(array[2], str);
    memmove(buf+len, str, tmp);
    len += tmp;

    memmove(buf+len, "\nreadable fds: ", 15);
    len += 15;
    tmp = itoa(array[3], str);
    memmove(buf+len, str, tmp);
    len += tmp;

    memmove(buf+len, "\nrefs per fd: ", 14);
    len += 14;
    tmp = itoa(array[4], str);
    memmove(buf+len, str, tmp);
    len += tmp;

    memmove(buf+len, "/", 1);
    len += 1;
    tmp = itoa(array[5], str);
    memmove(buf+len, str, tmp);
    len += tmp;
    buf[len]='\n';
    len++;

    if (off < len) {
      int size = len - off;
      size = size < n ? size : n;
      memmove(dst, (char *)((uint)buf+(uint)off), size);
      return size;
    }
  }else if(ip->inum == 1000){
    memset(buf,0,1024);
    int indexes[NINODE];
    int total = inode_counters(indexes); 

    de.inum = ip->inum;
    memmove(de.name, ".", 2);
    memmove(buf, (char*)&de, sizeof(de));

    de.inum = namei("proc")->inum;
    memmove(de.name, "..", 3);
    memmove(buf+sizeof(de), (char*)&de, sizeof(de));

    for(int i=0;i<total;i++){
      char inode_num[4];
      int inode_len = itoa(indexes[i], inode_num);
      de.inum = indexes[i]+INODEINF+1;
      memmove(de.name, inode_num, inode_len+1);
      memmove(buf + (i+2) * sizeof(de), (char*)&de, sizeof(de));
    }
    if (off < (total+2) * sizeof(de)) {
      int size = (total+2) * sizeof(de) - off;
      size = size < n ? size : n;
      memmove(dst, buf + off, size);
      return size;
    }

  }else if(ip->inum > 1000){
    char *namelist[NFUNCS] = { ".", "..", "name", "status"};
    ushort namelist_lengths[NFUNCS] = {1,2,4,6};
    struct dirent pids[NFUNCS];
    pids[0].inum=ip->inum;
    pids[1].inum=namei("proc")->inum;
		for(int i=0 ;i<NFUNCS;i++){
      if(i>1)
			  pids[i].inum=(ip->inum%1000) + (i+1)*100;
			memmove(pids[i].name,namelist[i],namelist_lengths[i]+1);
		}
    //ip->size = NFUNCS * sizeof(de);
    if (off < NFUNCS * sizeof(de)) {
      int size = NFUNCS * sizeof(de) - off;
      size = size < n ? size : n;
      memmove(dst, (char *)((uint)pids+(uint)off), size);
      return size;
    }
	}else if(ip->inum < INODEINF){
    int pid = ip->inum % 100;
	  int type = ip->inum - pid;
    struct proc *myproc = 0;
    acquire(&ptable.lock);
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if (p->pid == pid){
        myproc = p;
        break;
      }
    }
    release(&ptable.lock);

    if (type == NAME){
      memmove(buf, myproc->name, strlen(myproc->name));
      buf[strlen(myproc->name)]='\n';
      buf[strlen(myproc->name)+1]='\0';
      if (off < strlen(myproc->name)+1) {
        int size = strlen(myproc->name)+1 - off;
        size = size < n ? size : n;
        memmove(dst, (char *)((uint)buf+(uint)off), size);
        return size;
      }
    }else if (type == STATUS){ 
      memmove(buf, "state: ",7);
      switch(p->state){
        case SLEEPING:
          memmove(buf+7, "SLEEPING", 8);
          break;
        case RUNNING:
          memmove(buf+7, "RUNNING", 8);
          break;
        case RUNNABLE:
          memmove(buf+7, "RUNNABLE", 8);
          break;
        default:
          break;
      }
      memmove(buf+15, " memory size: ", 14);
      int len= itoa(p->sz, buf+29)+1;
      memmove(buf+29+len, "\n\0", 2);
      if (off < len+30) {
        int size = len+30 - off;
        size = size < n ? size : n;
        memmove(dst, (char *)((uint)buf+(uint)off), size);
        return size;
      }
    }
  }else if(ip->inum < 1000 && ip->inum > INODEINF){
    int nid = ip->inum - INODEINF-1;
	  struct inode *mynode = get_inode(nid);
    memmove(buf, "Device: ",8);
    int len= itoa(mynode->dev, buf+8);
    memmove(buf+8+len, "\nInode_number: ",15);
    len += itoa(mynode->inum, buf+23+len);
    memmove(buf+23+len, "\nis_valid: ",11);
    len += itoa(mynode->valid, buf+34+len);
    memmove(buf+34+len, "\ntype: ",7);
    len += itoa(mynode->type, buf+41+len);
    memmove(buf+41+len, "\nmajor_minor: (",15);
    len += itoa(mynode->major, buf+56+len);
    memmove(buf+56+len, ",",1);
    len += itoa(mynode->minor, buf+57+len);
    memmove(buf+57+len, ")\nhard_links: ",14);
    len += itoa(mynode->ref, buf+71+len);
    memmove(buf+71+len, "\nblocks_used: ",14);
    if(mynode->type==T_DEV)
      len += itoa(0, buf+85+len);
    else
      len += itoa(mynode->size, buf+85+len);
    memmove(buf+85+len, "\n",1);
    if (off < len+86) {
      int size = len+86 - off;
      size = size < n ? size : n;
      memmove(dst, (char *)((uint)buf+(uint)off), size);
      return size;
    }
  }

  return 0;
}

int
procfswrite(struct inode *ip, char *buf, int n)
{
  //read-only filesystem
  return -1;
}

void
procfsinit(void)
{
  devsw[PROCFS].isdir = procfsisdir;
  devsw[PROCFS].iread = procfsiread;
  devsw[PROCFS].write = procfswrite;
  devsw[PROCFS].read = procfsread;
}
