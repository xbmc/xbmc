"""Build the C client docs.
"""

from __future__ import with_statement
import os
import shutil
import socket
import subprocess
import time
import urllib2

def clean_dir(dir):
    try:
        shutil.rmtree(dir)
    except:
        pass
    os.makedirs(dir)

def gen_api(dir):
    clean_dir(dir)
    clean_dir("docs/source/doxygen")

    with open(os.devnull, 'w') as null:
        subprocess.call(["doxygen", "doxygenConfig"], stdout=null, stderr=null)

    os.rename("docs/source/doxygen/html", dir)

def gen_sphinx(dir):
    clean_dir(dir)
    os.chdir("docs/source/sphinx")

    with open(os.devnull, 'w') as null:
        subprocess.call(["make", "html"], stdout=null, stderr=null)

    os.chdir("../../../")
    if os.path.isdir("docs/source/sphinx/build/html"):
        os.rename("docs/source/sphinx/build/html", dir)

def version():
    """Get the driver version from doxygenConfig.
    """
    with open("doxygenConfig") as f:
        for line in f.readlines():
            if line.startswith("PROJECT_NUMBER"):
                return line.split("=")[1].strip()


def main():
    print("Generating Sphinx docs in docs/html")
    gen_sphinx("docs/html")
    print("Generating Doxygen docs in docs/html/api")
    gen_api("docs/html/api")


if __name__ == "__main__":
    main()

