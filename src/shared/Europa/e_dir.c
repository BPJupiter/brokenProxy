#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include "Clay/clay.h"

#ifdef _WIN32
#define EUROPA_WINDOWS_PATH_LENGTH_MAX (256 * 128 - 1)
#include <string.h>
#include <windows.h>
#include <stdlib.h>
#include <tchar.h>
#include <wchar.h>

typedef struct
{
    char d_name[EUROPA_WINDOWS_PATH_LENGTH_MAX];
}edirent;

typedef struct
{
    WIN32_FIND_DATAW data;
    HANDLE handle;
    edirent ent;
    uint drive_letter;
}DIR;

DIR *opendir(char *path)
{
    DIR *dir;
    if (path != NULL && path[0] != 0)
    {
        uint pos, i;
        WIN32_FIND_DATAW data;
        HANDLE handle;
        wchar_t unicode_path[EUROPA_WINDOWS_PATH_LENGTH_MAX];
        char *post_fix = "\\*.*";
        for (i = pos = 0; i < EUROPA_WINDOWS_PATH_LENGTH_MAX - 5 && path[pos] != 0; i++)
            unicode_path[i] = c_utf8_to_uint32(path, &pos);
        for (pos = 0; post_fix[pos] != 0; i++)
            unicode_path[i] = (wchar_t)post_fix[pos++];
        unicode_path[i] = 0;
        if ((handle = FindFirstFileW(unicode_path, &data)) != INVALID_HANDLE_VALUE)
        {
            dir = malloc(sizeof *dir);
            dir->handle = handle;
            dir->data = data;
            dir->ent.d_name[0] = 0;
            dir->drive_letter = -1;
            return dir;
        }
        return NULL;
    }
    else
    {
        dir = malloc(sizeof *dir);
        dir->handle = NULL;
        dir->ent.d_name[0] = 64;
        dir->ent.d_name[1] = ':';
        dir->ent.d_name[2] = '/';
        dir->ent.d_name[3] = 0;
        dir->drive_letter = GetLogicalDrives() * 2;
        return dir;
    }
}

void closedir(DIR *dir)
{
    if (dir->drive_letter == -1)
        FindClose(dir->handle);
    free(dir);
}

edirent *readdir(DIR *dir)
{
    if (dir->drive_letter == -1)
    {
        uint i, pos;
        if (FindNextFileW(dir->handle, &dir->data) != TRUE)
            return NULL;
        for (i = pos = 0; dir->data.cFileName[i] != 0 && pos < EUROPA_WINDOWS_PATH_LENGTH_MAX - 6; i++)
            pos += c_uint32_to_utf8(dir->data.cFileName[i], &dir->ent.d_name[pos]);
        dir->ent.d_name[pos] = 0;
        return &dir->ent;
    }
    else
    {
        uint letter, drive, i;
        drive = dir->drive_letter;
        letter = dir->ent.d_name[0] - 64;
        letter++;
        for (i = 0; i < letter; i++)
            drive /= 2;

        for (i = letter; i < 32; i++)
        {
            if (drive % 2 == 1)
            {
                dir->ent.d_name[0] = 64 + i;
                return &dir->ent;
            }
            drive /= 2;
        }
        return NULL;
    }
}

void *europa_path_watch(char *path, boolean subfolders)
{
    /* return FindFirstChangeNotificationW(path, subfolders, FILE_NOTIFY_CHANGE_FILE_NAME); */
}

    #define DIR_ROOT_PATH "/"
    #define DIR_HOME_PATH "."
#else

    #include <sys/types.h>
    #include <sys/dir.h>
    #include <dirent.h>
    #include <sys/statvfs.h>
    #include <unistd.h>
    #include <string.h>

    #define DIR_ROOT_PATH "/"
    #define DIR_HOME_PATH "."

#define edirent struct dirent
#endif

#ifdef _WIN32

