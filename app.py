import subprocess
import json
import os
import zipfile
import shutil
from flask import Flask, render_template, jsonify, request, redirect, url_for

app = Flask(__name__)

# ── config ────────────────────────────────────────────────────────────────────
BASE_DIR  = os.path.dirname(os.path.abspath(__file__))
MINIGIT   = os.path.join(BASE_DIR, "minigit.exe" if os.name == "nt" or os.path.isfile(os.path.join(BASE_DIR, "minigit.exe")) else "minigit")
REPOS_DIR = os.path.join(BASE_DIR, "repos")     # server-side repo storage

os.makedirs(REPOS_DIR, exist_ok=True)

# ── fallback: parse .minigit folder directly if binary cannot run ──────────────
def _parse_minigit_fallback(cmd_args, repo_dir):
    minigit_path = os.path.join(repo_dir, ".minigit")
    if not os.path.isdir(minigit_path):
        return {"error": "repository not initialized"}

    cmd = cmd_args[0] if cmd_args else ""

    if cmd == "log":
        head_file = os.path.join(minigit_path, "HEAD")
        current_branch = "main"
        if os.path.isfile(head_file):
            with open(head_file, encoding="utf-8", errors="replace") as f:
                current_branch = f.read().strip()

        branches_file = os.path.join(minigit_path, "branches.dat")
        head_id = -1
        if os.path.isfile(branches_file):
            with open(branches_file, encoding="utf-8", errors="replace") as f:
                for line in f:
                    parts = line.strip().split("|")
                    if len(parts) >= 2 and parts[0] == current_branch:
                        try:
                            head_id = int(parts[1])
                        except ValueError:
                            pass
                        break

        if head_id == -1:
            return {"commits": []}

        commits_file = os.path.join(minigit_path, "commits.dat")
        commits_by_id = {}
        if os.path.isfile(commits_file):
            with open(commits_file, encoding="utf-8", errors="replace") as f:
                for line in f:
                    parts = line.strip().split("|")
                    if len(parts) >= 4:
                        try:
                            cid = int(parts[0])
                            pid = int(parts[1])
                            ts = int(parts[2])
                            msg = "|".join(parts[3:])
                            commits_by_id[cid] = {
                                "id": cid,
                                "parentId": pid,
                                "timestamp": ts,
                                "message": msg
                            }
                        except ValueError:
                            pass

        commits = []
        curr = head_id
        seen = set()
        while curr != -1 and curr in commits_by_id and curr not in seen:
            seen.add(curr)
            c = commits_by_id[curr]
            commits.append(c)
            curr = c["parentId"]

        return {"commits": commits}

    elif cmd == "branches":
        head_file = os.path.join(minigit_path, "HEAD")
        current_branch = "main"
        if os.path.isfile(head_file):
            with open(head_file, encoding="utf-8", errors="replace") as f:
                current_branch = f.read().strip()

        branches = []
        branches_file = os.path.join(minigit_path, "branches.dat")
        if os.path.isfile(branches_file):
            with open(branches_file, encoding="utf-8", errors="replace") as f:
                for line in f:
                    parts = line.strip().split("|")
                    if len(parts) >= 2:
                        try:
                            bname = parts[0]
                            bhead = int(parts[1])
                            branches.append({
                                "name": bname,
                                "headId": bhead,
                                "current": (bname == current_branch)
                            })
                        except ValueError:
                            pass
        return {"branches": branches}

    elif cmd == "diff":
        if len(cmd_args) < 4:
            return {"diff": []}
        id1, id2, fname = cmd_args[1], cmd_args[2], cmd_args[3]
        f1_path = os.path.join(minigit_path, "snapshots", str(id1), fname)
        f2_path = os.path.join(minigit_path, "snapshots", str(id2), fname)

        lines1 = []
        if os.path.isfile(f1_path):
            with open(f1_path, encoding="utf-8", errors="replace") as f:
                lines1 = f.read().splitlines()

        lines2 = []
        if os.path.isfile(f2_path):
            with open(f2_path, encoding="utf-8", errors="replace") as f:
                lines2 = f.read().splitlines()

        n, m = len(lines1), len(lines2)
        c = [[0] * (m + 1) for _ in range(n + 1)]
        for i in range(1, n + 1):
            for j in range(1, m + 1):
                if lines1[i - 1] == lines2[j - 1]:
                    c[i][j] = c[i - 1][j - 1] + 1
                else:
                    c[i][j] = max(c[i - 1][j], c[i][j - 1])

        diff_items = []
        i, j = n, m
        while i > 0 or j > 0:
            if i > 0 and j > 0 and lines1[i - 1] == lines2[j - 1]:
                diff_items.append({"prefix": " ", "line": lines1[i - 1]})
                i -= 1
                j -= 1
            elif i > 0 and (j == 0 or c[i - 1][j] > c[i][j - 1]):
                diff_items.append({"prefix": "-", "line": lines1[i - 1]})
                i -= 1
            else:
                diff_items.append({"prefix": "+", "line": lines2[j - 1]})
                j -= 1

        diff_items.reverse()
        return {"diff": diff_items}

    return {"error": f"unrecognized command: {cmd}"}


