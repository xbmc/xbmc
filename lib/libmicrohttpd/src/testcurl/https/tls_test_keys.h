/*
     This file is part of libmicrohttpd
     (C) 2006, 2007, 2008 Christian Grothoff (and other contributing authors)

     This library is free software; you can redistribute it and/or
     modify it under the terms of the GNU Lesser General Public
     License as published by the Free Software Foundation; either
     version 2.1 of the License, or (at your option) any later version.

     This library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     Lesser General Public License for more details.

     You should have received a copy of the GNU Lesser General Public
     License along with this library; if not, write to the Free Software
     Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef MHD_TLS_TEST_KEYS_H
#define MHD_TLS_TEST_KEYS_H

/* Test Certificates */

/* Certificate Authority key */
const char ca_key_pem[] = "-----BEGIN RSA PRIVATE KEY-----\n"
  "MIIEowIBAAKCAQEA0EdlP613rjFvEj93tGo9fzBoKWU3CW+AbbfcJ397C89MyZ9J\n"
  "rlxyLGfa6qVX7CFVNmzgWWfcl2tHlw/fZmWtf/SFgrlkldvuGyY8H3n2HuMsWz/E\n"
  "h7n5VgwBX8NsP4eZNmikepxpr1mYx25K8FjnsKjAR9jGUSV8UfZ7VLIY0x/yqe+3\n"
  "32oqc4D/wJbV1AwwvC5Xf9rvHJwcZg57eqbDCL/4GDDk7d9Gark4XK6ZG+FnnxQn\n"
  "4a4jIdf4FoPp9s0EieHrHwYzs/uBqmfCSF4wXiaO8bmEwtbAsVbZH74Le7ggUbEe\n"
  "o+jan9XK0dE88AIImGzgoBnlic/Rr7J8OWA+iwIDAQABAoIBAEICZqXkUdpsw2F6\n"
  "qPMOergNPO3lrKg6ZO8hBs6j2fj3tcPuzljK5sqJDboxNejZ9Zo+rmnXf3Oj5fgL\n"
  "6UcYMYEsm4W/QRA3uEJ1fzeQnT7Ty9KNprlHaSzquCLEGlIWJSo3xu0vFlWjJUcL\n"
  "fwemfaOhD/OVUeEU6s5FOngwy6pZUsOajs3fNRtwBGuuXjniKZZlpSf2Wqu3xpHZ\n"
  "31OF1V0ycUCGPPFtpmUCtnZhS9L8QBTkNtfTIdXv6SfoBRFm0oXb0uL5HGft6yc7\n"
  "eYRXIscllQciqG3ymJ/y9o0E3A0YsBVauQyi7OEk+Kg8uoYOBkZCIY69hoN2Znlk\n"
  "OY5S5Z0CgYEA3j8pRAJzvc827KcX4vJf05HYD4aCyaI80fNmx1DgXfglTSGLQ361\n"
  "6i05YW8WtIvgkma3wF+jJOckBCW/7iq8wAX7Kz75WKGRyyTEb0wSfjx0G8grxX4d\n"
  "7sTIAAOnQj5WT6E/bkqxQZAYnVtIPxKtSlwts0H/bjPVYwSFchHK7t8CgYEA7+ks\n"
  "C0EMjF8CDeCfvbOUGiiqAvU3G20LEC3WlJM3AU+J9Jzp6AMkgaIA8J5oNdsbFBn4\n"
  "N12JPOO+7WRUk6Av8bsh4faE36ThnHohgAL8guRU7jIXvsFyO5yiY7/o/0lES0/V\n"
  "6xkh/Epj4MReuCGkiD9ifCVAo+dhHskeE9qbYdUCgYA4yBpa7eV0UUTPIcHQkew5\n"
  "ucFh9hPkQDcZzP4tXlR0rbmaAz/5dp4zvmoyopdCeZpezS+VTtn3y7Y/+QUYbILc\n"
  "7KpHWkeKhX0iUbp+VQlEh12C25mTU62CG3SdzFEnc5XJsoDqRNsUzSP80B2dP8BW\n"
  "h0aFzg7csRGLwtP1WOZoMQKBgQCrgsKd+Q8Dexh421DXyX3jhZalLrEKxlXWZy60\n"
  "YNo98aLqYRNHbpe2pR6O5nARsGYXZMlyq0flY9um0sc0Epyz79g1NoufZrxzpUw1\n"
  "u+zRlnKxJtaa5KjJvRzKuvPTLYnJXXXM8Na/Cl+E3F3qvQJm9QlvPyKLCmsAGz+J\n"
  "agsTUQKBgC0wqqJ6b1tbrAD8AVeeAn/IiP1rxYpc3x2s6ikFO2FMHXHC9wgrRPOc\n"
  "mkokV+DrUOv3I/7jG8wQA/FmBUPy562a1bObIKzg6CPXzrN68AmNnOIVU+H8fdxI\n"
  "iGyfT8WNpcRmtN11v34qXHwOWGQhpyyk2yNa8VIBSpkShq/EseZ1\n"
  "-----END RSA PRIVATE KEY-----\n";

