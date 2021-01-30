#include <stdio.h>
#include <string.h>
#include "filelist.h"

int move_file(filenode_s *file,dirnode_s *target)
{
    char dirname[1024];
    const char* filename;

    strncpy(dirname,get_path(&target->file),sizeof(dirname));
    strncat(dirname,file->name,sizeof(dirname));

    filename = get_path(file);
    return rename(filename,dirname);
}