# ── helper: run minigit and return parsed JSON ─────────────────────────────────
def run(cmd_args, repo_name=None):
    """
    Run the minigit binary with the given arguments.
    If repo_name is given, adds --repo <path> and --json flags.
    Falls back to direct .minigit parser if the binary cannot be executed.
    Returns parsed JSON dict, or {"error": "..."} on failure.
    """
    repo_dir = os.path.join(REPOS_DIR, repo_name) if repo_name else BASE_DIR
    args = [MINIGIT] + cmd_args + ["--json"]
    if repo_name:
        args += ["--repo", repo_dir]

    try:
        result = subprocess.run(
            args,
            capture_output=True,
            text=True,
            timeout=10
        )
        output = result.stdout.strip()
        if output:
            return json.loads(output)
    except Exception:
        pass

    return _parse_minigit_fallback(cmd_args, repo_dir)


def list_repos():
    if not os.path.isdir(REPOS_DIR):
        return []
    return [
        d for d in os.listdir(REPOS_DIR)
        if os.path.isdir(os.path.join(REPOS_DIR, d, ".minigit"))
    ]


def get_all_commits(repo_name):
    """
    Read all commits from .minigit/commits.dat in chronological order.
    Ensures all branches and branching history are included in visualizer and commit details.
    """
    minigit_path = os.path.join(REPOS_DIR, repo_name, ".minigit")
    commits_file = os.path.join(minigit_path, "commits.dat")
    commits = []
    if os.path.isfile(commits_file):
        with open(commits_file, encoding="utf-8", errors="replace") as f:
            for line in f:
                parts = line.strip().split("|")
                if len(parts) >= 4:
                    try:
                        cid = int(parts[0])
                        pid = int(parts[1])
                        ts = int(parts[2])
                        msg = "|".join(parts[3:])
                        commits.append({
                            "id": cid,
                            "parentId": pid,
                            "timestamp": ts,
                            "message": msg
                        })
                    except ValueError:
                        pass
    if not commits:
        commits = run(["log"], repo_name=repo_name).get("commits", [])
    return commits


# ── routes ────────────────────────────────────────────────────────────────────

@app.route("/")
def index():
    repos = list_repos()
    return render_template("index.html", repos=repos)


@app.route("/repo/<name>")
def repo(name):
    log_data      = run(["log"], repo_name=name)
    branches_data = run(["branches"], repo_name=name)
    commits  = log_data.get("commits", [])
    branches = branches_data.get("branches", [])
    return render_template("repo.html",
                           name=name,
                           commits=commits,
                           branches=branches)


@app.route("/repo/<name>/commit/<int:cid>")
def commit_detail(name, cid):
    commits  = get_all_commits(name)

    # find this commit and its parent
    target = next((c for c in commits if c["id"] == cid), None)
    if not target:
        return render_template("error.html", message=f"Commit {cid} not found"), 404

    parent_id = target.get("parentId", -1)
    diff_lines = []

    if parent_id != -1:
        # find all files in this commit's snapshot and diff each one
        snapshot_path = os.path.join(REPOS_DIR, name, ".minigit", "snapshots", str(cid))
        if os.path.isdir(snapshot_path):
            for fname in os.listdir(snapshot_path):
                diff_data = run(["diff", str(parent_id), str(cid), fname], repo_name=name)
                file_diff = diff_data.get("diff", [])
                if file_diff:
                    diff_lines.append({"file": fname, "lines": file_diff})

    return render_template("commit.html",
                           name=name,
                           commit=target,
                           diff_lines=diff_lines)


@app.route("/repo/<name>/branches")
def branches(name):
    branches_data = run(["branches"], repo_name=name)
    branches = branches_data.get("branches", [])
    return render_template("branches.html", name=name, branches=branches)


@app.route("/repo/<name>/visualizer")
def visualizer(name):
    commits       = get_all_commits(name)
    branches_data = run(["branches"], repo_name=name)
    branches      = branches_data.get("branches", [])

    # undo stack: read undo.dat directly
    undo_stack = []
    undo_path = os.path.join(REPOS_DIR, name, ".minigit", "undo.dat")
    if os.path.isfile(undo_path):
        with open(undo_path) as f:
            for line in f:
                line = line.strip()
                if "|" in line:
                    parts = line.split("|")
                    undo_stack.append({"branch": parts[0], "headId": int(parts[1])})

    return render_template("visualizer.html",
                           name=name,
                           commits=commits,
                           branches=branches,
                           undo_stack=undo_stack)


