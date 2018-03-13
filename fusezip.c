
/* Define version of fuse */
#define FUSE_USE_VERSION 26
 
/* All header files to be included */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <errno.h>
#include <fcntl.h>
#include <regex.h>
#include <zip.h>
#include <fuse.h>
#include <stdio.h> 
#include <sys/types.h> 
#include <sys/stat.h> 
#include <unistd.h> 
#include <errno.h> 
#include <string.h> 
#include <fcntl.h> 

/* Defining zip file paramenters */
#ifndef FUSEZIP_H
#define FUSEZIP_H

enum file_t
{
    ZIP_INVALID = -1,
    ZIP_FOLDER,
    ZIP_FILE,
};

#endif


static zip_t* ziparchive;
static char* zipname;

/* Appending slash to zip file if not present */
static char* append_slash(const char* path)
{
    char* search = malloc(strlen(path) + 2);
    strcpy(search, path);
    search[strlen(path)] ='/';
    search[strlen(path) + 1] = 0;

    return search;
}


static enum file_t fzip_file_type(const char* path)
{
    if (strcmp(path, "/") == 0)
        return ZIP_FOLDER;

    char* path_slash = append_slash(path + 1);
    int r1 = zip_name_locate(ziparchive, path_slash, 0);
    int r2 = zip_name_locate(ziparchive, path + 1, 0);

    free(path_slash);

    if (r1 != -1)
        return ZIP_FOLDER;
    else if (r2 != -1)
        return ZIP_FILE;
    else
        return ZIP_INVALID;
}


static int fzip_getattr(const char *path, struct stat *stbuf)
{

    if (strcmp(path, "/") == 0)
    {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        stbuf->st_size = 1;
        return 0;
    }

    zip_stat_t sb;
    zip_stat_init(&sb);

    char* path_slash = append_slash(path + 1);

    switch (fzip_file_type(path))
    {
    case ZIP_FILE:
        zip_stat(ziparchive, path + 1, 0, &sb);
        stbuf->st_mode = S_IFREG | 0777;
        stbuf->st_nlink = 1;
        stbuf->st_size = sb.size;
        stbuf->st_mtime = sb.mtime;
        break;
    case ZIP_FOLDER:
        zip_stat(ziparchive, path_slash, 0, &sb);
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        stbuf->st_size = 0;
        stbuf->st_mtime = sb.mtime;
        break;
    default:
        free(path_slash);
        return -ENOENT;
    }

    free(path_slash);
    return 0;
}


static int fzip_readdir(const char* path, void* buf, fuse_fill_dir_t filler,
                        off_t offset, struct fuse_file_info* fi)
{

    (void) offset;
    (void) fi;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    for (int i = 0; i < zip_get_num_entries(ziparchive, 0); i++)
    {
        struct stat st;
        memset(&st, 0, sizeof(st));
        zip_stat_t sb;
        zip_stat_index(ziparchive, i, 0, &sb);

        char* zippath = malloc(strlen(sb.name) + 2);
        *zippath = '/';
        strcpy(zippath + 1, sb.name);

        char* dpath = strdup(zippath);
        char* bpath = strdup(zippath);

        if (strcmp(path, dirname(dpath)) == 0)
        {
            if (zippath[strlen(zippath) - 1] == '/') zippath[strlen(zippath) - 1] = 0;
            fzip_getattr(zippath, &st);
            char* name = basename(bpath);
            if (filler(buf, name, &st, 0))
                break;
        }

        free(zippath);
        free(dpath);
        free(bpath);
    }

    return 0;
}


static int fzip_open(const char *path, struct fuse_file_info *fi)
{

    (void) fi;

    if(zip_name_locate(ziparchive, path + 1, 0) < 0)
        return -ENOENT; // some error that says the file does not exist

    return 0;
}


static int fzip_fstat(const char *filename)
{
     struct stat stat_p;        

      if ( -1 ==  stat (filename, &stat_p))
      {
        printf(" Error occoured attempting to stat %s\n", filename);
        exit(0);
      }
                    /* Print a few structure members.   */
       
      printf("Stats for %s \n", filename);

      printf("Modify time is %s", format_time(stat_p.st_mtime));

                    /* Access time does not get updated
                       if the filesystem is NFS mounted!    */

      printf("Access time is %s", format_time(stat_p.st_atime));
       
      printf("File size is   %d bytes\n", stat_p.st_size);

}

static int fzip_read(const char *path, char *buf, size_t size,
                     off_t offset, struct fuse_file_info* fi)
{
    int res;
    (void) fi;

    zip_stat_t sb;
    zip_stat_init(&sb);

    zip_stat(ziparchive, path + 1, 0, &sb);
    zip_file_t* file = zip_fopen(ziparchive, path + 1, 0);

    char temp[sb.size + size + offset];
    memset(temp, 0, sb.size + size + offset);

    res = zip_fread(file, temp, sb.size);

    if (res == -1)
        return -ENOENT;

    memcpy(buf, temp + offset, size);
    zip_fclose(file);

    return size;
}


static int fzip_mkdir(const char *path, mode_t mode)
{

    (void) mode;

    zip_dir_add(ziparchive, path + 1, 0);

    zip_close(ziparchive); // we have to close and reopen to write the changes
    ziparchive = zip_open(zipname, 0, NULL);

    return 0;
}


static int fzip_rename(const char *from, const char *to)
{

    zip_int64_t index = zip_name_locate(ziparchive, from + 1, 0);
    if (zip_file_rename(ziparchive, index, to + 1, 0)  == -1)
        return -ENOENT;

    zip_close(ziparchive); // we have to close and reopen to write the changes
    ziparchive = zip_open(zipname, 0, NULL);
    return 0;
}



static int fzip_mknod(const char* path, mode_t mode, dev_t rdev)
{
    printf("mknod: %s mode: %u\n", path, mode);

    (void) rdev;

    if ((mode & S_IFREG) == S_IFREG)
    {
        zip_source_t *s;

        if ((s = zip_source_buffer(ziparchive, NULL, 0, 0)) == NULL ||
                zip_file_add(ziparchive, path + 1, s, ZIP_FL_OVERWRITE) < 0)
        {
            zip_source_free(s);
            printf("Error adding file %s\n", path);
            return 0;
        }
    }

    return 0;
}



static int fzip_access(const char* path, int mask)
{
    printf("access: %s\n", path);

    (void) mask;

    if (fzip_file_type(path) >= 0)
        return 0;

    return -ENOENT;
}



static void fzip_close(void* private_data)
{
    (void) private_data;

    zip_close(ziparchive);
}


static struct fuse_operations fzip_oper =
{
    .access         = fzip_access,
    .getattr        = fzip_getattr,
    .readdir        = fzip_readdir,
    .open           = fzip_open,
    .read           = fzip_read,
    .mkdir          = fzip_mkdir,
    .mknod          = fzip_mknod,
    .rename         = fzip_rename,
    .fstat          = fzip_fstat,
    .close          = fzip_close,
};

int main(int argc, char *argv[])
{
    zipname = argv[1];
    ziparchive = zip_open(zipname, 0, NULL); 
    if (!ziparchive)
    {
        printf("Error opening file\n");
        return -1;
    }
    char* fuseargv[argc - 1];
    fuseargv[0] = argv[0];
    for (int i = 1; i < argc - 1; i++)
        fuseargv[i] = argv[i + 1];

    return fuse_main(argc - 1, fuseargv, &fzip_oper, NULL);
}
