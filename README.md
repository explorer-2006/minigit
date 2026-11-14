# MiniGit — Mini Version Control System

A simplified version control system built in C, inspired by Git. MiniGit lets you track changes to a project folder, save snapshots, compare versions, go back to older states, and push your history to a server where it can be viewed through a GitHub-style web interface.

Built as a Data Structures course project to demonstrate real-world use of linked lists, trees, stacks, recursion, sorting, searching, and file handling in C.

---

## Team contributions

| Team member | Commands implemented | Main data structures / concepts |
|---|---|---|
| **Jeevan Batra** | `init`, `add`, `commit`, `log` | File handling, commit linked list, parent-pointer traversal |
| **Janisha Patel** | `branch`, `switch`, `branches`, `search` | Branch linked list, tree formation, linear search |
| **Meet Manish Buddhdev** | `diff`, `revert`, `undo` | LCS recursion with 2D memoization, snapshot restore, stack |

---

## How it works

MiniGit has two parts:

- **C engine (`minigit.exe`)** — handles all the real logic: commits, branches, diff, revert, undo. Runs entirely from the terminal.
- **Flask web layer (`app.py`)** — calls the C engine via subprocess, reads the JSON it prints, and shows commits, branches, and diffs in a browser. The website is view-only.

```
Your laptop                        Server                      Browser
minigit.exe  ──(push.py)──►  repos/<name>/.minigit/  ──►  Flask web interface
```

---

## Project structure

```
MiniGit/
  minigit.h          ← shared structs and function declarations
  minigit.c          ← all C logic
  app.py             ← Flask backend
  push.py            ← script to upload a repo to the server
  templates/
    index.html       ← repo list
    repo.html        ← commit history and branches
    commit.html      ← commit detail with diff
    visualizer.html  ← live data structure visualizer
  repos/             ← server-side repo storage
  README.md
```

---

## Repository layout

Running `minigit init` inside a project folder creates:

```
.minigit/
  commits.dat      ← commit history (one line per commit)
  branches.dat     ← branch names and their head commit ids
  HEAD             ← name of the current branch
  staging.dat      ← files waiting to be committed
  undo.dat         ← undo stack (one commit id per line)
  snapshots/
    1/             ← full copy of the project at commit 1
    2/             ← full copy of the project at commit 2
    ...
```

### File formats

**commits.dat** — one commit per line:
```
id|parent_id|timestamp|message
1|0|1729251000|first version
2|1|1729254600|added multiply
```
`parent_id` of 0 means no parent (the first commit).

**branches.dat** — one branch per line:
```
main|2
dev|3
```

**HEAD** — just the current branch name:
```
main
```

**staging.dat** — one staged filename per line:
```
main.c
utils.c
```

**undo.dat** — one commit id per line, last line = top of stack:
```
1
2
```

