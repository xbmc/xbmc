#!/usr/bin/python3

import argparse
import glob
import json
import os.path
import pprint
import shutil
import sys

import requests
import tarfile

from dotenv import load_dotenv

from github import Github

addon_repo_base = "https://github.com/xbmc/repo-binary-addons"
addon_repo_branch = "Omega"
addon_repo_dir = "binary_addons_repo_tmp"
addon_repo_remote = "binary_addons_repo"

load_dotenv()

g = Github(os.environ["GITHUB_TOKEN"])


def get_current_github_rev(url, branch) -> str | None:
    repo = g.get_repo(url.split("/")[-2] + "/" + url.split("/")[-1].replace(".git", ""))

    try:
        branch_sha = repo.get_branch(branch).commit.sha
        return branch_sha
    except Exception as e:
        print("Error getting branch sha for {}: {}".format(branch, e))

    return None


def check_platform(def_file):
    ret = False
    platform_file = os.path.join(os.path.dirname(def_file), "platforms.txt")
    with open(platform_file, mode="r") as plat_file:
        platforms = plat_file.readline().split()
        if args.verbose:
            print("valid platforms for {}: {}".format(def_file, platforms))

        if ("all" in platforms or "linux" in platforms) and "!linux" not in platforms:
            return True
        for p in platforms:
            if p.startswith("!") and p != "!linux":
                ret = True
        return ret


def get_addon_definition(def_name, def_file) -> str:
    if args.verbose:
        print("get_addon_definition from", f)

    if not check_platform(def_file):
        raise Exception("platform mismatch")

    with open(def_file, mode="r") as addon_def:
        a_name, a_url, a_rev = addon_def.readline().split()
        a_type = get_addon_type(a_url)

    if a_name != def_name or a_type == "unknown":
        raise Exception("addon_definition_error")
    if args.verbose:
        print(
            "found addon details - name: {}, url: {}, git_rev: {}, type: {}".format(
                a_name, a_url, a_rev, a_type
            )
        )

    if a_name != "" and a_url != "" and a_rev != "" and a_type != "":
        return a_name, a_url, a_rev, a_type

    raise Exception("addon_definition_error")


def get_addon_type(addon_url) -> str:
    if addon_url.endswith((".tar.gz", ".tar.xz", ".tar.bz2", ".zip")):
        return "archive"
    elif addon_url.startswith(("https://", "http://")) or addon_url.endswith(".git"):
        return "git"

    return "unknown"


def set_build_type(a_data):
    if "build-options" not in a_data:
        a_data["build-options"] = {}
    if "config-opts" not in a_data:
        a_data["config-opts"] = []
    if args.release:
        if "-DCMAKE_BUILD_TYPE=Release" not in a_data["config-opts"]:
            a_data["config-opts"].append("-DCMAKE_BUILD_TYPE=Release")
        a_data["build-options"]["no-debuginfo"] = True
        a_data["build-options"]["cflags"] = "-g0"
        a_data["build-options"]["cxxflags"] = "-g0"
    else:
        if "-DCMAKE_BUILD_TYPE=Release" in a_data["config-opts"]:
            a_data["config-opts"].remove("-DCMAKE_BUILD_TYPE=Release")
        a_data["build-options"]["no-debuginfo"] = False
        a_data["build-options"]["cflags"] = a_data["build-options"]["cflags"].replace(
            "-g0", ""
        )
        a_data["build-options"]["cxxflags"] = a_data["build-options"][
            "cxxflags"
        ].replace("-g0", "")
        if a_data["build-options"]["cflags"].strip() == "":
            del a_data["build-options"]["cflags"]
        if a_data["build-options"]["cxxflags"].strip() == "":
            del a_data["build-options"]["cxxflags"]

    return a_data


