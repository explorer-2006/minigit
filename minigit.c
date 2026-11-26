#include "minigit.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <ctype.h>

Commit *commitHead = NULL;
Commit *commitTail = NULL;
Branch *branchHead = NULL;
Branch *branchTail = NULL;
char currentBranch[30] = "main";
int commitCount = 0;
int jsonOutput = 0;
char repoPath[500] = ".";

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
    char path[1024];
    sprintf(path, "%s/.minigit", repoPath);
    int retval = MKDIR(path);

    if (retval == -1) {
        printf("MiniGit repository already initialized!\n");
        return;
    }

    // Create snapshots directory
    sprintf(path, "%s/.minigit/snapshots", repoPath);
    if (MKDIR(path) == -1) {
        printf("Error creating snapshots directory!\n");
        return;
    }

    // Create required metadata files
    const char *files[] = {
        "commits.dat",
        "branches.dat",
        "staging.dat",
        "undo.dat"
    };

    FILE *fp;

    for (int i = 0; i < 4; i++) {
        sprintf(path, "%s/.minigit/%s", repoPath, files[i]);
        fp = fopen(path, "w");

        if (fp == NULL) {
            printf("Error creating %s\n", path);
            return;
        }

        fclose(fp);
    }

    // Initialize HEAD with main branch
    sprintf(path, "%s/.minigit/HEAD", repoPath);
    fp = fopen(path, "w");

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
    char path[1024];
    sprintf(path, "%s/.minigit/commits.dat", repoPath);
    FILE *pFile = fopen(path, "r");
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
        if (curr->parentId != -1) {
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
    char path[1024];
    sprintf(path, "%s/.minigit/commits.dat", repoPath);
    FILE *pFile = fopen(path, "w");
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

    char path[1024];
    sprintf(path, "%s/.minigit/branches.dat", repoPath);
    FILE *pFile = fopen(path, "r");
    if(pFile == NULL){
        return;
    }

    int headId;
    char name[30]; 
    
    while (fscanf(pFile, " %29[^|]|%d", name, &headId) == 2)
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

    char path[1024];
    sprintf(path, "%s/.minigit/branches.dat", repoPath);
    FILE *pFile = fopen(path, "w");
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
    char path[1024];                                        // buffer for HEAD file path
    sprintf(path, "%s/.minigit/HEAD", repoPath);            // construct path to HEAD
    FILE *pFile = fopen(path, "r");                         // open HEAD file for reading
    if (pFile == NULL) {                                    // if file doesn't exist or cannot be read
        return;                                             // exit gracefully without changing currentBranch
    }

    fscanf(pFile, "%s", currentBranch);                     // read active branch name into global currentBranch
    fclose(pFile);                                          // close HEAD file
}

void save_head() {
    char path[1024];                                        // buffer for HEAD file path
    sprintf(path, "%s/.minigit/HEAD", repoPath);            // construct path to HEAD
    FILE *pFile = fopen(path, "w");                         // open HEAD file for writing (overwrite)
    if (pFile == NULL) {                                    // check if file open failed
        return;                                             // return on failure
    }

    fprintf(pFile, "%s", currentBranch);                    // persist currentBranch name into HEAD file
    fclose(pFile);                                          // close HEAD file
}

void push_undo(const char *branch, int previousHeadId) {
    char path[1024];                                        // buffer for undo.dat file path
    sprintf(path, "%s/.minigit/undo.dat", repoPath);        // build path to undo stack file
    FILE *pFile = fopen(path, "a");                         // open in append mode to push to stack
    if (pFile == NULL) {                                    // handle file open failure
        return;                                             // return if file cannot be opened
    }

    fprintf(pFile, "%s|%d\n", branch, previousHeadId);      // push branch and previous HEAD ID onto stack
    fclose(pFile);                                          // close undo file
}

