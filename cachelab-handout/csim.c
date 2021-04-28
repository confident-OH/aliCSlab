#include "cachelab.h"
#include <unistd.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int vinfo = 0;  // 显示轨迹
int groupS;     // 组索引数
int groupL;     // 关联度
int blocksize;  //内存快地址位数
char fileAddr[50]; //内存访问文件地址
char cacheOP;
long long int memoryAddr;
long long int memorySize;
int hit_count = 0, miss_count = 0, eviction_count = 0;
typedef struct cache_cao{
    long long int tag;
    struct cache_cao *next;
}cache_cao;
cache_cao *cache;

void printf_usage(void)
{
    printf("Usage: ./csim-ref [-hv] -s <num> -E <num> -b <num> -t <file>\n");
    printf("Options:\n");
    printf("  -h         Print this help message.\n");
    printf("  -v         Optional verbose flag.\n");
    printf("  -s <num>   Number of set index bits.\n");
    printf("  -E <num>   Number of lines per set.\n");
    printf("  -b <num>   Number of block offset bits.\n");
    printf("  -t <file>  Trace file.\n");
    printf("Examples:\n");
    printf("  linux>  ./csim-ref -s 4 -E 1 -b 4 -t traces/yi.trace\n");
}

void cache_init(void)
{
    int maxl = 1 << groupS;
    cache = (cache_cao*)malloc(maxl*sizeof(cache_cao));
    for(int i = 0; i<maxl; i++){
        cache[i].next = NULL;
        cache[i].tag = 0;
    }
}

int visitMemory(long long int addr)
{
    int thisTag = addr >> (groupS + blocksize);
    int thisGroup = addr >> blocksize;
    thisGroup = thisGroup & ((1<<groupS) - 1);
    cache_cao *current = &cache[thisGroup];
    int i = 0;
    while(current->next != NULL){
        i++;
        if(current->next->tag == thisTag){
            if(i>1){
                cache_cao *item = current->next->next;
                current->next->next = cache[thisGroup].next;
                cache[thisGroup].next = current->next;
                current->next = item;
            }
            return 1;
        }
        else{
            current = current->next;
        }
    }
    current = cache[thisGroup].next;
    cache[thisGroup].next = (cache_cao*) malloc (sizeof(cache_cao));
    cache[thisGroup].next->tag = thisTag;
    cache[thisGroup].next->next = current;
    if(cache[thisGroup].tag != groupL){
        cache[thisGroup].tag++;
        return -1;
    }
    current = &cache[thisGroup];
    while(current->next->next!=NULL){
        current = current->next;
    }
    free(current->next);
    current->next = NULL;
    return -2;
}

int main(int argc, char* argv[])
{
    int Myoptid;
    int recv = 0;
    while((Myoptid = getopt(argc, argv, "h::v::s:E:b:t:")) != -1){
        switch (Myoptid)
        {
        case 'h':
            printf_usage();
            return 0;
        case 'v':
            vinfo = 1;
            break;
        case 's':
            groupS = atoi(optarg);
            break;
        case 'E':
            groupL = atoi(optarg);
            break;
        case 'b':
            blocksize = atoi(optarg);
            break;
        case 't':
            strcpy(fileAddr, optarg);
            break;
        default:
            printf("Undetermined configure parameter. \n");
            printf_usage();
            return 0;
        }
    }
    FILE *fp;
    if((fp = fopen(fileAddr, "r")) == NULL){
        printf("Error addr!\n");
        return 0;
    }
    cache_init();
    cacheOP = getc(fp);
    while(cacheOP!=EOF){
        if(cacheOP == 'I'){
            getc(fp);
            getc(fp);
        }
        else{
            cacheOP = getc(fp);
            getc(fp);
        }
        fscanf(fp, "%llx", &memoryAddr);
        fgetc(fp);
        fscanf(fp, "%lld", &memorySize);
        fgetc(fp);
        printf(" %c %llx,%lld\n", cacheOP, memoryAddr, memorySize);
        switch (cacheOP)
        {
        case 'I':
            break;
        
        case 'L':
            recv = visitMemory(memoryAddr);
            if(recv == 1){
                hit_count++;
            }
            else{
                miss_count++;
                if(recv == -2){
                    eviction_count++;
                }
            }
            break;

        case 'S':
            recv = visitMemory(memoryAddr);
            if(recv == 1){
                hit_count++;
            }
            else{
                miss_count++;
                if(recv == -2){
                    eviction_count++;
                }
            }
            break;

        case 'M':
            recv = visitMemory(memoryAddr);
            if(recv == 1){
                hit_count+=2;
            }
            else{
                miss_count++;
                hit_count++;
                if(recv == -2){
                    eviction_count++;
                }
            }
            break;    

        default:
            break;
        }
        if (vinfo)
        {
            switch(recv)
            {
                case 1:
                    printf("%c %llx,%lld hit\n", cacheOP, memoryAddr, memorySize);
                    break;
                case -1:
                    printf("%c %llx,%lld miss\n", cacheOP, memoryAddr, memorySize);
                    break;
                case -2:
                    printf("%c %llx,%lld miss eviction\n", cacheOP, memoryAddr, memorySize);
                    break;
            }
        }
        cacheOP = fgetc(fp);
    }
    printSummary(hit_count, miss_count, eviction_count);
    return 0;
}