**snapshots/** — each commit folder holds a complete copy of the entire project at that point. Commit 2's folder contains all files from commit 1 plus any staged changes.

---

## Commands

### Initialise a repository
```
minigit init
```
Creates `.minigit/` in the current folder. Fails if it already exists.

### Stage a file
```
minigit add <file>
```
Adds a file to the staging area. The file must exist. Nothing is saved permanently yet.

### Commit staged files
```
minigit commit "message"
```
Saves a snapshot of all staged files:
1. Creates `snapshots/<newId>/` and copies everything from the previous snapshot into it
2. Copies staged files on top (replacing old versions)
3. Creates a new commit node with the current branch head as its parent
4. Appends the commit to `commits.dat`
5. Updates the branch head in `branches.dat`
6. Pushes the old head onto `undo.dat`
7. Clears `staging.dat`

The first commit contains only the staged files since there is no previous snapshot.

### View history
```
minigit log
```
Loads `commits.dat` into a linked list, reads `HEAD` to find the current branch head, then follows parent pointers back to the first commit. Prints commits newest first.

### Compare two commits
```
minigit diff <id1> <id2> <file>
```
Loads the specified file from both snapshot folders into two line arrays. Runs the LCS (Longest Common Subsequence) algorithm recursively with a 2D memoization table. Prints unchanged lines, removed lines (marked `-`), and added lines (marked `+`).

### Revert to an older version
```
minigit revert <id>
minigit revert <id> --force
```
Copies all files from `snapshots/<id>/` back over the working folder. Creates a new commit with message `"Revert to commit <id>"` so history is never deleted. Pushes the old head onto the undo stack. Refuses if there are uncommitted changes unless `--force` is passed.

### Undo the last move
```
minigit undo
```
Pops the top commit id off `undo.dat`, moves the current branch head back to that commit, and restores the files from that snapshot into the working folder.

### Branch operations
```
minigit branch <name>    ← create a new branch at the current commit
minigit switch <name>    ← switch to a branch (refuses if uncommitted changes)
minigit branches         ← list all branches and their head commit ids
```

### Search commits
```
minigit search <keyword>
minigit search <id>
```
A numeric argument looks up the commit directly in the linked list by id. Text performs a linear scan over all commit messages and returns matching commits.

### Push to server
```
python push.py <repo-name>
```
Zips the `.minigit/` folder and uploads it to the Flask server. The server validates the upload, runs a fast-forward check (walks parent pointers from the pushed head to verify the server's current head is an ancestor), and stores the repo under `repos/<name>/.minigit/` on disk.

---

## Data structures

| Data structure | Where it is used |
|---|---|
| **Singly linked list** (next pointer) | All commits stored as nodes, traversed for log and search |
| **Tree** (parent pointer) | Commit history — when branches diverge, two commits share a parent, forming a tree |
| **Stack** | Undo history stored in `undo.dat`, push on commit/revert, pop on undo |
| **Linked list** (branch nodes) | All branches stored as nodes with name, headId, and next pointer |
| **Recursion** | LCS diff algorithm — compares two files line by line |
| **2D array** | Memoization table for the LCS recursion (`memo[i][j]`) |
| **File handling** | All `.dat` files and snapshot copies |
| **Dynamic memory** | `malloc` for every commit and branch node, `free` on exit |

### Commit node structure
```c
typedef struct Commit {
    int id;
    int parentId;
    long timestamp;
    char message[100];
    struct Commit *parent;   // history traversal (backwards)
    struct Commit *next;     // list traversal (forwards)
} Commit;
```

### Branch node structure
```c
typedef struct Branch {
    char name[30];
    int headId;
    struct Branch *next;
} Branch;
```

### How the commit history becomes a tree

While on a single branch, commits form a linear chain:
```
Commit 1 ←── Commit 2 ←── Commit 3   (main)
```

Once a second branch is created and a commit is made on it, two commits share the same parent:
```
Commit 1 ←── Commit 2 ←── Commit 3   (main)
                    └───── Commit 4   (dev)
```
Commit 3 and Commit 4 both have `parentId = 2`.

### LCS diff algorithm

The diff compares two versions of a file line by line using the Longest Common Subsequence algorithm:

- If either file is finished, return 0 (base case)
- If `A[i] == B[j]`, return `1 + LCS(i+1, j+1)` (lines match)
- Otherwise return `max(LCS(i+1, j), LCS(i, j+1))` (skip a line from one side)

The plain recursive version recomputes the same subproblems many times. The memoized version stores each result in `memo[i][j]`, making it run in O(n × m) time instead of exponential time.

---

## How the run cycle works

Every command is a fresh process:

1. Parse `argv` for the command and its arguments
2. Load `.dat` files into memory and rebuild pointer links
3. Run the command
4. Save any file that changed
5. Print one JSON object to stdout
6. Free all nodes and exit

Since the program exits after every command, the files are the permanent copy and memory is just the working copy.

---

## JSON output

Every command prints exactly one line of JSON to stdout. Flask reads this and either renders it on the page or returns it as an API response.

```json
{"status":"ok","commits":[{"id":2,"parent":1,"time":"2026-10-18 14:30","message":"added multiply"}]}
{"status":"ok","message":"Committed #2: added multiply"}
{"status":"error","message":"Nothing staged. Use add first."}
```

---

## Web interface

Flask runs `minigit.exe` with subprocess, reads the JSON output, and serves it through HTML templates. The website is view-only — all real operations happen through the C CLI.

Pages:
- `/` — list of all repos on the server
- `/repo/<name>` — commit history, current branch, branch selector
- `/repo/<name>/commit/<id>` — commit detail with diff (removed lines in red, added lines in green)
- `/repo/<name>/branches` — all branches and their head commits
- `/visualizer` — live animated visualizer showing the actual data structures as you use them

The visualizer shows the linked list of commits building up node by node, the undo stack filling and popping, the branch tree forming when branches diverge, and the LCS memo table filling during a diff.

---

## How to compile and run

**Requirements:**
- gcc (MinGW 64-bit on Windows)
- Python 3 with Flask (`pip install flask`)

**Compile:**
```
gcc minigit.c -o minigit
```

**Basic usage:**
```
mkdir myproject
cd myproject
copy ..\minigit.exe .

minigit init
minigit add main.c
minigit commit "first version"
minigit log
```

**Start the web server:**
```
cd ..
python app.py
```
Open `http://localhost:5000` in a browser.

**Push a repo:**
```
python push.py myproject
```

---

## Constraints

- Text files only (no binary files or sub-folders in v1)
- Maximum 1000 commits per repo
- Maximum 50 staged files per commit
- Commit messages: max 99 characters, no `|` character
- Diff is limited to files under 2000 lines (memo table memory limit)
