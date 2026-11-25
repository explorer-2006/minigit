#ifndef MINIGIT_H
#define MINIGIT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
    #include <direct.h>
    #include <io.h>
    #define MKDIR(p) _mkdir(p)
#else
    #include <sys/stat.h>
    #include <dirent.h>
    #define MKDIR(p) mkdir(p, 0755)
#endif

// required Structures 

typedef struct Commit{
    int id;
    int parentId;
    long timestamp;
    char message[100];
    struct Commit *parent;   // points to parent commit (history)
    struct Commit *next;     // points to next node (list traversal)
}Commit;

typedef struct Branch{
    char name[30];
    int headId;              // id of the latest commit on this branch
    struct Branch *next;     // next branch in the linked list
}Branch;

// globals

extern Commit *commitHead;        // head of the commit linked list
extern Commit *commitTail;        // tail (for appending new nodes)
extern Branch *branchHead;        // head of the branch linked list
extern Branch *branchTail;
extern char currentBranch[30];    // name of the current branch
extern int commitCount;           // total number of commits

// function declarations

// file helpers
int  copy_file(const char *src, const char *dst);// copies one file from src path to dst path... returns 1 on success, 0 on failure
void copy_dir(const char *src, const char *dst);// copies all files from one folder to another... used in commit to copy previous snapshot

// Every command starts by calling the load functions and ends by calling the save functions if anything changed.
void load_commits(); // reads commits.dat → builds linked list in memory
void save_commits(); // walks linked list → writes to commits.dat
void load_branches(); // reads branches.dat → builds branch list
void save_branches(); // writes branch list → branches.dat
void load_head(); // reads HEAD file → sets currentBranch
void save_head(); // writes currentBranch → HEAD file

// undo stack
void push_undo(const char *branch, int previousHeadId);
int pop_undo(char *branchOut);  // fills branchOut with branch name, returns previousHeadId, -1 if empty

// commands
void cmd_init();
void cmd_add(const char *file);
void cmd_commit(const char *msg);
void cmd_log();
void cmd_diff(int id1, int id2, const char *file);
void cmd_revert(int id, int force);
void cmd_undo();
void cmd_branch(const char *name);
void cmd_switch(const char *name);
void cmd_branches();
void cmd_search(const char *keyword);

#endif