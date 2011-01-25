/*
 * hdhomerun_control.h
 *
 * Copyright © 2006 Silicondust Engineering Ltd. <www.silicondust.com>.
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

struct hdhomerun_control_sock_t;

/*
 * Create a control socket.
 *
 * This function will not attempt to connect to the device.
 * The connection will be established when first used.
 *
 * uint32_t device_id = 32-bit device id of device. Set to HDHOMERUN_DEVICE_ID_WILDCARD to match any device ID.
 * uint32_t device_ip = IP address of device. Set to 0 to auto-detect.
 * struct hdhomerun_debug_t *dbg: Pointer to debug logging object. May be NULL.
 *
 * Returns a pointer to the newly created control socket.
 *
 * When no longer needed, the socket should be destroyed by calling hdhomerun_control_destroy.
 */
extern LIBTYPE struct hdhomerun_control_sock_t *hdhomerun_control_create(uint32_t device_id, uint32_t device_ip, struct hdhomerun_debug_t *dbg);
extern LIBTYPE void hdhomerun_control_destroy(struct hdhomerun_control_sock_t *cs);

/*
 * Get the actual device id or ip of the device.
 *
 * Returns 0 if the device id cannot be determined.
 */
extern LIBTYPE uint32_t hdhomerun_control_get_device_id(struct hdhomerun_control_sock_t *cs);
extern LIBTYPE uint32_t hdhomerun_control_get_device_ip(struct hdhomerun_control_sock_t *cs);
extern LIBTYPE uint32_t hdhomerun_control_get_device_id_requested(struct hdhomerun_control_sock_t *cs);
extern LIBTYPE uint32_t hdhomerun_control_get_device_ip_requested(struct hdhomerun_control_sock_t *cs);

extern LIBTYPE void hdhomerun_control_set_device(struct hdhomerun_control_sock_t *cs, uint32_t device_id, uint32_t device_ip);

/*
 * Get the local machine IP address used when communicating with the device.
 *
 * This function is useful for determining the IP address to use with set target commands.
 *
 * Returns 32-bit IP address with native endianness, or 0 on error.
 */
extern LIBTYPE uint32_t hdhomerun_control_get_local_addr(struct hdhomerun_control_sock_t *cs);

/*
 * Low-level communication.
 */
extern LIBTYPE int hdhomerun_control_send_recv(struct hdhomerun_control_sock_t *cs, struct hdhomerun_pkt_t *tx_pkt, struct hdhomerun_pkt_t *rx_pkt, uint16_t type);

/*
 * Get/set a control variable on the device.
 *
 * const char *name: The name of var to get/set (c-string). The supported vars is device/firmware dependant.
 * const char *value: The value to set (c-string). The format is device/firmware dependant.

 * char **pvalue: If provided, the caller-supplied char pointer will be populated with a pointer to the value
 *		string returned by the device, or NULL if the device returned an error string. The string will remain
 *		valid until the next call to a control sock function.
 * char **perror: If provided, the caller-supplied char pointer will be populated with a pointer to the error
 *		string returned by the device, or NULL if the device returned an value string. The string will remain
 *		valid until the next call to a control sock function.
 *
 * Returns 1 if the operation was successful (pvalue set, perror NULL).
 * Returns 0 if the operation was rejected (pvalue NULL, perror set).
 * Returns -1 if a communication error occurs.
 */
extern LIBTYPE int hdhomerun_control_get(struct hdhomerun_control_sock_t *cs, const char *name, char **pvalue, char **perror);
extern LIBTYPE int hdhomerun_control_set(struct hdhomerun_control_sock_t *cs, const char *name, const char *value, char **pvalue, char **perror);
extern LIBTYPE int hdhomerun_control_set_with_lockkey(struct hdhomerun_control_sock_t *cs, const char *name, const char *value, uint32_t lockkey, char **pvalue, char **perror);

/*
 * Upload new firmware to the device.
 *
 * FILE *upgrade_file: File pointer to read from. The file must have been opened in binary mode for reading.
 *
 * Returns 1 if the upload succeeded.
 * Returns 0 if the upload was rejected.
 * Returns -1 if an error occurs.
 */
extern LIBTYPE int hdhomerun_control_upgrade(struct hdhomerun_control_sock_t *cs, FILE *upgrade_file);

#ifdef __cplusplus
}
#endif
