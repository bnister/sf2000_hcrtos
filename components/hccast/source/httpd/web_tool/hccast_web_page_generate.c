#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../hccast_web_package.h"

#define MAX_PACKAGE_LEN     (1 * 1024 * 1024)

static int httpd_generate_header_file(unsigned char* path, unsigned char *buf, unsigned int size)
{
    int fd_bin = -1;
    int i;
    unsigned char str_start[256] = {0};
    unsigned char str_txt[256] = {0};
    unsigned char str_tmp[8] = {0};
    unsigned char *str_end = "\n};";
    unsigned char web_output_path[2048] = {0};

    sprintf(web_output_path, "%s/hccast_web_page.h", path);

    printf("web_page target output path: %s\n", web_output_path);
    fd_bin = open(web_output_path, O_CREAT | O_TRUNC | O_RDWR, 0666);
    if (fd_bin < 0)
    {
        perror("open1");
        return -1;
    }

    //write start string.
    sprintf(str_start, "static unsigned char g_web_page[%d] = {\n", size);
    write(fd_bin, str_start, strlen(str_start));

    //write data.
    for (i = 0; i < size; i ++)
    {
        memset(str_tmp, 0, 8);
        if (0 == (i % 16))
        {
            if (strlen(str_txt))
            {
                write(fd_bin, str_txt, strlen(str_txt));
                memset(str_txt, 0, 256);
            }
            if (i)
            {
                sprintf(str_tmp, "\n\t0x%.2x,", buf[i]);
            }
            else
            {
                sprintf(str_tmp, "\t0x%.2x,", buf[i]);
            }
            strcat(str_txt, str_tmp);
        }
        else
        {
            sprintf(str_tmp, " 0x%.2x,", buf[i]);
            strcat(str_txt, str_tmp);
        }
    }
    if (strlen(str_txt))
    {
        write(fd_bin, str_txt, strlen(str_txt));
    }

    //write end string.
    write(fd_bin, str_end, strlen(str_end));

    close(fd_bin);
}
//usage: ./web_generate ./web  <targer_out_path>
int main(int argc, char *argv[])
{
    struct dirent *entry = NULL;
    DIR *dir = NULL;
    unsigned char *buf = NULL;
    unsigned int total_size = 0;
    hccast_httpd_web_package_header_st *header = NULL;
    unsigned char path[1024] = {0};
    int fd_tmp;
    struct stat stat_buf;

    if (argc < 3)
    {
        return -1;
    }

    dir = opendir(argv[1]);
    if (!dir)
    {
        return -1;
    }

    buf = (unsigned char *)malloc(MAX_PACKAGE_LEN);
    if (!buf)
    {
        return -1;
    }
    memset(buf, 0, MAX_PACKAGE_LEN);
    header = (hccast_httpd_web_package_header_st *)buf;
    total_size = HTTPD_WEB_PACKAGE_HEADER_LEN;

    while (entry = readdir(dir))
    {
        if (DT_REG == entry->d_type)
        {
            strncpy(header->file_list[header->file_num].name, entry->d_name, HTTPD_WEB_FILE_NAME_MAX_LEN);
            memset(path, 0, 1024);
            snprintf(path, 1024, "%s/%s", argv[1], entry->d_name);

            fd_tmp = open(path, O_RDONLY);
            if (fd_tmp < 0)
            {
                perror("open2");
            }
            fstat(fd_tmp, &stat_buf);
            header->file_list[header->file_num].size = (unsigned int)stat_buf.st_size;
            header->file_list[header->file_num].offset = total_size;
            printf("file: %s, len: %d\n", entry->d_name, header->file_list[header->file_num].size);
            read(fd_tmp, buf + total_size, header->file_list[header->file_num].size);
            close(fd_tmp);

            total_size += header->file_list[header->file_num].size;
            header->file_num ++;
        }
    }

    printf("Package size: %d\n", total_size);
    httpd_generate_header_file(argv[2], buf, total_size);

    free(buf);
    closedir(dir);

    return 0;
}
