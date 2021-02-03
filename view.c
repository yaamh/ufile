#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <curses.h>
#include "filelist.h"
#include "view.h"
#include "file.h"

dirnode_s *cwdnode;
char basepath[256];
//根据文件结构体定位屏幕中的行号
int index_file(win_s *win,filenode_s *file)
{
    int i;
    for(i=0;i<win->printnumy;i++)
    {
        if(file == win->file_arr[win->printstarty + i].file)
            return i;
    }
    //如果在界面上找不到父文件，则向上查找
    i = 0;
    while(win->printstarty + i >= 0)
    {
        if(file == win->file_arr[win->printstarty + i].file)
            return i;
        i--;
    }
    return 0;
}

//清屏
void clear_win(win_s *win)
{
    wclear(win->window);
}

//刷新属性
void flush_attr(win_s *win)
{
    int i;
    attr_t attr;

    for(i=0;i<win->printnumy;i++)
    {
        attr=win->attr_arr[i];
        if(win->cutfile == win->file_arr[win->printstarty + i].file)
            attr|=A_BLINK;
        if(win->y == i)
            attr|=A_UNDERLINE;

        mvwchgat(win->window, i, win->x, win->w, attr, 0, NULL);
    }
}

//打印文件
int arr_filelist(void *node,void *args,int level)
{
    win_s *win = args;
    vfile_s *vf;

    //这里file个数超出范围，调整大小
    if(win->file_maxnum >= win->arr_maxnum)
    {
        win->file_arr = realloc(win->file_arr, sizeof(vfile_s)*(win->arr_maxnum + 32));
        win->arr_maxnum += 32;
    }
    vf = &win->file_arr[win->file_maxnum++];
    vf->file = node;
    vf->level = level;
    
    return 0;
}

//打印文件列表
void print_dirlist(win_s *win, dirnode_s *dirnode)
{
    int i;
    vfile_s *vf;
    
    win->file_maxnum = 0;
    foreach_file(dirnode, arr_filelist,win);
    
    win->printnumy = win->file_maxnum - win->printstarty;
    if(win->printnumy > win->h)
        win->printnumy = win->h;
    
    for(i=0;i<win->printnumy;i++)
    {
        vf = &win->file_arr[win->printstarty + i];
        
        if(vf->file->type == FT_DIR)
            mvwprintw(win->window,i,win->spe*vf->level,"%c%s",((dirnode_s*)vf->file)->showchild?'-':'+',vf->file->name);
        else
            mvwprintw(win->window,i,win->spe*vf->level,"%s",vf->file->name);
    }
}


//初始化窗口
void init_win(win_s *win, int h, int w, int y, int x)
{
    win->fullwin = newwin(h,w,y,x);
    win->window = derwin(win->fullwin,
            h - 2, w - 2, y + 1, x + 1);
    getmaxyx(win->window, win->h, win->w);
    win->y = win->x = 0;
    win->printstarty = 0;
    win->printnumy = 0;
    win->spe = 2;
    win->attr_arr = malloc(sizeof(attr_t)*win->h);
    memset(win->attr_arr,0,sizeof(attr_t)*win->h);
    win->file_arr = malloc(sizeof(vfile_s)*win->h);
    memset(win->file_arr,0,sizeof(vfile_s)*win->h);
    win->arr_maxnum = win->h;
    win->file_maxnum = 0;    
    keypad(win->window,TRUE);
    box(win->fullwin,0,0);
    wrefresh(win->fullwin);
}

//初始化界面
void init_view(view_s *view)
{
    init_win(&view->win[0],LINES,COLS,0,0);
    view->winnums = 1;

    //获取当前绝对路径
    getcwd(basepath,sizeof(basepath));
    //创建当前目录节点
    cwdnode = create_rootnode(basepath);
    cwdnode = get_filelist(cwdnode);
    cwdnode->showchild = 1;

    //打印文件列表
    print_dirlist(&view->win[0],cwdnode);

    //添加选中下划线
    view->curwin = &view->win[0];

    //刷新窗口
    flush_attr(view->curwin);
    wrefresh(view->curwin->window);
}

//移动选中窗口
void move_win(view_s *view,int type)
{
    view->curwin++;
    if(view->curwin == &view->win[view->winnums])
        view->curwin = &view->win[0];

}

//移动光标
void move_line(win_s *win,int type)
{
    if(type == 'j')
    {
        if(win->y + 1 < win->printnumy)
            //当光标未达到最下方文件，则光标下移
            win->y++;
        else
        {
            if(win->printstarty + win->printnumy < win->file_maxnum)
            {
                //当光标达到最下方，但下方还有文件时，文件上移                
                win->printfileindex++;
                clear_win(win);
                print_dirlist(win, cwdnode);
            }
        }
    }
    else if(type == 'k')
    {
        if(win->y > 0)
            //当光标未达最上方文件，光标上移
            win->y--;
        else
        {
            if(win->printstarty)
            {
                //若已达到最上方,但是上方还有文件,则文件下移
                win->printstarty--;
                clear_win(win);
                print_dirlist(win,cwdnode);
            }
        }
    }
}