/* Certificate Authority cert */
const char ca_cert_pem[] = "-----BEGIN CERTIFICATE-----\n"
  "MIIC6DCCAdKgAwIBAgIES0KCvTALBgkqhkiG9w0BAQUwFzEVMBMGA1UEAxMMdGVz\n"
  "dF9jYV9jZXJ0MB4XDTEwMDEwNTAwMDcyNVoXDTQ1MDMxMjAwMDcyNVowFzEVMBMG\n"
  "A1UEAxMMdGVzdF9jYV9jZXJ0MIIBHzALBgkqhkiG9w0BAQEDggEOADCCAQkCggEA\n"
  "0EdlP613rjFvEj93tGo9fzBoKWU3CW+AbbfcJ397C89MyZ9JrlxyLGfa6qVX7CFV\n"
  "NmzgWWfcl2tHlw/fZmWtf/SFgrlkldvuGyY8H3n2HuMsWz/Eh7n5VgwBX8NsP4eZ\n"
  "Nmikepxpr1mYx25K8FjnsKjAR9jGUSV8UfZ7VLIY0x/yqe+332oqc4D/wJbV1Aww\n"
  "vC5Xf9rvHJwcZg57eqbDCL/4GDDk7d9Gark4XK6ZG+FnnxQn4a4jIdf4FoPp9s0E\n"
  "ieHrHwYzs/uBqmfCSF4wXiaO8bmEwtbAsVbZH74Le7ggUbEeo+jan9XK0dE88AII\n"
  "mGzgoBnlic/Rr7J8OWA+iwIDAQABo0MwQTAPBgNVHRMBAf8EBTADAQH/MA8GA1Ud\n"
  "DwEB/wQFAwMHBAAwHQYDVR0OBBYEFP2olB4s2T/xuoQ5pT2RKojFwZo2MAsGCSqG\n"
  "SIb3DQEBBQOCAQEAebD5m+vZkVXa8y+QZ5GtsiR9gpH+LKtdWBjk1kmfSgvQI/xA\n"
  "aDCV/9BhdNGIBOTYGkln8urWd7g2Mj3TwKEAfNTUFpAsrBAlSSLTGYCSt72S2NsS\n"
  "L/qUxmj1W6X95UHXCo49mSZx3LlaY3mz1L87gq/kK0XpzA3g2uF25jt84RvshsXy\n"
  "clOc+eRrVETqFZqer96WB7kzFTv+qmROQKmW8X4a2A5r5Jl4vRwOz5/rEeB9Qs0K\n"
  "rmK8+5HgvWd80WB8BtfFtZfoY/hHVM8nLD3ELVJrOKiTeIACunQFyT5lV0QkdmSA\n"
  "CGInU7jzs8nu+s2avf6j+eVZUbVJ+dFMApTJgg==\n"
  "-----END CERTIFICATE-----\n";

