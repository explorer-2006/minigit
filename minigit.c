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
    commitHead = NULL;
    commitTail = NULL;
    commitCount = 0;

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

    branchHead = NULL;
    branchTail = NULL;

    FILE *pFile = fopen(".minigit/branches.dat", "r");
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
    // reads undo.dat, pops the last entry, rewrites file without it
    // fills branchOut with branch name, returns previousHeadId or -1

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
    
    while (fscanf(pFile, " %[^|]|%d", branchName, &previousHeadId) == 2){
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

void cmd_undo() {
    load_branches();
    load_head();

    char branchName[30];
    int previousHeadId = pop_undo(branchName);

    if(previousHeadId == -1){
        printf("Nothing to undo.\n");
        return;
    }

    if(strcmp(branchName, currentBranch) != 0){
        printf("Last undo entry is for a different branch.\n");
        return;
    }

    Branch *temp = branchHead;
    int found = 0;

    while (temp != NULL) {
        if(strcmp(temp->name, branchName) == 0){
            found = 1;
            break;
        }
        temp = temp->next;
    }

    if(!found){
        printf("Error: branch not found.\n");
        return;
    }

    temp->headId = previousHeadId;
    save_branches();

    printf("Undo successful, now at commit %d.\n", previousHeadId);
}

void cmd_revert(int id, int force) {
    // load commits, branches, head

    // find the commit with the given id in commitHead list
    // if not found, print "commit <id> not found" and return

    // if force == 0, check staging.dat
        // if staging has content, print "staged changes exist, use --force" and return

    // build snapshot path: .minigit/snapshots/<id>/
    // copy_dir(snapshotPath, ".")

    // find current branch in branchHead list
    // set branch->headId = id
    // save_branches()

    // push_undo(currentBranch, previous headId)  // so you can undo the revert

    // print "reverted to commit <id>"

    load_commits();
    load_branches();
    load_head();
    int found = 0;
    char snapshotPath[1024];

    Commit *temp;
    temp = commitHead;

    while (temp != NULL)
    {
        if(temp->id == id){
            found = 1;
            break;
        }
        temp = temp->next;
    }

    if(!found){
        printf("commit <%d> not found", id);
        return;
    }

    if(force == 0){
        FILE *pFile = fopen(".minigit/staging.dat", "r");

        if(pFile == NULL){
            printf("error opening staging file");
            return;
        }

        if(fgetc(pFile) != EOF){
            printf("staged changes exist, use --force");
            fclose(pFile);
            return;
        }
        
    }

    sprintf(snapshotPath, ".minigit/snapshots/%d", id);
        copy_dir(snapshotPath, ".");

        Branch *tempBranch;
        tempBranch = branchHead;

        while (tempBranch != NULL) {
            if (strcmp(tempBranch->name, currentBranch) == 0) {
                int previousHeadId = tempBranch->headId; 
                tempBranch->headId = id;
                save_branches();
                push_undo(currentBranch, previousHeadId);
                break;
            }
            tempBranch = tempBranch->next;
        }

        printf("reverted to commit <%d>\n", id);
    
}

void cmd_add(const char *file) {
    // check if the file actually exists (try fopen for reading)
    // if it doesn't exist, print error and return

    // open .minigit/staging.dat in append mode
    // write the filename on a new line
    // close the file

    // print "added <file>"

    // Check whether the file exists


    FILE *pFile = fopen(file, "rb");

    if (pFile == NULL) {
        printf("Error: file '%s' does not exist!\n", file);
        return;
    }

    fclose(pFile);

    // Open staging.dat in append mode
    pFile = fopen(".minigit/staging.dat", "a");

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

    FILE *pFile = fopen(".minigit/staging.dat", "r");

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

    sprintf(snapshotPath, ".minigit/snapshots/%d", newId);

    if (MKDIR(snapshotPath) == -1) {
        printf("Error creating snapshot directory!\n");
        return;
    }

    // Step 5: Copy previous snapshot into the new snapshot
    if (previousHeadId != -1) {

        char previousSnapshot[1024];

        sprintf(previousSnapshot, ".minigit/snapshots/%d", previousHeadId);

        copy_dir(previousSnapshot, snapshotPath);
    }

    // Step 6: Copy newly staged files into the snapshot
    for (int i = 0; i < fileCount; i++) {

        char destinationPath[2048];

        snprintf(destinationPath, sizeof(destinationPath), "%s/%s", snapshotPath, stagedFiles[i]);

        if (copy_file(stagedFiles[i], destinationPath) == 0) {
            printf("Error copying file: %s\n", stagedFiles[i]);
            return;
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
    pFile = fopen(".minigit/staging.dat", "w");

    if (pFile != NULL) {
        fclose(pFile);
    }

    // Step 12: Display success message
    printf("Committed: %s\n", msg);
}

void cmd_log() {

    // Find the current branch
    Branch *current = branchHead;

    while (current != NULL && strcmp(current->name, currentBranch) != 0) {
        current = current->next;
    }

    // Check whether the branch exists
    if (current == NULL) {
        printf("Error: Current branch not found!\n");
        return;
    }

    // Get the latest commit of the current branch
    int headId = current->headId;

    if (headId == -1) {
        printf("No commits yet.\n");
        return;
    }

    // Find the latest commit in the commit linked list
    Commit *temp = commitHead;

    while (temp != NULL && temp->id != headId) {
        temp = temp->next;
    }

    if (temp == NULL) {
        printf("Error: Commit not found!\n");
        return;
    }

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

    FILE *pFile = fopen(".minigit/staging.dat", "r"); //opens the staging file for reading
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
        char snapshotPath[500];
        sprintf(snapshotPath, ".minigit/snapshots/%d", target->headId); //builds the path to the snapshot for the target's latest commit
        copy_dir(snapshotPath, "."); //copies every file from that snapshot into the current folder
    }

    strcpy(currentBranch, name);  //updates the current branch name in memory
    save_head();  //writes the new name to HEAD

    printf("Switched to branch '%s'.\n", name);
}

//  BRANCHES
void cmd_branches() {
    
    load_branches();  //reads branches.dat into the linked list starting at branchHead
    load_head();     //reads HEAD into currentBranch, so the function knows which branch is active

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

int main() {
    cmd_init();
    load_commits();
    printf("commitCount: %d\n", commitCount);
    save_commits();
    printf("done\n");
    return 0;
}