int pop_undo(char *branchOut) {
    typedef struct commitNodes {                            // temporary linked list node for stack entries
        char branchName[30];                                // stores branch name of undo frame
        int previousHeadId;                                 // stores target head commit ID to restore
        struct commitNodes *next;                           // pointer to next stack entry
    } commitNodes;

    commitNodes *headCommit = NULL;                         // head of temporary undo list
    commitNodes *tailCommit = NULL;                         // tail of temporary undo list (top of stack)
    commitNodes *temp;                                      // iterator pointer
    char line[256];                                         // line buffer for reading undo.dat

    char path[1024];                                        // buffer for file path
    sprintf(path, "%s/.minigit/undo.dat", repoPath);        // construct path to undo.dat
    FILE *pFile = fopen(path, "r");                         // open undo stack file for reading
    if (pFile == NULL) {                                    // if file doesn't exist
        return -1;                                          // return -1 (stack is empty)
    }
    
    while (fgets(line, sizeof(line), pFile)) {              // read undo.dat line by line
        line[strcspn(line, "\r\n")] = '\0';                 // strip trailing newline and carriage return
        if (line[0] == '\0') continue;                      // skip empty lines safely

        char *sep = strchr(line, '|');                      // find delimiter separating branch and headId
        if (!sep) continue;                                 // skip malformed lines
        *sep = '\0';                                        // split line into branch string and headId string

        commitNodes *newNode = (commitNodes*)calloc(1, sizeof(commitNodes)); // allocate memory for new frame
        if (!newNode) continue;                             // safety check on allocation
        strncpy(newNode->branchName, line, sizeof(newNode->branchName) - 1); // store branch name
        newNode->previousHeadId = atoi(sep + 1);             // convert and store previousHeadId
        newNode->next = NULL;                               // initialize next pointer to NULL

        if (headCommit == NULL) {                           // if list is currently empty
            headCommit = tailCommit = newNode;              // first node becomes both head and tail
        } else {
            tailCommit->next = newNode;                     // append new node at the tail
            tailCommit = newNode;                           // update tail pointer to new node
        }
    }
    fclose(pFile);                                          // close read file handle

    if (headCommit == NULL) {                               // if no valid entries were found in stack
        return -1;                                          // return -1 indicating empty undo stack
    }

    strcpy(branchOut, tailCommit->branchName);               // copy popped branch name into output buffer
    int returnValue = tailCommit->previousHeadId;           // save popped previousHeadId to return

    pFile = fopen(path, "w");                               // open undo.dat in write mode to rewrite stack
    if (pFile != NULL) {
        temp = headCommit;                                  // start from base of stack
        while (temp != tailCommit) {                        // write all frames EXCEPT the last (popped) frame
            fprintf(pFile, "%s|%d\n", temp->branchName, temp->previousHeadId); // persist frame to file
            temp = temp->next;                              // advance to next node
        }
        fclose(pFile);                                      // close written file handle
    }

    temp = headCommit;                                      // free the temporary linked list memory
    while (temp != NULL) {
        commitNodes *next = temp->next;                     // remember next node
        free(temp);                                         // free current node
        temp = next;                                        // move forward
    }
    
    return returnValue;                                     // return popped previousHeadId
}

void cmd_undo() {
    load_branches();                                        // load all branch heads into linked list
    load_head();                                            // load active branch name into currentBranch

    char branchName[30];                                    // buffer to receive popped branch name
    int previousHeadId = pop_undo(branchName);              // pop top frame off undo stack

    if (previousHeadId == -1) {                             // check if stack was empty
        if (jsonOutput) {
            printf("{\"error\": \"Nothing to undo.\"}\n");  // JSON output on empty stack
        } else {
            printf("Nothing to undo.\n");                   // terminal output on empty stack
        }
        return;                                             // exit function
    }

    if (strcmp(branchName, currentBranch) != 0) {           // verify popped frame matches current active branch
        push_undo(branchName, previousHeadId);              // preserve stack frame so it is not lost

        if (jsonOutput) {
            printf("{\"error\": \"Last undo entry is for branch '%s'. Switch to '%s' to undo.\"}\n", branchName, branchName); // JSON error
        } else {
            printf("Last undo entry is for branch '%s'. Switch to '%s' to undo.\n", branchName, branchName); // guidance
        }
        return;                                             // exit function
    }

    Branch *temp = branchHead;                              // pointer to search for branch in linked list
    int found = 0;                                          // flag tracking whether branch was located

    while (temp != NULL) {                                  // traverse branch linked list
        if (strcmp(temp->name, branchName) == 0) {          // match branch by name
            found = 1;                                      // mark found
            break;                                          // stop searching
        }
        temp = temp->next;                                  // advance to next branch node
    }

    if (!found) {                                           // handle unexpected missing branch
        if (jsonOutput) {
            printf("{\"error\": \"branch not found.\"}\n"); // JSON error
        } else {
            printf("Error: branch not found.\n");           // terminal error
        }
        return;                                             // exit function
    }

    temp->headId = previousHeadId;                          // update branch HEAD pointer to previous commit ID
    save_branches();                                        // persist updated branch HEAD to branches.dat

    if (previousHeadId != -1) {                             // if previous commit exists (not root initialization)
        char snapshotPath[1024];                            // buffer for snapshot directory path
        sprintf(snapshotPath, "%s/.minigit/snapshots/%d", repoPath, previousHeadId); // build snapshot path
        copy_dir(snapshotPath, repoPath);                   // restore snapshot files back into working directory
    }

    if (jsonOutput) {
        printf("{\"success\": true, \"headId\": %d}\n", previousHeadId); // JSON confirmation
    } else {
        printf("Undo successful, now at commit %d.\n", previousHeadId);   // terminal success message
    }
}

