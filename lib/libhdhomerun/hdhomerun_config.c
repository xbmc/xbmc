/*
 * hdhomerun_config.c
 *
 * Copyright © 2006-2008 Silicondust Engineering Ltd. <www.silicondust.com>.
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

#include "hdhomerun.h"

/*
 * The console output format should be set to UTF-8, however in XP and Vista this breaks batch file processing.
 * Attempting to restore on exit fails to restore if the program is terminated by the user.
 * Solution - set the output format each printf.
 */
#if defined(__WINDOWS__)
#define printf console_printf
#define vprintf console_vprintf
#endif

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

static int cmd_set_internal(const char *item, const char *value)
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

static int cmd_set(const char *item, const char *value)
{
	if (strcmp(value, "-") == 0) {
		char *buffer = NULL;
		size_t pos = 0;

		while (1) {
			buffer = (char *)realloc(buffer, pos + 1024);
			if (!buffer) {
				fprintf(stderr, "out of memory\n");
				return -1;
			}

			size_t size = fread(buffer + pos, 1, 1024, stdin);
			pos += size;

			if (size < 1024) {
				break;
			}
		}

		buffer[pos] = 0;

		int ret = cmd_set_internal(item, buffer);

		free(buffer);
		return ret;
	}

	return cmd_set_internal(item, value);
}

static void cmd_scan_printf(FILE *fp, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);

	if (fp) {
		va_list apc;
		va_copy(apc, ap);

		vfprintf(fp, fmt, apc);
		fflush(fp);

		va_end(apc);
	}

	vprintf(fmt, ap);
	fflush(stdout);

	va_end(ap);
}

