#!/usr/bin/perl
# escape_sjis.pl

    binmode(STDIN);
    binmode(STDOUT);

    while (read(STDIN, $buf, 1))
    {
        if ($sjis)
        {
            $sjis = 0;

            if ($buf eq "\\")
            {
                print("\\");
            }
        }
        elsif ((((ord($buf) ^ 0x20) - 0xA1) & 0xFF) < 0x3C)
        {
            $sjis = !0;
        }

        print($buf);
    }

# [EOF]