void cmd_revert(int id, int force) {
    load_commits();                                         // load all commits into commit linked list
    load_branches();                                        // load branches into branch linked list
    load_head();                                            // load active branch from HEAD file
    int found = 0;                                          // flag tracking if target commit exists
    char snapshotPath[1024];                                // buffer for target snapshot path

    Commit *temp = commitHead;                              // start traversal from head of commit list
    while (temp != NULL) {                                  // search for target commit ID in list
        if (temp->id == id) {                               // check if current commit matches target ID
            found = 1;                                      // target commit found
            break;                                          // exit search loop
        }
        temp = temp->next;                                  // move to next commit node
    }

    if (!found) {                                           // if target commit was not found in history
        printf("commit <%d> not found", id);                // print error message
        return;                                             // abort revert
    }

    if (force == 0) {                                       // if --force flag was not provided
        char stagingPath[1024];                             // buffer for staging file path
        sprintf(stagingPath, "%s/.minigit/staging.dat", repoPath); // build path to staging.dat
        FILE *pFile = fopen(stagingPath, "r");              // open staging.dat to check for uncommitted changes

        if (pFile == NULL) {                                // handle staging file open error
            printf("error opening staging file");           // print file error
            return;                                         // abort revert
        }

        if (fgetc(pFile) != EOF) {                          // check if staging file has any staged files
            printf("staged changes exist, use --force");    // warn user about uncommitted staged files
            fclose(pFile);                                  // close staging file
            return;                                         // refuse revert unless --force is used
        }
        fclose(pFile);                                      // close staging file handle
    }

    sprintf(snapshotPath, "%s/.minigit/snapshots/%d", repoPath, id); // build path to target snapshot folder
    copy_dir(snapshotPath, repoPath);                       // copy all snapshot files into working directory

    Branch *tempBranch = branchHead;                        // pointer to find active branch node
    while (tempBranch != NULL) {                            // traverse branch linked list
        if (strcmp(tempBranch->name, currentBranch) == 0) { // locate current checked-out branch
            int previousHeadId = tempBranch->headId;        // record previous headId before changing
            tempBranch->headId = id;                        // update branch head to reverted commit ID
            save_branches();                                // persist updated headId to branches.dat
            push_undo(currentBranch, previousHeadId);       // push old head onto undo stack so revert can be undone
            break;                                          // done updating branch
        }
        tempBranch = tempBranch->next;                      // move to next branch
    }

    printf("reverted to commit <%d>\n", id);                // print confirmation message
}

void cmd_add(const char *file) {
    // check if the file actually exists (try fopen for reading)
    // if it doesn't exist, print error and return

    // open .minigit/staging.dat in append mode
    // write the filename on a new line
    // close the file

    // print "added <file>"

    // Check whether the file exists
    char checkPath[1024];
    FILE *pFile = fopen(file, "rb");
    if (pFile == NULL) {
        snprintf(checkPath, sizeof(checkPath), "%s/%s", repoPath, file);
        pFile = fopen(checkPath, "rb");
    }

    if (pFile == NULL) {
        printf("Error: file '%s' does not exist!\n", file);
        return;
    }

    fclose(pFile);

    // Open staging.dat in append mode
    char stagingPath[1024];
    sprintf(stagingPath, "%s/.minigit/staging.dat", repoPath);
    pFile = fopen(stagingPath, "a");

    if (pFile == NULL) {
        printf("Error opening staging file!\n");
        return;
    }

    // Write the filename into staging.dat
    fprintf(pFile, "%s\n", file);

    fclose(pFile);

    // Display success message
    printf("Added '%s' to staging area.\n", file);
}