void europa_get_pwd(char *output, uint32 output_size)
{
    uint length;
    char *last_slash;
    if (GetModuleFileName(NULL, output, output_size) == 0)
    {
        printf("Error getting module filename\n");
        return;
    }

    length = strlen(output);
    last_slash = &output[length - 1];

    while (*last_slash != '\\')
    {
        *last_slash = '\0';
        last_slash--;
    }
}

void europa_set_pwd(char *new_dir)
{
    if (SetCurrentDirectoryA(new_dir) == 0) {
        printf("Error settings current directory. Error code: %lu\n", GetLastError());
    }
}

#else

void europa_get_pwd(char *output, uint32 output_size)
{
    if (getcwd(output, output_size) == NULL)
    {
        printf("Error getting module filename\n");
        return;
    }
}

void europa_set_pwd(char *new_dir)
{
    if (chdir(new_dir) != 0) {
        perror("Error setting current directory");
    }
}

#endif

boolean europa_path_search(char *file, boolean partial, char *path, boolean folders, uint number, char *out_buffer, uint out_buffer_size)
{
    uint i, j, found = 0;
    DIR *d;
    edirent *ent;
    UNUSED(folders);
    d = opendir(path);
    if(d != NULL)
    {
        ent = readdir(d);
        if(ent != NULL)
        {
            out_buffer_size--;
            while(TRUE)
            {
                if(file == NULL)
                {
                    if(found == number)
                    {
                        for(i = 0; ent->d_name[i] != 0 && i < out_buffer_size; i++)
                            out_buffer[i] = ent->d_name[i];
                        out_buffer[i] = 0;
                        closedir(d);
                        return TRUE;
                    }
                    found++;
                }
                else
                {
                    if(partial)
                    {
                        for(i = 0; ent->d_name[i] != 0; i++)
                        {
                            if(ent->d_name[i] == file[0])
                            {
                                for(j = 0; ent->d_name[i + j] != 0 && file[j] != 0 && (ent->d_name[i + j] == file[j]); j++)
                                    ;
                                if(file[j] == 0)
                                {
                                    if(found == number)
                                    {
                                        for(i = 0; ent->d_name[i] != 0 && i < out_buffer_size; i++)
                                            out_buffer[i] = ent->d_name[i];
                                        out_buffer[i] = 0;
                                        closedir(d);
                                        return TRUE;
                                    }
                                    found++;
                                    break;
                                }
                            }
                        }
                    }
                    else
                    {
                        for(i = 0; ent->d_name[i] != 0 && ent->d_name[i] == file[i]; i++)
                            ;
                        if(ent->d_name[i] == file[i])
                        {
                            if(found == number)
                            {
                                for(i = 0; ent->d_name[i] != 0 && i < out_buffer_size; i++)
                                    out_buffer[i] = ent->d_name[i];
                                out_buffer[i] = 0;
                                closedir(d);
                                return TRUE;
                            }
                            found++;
                        }
                    }
                }
                ent = readdir(d);
                if(ent == NULL)
                    break;
            }
        }
        closedir(d);
    }
    return FALSE;
}

void *europa_path_dir_open(char *path)
{
    return opendir(path);
}

boolean europa_path_dir_next(DIR *d, char *file_name_buffer, uint buffer_size)
{
    uint i;
    edirent *ent;
    if(d != NULL)
    {
        ent = readdir(d);
        if(ent != NULL)
        {
            if(buffer_size != 0)
            {
                buffer_size--;
                for(i = 0; ent->d_name[i] != 0 && i < buffer_size; i++)
                    file_name_buffer[i] = ent->d_name[i];
                file_name_buffer[i] = 0;
            }
            return TRUE;
        }
    }
    return FALSE;
}

boolean europa_path_is_dir(char *path)
{
    DIR *d;
    d = opendir(path);
    if (d != NULL)
    {
        closedir(d);
        return TRUE;
    }
    return FALSE;
}

void europa_path_dir_close(DIR *d)
{
    closedir(d);
}

