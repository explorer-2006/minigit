"""
push.py — upload the local .minigit/ folder to the MiniGit Flask server

Usage:
    python push.py <repo-name> [server-url]

Example:
    python push.py calculator
    python push.py calculator http://192.168.1.10:5000
"""

import sys
import os
import zipfile
import tempfile
import requests

DEFAULT_SERVER = "http://127.0.0.1:5000"


def push(repo_name, server_url, force=False):
    minigit_dir = ".minigit"
    if not os.path.isdir(minigit_dir):
        print("Error: no .minigit folder found. Run 'minigit init' first.")
        sys.exit(1)

    # create a temp zip of .minigit/
    with tempfile.NamedTemporaryFile(suffix=".zip", delete=False) as tmp:
        tmp_path = tmp.name

    with zipfile.ZipFile(tmp_path, "w", zipfile.ZIP_DEFLATED) as zf:
        for root, dirs, files in os.walk(minigit_dir):
            for file in files:
                abs_path = os.path.join(root, file)
                arcname  = os.path.relpath(abs_path, ".")
                zf.write(abs_path, arcname)

    print(f"Pushing '{repo_name}' to {server_url} ...")

    url = f"{server_url}/repo/{repo_name}/push"
    data = {"force": "true"} if force else {}
    with open(tmp_path, "rb") as f:
        response = requests.post(
            url,
            files={"file": (f"{repo_name}.zip", f, "application/zip")},
            data=data
        )

    os.remove(tmp_path)

    if response.ok:
        data = response.json()
        print(f"Success: {data.get('message', 'pushed')}")
    else:
        try:
            err = response.json().get("error", response.text)
        except Exception:
            err = response.text
        print(f"Error {response.status_code}: {err}")
        sys.exit(1)


if __name__ == "__main__":
    args = [a for a in sys.argv[1:] if a not in ("--force", "-f")]
    force = ("--force" in sys.argv) or ("-f" in sys.argv)

    if len(args) < 1:
        print("Usage: python push.py <repo-name> [server-url] [--force]")
        sys.exit(1)

    repo_name  = args[0]
    server_url = args[1] if len(args) >= 2 else DEFAULT_SERVER
    push(repo_name, server_url, force=force)