void cmd_commit(const char *msg) {
    load_commits();
    load_branches();
    load_head();

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


    // Step 1: Read all staged filenames
    char stagedFiles[100][100];
    int fileCount = 0;

    char stagingPath[1024];
    sprintf(stagingPath, "%s/.minigit/staging.dat", repoPath);
    FILE *pFile = fopen(stagingPath, "r");

    if (pFile == NULL) {
        printf("Nothing to commit!\n");
        return;
    }

    while (fileCount < 100 &&
           fgets(stagedFiles[fileCount], sizeof(stagedFiles[fileCount]), pFile) != NULL) {

        // Remove newline character
        stagedFiles[fileCount][strcspn(stagedFiles[fileCount], "\r\n")] = '\0';

        if (stagedFiles[fileCount][0] != '\0') {
            fileCount++;
        }
    }

    fclose(pFile);

    // Check whether staging area is empty
    if (fileCount == 0) {
        printf("Nothing to commit!\n");
        return;
    }

    // Step 2: Determine the new commit ID
    int newId = 1;

    Commit *tempCommit = commitHead;

    while (tempCommit != NULL) {
        if (tempCommit->id >= newId) {
            newId = tempCommit->id + 1;
        }

        tempCommit = tempCommit->next;
    }

    // Step 3: Find the current branch
    Branch *current = branchHead;

    while (current != NULL && strcmp(current->name, currentBranch) != 0) {
        current = current->next;
    }

    if (current == NULL) {
        printf("Error: Current branch not found!\n");
        return;
    }

    // Store the previous commit ID
    int previousHeadId = current->headId;

    // Step 4: Create snapshot directory
    char snapshotPath[1024];

    sprintf(snapshotPath, "%s/.minigit/snapshots/%d", repoPath, newId);

    if (MKDIR(snapshotPath) == -1) {
        printf("Error creating snapshot directory!\n");
        return;
    }

    // Step 5: Copy previous snapshot into the new snapshot
    if (previousHeadId != -1) {

        char previousSnapshot[1024];

        sprintf(previousSnapshot, "%s/.minigit/snapshots/%d", repoPath, previousHeadId);

        copy_dir(previousSnapshot, snapshotPath);
    }

    // Step 6: Copy newly staged files into the snapshot
    for (int i = 0; i < fileCount; i++) {

        char destinationPath[2048];

        snprintf(destinationPath, sizeof(destinationPath), "%s/%s", snapshotPath, stagedFiles[i]);

        if (copy_file(stagedFiles[i], destinationPath) == 0) {
            char srcPath[1024];
            snprintf(srcPath, sizeof(srcPath), "%s/%s", repoPath, stagedFiles[i]);
            if (copy_file(srcPath, destinationPath) == 0) {
                printf("Error copying file: %s\n", stagedFiles[i]);
                return;
            }
        }
    }

    // Step 7: Create a new commit node
    Commit *newCommit = (Commit *)calloc(1, sizeof(Commit));

    if (newCommit == NULL) {
        printf("Memory allocation failed!\n");
        return;
    }

    newCommit->id = newId;
    newCommit->parentId = previousHeadId;
    newCommit->timestamp = time(NULL);

    strncpy(newCommit->message, msg, sizeof(newCommit->message) - 1);

    newCommit->next = NULL;

    // Connect parent pointer
    newCommit->parent = NULL;

    if (previousHeadId != -1) {
        Commit *search = commitHead;

        while (search != NULL) {
            if (search->id == previousHeadId) {
                newCommit->parent = search;
                break;
            }

            search = search->next;
        }
    }

    // Step 8: Append commit to linked list
    if (commitHead == NULL) {
        commitHead = commitTail = newCommit;
    }
    else {
        commitTail->next = newCommit;
        commitTail = newCommit;
    }

    commitCount++;

    // Save commit information
    save_commits();

    // Step 9: Store undo information
    push_undo(currentBranch, previousHeadId);

    // Step 10: Update current branch HEAD
    current->headId = newId;

    save_branches();

    // Step 11: Clear staging.dat
    pFile = fopen(stagingPath, "w");

    if (pFile != NULL) {
        fclose(pFile);
    }

    // Step 12: Display success message
    printf("Committed: %s\n", msg);
}

