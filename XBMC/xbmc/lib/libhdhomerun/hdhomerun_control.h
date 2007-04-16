/*
 * hdhomerun_control.h
 *
 * Copyright © 2006 Silicondust Engineering Ltd. <www.silicondust.com>.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
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
 *
 * Returns a pointer to the newly created control socket.
 *
 * When no longer needed, the socket should be destroyed by calling hdhomerun_control_destroy.
 */
extern struct hdhomerun_control_sock_t *hdhomerun_control_create(uint32_t device_id, uint32_t device_ip);
extern void hdhomerun_control_destroy(struct hdhomerun_control_sock_t *cs);

/*
 * Get the actual device id or ip of the device.
 *
 * Returns 0 if the device id cannot be determined.
 */
extern uint32_t hdhomerun_control_get_device_id(struct hdhomerun_control_sock_t *cs);
extern uint32_t hdhomerun_control_get_device_ip(struct hdhomerun_control_sock_t *cs);

/*
 * Get the local machine IP address used when communicating with the device.
 *
 * This function is useful for determining the IP address to use with set target commands.
 *
 * Returns 32-bit IP address with native endianness, or 0 on error.
 */
extern uint32_t hdhomerun_control_get_local_addr(struct hdhomerun_control_sock_t *cs);

/*
 * Get the low-level socket handle.
 */
extern int hdhomerun_control_get_sock(struct hdhomerun_control_sock_t *cs);

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
extern int hdhomerun_control_get(struct hdhomerun_control_sock_t *cs, const char *name, char **pvalue, char **perror);
extern int hdhomerun_control_set(struct hdhomerun_control_sock_t *cs, const char *name, const char *value, char **pvalue, char **perror);

/*
 * Upload new firmware to the device.
 *
 * FILE *upgrade_file: File pointer to read from. The file must have been opened in binary mode for reading.
 *
 * Returns 1 if the upload succeeded.
 * Returns 0 if the upload was rejected.
 * Returns -1 if an error occurs.
 */
extern int hdhomerun_control_upgrade(struct hdhomerun_control_sock_t *cs, FILE *upgrade_file);

#ifdef __cplusplus
}
#endif
