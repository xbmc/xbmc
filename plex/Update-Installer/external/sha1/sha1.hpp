/* public api for steve reid's public domain SHA-1 implementation */
/* this file is in the public domain */
/* modified/c++ified by Kristoffer Gronlund */

#ifndef __SHA1_H
#define __SHA1_H

#include <stdint.h>
#include <string.h>
#include <string>

#ifdef _MSC_VER
#define snprintf _snprintf
#else
#include <stdio.h>
#endif

struct SHA1 {
    static const uint32_t SHA1_DIGEST_SIZE = 20;

    struct digest {
        uint32_t _data32[5];
        uint8_t _data8[SHA1_DIGEST_SIZE];

        uint8_t& operator[](int idx) {
            return _data8[idx];
        }

        const uint8_t& operator[](int idx) const {
            return _data8[idx];
        }

        operator const uint8_t* () const {
            return _data8;
        }

        bool operator==(const digest& o) const {
            return memcmp(_data8, o._data8, SHA1_DIGEST_SIZE) == 0;
        }

        bool operator!=(const digest& o) const {
            return memcmp(_data8, o._data8, SHA1_DIGEST_SIZE) != 0;
        }

        uint32_t at32(int idx) const {
            return _data32[idx];
        }

        std::string hex() const {
            char ret[41];
            char* c = ret;
            for (uint32_t i = 0; i < SHA1_DIGEST_SIZE; i++) {
                snprintf(c, 3, "%02x", _data8[i]);
                c += 2;
            }
            *c = '\0';
            return ret;
        }
    };

    uint32_t state[5];
    uint32_t count[2];
    uint8_t  buffer[64];

    SHA1();
    SHA1(const uint8_t* data, const size_t len);

    SHA1& reset();
    SHA1& update(const uint8_t* data, const size_t len);
    digest end();
};

#endif /* __SHA1_H */