void cmd_log() {
    load_commits();
    load_branches();
    load_head();

    // Find the current branch
    Branch *current = branchHead;

    while (current != NULL && strcmp(current->name, currentBranch) != 0) {
        current = current->next;
    }

    // Check whether the branch exists
    if (current == NULL) {
        if (jsonOutput) printf("{\"error\": \"branch not found\"}\n");
        else printf("Error: Current branch not found!\n");
        return;
    }

    // Get the latest commit of the current branch
    int headId = current->headId;

    if (headId == -1) {
        if (jsonOutput) printf("{\"commits\": []}\n");
        else printf("No commits yet.\n");
        return;
    }

    // Find the latest commit in the commit linked list
    Commit *temp = commitHead;

    while (temp != NULL && temp->id != headId) {
        temp = temp->next;
    }

    if (temp == NULL) {
        if (jsonOutput) printf("{\"error\": \"commit not found\"}\n");
        else printf("Error: Commit not found!\n");
        return;
    }

    if (jsonOutput) {
        Commit *arr[500];
        int count = 0;
        while (temp != NULL) {
            arr[count++] = temp;
            temp = temp->parent;
        }
        printf("{\"commits\": [");
        for (int i = 0; i < count; i++) {
            printf("{\"id\": %d, \"parentId\": %d, \"timestamp\": %ld, \"message\": \"%s\"}",
                arr[i]->id, arr[i]->parentId, arr[i]->timestamp, arr[i]->message);
            if (i < count - 1) printf(",");
        }
        printf("]}\n");
    } else {
        // Traverse backwards using parent pointers
        while (temp != NULL) {

            printf("\n----------------------------------\n");

            printf("Commit ID : %d\n", temp->id);
            printf("Parent ID : %d\n", temp->parentId);

            time_t when = temp->timestamp;
            printf("Date      : %s", ctime(&when));

            printf("Message   : %s\n", temp->message);

            temp = temp->parent;
        }

        printf("----------------------------------\n");
    }
}



//  BRANCH 

void cmd_branch(const char *name) {

    load_branches();  //reads .minigit/branches.dat into the branchHead linked list
    load_head();    //reads .minigit/HEAD into currentBranch, so we know which branch we're on

    // 1. Check the branch name

    // it can't be empty,30 or more characters, or contain a space or '|'

    if (strlen(name) == 0 || strlen(name) >= 30 || strchr(name, '|') != NULL || strchr(name, ' ') != NULL) {
        printf("Error: invalid branch name!\n");
        return;
    }

    // 2. Walk the branch list

    Branch *current = NULL; //current will point to the branch we're on
    Branch *last = NULL;    //last will end up at the final node of the list
    Branch *temp = branchHead; //Starts a pointer at the first branch, so we can walk the list

    while (temp != NULL) {   //loops over every branch until the end of the list
        if (strcmp(temp->name, name) == 0) { //checks whether this branch already has the requested name. If it does, it prints an error and returns
            printf("Error: branch '%s' already exists!\n", name);
            return;
        }

        if (strcmp(temp->name, currentBranch) == 0) {   //saves the pointer to the current branch when we find it
            current = temp;
        }

        last = temp;    //after the loop it points to the last branch
        temp = temp->next; //moves to the next branch
    }

    if (current == NULL) {  //protects against a broken HEAD file
        printf("Error: Current branch not found!\n");
        return;
    }

    //3. Create the new branch

    Branch *newBranch = (Branch *)calloc(1, sizeof(Branch));  // with calloc, next starts as NULL, so the list ends correctly even before we set it

    if (newBranch == NULL) {  //checks that the memory was actually allocated
        printf("Memory allocation failed!\n");
        return;
    }

    strcpy(newBranch->name, name); //copies the new name into the node
    newBranch->headId = current->headId;  //makes the new branch point to the same commit as the current branch
    newBranch->next = NULL; //marks the new node as the end of the list

    // 4. Add it to the end of the list
    if (last == NULL) {  //since the list was empty , the new node becomes the first branch
        branchHead = newBranch;
    } else {                   // links the new node after the last one
        last->next = newBranch;
    }

    // 5. Save and print a message
    save_branches();   //writes the whole list back to branches.dat

    if (newBranch->headId == -1) {   //-1 means the branch has no commits yet , so the message leaves out the commit number
        printf("Created branch '%s' (no commits yet).\n", name);
    } else {
        printf("Created branch '%s' at commit %d.\n", name, newBranch->headId);
    }
}

