#include "minigit.h"
#include<errno.h>

Commit *commitHead = NULL;
Commit *commitTail = NULL;
Branch *branchHead = NULL;
Branch *branchTail = NULL;
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

    #ifdef _WIN32
        // windows version with _finddata_t
        struct _finddata_t fileInfo;
        intptr_t handle;

        char searchPath[500];
        sprintf(searchPath, "%s/*", src);

        handle = _findfirst(searchPath, &fileInfo);
        if (handle == -1) {
            printf("src directory not found\n");
            return;
        }

        do {
            if (strcmp(fileInfo.name, ".") == 0 || strcmp(fileInfo.name, "..") == 0)
                continue;
            if (fileInfo.attrib & _A_SUBDIR)
                continue;

            char srcPath[500], dstPath[500];
            sprintf(srcPath, "%s/%s", src, fileInfo.name);
            sprintf(dstPath, "%s/%s", dst, fileInfo.name);
            copy_file(srcPath, dstPath);

        } while (_findnext(handle, &fileInfo) == 0);

        _findclose(handle);


    #else
        //linux version with dirent.h
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
    #endif
}

void cmd_init() {
    // try to create .minigit/ using MKDIR
    // if it returns -1, print "already initialized" and return

    // create .minigit/snapshots/

    // create and close empty commits.dat
    // create and close empty branches.dat
    // create and close empty staging.dat
    // create and close empty undo.dat

    // open HEAD for writing, write "main", close it

    // print "initialized empty MiniGit repository"



    // Create the main .minigit directory
    int retval = MKDIR(".minigit");

    if (retval == -1) {
        printf("MiniGit repository already initialized!\n");
        return;
    }

    // Create snapshots directory
    if (MKDIR(".minigit/snapshots") == -1) {
        printf("Error creating snapshots directory!\n");
        return;
    }

    // Create required metadata files
    const char *files[] = {
        ".minigit/commits.dat",
        ".minigit/branches.dat",
        ".minigit/staging.dat",
        ".minigit/undo.dat"
    };

    FILE *fp;

    for (int i = 0; i < 4; i++) {
        fp = fopen(files[i], "w");

        if (fp == NULL) {
            printf("Error creating %s\n", files[i]);
            return;
        }

        fclose(fp);
    }

    // Initialize HEAD with main branch
    fp = fopen(".minigit/HEAD", "w");

    if (fp == NULL) {
        printf("Error creating HEAD file!\n");
        return;
    }

    fprintf(fp, "main\n");
    fclose(fp);

    // Initialize global commit variables
    commitHead = NULL;
    commitTail = NULL;
    commitCount = 0;

    // Initialize default main branch
    Branch *newBranch = (Branch *)calloc(1, sizeof(Branch));

    if (newBranch == NULL) {
        printf("Memory allocation failed!\n");
        return;
    }

    strcpy(newBranch->name, "main");
    newBranch->headId = -1;
    newBranch->next = NULL;

    branchHead = newBranch;

    strcpy(currentBranch, "main");

    // Save initial branch information
    save_branches();

    printf("Initialized empty MiniGit repository!\n");
}


void load_commits() {
    // open .minigit/commits.dat for reading
    // if file doesn't exist or is empty, just return

    // loop: read one line at a time with fgets

        // parse the line: id, parentId, timestamp, message
        // allocate a new Commit node with calloc
        // set node fields: id, parentId, timestamp, message
        // set next and parent pointers to NULL (calloc already does this)

        // if commitHead is NULL, this is the first node
            // commitHead = commitTail = newNode
        // else append to tail
            // commitTail->next = newNode
            // commitTail = newNode

        // increment commitCount

    // close the file
    int id, parentId;
    long timestamp;
    char message[100]; 
    FILE *pFile = fopen(".minigit/commits.dat", "r");
    if(pFile == NULL){
        return;
    }
    
    while (fscanf(pFile, "%d|%d|%ld|%[^\n]", &id, &parentId, &timestamp, message) == 4)
    {
        Commit *newNode = (Commit*)calloc(1, sizeof(Commit));
        newNode->id = id;
        newNode->parentId = parentId;
        newNode->timestamp = timestamp;
        strcpy(newNode->message, message);
        
        newNode->parent = newNode->next = NULL;

        if (commitHead == NULL)
        {
            commitHead = commitTail = newNode;
        }
        else{
            commitTail->next = newNode;
            commitTail = newNode;
        }
        commitCount++;   
    }
    // second pass to reconnect parent pointers
    Commit *curr = commitHead;
    while (curr != NULL) {
        if (curr->parentId != 0) {
            // find the node with id == parentId
            Commit *search = commitHead;
            while (search != NULL) {
                if (search->id == curr->parentId) {
                    curr->parent = search;
                    break;
                }
                search = search->next;
            }
        }
        curr = curr->next;
    }
    
    fclose(pFile);

}

void save_commits() {
    // open .minigit/commits.dat for writing ("w")
    // if file open fails, print error and return

    // loop: walk the linked list from commitHead to NULL
        // for each node, write: id|parentId|timestampstamp|message\n

    // close the file
    FILE *pFile = fopen(".minigit/commits.dat", "w");
    if(pFile == NULL){
        printf("Error opening the commits file... please check the path");
        return;
    }
    Commit *currentNode = commitHead;
    while (currentNode != NULL)
    {
        fprintf(pFile, "%d|%d|%ld|%s\n", currentNode->id,currentNode->parentId,currentNode->timestamp,currentNode->message);
        currentNode = currentNode->next;
    }
    fclose(pFile);
}