# ── API endpoints (used by visualizer JS) ─────────────────────────────────────

@app.route("/api/<name>/log")
def api_log(name):
    if request.args.get("all") == "1":
        return jsonify({"commits": get_all_commits(name)})
    return jsonify(run(["log"], repo_name=name))


@app.route("/api/<name>/commits")
def api_commits(name):
    return jsonify({"commits": get_all_commits(name)})


@app.route("/api/<name>/branches")
def api_branches(name):
    return jsonify(run(["branches"], repo_name=name))


@app.route("/api/<name>/diff/<int:id1>/<int:id2>/<path:fname>")
def api_diff(name, id1, id2, fname):
    return jsonify(run(["diff", str(id1), str(id2), fname], repo_name=name))


@app.route("/api/<name>/undo")
def api_undo(name):
    undo_stack = []
    undo_path = os.path.join(REPOS_DIR, name, ".minigit", "undo.dat")
    if os.path.isfile(undo_path):
        with open(undo_path, encoding="utf-8", errors="replace") as f:
            for line in f:
                line = line.strip()
                if "|" in line:
                    parts = line.split("|")
                    if len(parts) >= 2:
                        try:
                            undo_stack.append({"branch": parts[0], "headId": int(parts[1])})
                        except ValueError:
                            pass
    return jsonify({"undo_stack": undo_stack})


# ── push endpoint ─────────────────────────────────────────────────────────────

@app.route("/repo/<name>/push", methods=["POST"])
def push(name):
    if "file" not in request.files:
        return jsonify({"error": "no file uploaded"}), 400

    zfile = request.files["file"]
    if not zfile.filename.endswith(".zip"):
        return jsonify({"error": "must be a .zip file"}), 400

    # save zip to temp location
    tmp_zip  = os.path.join(REPOS_DIR, f"_tmp_{name}.zip")
    tmp_dir  = os.path.join(REPOS_DIR, f"_tmp_{name}")
    dest_dir = os.path.join(REPOS_DIR, name)

    try:
        zfile.save(tmp_zip)

        # unzip
        with zipfile.ZipFile(tmp_zip, "r") as z:
            z.extractall(tmp_dir)

        # validate: must have commits.dat
        commits_path = os.path.join(tmp_dir, ".minigit", "commits.dat")
        if not os.path.isfile(commits_path):
            return jsonify({"error": "invalid repo: missing commits.dat"}), 400

        # fast-forward / preservation check if repo already exists
        if os.path.isdir(dest_dir):
            existing_log = run(["log"], repo_name=name)
            existing_commits = existing_log.get("commits", [])

            is_force = request.form.get("force") == "true" or request.args.get("force") == "true"

            if existing_commits and not is_force:
                server_head = existing_commits[0]["id"]

                # read pushed commits
                pushed_commits = {}
                with open(commits_path) as f:
                    for line in f:
                        parts = line.strip().split("|")
                        if len(parts) >= 2:
                            try:
                                cid = int(parts[0])
                                pid = int(parts[1])
                                pushed_commits[cid] = pid
                            except ValueError:
                                pass

                # If server_head is preserved in pushed_commits (e.g. on another branch), push is valid!
                if pushed_commits and (server_head not in pushed_commits):
                    # Check if server_head is an ancestor of any pushed commit
                    found = False
                    for p_head in pushed_commits.keys():
                        current = p_head
                        seen = set()
                        while current != -1 and current not in seen:
                            if current == server_head:
                                found = True
                                break
                            seen.add(current)
                            current = pushed_commits.get(current, -1)
                        if found:
                            break

                    if not found:
                        return jsonify({"error": "not a fast-forward push. Use 'python push.py <repo> --force' to overwrite."}), 409

        # replace existing repo
        if os.path.isdir(dest_dir):
            shutil.rmtree(dest_dir)

        # move temp to final destination
        shutil.move(tmp_dir, dest_dir)
        return jsonify({"status": "ok", "message": f"repo '{name}' updated"})

    except Exception as e:
        return jsonify({"error": str(e)}), 500
    finally:
        if os.path.isfile(tmp_zip):
            os.remove(tmp_zip)
        if os.path.isdir(tmp_dir):
            shutil.rmtree(tmp_dir)


if __name__ == "__main__":
    app.run(debug=True, port=5000)