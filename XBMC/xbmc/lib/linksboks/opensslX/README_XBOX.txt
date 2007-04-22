This is a distribution of OpenSSL 0.9.7d that compiles on Xbox
as a static lib. Doesn't use Makefiles like the original (it's a VS.net
"project") but had to make a prebuild.bat batch file to do some copy's :)

Note that the PRNG is initialized with random bytes we get from XNetRandom.

From the XDK documentation:
"XNetRandom uses an algorithm for generating random bits based on RFC 1750
(Randomness Recommendations for Security; see
http://www.faqs.org/rfcs/rfc1750.html). Randomness is not based on predictable
pseudo-random mathematical number generators. Instead, randomness is generated
using a combination of random input sources, including physical sources of
entropy such as hard disk seek time."

Seem cool enough ;)


Feel free to do whatever you want with it. Just respect the original
OpenSSL license (check out the LICENSE file).

Cheers,

-- 
ysbox <ysbox@online.fr>