static int cmd_scan(const char *tuner_str, const char *filename)
{
	if (hdhomerun_device_set_tuner_from_str(hd, tuner_str) <= 0) {
		fprintf(stderr, "invalid tuner number\n");
		return -1;
	}

	char *channelmap;
	if (hdhomerun_device_get_tuner_channelmap(hd, &channelmap) <= 0) {
		fprintf(stderr, "failed to query channelmap from device\n");
		return -1;
	}

	const char *channelmap_scan_group = hdhomerun_channelmap_get_channelmap_scan_group(channelmap);
	if (!channelmap_scan_group) {
		fprintf(stderr, "unknown channelmap '%s'\n", channelmap);
		return -1;
	}

	if (hdhomerun_device_channelscan_init(hd, channelmap_scan_group) <= 0) {
		fprintf(stderr, "failed to initialize channel scan\n");
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

	int ret;
	while (1) {
		struct hdhomerun_channelscan_result_t result;
		ret = hdhomerun_device_channelscan_advance(hd, &result);
		if (ret <= 0) {
			break;
		}

		cmd_scan_printf(fp, "SCANNING: %lu (%s)\n",
			result.frequency, result.channel_str
		);

		ret = hdhomerun_device_channelscan_detect(hd, &result);
		if (ret <= 0) {
			break;
		}

		cmd_scan_printf(fp, "LOCK: %s (ss=%u snq=%u seq=%u)\n",
			result.status.lock_str, result.status.signal_strength,
			result.status.signal_to_noise_quality, result.status.symbol_error_quality
		);

		int i;
		for (i = 0; i < result.program_count; i++) {
			struct hdhomerun_channelscan_program_t *program = &result.programs[i];
			cmd_scan_printf(fp, "PROGRAM %s\n", program->program_str);
		}
	}

	if (fp) {
		fclose(fp);
	}
	if (ret < 0) {
		fprintf(stderr, "communication error sending request to hdhomerun device\n");
	}
	return ret;
}

static bool_t cmd_saving = FALSE;

static void cmd_save_abort(int arg)
{
	cmd_saving = FALSE;
}

static int cmd_save(const char *tuner_str, const char *filename)
{
	if (hdhomerun_device_set_tuner_from_str(hd, tuner_str) <= 0) {
		fprintf(stderr, "invalid tuner number\n");
		return -1;
	}

	FILE *fp;
	if (strcmp(filename, "null") == 0) {
		fp = NULL;
	} else if (strcmp(filename, "-") == 0) {
		fp = stdout;
	} else {
		fp = fopen(filename, "wb");
		if (!fp) {
			fprintf(stderr, "unable to create file %s\n", filename);
			return -1;
		}
	}

	int ret = hdhomerun_device_stream_start(hd);
	if (ret <= 0) {
		fprintf(stderr, "unable to start stream\n");
		return ret;
	}

	signal(SIGINT, cmd_save_abort);
	signal(SIGPIPE, cmd_save_abort);

	struct hdhomerun_video_stats_t stats_old, stats_cur;
	hdhomerun_device_get_video_stats(hd, &stats_old);

	uint64_t next_progress = getcurrenttime() + 1000;

	cmd_saving = TRUE;
	while (cmd_saving) {
		uint64_t loop_start_time = getcurrenttime();

		size_t actual_size;
		uint8_t *ptr = hdhomerun_device_stream_recv(hd, VIDEO_DATA_BUFFER_SIZE_1S, &actual_size);
		if (!ptr) {
			msleep(64);
			continue;
		}

		if (fp) {
			if (fwrite(ptr, 1, actual_size, fp) != actual_size) {
				fprintf(stderr, "error writing output\n");
				return -1;
			}
		}

		if (loop_start_time >= next_progress) {
			next_progress += 1000;
			if (loop_start_time >= next_progress) {
				next_progress = loop_start_time + 1000;
			}

			hdhomerun_device_get_video_stats(hd, &stats_cur);

			if (stats_cur.overflow_error_count > stats_old.overflow_error_count) {
				fprintf(stderr, "o");
			} else if (stats_cur.network_error_count > stats_old.network_error_count) {
				fprintf(stderr, "n");
			} else if (stats_cur.transport_error_count > stats_old.transport_error_count) {
				fprintf(stderr, "t");
			} else if (stats_cur.sequence_error_count > stats_old.sequence_error_count) {
				fprintf(stderr, "s");
			} else {
				fprintf(stderr, ".");
			}

			stats_old = stats_cur;
			fflush(stderr);
		}

		int32_t delay = 64 - (int32_t)(getcurrenttime() - loop_start_time);
		if (delay <= 0) {
			continue;
		}

		msleep(delay);
	}

	if (fp) {
		fclose(fp);
	}

	hdhomerun_device_stream_stop(hd);
	hdhomerun_device_get_video_stats(hd, &stats_cur);

	fprintf(stderr, "\n");
	fprintf(stderr, "-- Video statistics --\n");
	fprintf(stderr, "%u packets received, %u overflow errors, %u network errors, %u transport errors, %u sequence errors\n",
		(unsigned int)stats_cur.packet_count, 
		(unsigned int)stats_cur.overflow_error_count,
		(unsigned int)stats_cur.network_error_count, 
		(unsigned int)stats_cur.transport_error_count, 
		(unsigned int)stats_cur.sequence_error_count);

	return 0;
}

static int cmd_upgrade(const char *filename)
{
	FILE *fp = fopen(filename, "rb");
	if (!fp) {
		fprintf(stderr, "unable to open file %s\n", filename);
		return -1;
	}

	printf("uploading firmware...\n");
	if (hdhomerun_device_upgrade(hd, fp) <= 0) {
		fprintf(stderr, "error sending upgrade file to hdhomerun device\n");
		fclose(fp);
		return -1;
	}
	sleep(2);

	printf("upgrading firmware...\n");
	sleep(8);

	printf("rebooting...\n");
	int count = 0;
	char *version_str;
	while (1) {
		if (hdhomerun_device_get_version(hd, &version_str, NULL) >= 0) {
			break;
		}

		count++;
		if (count > 30) {
			fprintf(stderr, "error finding device after firmware upgrade\n");
			fclose(fp);
			return -1;
		}

		sleep(1);
	}

	printf("upgrade complete - now running firmware %s\n", version_str);
	return 0;
}

static int cmd_execute(void)
{
	char *ret_value;
	char *ret_error;
	if (hdhomerun_device_get_var(hd, "/sys/boot", &ret_value, &ret_error) < 0) {
		fprintf(stderr, "communication error sending request to hdhomerun device\n");
		return -1;
	}

	if (ret_error) {
		printf("%s\n", ret_error);
		return 0;
	}

	char *end = ret_value + strlen(ret_value);
	char *pos = ret_value;

	while (1) {
		if (pos >= end) {
			break;
		}

		char *eol_r = strchr(pos, '\r');
		if (!eol_r) {
			eol_r = end;
		}

		char *eol_n = strchr(pos, '\n');
		if (!eol_n) {
			eol_n = end;
		}

		char *eol = min(eol_r, eol_n);

		char *sep = strchr(pos, ' ');
		if (!sep || sep > eol) {
			pos = eol + 1;
			continue;
		}

		*sep = 0;
		*eol = 0;

		char *item = pos;
		char *value = sep + 1;

		printf("set %s \"%s\"\n", item, value);

		cmd_set_internal(item, value);

		pos = eol + 1;
	}

	return 1;
}

static int main_cmd(int argc, char *argv[])
{
	if (argc < 1) {
		return help();
	}

	char *cmd = *argv++; argc--;

	if (contains(cmd, "key")) {
		if (argc < 2) {
			return help();
		}
		uint32_t lockkey = strtoul(argv[0], NULL, 0);
		hdhomerun_device_tuner_lockkey_use_value(hd, lockkey);

		cmd = argv[1];
		argv+=2; argc-=2;
	}

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

	if (contains(cmd, "execute")) {
		return cmd_execute();
	}

	return help();
}

static int main_internal(int argc, char *argv[])
{
#if defined(__WINDOWS__)
	/* Initialize network socket support. */
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
	hd = hdhomerun_device_create_from_str(id_str, NULL);
	if (!hd) {
		fprintf(stderr, "invalid device id: %s\n", id_str);
		return -1;
	}

	/* Device ID check. */
	uint32_t device_id_requested = hdhomerun_device_get_device_id_requested(hd);
	if (!hdhomerun_discover_validate_device_id(device_id_requested)) {
		fprintf(stderr, "invalid device id: %08lX\n", (unsigned long)device_id_requested);
	}

	/* Connect to device and check model. */
	const char *model = hdhomerun_device_get_model_str(hd);
	if (!model) {
		fprintf(stderr, "unable to connect to device\n");
		hdhomerun_device_destroy(hd);
		return -1;
	}

	/* Command. */
	int ret = main_cmd(argc, argv);

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