//  SWITCH
void cmd_switch(const char *name) {
    load_branches();         // reads branches.dat into the linked list starting at branchHead
    load_head();             // reads HEAD into currentBranch, so the function knows which branch you're on

    if (strcmp(name, currentBranch) == 0) {  //stops if we're already on that branch (two strings match)
        printf("Already on branch '%s'.\n", name);
        return;
    }

    //1. Find the target branch in the linked list

    Branch *target = NULL;    //starts with no branch found , if the search fails it stays NULL
    for (Branch *b = branchHead; b != NULL; b = b->next) {  //walks the list one branch at a time
        if (strcmp(b->name, name) == 0) {  //checks whether this branch has the name you asked for
            target = b; //remembers the matching branch
            break;
        }
    }

    if (target == NULL) {   //the branch doesn't exist,so the function prints and error and returns
        printf("Error: branch '%s' does not exist.\n", name);
        return;
    }

    //2. Check for staged changes

    char stagingPath[1024];
    sprintf(stagingPath, "%s/.minigit/staging.dat", repoPath);
    FILE *pFile = fopen(stagingPath, "r"); //opens the staging file for reading
    if (pFile != NULL) {   //fopen returns NULL if the file can't be opened
        int hasStaged = (fgetc(pFile) != EOF);  //if the file is empty it returns EOF, so hasStaged is 0,if naything is there it's 1
        fclose(pFile);

        if (hasStaged) {  //if there are staged files ,prints a message and returns
            printf("Cannot switch: you have staged changes. Commit or revert them first.\n");
            return;
        }
    }

    //3. Restoring files

    if (target->headId != -1) {     //-1 means the branch has no commits , so nothing to restore
        char snapshotPath[1024];
        sprintf(snapshotPath, "%s/.minigit/snapshots/%d", repoPath, target->headId); //builds the path to the snapshot for the target's latest commit
        copy_dir(snapshotPath, repoPath); //copies every file from that snapshot into the repo folder
    }

    strcpy(currentBranch, name);  //updates the current branch name in memory
    save_head();  //writes the new name to HEAD

    printf("Switched to branch '%s'.\n", name);
}

//  BRANCHES
void cmd_branches() {
    
    load_branches();  //reads branches.dat into the linked list starting at branchHead
    load_head();     //reads HEAD into currentBranch, so the function knows which branch is active

    if (jsonOutput) {
        printf("{\"branches\": [");
        Branch *temp = branchHead;
        int first = 1;
        while (temp != NULL) {
            if (!first) printf(",");
            printf("{\"name\": \"%s\", \"headId\": %d, \"current\": %s}",
                temp->name,
                temp->headId,
                strcmp(temp->name, currentBranch) == 0 ? "true" : "false");
            first = 0;
            temp = temp->next;
        }
        printf("]}\n");
        return;
    }

    if (branchHead == NULL) {   //branchHead is NULL when the list is empty, meaning no branches exist
        printf("No branches yet.\n");
        return;
    }

    Branch *temp = branchHead;   //starts a pointer at the first branch , temp is used so branchHead itself isn't moved

    while (temp != NULL) {     //repeats for each branch until the end of the list
        if (strcmp(temp->name, currentBranch) == 0) {   //strcmp returns 0 when two strings match, so this checks whether this branch is the active one
            printf("* %s (commit %d)\n", temp->name, temp->headId);  //prints the active branch with a * in front, its name, and the commit it points to
        } else {
            printf("  %s (commit %d)\n", temp->name, temp->headId);  //prints the same line with two spaces instead of *, so the names line up
        }

        temp = temp->next;   //moves to the next branch in the list
    }
}

//  SEARCH 

void cmd_search(const char *keyword) {

    load_commits();   //reads commits.dat into the linked list starting at commitHead

    if (strlen(keyword) == 0) {  //checks whether anything was typed
        printf("Usage: search <keyword or commit id>\n");
        return;
    }

    int isNumber = 1; //assuming the keyword is a number

    for (int i = 0; keyword[i] != '\0'; i++) {  //goes through the keyword one character at a time
        if (keyword[i] < '0' || keyword[i] > '9') {  //checks whether this character is outside the range of digits
            isNumber = 0;  //if non-digit is found record that keyword and stop checking
            break;
        }
    }

    if (isNumber) {  //runs the id search only when every character was a digit
        int wantedId = atoi(keyword); //converts the text to a number,
        Commit *temp = commitHead;  //starts a pointer at the first commit

        while (temp != NULL) {
            if (temp->id == wantedId) {  //checks whether this commit has the id you asked for
                time_t when = temp->timestamp;  //copies the timestamp into a time_t variable
                printf("Commit ID : %d\n", temp->id);
                printf("Date      : %s", ctime(&when));
                printf("Message   : %s\n", temp->message);
                return;
            }
            temp = temp->next;  //moves to the next commit
        }

        printf("No commit with id %d.\n", wantedId);
        return;
    }

    char lowerKeyword[100];   //lowercase copy of the keyword
    strncpy(lowerKeyword, keyword, 99);  //copies at most 99 characters
    lowerKeyword[99] = '\0';

    for (int i = 0; lowerKeyword[i] != '\0'; i++) {  //changes each character to lowercase, so the search ignores case
        lowerKeyword[i] = tolower(lowerKeyword[i]);
    }

    int matches = 0;  //counts how many commits match
    Commit *temp = commitHead;

    while (temp != NULL) {
    
        char lowerMessage[100];  //a lowercase copy of the current commit's message
        strncpy(lowerMessage, temp->message, 99); //copy the message and lowercase it, the same way the keyword was handled
        lowerMessage[99] = '\0';

        for (int i = 0; lowerMessage[i] != '\0'; i++) {
            lowerMessage[i] = tolower(lowerMessage[i]);
        }

        if (strstr(lowerMessage, lowerKeyword) != NULL) {  //looks for the keyword anywhere inside the message
            time_t when = temp->timestamp;
            printf("Commit ID : %d\n", temp->id);
            printf("Date      : %s", ctime(&when));
            printf("Message   : %s\n\n", temp->message);
            matches++;
        }

        temp = temp->next; //moves to the next commit
    }

  
    if (matches == 0) {
        printf("No commits matched '%s'.\n", keyword);
    } else {
        printf("%d commit(s) matched '%s'.\n", matches, keyword);
    }
}

