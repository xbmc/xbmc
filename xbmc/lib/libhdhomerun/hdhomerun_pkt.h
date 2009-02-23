/*
 * hdhomerun_pkt.h
 *
 * Copyright © 2005-2006 Silicondust Engineering Ltd. <www.silicondust.com>.
 *
 * This library is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 * 
 * As a special exception to the GNU Lesser General Public License,
 * you may link, statically or dynamically, an application with a
 * publicly distributed version of the Library to produce an
 * executable file containing portions of the Library, and
 * distribute that executable file under terms of your choice,
 * without any of the additional requirements listed in clause 4 of
 * the GNU Lesser General Public License.
 * 
 * By "a publicly distributed version of the Library", we mean
 * either the unmodified Library as distributed by Silicondust, or a
 * modified version of the Library that is distributed under the
 * conditions defined in the GNU Lesser General Public License.
 */
#ifdef __cplusplus
extern "C" {
#endif

/*
 * The discover protocol (UDP port 65001) and control protocol (TCP port 65001)
 * both use the same packet based format:
 *	uint16_t	Packet type
 *	uint16_t	Payload length (bytes)
 *	uint8_t[]	Payload data (0-n bytes).
 *	uint32_t	CRC (Ethernet style 32-bit CRC)
 *
 * All variables are big-endian except for the crc which is little-endian.
 *
 * Valid values for the packet type are listed below as defines prefixed
 * with "HDHOMERUN_TYPE_"
 *
 * Discovery:
 *
 * The payload for a discovery request or reply is a simple sequence of
 * tag-length-value data:
 *	uint8_t		Tag
 *	varlen		Length
 *	uint8_t[]	Value (0-n bytes)
 *
 * The length field can be one or two bytes long.
 * For a length <= 127 bytes the length is expressed as a single byte. The
 * most-significant-bit is clear indicating a single-byte length.
 * For a length >= 128 bytes the length is expressed as a sequence of two bytes as follows:
 * The first byte is contains the least-significant 7-bits of the length. The
 * most-significant bit is then set (add 0x80) to indicate that it is a two byte length.
 * The second byte contains the length shifted down 7 bits.
 *
 * A discovery request packet has a packet type of HDHOMERUN_TYPE_DISCOVER_REQ and should
 * contain two tags: HDHOMERUN_TAG_DEVICE_TYPE and HDHOMERUN_TAG_DEVICE_ID.
 * The HDHOMERUN_TAG_DEVICE_TYPE value should be set to HDHOMERUN_DEVICE_TYPE_TUNER.
 * The HDHOMERUN_TAG_DEVICE_ID value should be set to HDHOMERUN_DEVICE_ID_WILDCARD to match
 * all devices, or to the 32-bit device id number to match a single device.
 *
 * The discovery response packet has a packet type of HDHOMERUN_TYPE_DISCOVER_RPY and has the
 * same format as the discovery request packet with the two tags: HDHOMERUN_TAG_DEVICE_TYPE and
 * HDHOMERUN_TAG_DEVICE_ID. In the future additional tags may also be returned - unknown tags
 * should be skipped and not treated as an error.
 *
 * Control get/set:
 *
 * The payload for a control get/set request is a simple sequence of tag-length-value data
 * following the same format as for discover packets.
 *
 * A get request packet has a packet type of HDHOMERUN_TYPE_GETSET_REQ and should contain
 * the tag: HDHOMERUN_TAG_GETSET_NAME. The HDHOMERUN_TAG_GETSET_NAME value should be a sequence
 * of bytes forming a null-terminated string, including the NULL. The TLV length must include
 * the NULL character so the length field should be set to strlen(str) + 1.
 *
 * A set request packet has a packet type of HDHOMERUN_TYPE_GETSET_REQ (same as a get request)
 * and should contain two tags: HDHOMERUN_TAG_GETSET_NAME and HDHOMERUN_TAG_GETSET_VALUE.
 * The HDHOMERUN_TAG_GETSET_NAME value should be a sequence of bytes forming a null-terminated
 * string, including the NULL.
 * The HDHOMERUN_TAG_GETSET_VALUE value  should be a sequence of bytes forming a null-terminated
 * string, including the NULL.
 *
 * The get and set reply packets have the packet type HDHOMERUN_TYPE_GETSET_RPY and have the same
 * format as the set request packet with the two tags: HDHOMERUN_TAG_GETSET_NAME and
 * HDHOMERUN_TAG_GETSET_VALUE. A set request is also implicit get request so the updated value is
 * returned.
 *
 * If the device encounters an error handling the get or set request then the get/set reply packet
 * will contain the tag HDHOMERUN_TAG_ERROR_MESSAGE. The format of the value is a sequence of
 * bytes forming a null-terminated string, including the NULL.
 *
 * In the future additional tags may also be returned - unknown tags should be skipped and not
 * treated as an error.
 *
 * Security note: The application should not rely on the NULL character being present. The
 * application should write a NULL character based on the TLV length to protect the application
 * from a potential attack.
 *
 * Firmware Upgrade:
 *
 * A firmware upgrade packet has a packet type of HDHOMERUN_TYPE_UPGRADE_REQ and has a fixed format:
 *	uint32_t	Position in bytes from start of file.
 *	uint8_t[256]	Firmware data (256 bytes)
 *
 * The data must be uploaded in 256 byte chunks and must be uploaded in order.
 * The position number is in bytes so will increment by 256 each time.
 *
 * When all data is uploaded it should be signaled complete by sending another packet of type
 * HDHOMERUN_TYPE_UPGRADE_REQ with payload of a single uint32_t with the value 0xFFFFFFFF.
 */

#define HDHOMERUN_DISCOVER_UDP_PORT 65001
#define HDHOMERUN_CONTROL_TCP_PORT 65001

#define HDHOMERUN_MAX_PACKET_SIZE 1460
#define HDHOMERUN_MAX_PAYLOAD_SIZE 1452

#define HDHOMERUN_TYPE_DISCOVER_REQ 0x0002
#define HDHOMERUN_TYPE_DISCOVER_RPY 0x0003
#define HDHOMERUN_TYPE_GETSET_REQ 0x0004
#define HDHOMERUN_TYPE_GETSET_RPY 0x0005
#define HDHOMERUN_TYPE_UPGRADE_REQ 0x0006
#define HDHOMERUN_TYPE_UPGRADE_RPY 0x0007

#define HDHOMERUN_TAG_DEVICE_TYPE 0x01
#define HDHOMERUN_TAG_DEVICE_ID 0x02
#define HDHOMERUN_TAG_GETSET_NAME 0x03
#define HDHOMERUN_TAG_GETSET_VALUE 0x04
#define HDHOMERUN_TAG_GETSET_LOCKKEY 0x15
#define HDHOMERUN_TAG_ERROR_MESSAGE 0x05

#define HDHOMERUN_DEVICE_TYPE_WILDCARD 0xFFFFFFFF
#define HDHOMERUN_DEVICE_TYPE_TUNER 0x00000001
#define HDHOMERUN_DEVICE_ID_WILDCARD 0xFFFFFFFF

#define HDHOMERUN_MIN_PEEK_LENGTH 4

struct hdhomerun_pkt_t {
	uint8_t *pos;
	uint8_t *start;
	uint8_t *end;
	uint8_t *limit;
	uint8_t buffer[3074];
};

extern LIBTYPE struct hdhomerun_pkt_t *hdhomerun_pkt_create(void);
extern LIBTYPE void hdhomerun_pkt_destroy(struct hdhomerun_pkt_t *pkt);
extern LIBTYPE void hdhomerun_pkt_reset(struct hdhomerun_pkt_t *pkt);

extern LIBTYPE uint8_t hdhomerun_pkt_read_u8(struct hdhomerun_pkt_t *pkt);
extern LIBTYPE uint16_t hdhomerun_pkt_read_u16(struct hdhomerun_pkt_t *pkt);
extern LIBTYPE uint32_t hdhomerun_pkt_read_u32(struct hdhomerun_pkt_t *pkt);
extern LIBTYPE size_t hdhomerun_pkt_read_var_length(struct hdhomerun_pkt_t *pkt);
extern LIBTYPE uint8_t *hdhomerun_pkt_read_tlv(struct hdhomerun_pkt_t *pkt, uint8_t *ptag, size_t *plength);

extern LIBTYPE void hdhomerun_pkt_write_u8(struct hdhomerun_pkt_t *pkt, uint8_t v);
extern LIBTYPE void hdhomerun_pkt_write_u16(struct hdhomerun_pkt_t *pkt, uint16_t v);
extern LIBTYPE void hdhomerun_pkt_write_u32(struct hdhomerun_pkt_t *pkt, uint32_t v);
extern LIBTYPE void hdhomerun_pkt_write_var_length(struct hdhomerun_pkt_t *pkt, size_t v);
extern LIBTYPE void hdhomerun_pkt_write_mem(struct hdhomerun_pkt_t *pkt, const void *mem, size_t length);

extern LIBTYPE bool_t hdhomerun_pkt_open_frame(struct hdhomerun_pkt_t *pkt, uint16_t *ptype);
extern LIBTYPE void hdhomerun_pkt_seal_frame(struct hdhomerun_pkt_t *pkt, uint16_t frame_type);

#ifdef __cplusplus
}
#endif
