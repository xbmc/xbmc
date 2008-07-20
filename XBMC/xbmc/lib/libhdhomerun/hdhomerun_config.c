/*
 * hdhomerun_config.c
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

#include "hdhomerun.h"

#include <pthread.h>

static const char *appname;

struct hdhomerun_device_t *hd;

static int help(void)
{
	printf("Usage:\n");
	printf("\t%s discover\n", appname);
	printf("\t%s <id> get help\n", appname);
	printf("\t%s <id> get <item>\n", appname);
	printf("\t%s <id> set <item> <value>\n", appname);
	printf("\t%s <id> scan <tuner> [<filename>]\n", appname);
	printf("\t%s <id> save <tuner> <filename>\n", appname);
	printf("\t%s <id> upgrade <filename>\n", appname);
	return -1;
}

static void extract_appname(const char *argv0)
{
	const char *ptr = strrchr(argv0, '/');
	if (ptr) {
		argv0 = ptr + 1;
	}
	ptr = strrchr(argv0, '\\');
	if (ptr) {
		argv0 = ptr + 1;
	}
	appname = argv0;
}

static bool_t contains(const char *arg, const char *cmpstr)
{
	if (strcmp(arg, cmpstr) == 0) {
		return TRUE;
	}

	if (*arg++ != '-') {
		return FALSE;
	}
	if (*arg++ != '-') {
		return FALSE;
	}
	if (strcmp(arg, cmpstr) == 0) {
		return TRUE;
	}

	return FALSE;
}

static uint32_t parse_ip_addr(const char *str)
{
	unsigned long a[4];
	if (sscanf(str, "%lu.%lu.%lu.%lu", &a[0], &a[1], &a[2], &a[3]) != 4) {
		return 0;
	}

	return (uint32_t)((a[0] << 24) | (a[1] << 16) | (a[2] << 8) | (a[3] << 0));
}

static int discover_print(char *target_ip_str)
{
	uint32_t target_ip = 0;
	if (target_ip_str) {
		target_ip = parse_ip_addr(target_ip_str);
		if (target_ip == 0) {
			fprintf(stderr, "invalid ip address: %s\n", target_ip_str);
			return -1;
		}
	}

	struct hdhomerun_discover_device_t result_list[64];
	int count = hdhomerun_discover_find_devices_custom(target_ip, HDHOMERUN_DEVICE_TYPE_TUNER, HDHOMERUN_DEVICE_ID_WILDCARD, result_list, 64);
	if (count < 0) {
		fprintf(stderr, "error sending discover request\n");
		return -1;
	}
	if (count == 0) {
		printf("no devices found\n");
		return 0;
	}

	int index;
	for (index = 0; index < count; index++) {
		struct hdhomerun_discover_device_t *result = &result_list[index];
		printf("hdhomerun device %08lX found at %u.%u.%u.%u\n",
			(unsigned long)result->device_id,
			(unsigned int)(result->ip_addr >> 24) & 0x0FF, (unsigned int)(result->ip_addr >> 16) & 0x0FF,
			(unsigned int)(result->ip_addr >> 8) & 0x0FF, (unsigned int)(result->ip_addr >> 0) & 0x0FF
		);
	}

	return count;
}

static int cmd_get(const char *item)
{
	char *ret_value;
	char *ret_error;
	if (hdhomerun_device_get_var(hd, item, &ret_value, &ret_error) < 0) {
		fprintf(stderr, "communication error sending request to hdhomerun device\n");
		return -1;
	}

	if (ret_error) {
		printf("%s\n", ret_error);
		return 0;
	}

	printf("%s\n", ret_value);
	return 1;
}

static int cmd_set(const char *item, const char *value)
{
	char *ret_error;
	if (hdhomerun_device_set_var(hd, item, value, NULL, &ret_error) < 0) {
		fprintf(stderr, "communication error sending request to hdhomerun device\n");
		return -1;
	}

	if (ret_error) {
		printf("%s\n", ret_error);
		return 0;
	}

	return 1;
}

static int cmd_streaminfo(const char *tuner_str)
{
	fprintf(stderr, "streaminfo: use \"get /tuner<n>/streaminfo\"\n");
	return -1;
}

static int cmd_scan_callback(va_list ap, const char *type, const char *str)
{
	FILE *fp = va_arg(ap, FILE *);
	if (fp) {
		fprintf(fp, "%s: %s\n", type, str);
		fflush(fp);
	}

	printf("%s: %s\n", type, str);

	return 1;
}

static int cmd_scan(const char *tuner_str, const char *filename)
{
	if (hdhomerun_device_set_tuner_from_str(hd, tuner_str) <= 0) {
		fprintf(stderr, "invalid tuner number\n");
		return -1;
	}

	FILE *fp = NULL;
	if (filename) {
		fp = fopen(filename, "w");
		if (!fp) {
			fprintf(stderr, "unable to create file: %s\n", filename);
			return -1;
		}
	}

	int ret = channelscan_execute(hd, cmd_scan_callback, fp);

	if (fp) {
		fclose(fp);
	}
	return ret;
}

static int cmd_save(const char *tuner_str, const char *filename)
{
	if (hdhomerun_device_set_tuner_from_str(hd, tuner_str) <= 0) {
		fprintf(stderr, "invalid tuner number\n");
		return -1;
	}

	FILE *fp = fopen(filename, "wb");
	if (!fp) {
		fprintf(stderr, "unable to create file %s\n", filename);
		return -1;
	}

	int ret = hdhomerun_device_stream_start(hd);
	if (ret <= 0) {
		fprintf(stderr, "unable to start stream\n");
		fclose(fp);
		return ret;
	}

	uint64_t next_progress = getcurrenttime() + 1000;
	while (1) {
		usleep(64000);

		size_t actual_size;
		uint8_t *ptr = hdhomerun_device_stream_recv(hd, VIDEO_DATA_BUFFER_SIZE_1S, &actual_size);
		if (!ptr) {
			continue;
		}

		fwrite(ptr, 1, actual_size, fp);

		uint64_t current_time = getcurrenttime();
		if (current_time >= next_progress) {
			next_progress = current_time + 1000;
			printf(".");
			fflush(stdout);
		}
	}
}

static int cmd_upgrade(const char *filename)
{
	FILE *fp = fopen(filename, "rb");
	if (!fp) {
		fprintf(stderr, "unable to open file %s\n", filename);
		return -1;
	}

	if (hdhomerun_device_upgrade(hd, fp) <= 0) {
		fprintf(stderr, "error sending upgrade file to hdhomerun device\n");
		fclose(fp);
		return -1;
	}

	printf("upgrade complete\n");
	return 0;
}

static int main_cmd(int argc, char *argv[])
{
	if (argc < 1) {
		return help();
	}

	char *cmd = *argv++; argc--;

	if (contains(cmd, "get")) {
		if (argc < 1) {
			return help();
		}
		return cmd_get(argv[0]);
	}

	if (contains(cmd, "set")) {
		if (argc < 2) {
			return help();
		}
		return cmd_set(argv[0], argv[1]);
	}

	if (contains(cmd, "streaminfo")) {
		if (argc < 1) {
			return help();
		}
		return cmd_streaminfo(argv[0]);
	}

	if (contains(cmd, "scan")) {
		if (argc < 1) {
			return help();
		}
		if (argc < 2) {
			return cmd_scan(argv[0], NULL);
		} else {
			return cmd_scan(argv[0], argv[1]);
		}
	}

	if (contains(cmd, "save")) {
		if (argc < 2) {
			return help();
		}
		return cmd_save(argv[0], argv[1]);
	}

	if (contains(cmd, "upgrade")) {
		if (argc < 1) {
			return help();
		}
		return cmd_upgrade(argv[0]);
	}

	return help();
}

static int main_internal(int argc, char *argv[])
{
#if defined(__WINDOWS__)
	//Start pthreads
	pthread_win32_process_attach_np();

	// Start WinSock
	WORD wVersionRequested = MAKEWORD(2, 0);
	WSADATA wsaData;
	WSAStartup(wVersionRequested, &wsaData);
#endif

	extract_appname(argv[0]);
	argv++;
	argc--;

	if (argc == 0) {
		return help();
	}

	char *id_str = *argv++; argc--;
	if (contains(id_str, "help")) {
		return help();
	}
	if (contains(id_str, "discover")) {
		if (argc < 1) {
			return discover_print(NULL);
		} else {
			return discover_print(argv[0]);
		}
	}

	/* Device object. */
	hd = hdhomerun_device_create_from_str(id_str);
	if (!hd) {
		fprintf(stderr, "invalid device id: %s\n", id_str);
		return -1;
	}

	/* Connect to device and check firmware version. */
	int ret = hdhomerun_device_firmware_version_check(hd, 0);
	if (ret < 0) {
		fprintf(stderr, "unable to connect to device\n");
		hdhomerun_device_destroy(hd);
		return -1;
	}
	if (ret == 0) {
		fprintf(stderr, "WARNING: firmware upgrade needed for all operations to function\n");
	}

	/* Command. */
	ret = main_cmd(argc, argv);

	/* Cleanup. */
	hdhomerun_device_destroy(hd);

	/* Complete. */
	return ret;
}

int main(int argc, char *argv[])
{
	int ret = main_internal(argc, argv);
	if (ret <= 0) {
		return 1;
	}
	return 0;
}