void cmd_diff(int id1, int id2, const char *file) {
    char snapshotPath1[1024];                               // buffer for file path in first snapshot
    char snapshotPath2[1024];                               // buffer for file path in second snapshot
    sprintf(snapshotPath1, "%s/.minigit/snapshots/%d/%s", repoPath, id1, file); // construct path for commit id1
    sprintf(snapshotPath2, "%s/.minigit/snapshots/%d/%s", repoPath, id2, file); // construct path for commit id2

    FILE *pFile1 = fopen(snapshotPath1, "r");               // open commit id1 snapshot file for reading
    FILE *pFile2 = fopen(snapshotPath2, "r");               // open commit id2 snapshot file for reading

    if (pFile1 == NULL) {                                   // verify first file opened successfully
        printf("error opening the first file!!");           // print error message
        return;                                             // exit function
    } else if (pFile2 == NULL) {                            // verify second file opened successfully
        printf("error opening the second file!!");          // print error message
        fclose(pFile1);                                     // close first opened file
        return;                                             // exit function
    }

    char A[200][256];                                       // array storing lines of file 1 (old version)
    char B[200][256];                                       // array storing lines of file 2 (new version)
    int n = 0, m = 0;                                       // line counters for file 1 and file 2

    while (n < 200 && fgets(A[n], sizeof(A[n]), pFile1) != NULL) { // read lines of file 1 into array A
        A[n][strcspn(A[n], "\r\n")] = '\0';                 // strip trailing newline and carriage return
        n++;                                                // increment line count for file 1
    }

    while (m < 200 && fgets(B[m], sizeof(B[m]), pFile2) != NULL) { // read lines of file 2 into array B
        B[m][strcspn(B[m], "\r\n")] = '\0';                 // strip trailing newline and carriage return
        m++;                                                // increment line count for file 2
    }

    fclose(pFile1);                                         // close file 1 handle
    fclose(pFile2);                                         // close file 2 handle

    int c[n+1][m+1];                                        // 2D memoization table for LCS DP algorithm
    memset(c, 0, sizeof(c));                                // initialize all DP matrix cells to 0

    for (int i = 1; i <= n; i++) {                          // iterate through lines of file 1
        for (int j = 1; j <= m; j++) {                      // iterate through lines of file 2
            if (strcmp(A[i-1], B[j-1]) == 0) {              // if lines match: extend longest common subsequence
                c[i][j] = 1 + c[i-1][j-1];                  // recurrence: diagonal cell + 1
            } else {                                        // if lines differ: take maximum of adjacent cells
                c[i][j] = c[i][j-1] > c[i-1][j] ? c[i][j-1] : c[i-1][j]; // recurrence: max(left, top)
            }
        }
    }

    char diff[400][260];                                    // stores generated diff output lines
    char prefix[400];                                       // stores diff prefix symbol (' ', '+', '-')
    int diffCount = 0;                                      // total number of diff entries

    int i = n, j = m;                                       // backtrack pointers starting at bottom-right of DP table
    while (i > 0 || j > 0) {                                // backtrack through memoization table to construct diff
        if (i > 0 && j > 0 && strcmp(A[i-1], B[j-1]) == 0) { // matching line in both versions (context line)
            strcpy(diff[diffCount], A[i-1]);                // record unchanged line content
            prefix[diffCount] = ' ';                        // ' ' prefix signifies identical context line
            i--; j--;                                       // move diagonally backward in DP table
        } else if (i > 0 && (j == 0 || c[i-1][j] > c[i][j-1])) { // line deleted in new version
            strcpy(diff[diffCount], A[i-1]);                // record removed line content
            prefix[diffCount] = '-';                        // '-' prefix signifies deleted line
            i--;                                            // move up in DP table
        } else {                                            // line added in new version
            strcpy(diff[diffCount], B[j-1]);                // record newly inserted line content
            prefix[diffCount] = '+';                        // '+' prefix signifies added line
            j--;                                            // move left in DP table
        }
        diffCount++;                                        // increment diff lines counter
    }

    if (jsonOutput) {                                       // format output as JSON if requested by web server
        printf("{\"diff\": [");                             // opening JSON diff array
        for (int k = diffCount - 1; k >= 0; k--) {          // print in reverse (chronological forward order)
            printf("{\"prefix\": \"%c\", \"line\": \"%s\"}", prefix[k], diff[k]); // diff item JSON object
            if (k > 0) printf(",");                         // comma separator between elements
        }
        printf("]}");                                       // closing JSON array
    } else {                                                // standard terminal diff format
        for (int k = diffCount - 1; k >= 0; k--) {          // print in reverse (chronological forward order)
            printf("%c %s\n", prefix[k], diff[k]);          // output format e.g. "+ line" or "- line"
        }
    }
}