/* test server CA signed certificates */
const char srv_signed_cert_pem[] = "-----BEGIN CERTIFICATE-----\n"
  "MIIDGzCCAgWgAwIBAgIES0KCvTALBgkqhkiG9w0BAQUwFzEVMBMGA1UEAxMMdGVz\n"
  "dF9jYV9jZXJ0MB4XDTEwMDEwNTAwMDcyNVoXDTQ1MDMxMjAwMDcyNVowFzEVMBMG\n"
  "A1UEAxMMdGVzdF9jYV9jZXJ0MIIBHzALBgkqhkiG9w0BAQEDggEOADCCAQkCggEA\n"
  "vfTdv+3fgvVTKRnP/HVNG81cr8TrUP/iiyuve/THMzvFXhCW+K03KwEku55QvnUn\n"
  "dwBfU/ROzLlv+5hotgiDRNFT3HxurmhouySBrJNJv7qWp8ILq4sw32vo0fbMu5BZ\n"
  "F49bUXK9L3kW2PdhTtSQPWHEzNrCxO+YgCilKHkY3vQNfdJ020Q5EAAEseD1YtWC\n"
  "IpRvJzYlZMpjYB1ubTl24kwrgOKUJYKqM4jmF4DVQp4oOK/6QYGGh1QmHRPAy3CB\n"
  "II6sbb+sZT9cAqU6GYQVB35lm4XAgibXV6KgmpVxVQQ69U6xyoOl204xuekZOaG9\n"
  "RUPId74Rtmwfi1TLbBzo2wIDAQABo3YwdDAMBgNVHRMBAf8EAjAAMBMGA1UdJQQM\n"
  "MAoGCCsGAQUFBwMBMA8GA1UdDwEB/wQFAwMHIAAwHQYDVR0OBBYEFOFi4ilKOP1d\n"
  "XHlWCMwmVKr7mgy8MB8GA1UdIwQYMBaAFP2olB4s2T/xuoQ5pT2RKojFwZo2MAsG\n"
  "CSqGSIb3DQEBBQOCAQEAHVWPxazupbOkG7Did+dY9z2z6RjTzYvurTtEKQgzM2Vz\n"
  "GQBA+3pZ3c5mS97fPIs9hZXfnQeelMeZ2XP1a+9vp35bJjZBBhVH+pqxjCgiUflg\n"
  "A3Zqy0XwwVCgQLE2HyaU3DLUD/aeIFK5gJaOSdNTXZLv43K8kl4cqDbMeRpVTbkt\n"
  "YmG4AyEOYRNKGTqMEJXJoxD5E3rBUNrVI/XyTjYrulxbNPcMWEHKNeeqWpKDYTFo\n"
  "Bb01PCthGXiq/4A2RLAFosadzRa8SBpoSjPPfZ0b2w4MJpReHqKbR5+T2t6hzml6\n"
  "4ToyOKPDmamiTuN5KzLN3cw7DQlvWMvqSOChPLnA3Q==\n"
  "-----END CERTIFICATE-----\n";

/* test server key */
const char srv_signed_key_pem[] = "-----BEGIN RSA PRIVATE KEY-----\n"
  "MIIEowIBAAKCAQEAvfTdv+3fgvVTKRnP/HVNG81cr8TrUP/iiyuve/THMzvFXhCW\n"
  "+K03KwEku55QvnUndwBfU/ROzLlv+5hotgiDRNFT3HxurmhouySBrJNJv7qWp8IL\n"
  "q4sw32vo0fbMu5BZF49bUXK9L3kW2PdhTtSQPWHEzNrCxO+YgCilKHkY3vQNfdJ0\n"
  "20Q5EAAEseD1YtWCIpRvJzYlZMpjYB1ubTl24kwrgOKUJYKqM4jmF4DVQp4oOK/6\n"
  "QYGGh1QmHRPAy3CBII6sbb+sZT9cAqU6GYQVB35lm4XAgibXV6KgmpVxVQQ69U6x\n"
  "yoOl204xuekZOaG9RUPId74Rtmwfi1TLbBzo2wIDAQABAoIBADu09WSICNq5cMe4\n"
  "+NKCLlgAT1NiQpLls1gKRbDhKiHU9j8QWNvWWkJWrCya4QdUfLCfeddCMeiQmv3K\n"
  "lJMvDs+5OjJSHFoOsGiuW2Ias7IjnIojaJalfBml6frhJ84G27IXmdz6gzOiTIer\n"
  "DjeAgcwBaKH5WwIay2TxIaScl7AwHBauQkrLcyb4hTmZuQh6ArVIN6+pzoVuORXM\n"
  "bpeNWl2l/HSN3VtUN6aCAKbN/X3o0GavCCMn5Fa85uJFsab4ss/uP+2PusU71+zP\n"
  "sBm6p/2IbGvF5k3VPDA7X5YX61sukRjRBihY8xSnNYx1UcoOsX6AiPnbhifD8+xQ\n"
  "Tlf8oJUCgYEA0BTfzqNpr9Wxw5/QXaSdw7S/0eP5a0C/nwURvmfSzuTD4equzbEN\n"
  "d+dI/s2JMxrdj/I4uoAfUXRGaabevQIjFzC9uyE3LaOyR2zhuvAzX+vVcs6bSXeU\n"
  "pKpCAcN+3Z3evMaX2f+z/nfSUAl2i4J2R+/LQAWJW4KwRky/m+cxpfUCgYEA6bN1\n"
  "b73bMgM8wpNt6+fcmS+5n0iZihygQ2U2DEud8nZJL4Nrm1dwTnfZfJBnkGj6+0Q0\n"
  "cOwj2KS0/wcEdJBP0jucU4v60VMhp75AQeHqidIde0bTViSRo3HWKXHBIFGYoU3T\n"
  "LyPyKndbqsOObnsFXHn56Nwhr2HLf6nw4taGQY8CgYBoSW36FLCNbd6QGvLFXBGt\n"
  "2lMhEM8az/K58kJ4WXSwOLtr6MD/WjNT2tkcy0puEJLm6BFCd6A6pLn9jaKou/92\n"
  "SfltZjJPb3GUlp9zn5tAAeSSi7YMViBrfuFiHObij5LorefBXISLjuYbMwL03MgH\n"
  "Ocl2JtA2ywMp2KFXs8GQWQKBgFyIVv5ogQrbZ0pvj31xr9HjqK6d01VxIi+tOmpB\n"
  "4ocnOLEcaxX12BzprW55ytfOCVpF1jHD/imAhb3YrHXu0fwe6DXYXfZV4SSG2vB7\n"
  "IB9z14KBN5qLHjNGFpMQXHSMek+b/ftTU0ZnPh9uEM5D3YqRLVd7GcdUhHvG8P8Q\n"
  "C9aXAoGBAJtID6h8wOGMP0XYX5YYnhlC7dOLfk8UYrzlp3xhqVkzKthTQTj6wx9R\n"
  "GtC4k7U1ki8oJsfcIlBNXd768fqDVWjYju5rzShMpo8OCTS6ipAblKjCxPPVhIpv\n"
  "tWPlbSn1qj6wylstJ5/3Z+ZW5H4wIKp5jmLiioDhcP0L/Ex3Zx8O\n"
  "-----END RSA PRIVATE KEY-----\n";

