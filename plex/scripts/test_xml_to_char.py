import sys
import requests

if __name__ == '__main__':
  if len(sys.argv) < 2:
    print "need url"
    sys.exit(1)

  r = requests.get(sys.argv[1])
  r.encoding = "utf-8"
  data = r.text

  for line in data.split("\n"):
    line = line.replace("\"", "\\\"")
    print "\"%s\"" % line.encode("utf-8")

