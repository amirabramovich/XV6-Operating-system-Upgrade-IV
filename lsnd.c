#include "types.h"
#include "user.h"
#include "fcntl.h"
#include "stat.h"
#include "fs.h"

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

int main(int argc, char *argv[]){

    char path[50];
    char buf[1024];
    int num = inodes_counter();
    strcpy(path, "/proc/inodeinfo/");
    for(int i=0;i<num;i++){
        itoa(i, path+16);
        int fd = open(path, O_RDONLY);
        read(fd, buf, 1024);
        printf(1, "%s\n", buf);
    }
    exit();
}