boolean europa_path_file_stats(char *file_name, size_t *size, uint64 *create_time, uint64 *modify_time)
{
    struct stat file_stats[80];
    if(!stat(file_name, file_stats))
    {
        if(size != NULL)
            *size = file_stats[0].st_size;
        if(create_time != NULL)
            *create_time = file_stats[0].st_ctime;
        if(modify_time != NULL)
            *modify_time = file_stats[0].st_mtime;
    }
    return TRUE;
}

#ifdef _WIN32

boolean europa_path_volume_stats(char *path, size_t *block_size, size_t *free_size, size_t *used_size, size_t *total_size)
{
    char drive[] = {'C', ':', '\\', 0};
    DWORD sectors_per_cluster, bytes_per_sector, free_clusters, total_clusters;
    uint64 bs;
    drive[0] = path[0];
    if(GetDiskFreeSpaceA(drive, &sectors_per_cluster, &bytes_per_sector, &free_clusters, &total_clusters))
    {
        bs = (uint64)sectors_per_cluster * (uint64)bytes_per_sector;
        if(block_size != NULL)
            *block_size = sectors_per_cluster;
        if(free_size != NULL)
            *free_size = bs * (uint64)free_clusters;
        if(used_size != NULL)
            *used_size = bs * (uint64)(total_clusters - free_clusters);
        if(total_size != NULL)
            *total_size = bs * (uint64)total_clusters;
/*		printf("%s %u %u %u", drive, (uint)bs,
                            (uint)(bs * (uint64)free_clusters / (uint64)(1024 * 1024)),
                            (uint)(bs * (uint64)total_clusters / (uint64)(1024 * 1024)));
 */     return TRUE;
    }
    if(block_size != NULL)
        *block_size = 4096;
    if(free_size != NULL)
        *free_size = 0;
    if(used_size != NULL)
        *used_size = 0;
    if(total_size != NULL)
        *total_size = 0;
    return FALSE;
}
#else

boolean europa_path_volume_stats(char *path, size_t *block_size, size_t *free_size, size_t *used_size, size_t *total_size)
{
    UNUSED(path); UNUSED(block_size); UNUSED(free_size); UNUSED(used_size); UNUSED(total_size);
/*	struct statvfs buf;
    if(0 == statvfs(path, &buf))
    {
 * block_size = buf.f_bsize;
 * free_size = buf.f_bavail * buf.f_bsize
 * used_size = buf.f_blocks * buf.f_frsize - buf.f_blocks * buf.f_bsize
 * total_size = buf.f_blocks * buf.f_frsize;
    }*/
    return FALSE;
}
#endif

void europa_path_test(void)
{
    char file[128];
    uint i;
    void *dir;

    dir = europa_path_dir_open("./");
    printf("dir Poninter %p\n", dir);
    if(dir != NULL)
    {
        for(i = 0; europa_path_dir_next(dir, file, 128); i++)
            printf("FILE:%s\n", file);
        europa_path_dir_close(dir);
    }
    exit(0);
}

#ifdef _WIN32

#define EUROPA_WINDOWS_PATH_LENGTH_MAX (256 * 128 - 1)


uint32 c_utf8_to_uint32(char *c, uint *pos); /* converts a string c at position pos to a unt32. pos will be modifyed to jump forward the number of bytes the character takes up. */
uint c_uint32_to_utf8(uint32 character, char *out); /*convets a 32 bit unicode charcter to UTF8. out param needs space for at least 6 8bit characters. returns the number of ytes used. */


char invalid_characters[] = {'>', '<', '\"', '|', '?', '*'};
/* CON, PRN, AUX, NUL, COM1, COM2, COM3, COM4, COM5, COM6, COM7, COM8, COM9, LPT1, LPT2, LPT3, LPT4, LPT5, LPT6, LPT7, LPT8, and LPT9
*/
int europa_path_rename(char *old_name, char *new_name)
{
    return MoveFileExA(old_name, new_name, MOVEFILE_REPLACE_EXISTING);
}

