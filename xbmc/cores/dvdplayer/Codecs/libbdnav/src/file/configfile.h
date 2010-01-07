
#ifndef CONFIGFILE_H_
#define CONFIGFILE_H_

#include <stdint.h>
#include <unistd.h>

typedef struct configfile CONFIGFILE;
struct configfile {
    size_t size;
    uint8_t *buf;
};

enum configfile_types {
    KF_DK_ARRAY = 0x01,
    KF_PK_ARRAY,
    KF_HOST_PRIV_KEY,
    KF_HOST_CERT,
    KF_HOST_NONCE,
    KF_HOST_KEY_POINT
};

CONFIGFILE *configfile_open(const char *path);
void configfile_close(CONFIGFILE *kf);
uint8_t *configfile_record(CONFIGFILE *kf, enum configfile_types type, uint16_t *entries, size_t *entry_len);

/* Keys are stored in a binary file in a record format which is queried from this code
 *
 * Record format:
 *          0                   | type
 *          1-3                 | length
 *          4-5                 | num entries
 *          6-9                | entry length
 *          10-(9+entry_length)| entry 1
 *          .
 *          .
 *          length-1            | end
 *
 */

#endif /* CONFIGFILE_H_ */
