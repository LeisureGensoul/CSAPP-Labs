#include "stdio.h"
#include "string.h"
#include "stdbool.h"
#include "stdlib.h"
#include "getopt.h"
#include "unistd.h"
#include "limits.h"
#include "cachelab.h"

// 行数组结构
typedef struct {
    int valid;      // 有效位,1有效，0无效
    int tag;        // 标记
    int time;   // 模拟倒计时
    char* block;    // 缓存块
} cache_line_t;

// 缓存数组结构
typedef struct {
    int S;              // 组的数量
    int E;              // 每组的行数
    int B;              // 块的大小
    cache_line_t ***cache_lines;    // 行的数组
} cache_t;

cache_t Cache;   // 缓存实例
int Hits = 0, Misses = 0, Evicts = 0;    // 三个需要输出的参数
static const int max_time = INT_MAX;    //倒计时的初始最大值

char filename[100];

int s,E,b;

int verbose=false;

void initLines() {
    // cache_line_t ** : 指向一个组 
    // cache_line_t *  : 指向一个行

    // 初始化组空间
    Cache.cache_lines = (cache_line_t ***)malloc(Cache.S * sizeof(cache_line_t **));
    for (int i=0; i < Cache.S; ++i) {
        Cache.cache_lines[i] = (cache_line_t **)malloc(Cache.E * sizeof(cache_line_t *));
    }
    // 为每个组中的行申请空间
    for (int i=0; i < Cache.S; ++i) {
        for (int j=0; j < Cache.E; ++j) {
            Cache.cache_lines[i][j] = (cache_line_t *)malloc(sizeof(cache_line_t));
        }
    }
    // 初始化每个行的参数和块
    for (int i=0; i < Cache.S; ++i) {
        for (int j=0; j < Cache.E; ++j) {
            Cache.cache_lines[i][j]->valid = 0;
            Cache.cache_lines[i][j]->tag = -1;
            Cache.cache_lines[i][j]->time = max_time;
            Cache.cache_lines[i][j]->block = (char *)malloc(sizeof(char) * Cache.B);
        }
    }
}

void initCache(int s, int e, int b) {
    Cache.S = 1 << s;
    Cache.E = e;
    Cache.B = 1 << b;
    initLines();
}

void freeCache() {
    // 清理每个block指针
    for (int i=0; i < Cache.S; ++i) {
        for (int j = 0; j < Cache.E; ++j) {
            free(Cache.cache_lines[i][j]->block);
        }
    }
    // 清理行
    for (int i=0; i < Cache.S; ++i) {
        for (int j = 0; j < Cache.E; ++j) {
            free(Cache.cache_lines[i][j]);
        }
    }
    // 清理组
    for (int i=0; i < Cache.S; ++i) {
        free(Cache.cache_lines[i]);
    }
    // 清理cache_lines
    free(Cache.cache_lines);
}

// 获取命中的行号
int getLine(int set_id, int tag) {
    for (int i=0; i<Cache.E; ++i) {
        // 有效位置位且tag匹配则表示命中
        if (Cache.cache_lines[set_id][i]->valid && (Cache.cache_lines[set_id][i]->tag == tag)) {
            return i;
        }
    }
    return -1;
}

// 获取组set_id中的空行行号
int getEmptyLine(int set_id) {
    for (int i=0; i<Cache.E; ++i) {
        // 有效位没有置位则表示空行
        if (Cache.cache_lines[set_id][i]->valid == 0) {
            return i;
        }
    }
    return -1;
}

// 获取该组中LRU的牺牲行行号
int getLRULine(int set_id) {
    int line_index = 0, min_stamp = max_time;
    // 用for找到组中最小的倒计时和对应的行号
    for (int i=0; i<Cache.E; ++i) {
        if (Cache.cache_lines[set_id][i]->time < min_stamp) {
            line_index = i;
            min_stamp = Cache.cache_lines[set_id][i]->time;
        }
    }
    return line_index;
}

// 访问某一特定的行
void updateLine(int set_id, int line_id, int tag) {
    // 更新当前行
    Cache.cache_lines[set_id][line_id]->valid = 1;
    Cache.cache_lines[set_id][line_id]->tag = tag;
    Cache.cache_lines[set_id][line_id]->time = max_time;

    // 其他行倒计时减一
    for (int i=0; i < Cache.S; ++i) {
        for (int j = 0; j < Cache.E; ++j) {
            Cache.cache_lines[i][j]->time--;
        }
    }
}


// getLine: 在对应组中查找对应的tag，命中的话返回行号，否则返回-1.
// getEmptyLine: 从组中找到一个空行并返回行号，否则返回-1
// getLRULine：找到组中根据LRU策略找到的行号
// updateLine：更新对应行的tag并刷新倒计时

// 访问Cache，set_id为组号
void accessCache(int set_id, int tag) {
    int line_id = getLine(set_id, tag);
    if (line_id == -1) {    // 缓存未命中
        Misses++;
        line_id = getEmptyLine(set_id);
        if (line_id==-1) {  // 组中没有空行，需要缓存替换
            Evicts++;
            line_id = getLRULine(set_id);
        }
        updateLine(set_id, line_id, tag);
    } else {
        Hits++;
        updateLine(set_id, line_id, tag);
    }
}

// 通过追踪文件模拟对整个缓存的使用
void simTrace(int s, int E, int b, char *filename) {
    FILE *file_ptr;
    file_ptr = fopen(filename, "r");
    if (file_ptr==NULL) {
        printf("找不到追踪文件%s\n", filename);
        exit(1);
    }
    // 对应trace中的三个元素
    char operation;
    unsigned address;
    int size;
    // 从追踪文件中读取访问数据
    // 注意第二个参数最开始的空格
    while (fscanf(file_ptr, " %c %x,%d", &operation, &address, &size) > 0) {
        int tag = address  >> (s+b);    // 从地址中拆出tag
        int set_id = (address >> b) & ((unsigned)(-1) >> (8*sizeof(unsigned)-s));   // 从地址中拆出组id
        switch (operation) {
        case 'M':   // 修改需要两次访存：取出和重新保存
            accessCache(set_id, tag);
            accessCache(set_id, tag);
            break;
        case 'L':
            accessCache(set_id, tag);
            break;
        case 'S':
            accessCache(set_id, tag);
            break;
        }
    }
    fclose(file_ptr);
}

void getoptions(int argc,char* argv[])
{
    char opt;
	
	// 从命令行获取对应参数的值
    while ((opt=getopt(argc, argv, "hvs:E:b:t:"))!=-1) {
        switch (opt)
        {
			case 'h':
                printf("you wish help, This is no help!");
                exit(0);
            case 'v':
                verbose=true;
                break;
			case 's':
				s = atoi(optarg);
				break;
			case 'E':
				E = atoi(optarg);
				break;
			case 'b':
				b = atoi(optarg);
				break;
			case 't':
				strcpy(filename, optarg);
				break;
			default:
				printf("Cannot read the flag %c\n", opt);
				exit(1);
        }
    }
}

int main(int argc, char **argv)
{
    getoptions(argc,argv);

    initCache(s,E,b);
    simTrace(s,E,b,filename);
    printSummary(Hits, Misses, Evicts);
    freeCache();

    return 0;
}