int europa_path_remove(char *path)
{
    wchar_t wide_name[EUROPA_WINDOWS_PATH_LENGTH_MAX];
    uint pos = 0, i;
    for(i = pos = 0; i < EUROPA_WINDOWS_PATH_LENGTH_MAX - 1 && path[pos] != 0; i++)
        wide_name[i] = c_utf8_to_uint32(path, &pos);
    wide_name[i] = 0;
    return _wremove(wide_name);
}

int europa_path_make_dir(char *path)
{
    wchar_t wide_name[EUROPA_WINDOWS_PATH_LENGTH_MAX];
    uint pos = 0, i;
    for(i = pos = 0; i < EUROPA_WINDOWS_PATH_LENGTH_MAX - 1 && path[pos] != 0; i++)
        wide_name[i] = c_utf8_to_uint32(path, &pos);
    wide_name[i] = 0;
    return _wmkdir(wide_name);
}

/*
   FILE *europa_path_open(char *path, char *mode)
   {
    FILE *f = NULL;
    uint i;
    char encoding_mode[32], *p, *encoding = ", ccs=UTF-8";
    p = encoding_mode;
    for(i = 0; i < 3 && mode[i] > ' '; i++)
 * p++ = mode[i];
    for(i = 0; encoding[i] != 0; i++)
 * p++ = encoding[i];
 * p++ = 0;
    if(0 == fopen_s(&f, path, encoding_mode))
        return f;
    return NULL;
   }*/
FILE *europa_path_open(char *path, char *mode)
{
    return fopen(path, mode);
}

uint8 *europa_path_load(char *path, size_t *size)
{
    char *buffer;
    uint allocation, i;
    FILE *f;
    f = fopen(path, "rb");
    if(f == NULL)
    {
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    allocation = ftell(f);
    if(allocation == 0)
    {
        fclose(f);
        return NULL;
    }
    rewind(f);
    buffer = malloc(allocation + 1);
    memset(buffer, 0, allocation + 1);
    fread(buffer, 1, allocation, f);
    fclose(f);
    buffer[allocation] = 0;
    if(size != NULL)
        *size = allocation;
    return buffer;
}
#else

int europa_path_rename(char *old_name, char *new_name)
{
    return rename(old_name, new_name);
}

int europa_path_remove(char *path)
{
    return remove(path);
}

int europa_path_make_dir(char *path)
{
    return mkdir(path, S_IRWXU);
}

FILE *europa_path_open(char *path, char *mode)
{
    return fopen(path, mode);
}

#endif

FILE *europa_project_root_fopen(const char *root_folder_name, const char *filename, char *perms)
{
    char root_dir[256] = "\0";
    char *dir_ptr = NULL;

    if (strlen(root_folder_name) > 32) return NULL;
    if (strlen(filename) > 128) return NULL;

    europa_get_pwd(root_dir, sizeof(root_dir));
    dir_ptr = strstr(root_dir, root_folder_name);
    if (NULL == dir_ptr)
        return NULL;
    dir_ptr += strlen(root_folder_name);
    if (*dir_ptr != '/')
        *(dir_ptr++) = '/';
    *dir_ptr = '\0';
    strcat(root_dir, filename);
    return europa_path_open(root_dir, perms);
}

int europa_pwd_to_root(const char *root_folder_name)
{
    char root_dir[256] = "\0";
    char *dir_ptr = NULL;

    europa_get_pwd(root_dir, sizeof(root_dir));
    dir_ptr = strstr(root_dir, root_folder_name);
    if (NULL == dir_ptr)
        return 1;
    dir_ptr += strlen(root_folder_name);
    if (*dir_ptr != '/')
        *(dir_ptr++) = '/';
    *dir_ptr = '\0';
    europa_set_pwd(root_dir);
    return 0;
}
