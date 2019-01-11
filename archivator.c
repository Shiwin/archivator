#include <sys/stat.h>
#include <sys/queue.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#define MAX_FILENAME_LENGTH 1024
#define TEMP_BUFFER_SIZE 8096
#define ARCHIVE_PERMISSIONS 0777
typedef int dirent_handler(const struct dirent *current_dirent, const int fd, const char *relative_path);
typedef enum _file_type
{
    REGULAR_FILE,
    DIRECTORY
} file_type;

#pragma pack(push, 1)
struct archive_meta_info
{
    int number_of_dirs;
    int number_of_regular;
};

struct file_meta_info
{
    const char *path_to_file; // relative path to file
    const char *filename;
    file_type type; // regular file or a directory
    long size;      // only for regular
};
#pragma pack(pop)

/*
d_name == "." or ".." => 0 (false)
else => 1 (true)
*/
int validate_name(const char *d_name)
{
    return strcmp(".", d_name) != 0 && strcmp("..", d_name) != 0;
}
/*
    Just print the name of file which is represented
    by the input parameter
*/
int dump_dirent(const struct dirent *current_dirent, const int fd, const char *relative_path)
{
    int result = 1;
    const char *d_name = current_dirent->d_name;
    if (!d_name)
    {
        printf("dump_dirent: the name is empty :(\n");
        result = !result;
    }
    else
    {
        if (validate_name(d_name))
        {
            printf("%s\n", d_name);
        }
    }
    return result;
}

/*
Return values:
-2 - Critical: Error writing to the archive. May be stop program with error status(???)
1 - OK
0 - current file was not written to an archive
*/
int archive_dirent(const struct dirent *current_dirent, const int fd, const char *relative_path)
{
    int result = 1;
    const char *d_name = current_dirent->d_name;
    if (fd < 0)
    {
        printf("Error: can't write to archive\n");
        result = !result;
        goto exit;
    }
    if (d_name)
    {
        if (validate_name(d_name))
        {
            struct stat current_stat;
            if (stat(d_name, &current_stat))
            {
                printf("Can't get stat for the file: %s. "
                       "Igrone it.\n",
                       d_name);
                result = !result;
                goto exit;
            }
            if (S_ISREG(current_stat.st_mode))
            {
                printf("%s is a regular file\n", d_name);
                struct file_meta_info m_i =
                    {
                        .type = REGULAR_FILE,
                        .filename = d_name,
                        .path_to_file = relative_path,
                        .size = current_stat.st_size,
                    };

                ssize_t written_bytes = write(fd, &m_i, sizeof(m_i));
                char temp_buff[TEMP_BUFFER_SIZE];
                int read_fd = open(d_name, O_RDONLY);
                if (read_fd < 0)
                {
                    printf("Can't open file for reading: %s\n", d_name);
                    result = !result;
                }
                long total_written_file_bytes = 0;
                ssize_t read_bytes = read(read_fd, temp_buff, TEMP_BUFFER_SIZE);
                while (read_bytes > 0)
                {
                    if (read_bytes < 0)
                    {
                        printf("Reading file error: %s\n", d_name);
                    }
                    written_bytes = write(fd, temp_buff, read_bytes);
                    if (written_bytes != read_bytes)
                    {
                        printf("Error during writing %s to the archive\n", d_name);
                        result = -2;
                        goto close_reading_file;
                    }
                    total_written_file_bytes += written_bytes;
                    read_bytes = read(read_fd, temp_buff, TEMP_BUFFER_SIZE);
                }
                if (read_bytes < 0)
                {
                    printf("Reading file error: %s\n", d_name);
                    result = !result;
                }
                if (total_written_file_bytes != m_i.size)
                {
                    printf("Error during writing %s to the archive\n", d_name);
                    result = -2;
                }
            close_reading_file:
                if (close(read_fd))
                {
                    printf("Closing file error: %s\n", d_name);
                }
                goto exit;
            }
            if (S_ISDIR(current_stat.st_mode))
            {
                printf("%s is a directory\n", d_name);
                goto exit;
            }
            printf("Unsupported file type of file: name=%s\n", d_name);
            result = !result;
        }
    }
exit:
    return result;
}

int main(int argc, char const *argv[])
{
    if (!(argc == 2 || argc == 3))
    {
        printf("Invalid number of arguments: %d\n", argc);
        return -1;
    }

    const char *dirname = argv[1];

    DIR *dir;
    if ((dir = opendir(dirname)) == NULL)
    {
        printf("Cannot open directory %s\n", dirname);
        return -1;
    }

    dirent_handler *dh = archive_dirent;
    int fd = open(dirname,
                  O_WRONLY | O_CREAT | O_EXCL, ARCHIVE_PERMISSIONS);
    if (fd < 0)
    {
        printf("Can't create archive file: %s\n", strerror(errno));
        return -1;
    }

    struct dirent *current_dirent;

    // iter over files
    while ((current_dirent = readdir(dir)) != NULL)
    {
        dh(current_dirent, fd, dirname);
    }

    if (close(fd))
    {
        printf("Error during writing an archive."
               "Can't close an archive file\n");
    }
    return 0;
}
