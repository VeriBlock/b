#!/usr/bin/env python3

import subprocess, os

initialCommit = "HEAD~"
if "VBK_FORK_COMMIT" in os.environ:
    initialCommit = os.environ["VBK_FORK_COMMIT"]

subprocess.run(["git", "reset", "--soft", initialCommit])
subprocess.run(["git", "add", "--all", "."])
subprocess.run(["git", "rm", "--cached", "-r", "build_msvc"])
subprocess.run(["git", "commit", "-m", "Auto squash commits from master since " + initialCommit])
subprocess.run(["git", "add", "build_msvc"])
subprocess.run(["git", "commit", "-m", "Auxiliary changes"])