def update_addon_repo():
    if args.verbose:
        print("downloading binary addon repo")

    if os.path.isdir(addon_repo_dir):
        shutil.rmtree(addon_repo_dir)
    addon_repo_url = (
        addon_repo_base + "/archive/refs/heads/" + addon_repo_branch + ".tar.gz"
    )
    response = requests.get(addon_repo_url, stream=True)
    try:
        with tarfile.open(fileobj=response.raw, mode="r|gz") as tarball:
            tarball.extractall(addon_repo_dir)
    except tarfile.ReadError as e:
        print(
            "Error downloading repository tarball {}, did you specify an existing branch from {}?".format(
                addon_repo_url, addon_repo_base
            )
        )
        sys.exit(2)


### Main ###
pp = pprint.PrettyPrinter(indent=4)
parser = argparse.ArgumentParser()
parser.add_argument(
    "-u", "--update_repo", help="force updating binary addons repo", action="store_true"
)
parser.add_argument(
    "-v", "--verbose", help="enable verbose output", action="store_true"
)
parser.add_argument(
    "-b",
    "--branch",
    help="override repo branch (default: {})".format(addon_repo_branch),
)
parser.add_argument(
    "-r", "--release", help="enable release builds", action="store_true"
)
args = parser.parse_args()

if args.branch:
    addon_repo_branch = args.branch

if args.update_repo or not os.path.isdir(addon_repo_dir):
    update_addon_repo()

# find all available addons in repo
repo_file_list = glob.glob(
    addon_repo_dir + "/*-" + addon_repo_branch + "/*/*.txt", recursive=True
)
if len(repo_file_list) == 0:
    print("Warning: couldn't find any repository files, forcing repo update")
    update_addon_repo()
    repo_file_list = glob.glob(
        addon_repo_dir + "/*-" + addon_repo_branch + "/*/*.txt", recursive=True
    )

if args.verbose:
    print("{}: {}".format("addon_files", repo_file_list))

skipped_addons = {}
missing_addons = []

for f in repo_file_list:
    definition_file = os.path.basename(f)
    if definition_file == "platforms.txt":
        continue

    try:
        addon_id = definition_file.rsplit(".txt", maxsplit=1)[0]
        (name, url, rev, atype) = get_addon_definition(addon_id, f)
    except Exception as e:
        print(
            "Error parsing addon definition in {}: {} - skipping".format(
                definition_file, e
            )
        )
        skipped_addons[addon_id] = e.__str__()
        continue

    addon_json = os.path.join("../addons/", addon_id, addon_id + ".json")

    if not os.path.exists(addon_json):
        skipped_addons[addon_id] = "not found in existing flatpak addons"
        missing_addons.append(addon_id)
        continue

    print("updating", addon_id)

    # parse and update manifests
    with open(addon_json, mode="r+") as jf:
        addon_data = json.load(jf)
        if args.verbose:
            print(addon_data)

        if addon_data["name"] != name:
            print(
                "Error: skipping addon due to name mismatch: {} vs {}".format(
                    name, addon_data["name"]
                )
            )
            skipped_addons[addon_id] = "name mismatch: {} vs {}".format(
                name, addon_data["name"]
            )
            continue

        addon_data = set_build_type(addon_data)

        i = 0
        while i < len(addon_data["sources"]) and addon_data["sources"][i][
            "type"
        ] not in ["git", "archive"]:
            i = i + 1

        addon_data["sources"][i]["url"] = url
        addon_data["sources"][i]["type"] = atype

        if get_current_github_rev(url, rev) and (
            addon_repo_branch == rev or rev == "master"
        ):
            addon_data["sources"][i]["commit"] = get_current_github_rev(url, rev)
        else:
            addon_data["sources"][i]["commit"] = get_current_github_rev(url, "master")

        if "tag" in addon_data["sources"][i]:
            del addon_data["sources"][i]["tag"]

        # save file
        jf.seek(0)
        jf.write(json.dumps(addon_data, indent=4))
        jf.write("\n")
        jf.truncate()

print("\n\n### DONE ###\nskipped addons:")
pp.pprint(skipped_addons)
print("\nmissing addons:")
pp.pprint(missing_addons)
