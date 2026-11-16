#include "minigit.h"

Commit *commitHead = NULL;
Commit *commitTail = NULL;
Branch *branchHead = NULL;
char currentBranch[30] = "main";
int commitCount = 0;

int copy_file(const char *src, const char *dst) {
    // open src for reading in binary mode
    // open dst for writing in binary mode
    // if either fails, return 0
    // loop: read chunk into buffer, write buffer to dst
    // close both files
    // return 1
    FILE *pFile_src = fopen(src, "rb");
    if (pFile_src == NULL) {
        printf("error opening source file\n");
        return 0;
    }

    FILE *pFile_dst = fopen(dst, "wb");
    if (pFile_dst == NULL) {
        printf("error opening destination file\n");
        fclose(pFile_src);
        return 0;
    }

    char buffer[4096];
    size_t n;
    while ((n = fread(buffer, 1, sizeof(buffer), pFile_src)) > 0) {
        fwrite(buffer, 1, n, pFile_dst);
    }

    fclose(pFile_src);
    fclose(pFile_dst);
    return 1;
}

void copy_dir(const char *src, const char *dst) {
    // open the source folder for reading
    // if folder doesn't exist or is empty, just return
    // loop through every file in the folder
        // skip "." and ".." (these are not real files)
        // build full source path: src/filename
        // build full destination path: dst/filename
        // call copy_file(srcPath, dstPath)
    // close the folder

    DIR *dir = opendir(src);
    if (dir == NULL) {
        printf("src directory not found\n");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char srcPath[500], dstPath[500];
        sprintf(srcPath, "%s/%s", src, entry->d_name);
        sprintf(dstPath, "%s/%s", dst, entry->d_name);
        copy_file(srcPath, dstPath);
    }

    closedir(dir);
}

int main() {
    // create a test folder with a file first manually
    // then call copy_dir
    copy_dir(".minigit/snapshots/1", ".minigit/snapshots/2");
    printf("done\n");
    return 0;
}