//张开目录
void toggle_dir(win_s *win)
{
    dirnode_s *pdir;
    filenode_s *file;
    
    if(win->y < win->printnumy)
    {
        file = win->file_arr[win->printstarty + win->y].file;
        if(file->type == DT_DIR)
        {
            pdir = (dirnode_s*)file;
            get_filelist(pdir);
            pdir->showchild = !pdir->showchild;
            clear_win(win);
            print_dirlist(win, cwdnode);
        }
        else
        {
        }
    }
}

//折叠父目录
void fold_dir(win_s *win)
{
    filenode_s *file;
    
    if(win->y < win->printnumy)
    {
        file = win->file_arr[win->printstarty + win->y].file;
        if(!file->father->file.father)
            return;
        ((dirnode_s*)file->father)->showchild = 0;
        win->y = index_file(win,(filenode_s*)file->father);
        if(win->y < 0)
        {
            win->printstarty += win->y;
            win->y = 0;
        }
        clear_win(win);
        print_dirlist(win,cwdnode);
    }
}

//黏贴文件
void past_file(win_s *win)
{
    filenode_s *pfile;
    dirnode_s *fatherdir;

    if(!win->cutfile)
        return;

    if(win->y < win->printnumy)
    {
        pfile = win->file_arr[win->printstarty + win->y].file;
        if(pfile->type == DT_DIR)
        {
            if(((dirnode_s*)pfile)->showchild)
                fatherdir = (dirnode_s*)pfile;
            else
                fatherdir = pfile->father;
        }
        else
            fatherdir = pfile->father;

        if(win->cutfile->type == DT_DIR)
        {
            //目录拷贝要满足2个条件，1.负责文件不能本来就在目标文件夹下 2.复制文件不能是目标文件本身或直系父目录
            if(win->cutfile->father == fatherdir)
                    return;

            dirnode_s *pdir = fatherdir;
            while(pdir)
            {
                if(pdir == (dirnode_s*)win->cutfile)
                    return;
                pdir = pdir->file.father;
            }
        }
        else
        {
            //文件复制需满足1个条件，文件不能本身在目标文件夹下
            pfile = win->cutfile;
            if(pfile->father == fatherdir)
                return;
        }

        //检测文件复制是否成功
        if(insert_checkfile(fatherdir,win->cutfile))
            return;
        if(win->bcpyfile)
        {
            //复制文件
            copy_file(win->cutfile, fatherdir);
            insert_filenode(fatherdir, win->cutfile,TRUE);
        }
        else
        {
            if(move_file(win->cutfile, fatherdir))
                return;
            delete_file(win->cutfile);
            insert_file(fatherdir, win->cutfile,FALSE);
        }
    }
    win->cutfile = NULL;
    clear_win(win);
    print_dirlist(win,cwdnode);
}

//剪切文件
void cut_file(win_s*win)
{
    if(win->y < win->printnumy)
        win->cutfile = win->file_arr[win->printstarty + win->y].file;
}

//复制文件
void cpy_file(win_s *win)
{
    if(win->y < win->printnumy)
    {
        win->cutfile = win->file_arr[win->printstarty + win->y].file;
        win->bcpyfile = 1;
    }
}

//删除文件
void del_file(win_s*win)
{
    filenode_s *file;
    if(win->y < win->printnumy)
    {
        file = win->file_arr[win->printstarty + win->y].file;
        delete_file(file);
        delete_filenode(file);
        clear_win(win);
        print_dirlist(win,cwdnode);
    }
}

//显示界面
void* show_view(void * arg)
{
    int ch;
    view_s view;

    initscr();
    start_color();
    noecho();
    cbreak();
    curs_set(FALSE);
    refresh();

    init_view(&view);

    int run = TRUE;

    while(run)
    {
        ch = getch();
        switch(ch)
        {
            case 'q':
                run = FALSE;
                break;
            case 'j':
            case 'k':
                move_line(view.curwin,ch);
                break;
            case 'h':
            case 'l':
                move_win(&view,ch);
                break;
            case '\n':
                toggle_dir(view.curwin);
                break;
            case 'x':
                fold_dir(view.curwin);
                break;
            case 'd':
                if('d' == getch())
                    cut_file(view.curwin);
                break;
            case 'y':
                if('y' == getch())
                    cpy_file(view.curwin);
                break;
            case 'D':
                del_file(view.curwin);
                break;
            case 'p':
                past_file(view.curwin);
                break;
            case 0x1b:  //撤销复制
                view.curwin->cutfile = NULL;
                break;
        }
        flush_attr(view.curwin);
        wrefresh(view.curwin->window);
    }
    endwin();
}
































