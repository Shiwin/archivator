#include <sys/stat.h>
#include <sys/queue.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#define MAX_FILENAME_LENGTH 128
#define TEMP_BUFFER_SIZE 8096
#define ARCHIVE_PERMISSIONS 0777
typedef int dirent_handler(const struct dirent *current_dirent, const int fd, const char *relative_path);
typedef enum _file_type
{
    REGULAR_FILE,
    DIRECTORY
} file_type;

#pragma pack(push, 1)
struct file_meta_info
{
    char path_to_file[MAX_FILENAME_LENGTH]; // relative path to file
    long size;
};
#pragma pack(pop)

int archive_dirent(const struct dirent *c_d,
                   const int fd, const char *relative_path);
int archive_folder(const char *dirname, const int archive_fd);
int archive_folder(const char *dirname, const int archive_fd);
int dump_folder(const char *dirname);

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
            // construct relative path to file
            char d_name_path[MAX_FILENAME_LENGTH];
            strcpy(d_name_path, relative_path);
            strcat(d_name_path, "/");
            strcat(d_name_path, d_name);

            struct stat current_stat;
            int read_fd = open(d_name_path, O_RDONLY);

            if (read_fd < 0)
            {
                printf("Can't open file for reading: %s\n", d_name_path);
                result = !result;
                goto exit;
            }

            if (fstat(read_fd, &current_stat) != 0)
            {
                printf("Can't get stat for the file: %s. "
                       "Igrone it.\n",
                       d_name);
                result = !result;
                goto exit;
            }
            if (S_ISREG(current_stat.st_mode))
            {
                printf("%s\n", d_name_path);
                goto close_reading_file;
            }
            if (S_ISDIR(current_stat.st_mode))
            {
                dump_folder(d_name_path);
                goto exit;
            }
            printf("Unsupported file type of file: name=%s\n", d_name);
            result = !result;
        close_reading_file:
            if (close(read_fd))
            {
                printf("Closing file error: %s\n", d_name_path);
            }
            goto exit;
        }
    }
exit:
    return result;
}

int dirent_handler_metafunction(dirent_handler dh, const char *dirname, const int archive_fd)
{

    DIR *dir;
    if ((dir = opendir(dirname)) == NULL)
    {
        printf("Cannot open directory %s\n", dirname);
        return -1;
    }

    struct dirent *current_dirent;

    // iter over files
    while ((current_dirent = readdir(dir)) != NULL)
    {
        dh(current_dirent, archive_fd, dirname);
    }
    return 0;
}

int archive_folder(const char *dirname, const int archive_fd)
{
    return dirent_handler_metafunction(archive_dirent, dirname, archive_fd);
}

int dump_folder(const char *dirname)
{
    return dirent_handler_metafunction(dump_dirent, dirname, 0);
}

int dump_archive(const char *dirname, int fd)
{
    size_t file_meta_info_size = sizeof(struct file_meta_info);
    char buf[file_meta_info_size];
    while (file_meta_info_size == read(fd, buf, file_meta_info_size))
    {
        struct file_meta_info *mi = (struct file_meta_info *)buf;
        printf("%s %ld\n", mi->path_to_file, mi->size);
        lseek(fd, mi->size, SEEK_CUR);
    }
    return 0;
}

/*
Return values:
-2 - Critical: Error writing to the archive. May be stop program with error status(???)
1 - OK
0 - current file was not written to an archive
*/
int archive_dirent(const struct dirent *c_d,
                   const int fd, const char *relative_path)
{
    int result = 1;
    const char *d_name = c_d->d_name;
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
            // construct relative path to file
            char d_name_path[MAX_FILENAME_LENGTH];
            strcpy(d_name_path, relative_path);
            strcat(d_name_path, "/");
            strcat(d_name_path, d_name);

            struct stat current_stat;
            int read_fd = open(d_name_path, O_RDONLY);

            if (read_fd < 0)
            {
                printf("Can't open file for reading: %s\n", d_name_path);
                result = !result;
                goto exit;
            }

            if (fstat(read_fd, &current_stat) != 0)
            {
                printf("Can't get stat for the file: %s. "
                       "Igrone it.\n",
                       d_name);
                result = !result;
                goto exit;
            }
            if (S_ISREG(current_stat.st_mode))
            {
                struct file_meta_info m_i =
                    {
                        .size = current_stat.st_size,
                    };
                strcpy(m_i.path_to_file, d_name_path);

                ssize_t written_bytes = write(fd, &m_i, sizeof(m_i));
                char temp_buff[TEMP_BUFFER_SIZE];
                long total_written_file_bytes = 0;
                ssize_t read_bytes = read(read_fd, temp_buff, TEMP_BUFFER_SIZE);
                if (read_bytes < 0)
                {
                    printf("Reading file error: %s\n", d_name);
                    result = !result;
                    goto close_reading_file;
                }
                while (read_bytes > 0)
                {
                    if (read_bytes < 0)
                    {
                        printf("Reading file error: %s\n", d_name);
                        result = !result;
                        goto close_reading_file;
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
                if (total_written_file_bytes != m_i.size)
                {
                    printf("Error during writing %s to the archive\n", d_name);
                    result = -2;
                }
                goto close_reading_file;
            }
            if (S_ISDIR(current_stat.st_mode))
            {
                archive_folder(d_name_path, fd);
                goto exit;
            }
            printf("Unsupported file type of file: name=%s\n", d_name);
            result = !result;
        close_reading_file:
            if (close(read_fd))
            {
                printf("Closing file error: %s\n", d_name_path);
            }
            goto exit;
        }
    }
exit:
    return result;
}

int main(int argc, char const *argv[])
{
    if (!(argc == 2 || argc == 3 || argc == 4))
    {
        printf("Invalid number of arguments: %d\n"
               "Usage example: ./archivator <dir>\n"
               "               ./archivator <dir> <archive_name>\n"
               "               ./archivator <archive_name> -C <extract to> \n",
               argc);
        return -1;
    }

    {
        const char *dirname = argv[1];
        const char *archive_name = argv[2];

        if (argc == 2)
        {
            dump_folder(dirname);
        }

        if (argc == 3)
        {
            int fd = open(archive_name,
                          O_WRONLY | O_CREAT | O_EXCL, ARCHIVE_PERMISSIONS);
            if (fd < 0)
            {
                printf("Can't create archive file: %s\n", strerror(errno));
                return -1;
            }
            archive_folder(dirname, fd);
            if (close(fd))
            {
                printf("Error during writing an archive."
                       "Can't close an archive file\n");
            }
        }
    }

    if (argc == 4)
    {
        const char *dirname = argv[3];
        const char *archive_name = argv[1];

        int fd = open(archive_name, O_RDONLY, ARCHIVE_PERMISSIONS);
        if (fd < 0)
        {
            printf("Can't open archive file: %s\n", strerror(errno));
            return -1;
        }

        dump_archive(dirname, fd);

        close(fd);
    }

    return 0;
}
