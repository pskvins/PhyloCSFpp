#pragma once

#include <cassert>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>

#include <string>

#define OUT_INFO    "\033[1;34m"
#define OUT_ERROR   "\033[1;31m"
#define OUT_RESET   "\033[0m"
#define OUT_DEL     "\x1b[K"
#define OUT_BOLD    "\033[1m"

void append(FILE * file_dest, const char path_src[])
{
    FILE *file_src = fopen(path_src, "rb");
    if (!file_src)
        abort();

    char buf[BUFSIZ];
    size_t n;
    while ((n = fread(buf, 1, sizeof buf, file_src)) > 0)
        if (fwrite(buf, 1, n, file_dest) != n)
            abort();

    if (ferror(file_src))
        abort();

    fclose(file_src);
}

bool create_directory(const std::string & path)
{
    struct stat st;

    if (stat(path.c_str(), &st) == -1)
    {
        mkdir(path.c_str(), 0764);
        return true;
    }
    return false;
}

void my_fprintf(FILE* f, const char *format_str, const float d)
{
    char buf[12];
    sprintf(buf, format_str, d);
    for (int8_t i = strlen(buf); i >= 0; --i)
    {
        if (buf[i] == '.')
        {
            buf[i + 1] = '0';
            break;
        }
        if (isdigit(buf[i]))
        {
            if (buf[i] != '0')
                break;
            else
                buf[i] = 0; // cut off the 0
        }
    }
    fprintf(f, "%s\n", buf);
}

int system_with_return(std::string cmd, bool verbose = true)
{
    if (verbose)
        printf(OUT_INFO "%s\n" OUT_RESET, cmd.c_str());
    else
        cmd  += " > /dev/null 2>&1";
    int ret = system(cmd.c_str());
    return WEXITSTATUS(ret);
}