/* test server self signed certificates */
const char srv_self_signed_cert_pem[] = "-----BEGIN CERTIFICATE-----\n"
  "MIIC+jCCAeSgAwIBAgIES0KCvTALBgkqhkiG9w0BAQUwFzEVMBMGA1UEAxMMdGVz\n"
  "dF9jYV9jZXJ0MB4XDTEwMDEwNTAwMDcyNVoXDTQ1MDMxMjAwMDcyNVowFzEVMBMG\n"
  "A1UEAxMMdGVzdF9jYV9jZXJ0MIIBHzALBgkqhkiG9w0BAQEDggEOADCCAQkCggEA\n"
  "tDEagv3p9OUhUL55jMucxjNK9N5cuozhcnrwDfBSU6oVrqm5kPqO1I7Cggzw68Y5\n"
  "jhTcBi4FXmYOZppm1R3MhSJ5JSi/67Q7X4J5rnJLXYGN27qjMpnoGQ/2xmsNG/is\n"
  "i+h/2vbtPU+WP9SEJnTfPLLpZ7KqCAk7FUUzKsuLx3/SOKtdkrWxPKwYTgnDEN6D\n"
  "JL7tEzCnG5DFc4mQ7YW9PaRdC3rS1T8PvQ3jB2BUnohM0cFvKRuiU35tU7h7CPbL\n"
  "4L66VglXoiwqmgcrwI2U968bD0+wRQ5c5bzNoshJOzN6CTMh1IhbklSh/Z6FA/e8\n"
  "hj0yVo2tdllXuJGVs3PIEwIDAQABo1UwUzAMBgNVHRMBAf8EAjAAMBMGA1UdJQQM\n"
  "MAoGCCsGAQUFBwMBMA8GA1UdDwEB/wQFAwMHIAAwHQYDVR0OBBYEFDfU7pAv9LYn\n"
  "n7jb4WHl4+Vgi2FnMAsGCSqGSIb3DQEBBQOCAQEAkaembPQMmv6OOjbIod8zTatr\n"
  "x5Bwkwp3TOE1NRyy2OytzFIYRUkNrZYlcmrxcbNNycIK41CNVXbriFCF8gcmIq9y\n"
  "vaKZn8Gcy+vGggv+1BP9IAPBGKRwSi0wmq9JoGE8hx+qqTpRSdfbM/cps/09hicO\n"
  "0EIR7kWEbvnpMBcMKYOtYE9Gce7rdSMWVAsKc174xn8vW6TxCUvmWFv5DPg5HG1v\n"
  "y1SUX73qafRo+W6FN4UC/DHfwRhF8RSKEnVbmgDVCs6GHdKBjU2qRgYyj6nWZqK1\n"
  "XFUTWgia+Fl3D9vlsXaFcSZKA0Bq1eojl0B0AfeYAxTFwPWXscKvt/bXZfH8bg==\n"
  "-----END CERTIFICATE-----\n";

