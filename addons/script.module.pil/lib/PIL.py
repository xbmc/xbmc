import imp
import site

fp, path, descr = imp.find_module('PIL', site.getsitepackages())
try:
   imp.load_module('PIL', fp, path, descr)
finally:
    if fp:
        fp.close()

