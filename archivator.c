#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>

typedef int dirent_handler(const struct dirent *current_dirent);

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
int dump_dirent(const struct dirent *current_dirent)
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

int archive_dirent(const struct dirent *current_dirent)
{
    int result = 1;
    const char *d_name = current_dirent->d_name;
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
                goto exit;
            }
            if (S_ISDIR(current_stat.st_mode))
            {
                printf("%s is a directory\n", d_name);
                goto exit;
            }
            printf("Unsupported file type of file: name=%s st_mode=0x%h\n", d_name);
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

    struct dirent *current_dirent;

    // iter over files
    while ((current_dirent = readdir(dir)) != NULL)
    {
        dh(current_dirent);
    }

    return 0;
}
