#ifndef _VCOM_DEBUG_H
#define _VCOM_DEBUG_H
#include <time.h>
#define ALL_SPACE 12
/* 
 * To show the stack content
 * The format should be like this :
 *
 * Function : <func>
 * Time : <hh/mm/ss>
 * State : <state>
 * |            |
 * |    Sync    |
 * |   Net Up   |
 * |  Net Down  |
 * --------------
 */ 
static inline void _lineup(struct stk_vc *stk, int fram, int count)
{
    int i;
    for(i = 0; i < fram/2; i++){
        printf(" ");
    }
    printf("%s", stk->stk_stat[count]->name());
     for(i = 0; i < fram/2; i++){
        printf(" ");
    }
    if(fram % 2){
        printf(" ");
    }
}

static inline void stk_display(struct stk_vc *stk, const char *func)
{
    int i;
  	int len, fram;
	time_t t = time(NULL);
	struct tm *lt = localtime(&t);
 
    if(stk_empty(stk)){
        printf("stack empty ...\n");
        return;
    }
	printf("Function : %s\n", func);
	printf("Time : %02d:%02d:%02d\n", lt->tm_hour, lt->tm_min, lt->tm_sec);
	printf("Stack :\n");
	printf("|%*s|\n", ALL_SPACE, " ");
    for(i = stk->top; i >= 0; i--){
		len = strlen(stk->stk_stat[i]->name());
		fram = ALL_SPACE - len;
		printf("|");
		_lineup(stk, fram, i);
        printf("|\n");
    }
	for(i = 0; i <= ALL_SPACE+2; i++){
		printf("-");
	}
    printf("\n");   
}
#endif