int main(int argc, char *argv[])
{
    // minigit init
    // minigit add <file>
    // minigit commit "message"
    // minigit log
    // minigit diff <id1> <id2> <file>
    // minigit revert <id> [--force]
    // minigit undo
    // minigit branch <name>
    // minigit switch <name>
    // minigit branches
    // minigit search <keyword>

    
    // check all args for --json and --repo

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--repo") == 0 && i + 1 < argc) {
            strcpy(repoPath, argv[i + 1]);
#ifdef _WIN32
            if (strncmp(repoPath, "/tmp", 4) == 0) {
                struct stat st;
                if (stat(repoPath, &st) != 0) {
                    char *tempEnv = getenv("TEMP");
                    if (!tempEnv) tempEnv = getenv("TMP");
                    if (tempEnv) {
                        char resolved[500];
                        snprintf(resolved, sizeof(resolved), "%s%s", tempEnv, repoPath + 4);
                        strcpy(repoPath, resolved);
                    }
                }
            }
#endif
            size_t rlen = strlen(repoPath);
            while (rlen > 1 && (repoPath[rlen - 1] == '/' || repoPath[rlen - 1] == '\\')) {
                repoPath[--rlen] = '\0';
            }
        }
        if (strcmp(argv[i], "--json") == 0) {
            jsonOutput = 1;
        }
    }

    if (argc < 2)
    {
        printf("Usage: minigit <command> [args]\n");
        return 1;
    }

    if (strcmp(argv[1], "init") == 0)
    {
        cmd_init();
    }
    else if (strcmp(argv[1], "add") == 0)
    {
        if (argc < 3)
        {
            printf("Usage: minigit add <file>\n");
            return 1;
        }
        cmd_add(argv[2]);
    }
    else if (strcmp(argv[1], "commit") == 0)
    {
        if (argc < 3)
        {
            printf("Usage: minigit commit \"message\"\n");
            return 1;
        }
        cmd_commit(argv[2]);
    }
    else if (strcmp(argv[1], "log") == 0)
    {
        cmd_log();
    }
    else if (strcmp(argv[1], "diff") == 0)
    {
        if (argc < 5)
        {
            printf("Usage: minigit diff <id1> <id2> <file>\n");
            return 1;
        }
        cmd_diff(atoi(argv[2]), atoi(argv[3]), argv[4]);
    }
    else if (strcmp(argv[1], "revert") == 0)
    {
        if (argc < 3)
        {
            printf("Usage: minigit revert <id> [--force]\n");
            return 1;
        }
        int force = (argc >= 4 && strcmp(argv[3], "--force") == 0);
        cmd_revert(atoi(argv[2]), force);
    }
    else if (strcmp(argv[1], "undo") == 0)
    {
        cmd_undo();
    }
    else if (strcmp(argv[1], "branch") == 0)
    {
        if (argc < 3)
        {
            printf("Usage: minigit branch <name>\n");
            return 1;
        }
        cmd_branch(argv[2]);
    }
    else if (strcmp(argv[1], "switch") == 0)
    {
        if (argc < 3)
        {
            printf("Usage: minigit switch <name>\n");
            return 1;
        }
        cmd_switch(argv[2]);
    }
    else if (strcmp(argv[1], "branches") == 0)
    {
        cmd_branches();
    }
    else if (strcmp(argv[1], "search") == 0)
    {
        if (argc < 3)
        {
            printf("Usage: minigit search <keyword>\n");
            return 1;
        }
        cmd_search(argv[2]);
    }
    else
    {
        printf("Unknown command: %s\n", argv[1]);
        return 1;
    }
    
    return 0;
}