/* test server key */
const char srv_key_pem[] = "-----BEGIN RSA PRIVATE KEY-----\n"
  "MIIEpAIBAAKCAQEAtDEagv3p9OUhUL55jMucxjNK9N5cuozhcnrwDfBSU6oVrqm5\n"
  "kPqO1I7Cggzw68Y5jhTcBi4FXmYOZppm1R3MhSJ5JSi/67Q7X4J5rnJLXYGN27qj\n"
  "MpnoGQ/2xmsNG/isi+h/2vbtPU+WP9SEJnTfPLLpZ7KqCAk7FUUzKsuLx3/SOKtd\n"
  "krWxPKwYTgnDEN6DJL7tEzCnG5DFc4mQ7YW9PaRdC3rS1T8PvQ3jB2BUnohM0cFv\n"
  "KRuiU35tU7h7CPbL4L66VglXoiwqmgcrwI2U968bD0+wRQ5c5bzNoshJOzN6CTMh\n"
  "1IhbklSh/Z6FA/e8hj0yVo2tdllXuJGVs3PIEwIDAQABAoIBAAEtcg+LFLGtoxjq\n"
  "b+tFttBJfbRcfdG6ocYqBGmUXF+MgFs573DHX3sHNOQxlaNHtSgIclF1eYgNZFFt\n"
  "VLIoBFTzfEQXoFosPUDoEuqVMeXLttmD7P2jwL780XJLZ4Xj6GY07npq1iGBcEZf\n"
  "yCcdoyGkr9jgc5Auyis8DStGg/jfUBC4NBvF0GnuuNPAdYRPKUpKw9EatI+FdMjy\n"
  "BuroD90fhdkK8EwMEVb9P17bdIc1MCIZFpUE9YHjVdK/oxCUhQ8KRfdbI4JU5Zh3\n"
  "UtO6Jm2wFuP3VmeVpPvE/C2rxI70pyl6HMSiFGNc0rhJYCQ+yhohWj7nZ67H4vLx\n"
  "plv5LxkCgYEAz7ewou8oFafDAMNoxaqKudvUg+lxXewdLDKaYBF5ACi9uAPCJ+v7\n"
  "M5c/fvPFn/XHzo7xaXbtTAH3Z5xzBs+80OsvL+e1Ut4xR+ELRkybknh/s2wQeABk\n"
  "Kb0vA59ukQGj12LV5phZMaVoXe6KJ7hZnN62d3K6m1wGE/k58i4pPLUCgYEA3hN8\n"
  "G95zW7g0jVdSr+KUeVmephph9yh8Yb+3I3ojwOIv6d45TopGx8pFZlnBAMZf1ZQx\n"
  "DIhzJNnaqZy/4w7RNaOGWnPA/5f+MIoHBiLGEEmfHC3lt087Yp9OuwDUHwpETYdV\n"
  "o+KBCvVh60Et3bZUgF/1k/3YXxn8J5dsmJsjNqcCgYBLflyRa1BrRnTGMz9CEDCp\n"
  "Si9b3h1Y4Hbd2GppHhCXMTd6yMrpDYhYANGQB3M9Juv+s88j4JhwNoq/uonH4Pqk\n"
  "B8Y3qAQr4RuSH0WkwDUOsALhqBX4N1QwI1USAQEDbNAqeP5698X7GD3tXcQSmZrg\n"
  "O8WfdjBCRNjkq4EW9xX/vQKBgQDONtmwJ0iHiu2BseyeVo/4fzfKlgUSNQ4K1rOA\n"
  "xhIdMeu8Bxa/z7caHsGC4SVPSuYCtbE2Kh6BwapChcPJXCD45fgEViiJLuJiwEj1\n"
  "caTpyvNsf1IoffJvCe9ZxtMyX549P8ZOgC3Dt0hN5CBrGLwu2Ox5l+YrqT10pi+5\n"
  "JZX1UQKBgQCrcXrdkkDAc/a4+PxNRpJRLcU4fhv8/lr+UWItE8eUe7bd25bTQfQm\n"
  "VpNKc/kAJ66PjIED6fy3ADhd2y4naT2a24uAgQ/M494J68qLnGh6K4JU/09uxR2v\n"
  "1i2q/4FNLdFFk1XP4iNnTHRLZ+NYr2p5Y9RcvQfTjOauz8Ahav0lyg==\n"
  "-----END RSA PRIVATE KEY-----\n";

#endif
