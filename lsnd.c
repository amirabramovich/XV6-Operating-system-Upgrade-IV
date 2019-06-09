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

void filter_numbers(char str[]){
    int i = 0;
    int j = 0;
    int flag = 0;
    while(str[i]){
        if((str[i]>='0'&&str[i]<='9')||str[i]==' '||str[i]=='('||str[i]==')'||str[i]==','){
            if(flag){
                str[j]=str[i];
                j++;
            }
            flag = 1;
        }
        i++;
    }
    str[j]='\0';
}

int main(int argc, char *argv[]){

    char path[50];
    char buf[1024];
    int num = inodes_counter();
    strcpy(path, "/proc/inodeinfo/");
    printf(1,"%s","#device | #inode | is valid | type | (major,minor) | hard links | blocks used\n");
    for(int i=0;i<num;i++){
        itoa(i, path+16);
        int fd = open(path, O_RDONLY);
        read(fd, buf, 1024);
        //printf(1, "%s\n", buf);
        filter_numbers(buf);
        printf(1,"%s\n", buf);
    }
    exit();
}