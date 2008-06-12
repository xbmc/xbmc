#! /usr/bin/python2.2

def run_trial():
    # import tdbutil
    from samba.tdbpack import pack
    
    for i in xrange(500000):
        pack("ddffd", (10, 2, "mbp", "martin", 0))
        #s = "\n\0\0\0" + "\x02\0\0\0" + "mbp\0" + "martin\0" + "\0\0\0\0"

if __name__ == '__main__':
    run_trial()