void load_branches() {
    // open .minigit/branches.dat for reading
    // if file doesn't exist or is empty, just return

    // loop: read one line at a time
        // parse: name, headId
        // allocate new Branch node with calloc
        // set node fields: name, headId
        // append to branchHead list

    // close the file

    FILE *pFile = fopen(".minigit/branches.dat", "r");
    if(pFile == NULL){
        return;
    }

    int headId;
    char name[30]; 
    
    while (fscanf(pFile, "%[^|]|%d", name, &headId) == 2)
    {
        Branch *newNode= (Branch*)calloc(1, sizeof(Branch));
        newNode->headId = headId;
        strcpy(newNode->name, name);
        newNode->next = NULL;

        if(branchHead == NULL){
            branchHead = branchTail= newNode;
        }
        else{
            branchTail->next = newNode;
            branchTail = newNode;
        }
    }
    
    fclose(pFile);    
}

void save_branches() {
    // open .minigit/branches.dat for writing
    // if file open fails, print error and return

    // loop: walk branch list from branchHead to NULL
        // write: name|headId\n

    // close the file

    FILE *pFile = fopen(".minigit/branches.dat", "w");
    if(pFile == NULL){
        printf("Error writing the branches file... please check the path");
        return;
    }
    Branch *currentNode = branchHead;
    while (currentNode != NULL)
    {
        fprintf(pFile, "%s|%d\n", currentNode->name, currentNode->headId);
        currentNode = currentNode->next;
    }
    fclose(pFile);
}

void load_head() {
    // open .minigit/HEAD for reading
    // if file doesn't exist, just return
    // read the branch name into currentBranch
    // close the file

    FILE *pFile = fopen(".minigit/HEAD", "r");
    if(pFile == NULL){
        return;
    }

    fscanf(pFile, "%s", currentBranch);
    fclose(pFile);
}

void save_head() {
    // open .minigit/HEAD for writing
    // if file open fails, print error and return
    // write currentBranch to the file
    // close the file
    FILE *pFile = fopen(".minigit/HEAD", "w");
    if(pFile == NULL){
        return;
    }

    fprintf(pFile, "%s", currentBranch);
    fclose(pFile);
}

void push_undo(const char *branch, int previousHeadId) {
    // open .minigit/undo.dat in append mode ("a")
    // if file open fails, print error and return
    // write: branch|previousHeadId\n
    // close the file
    FILE *pFile = fopen(".minigit/undo.dat", "a");
    if(pFile == NULL){
        return;
    }

    fprintf(pFile, "%s|%d\n",branch, previousHeadId);
    fclose(pFile);
}

int pop_undo(char *branchOut) {
    // open .minigit/undo.dat for reading
    // if file open fails or is empty, return -1
    // if no lines, return -1

    // parse the last line: branch|previousHeadId
    // store branch name into branchOut
    // store previousHeadId

    // rewrite the file with all lines except the last one

    // return previousHeadId

    typedef struct commitNodes 
    {
        char branchName[30];
        int previousHeadId;
        struct commitNodes *next;
    }commitNodes;

    commitNodes *headCommit = NULL;
    commitNodes *tailCommit = NULL;
    commitNodes *temp;
    char branchName[30];
    int previousHeadId;

    FILE *pFile = fopen(".minigit/undo.dat", "r");
    if(pFile == NULL){
        return -1;
    }
    
    while (fscanf(pFile, "%[^|]|%d", branchName, &previousHeadId) == 2){
        commitNodes *newNode = (commitNodes*)calloc(1, sizeof(commitNodes));
        strcpy(newNode->branchName, branchName);
        newNode->previousHeadId = previousHeadId;
        newNode->next = NULL;

        if(headCommit == NULL){
            headCommit = tailCommit = newNode;
        }
        else{
            tailCommit->next = newNode;
            tailCommit = newNode;
        }
    }

    if(headCommit == NULL){
        fclose(pFile);
        return -1;
    }
    fclose(pFile);

    strcpy(branchOut, tailCommit->branchName);
    int returnValue = tailCommit->previousHeadId;
 
    pFile = fopen(".minigit/undo.dat", "w");
    temp = headCommit;
    while(temp != tailCommit){
        fprintf(pFile, "%s|%d\n",temp->branchName,temp->previousHeadId);
        temp = temp->next;
    }

    fclose(pFile);

    temp = headCommit;
    while (temp != NULL) {
        commitNodes *next = temp->next;
        free(temp);
        temp = next;
    }
    
    return returnValue;
}

void cmd_add(const char *file) {
    // check if the file actually exists (try fopen for reading)
    // if it doesn't exist, print error and return

    // open .minigit/staging.dat in append mode
    // write the filename on a new line
    // close the file

    // print "added <file>"
}

void cmd_commit(const char *msg) {
    // load staging.dat — read all staged filenames into an array
    // if staging is empty, print "nothing to commit" and return

    // determine new commit id: commitCount + 1 (or max id + 1)
    // find the current branch's head commit id (walk branchHead list)

    // create snapshot folder: .minigit/snapshots/<newId>/
    // if previous commit exists, copy_dir from previous snapshot into new snapshot
    // then copy each staged file into the new snapshot folder

    // create new Commit node, set id, parentId, timestamp (time(NULL)), message
    // append to commit linked list
    // save_commits()

    // update current branch's headId to newId
    // save_branches()

    // push_undo(currentBranch, previousHeadId)

    // clear staging.dat (open in "w" mode and close immediately)

    // print "committed: <msg>"
}

int main() {
    cmd_init();
    load_commits();
    printf("commitCount: %d\n", commitCount);
    save_commits();
    printf("done\n");
    return 0;
}