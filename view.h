#ifndef __VIEW_H__
#define __VIEW_H__

#include <curses.h>


typedef struct
{
    WINDOW *fullwin;
    WINDOW *window;
    int x,y,w,h;
    int printstarty,printendy;
    int printfileindex,printindex;
    int spe;
    attr_t *attr_arr;
    void **file_arr;
    void  *cutfile;
} win_s;


typedef struct
{
    int winnums;
    win_s  win[4];
    win_s *curwin;
}view_s;

void* show_view(void* arg);


#endif
