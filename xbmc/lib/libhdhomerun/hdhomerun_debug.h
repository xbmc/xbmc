/*
 * hdhomerun_debug.h
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

/*
 * The debug logging includes optional support for connecting to the
 * Silicondust support server. This option should not be used without
 * being explicitly enabled by the user. Debug information should be
 * limited to information useful to diagnosing a problem.
 *  - Silicondust.
 */

#ifdef __cplusplus
extern "C" {
#endif

struct hdhomerun_debug_t;

extern LIBTYPE struct hdhomerun_debug_t *hdhomerun_debug_create(void);
extern LIBTYPE void hdhomerun_debug_destroy(struct hdhomerun_debug_t *dbg);

extern LIBTYPE void hdhomerun_debug_set_prefix(struct hdhomerun_debug_t *dbg, const char *prefix);
extern LIBTYPE void hdhomerun_debug_set_filename(struct hdhomerun_debug_t *dbg, const char *filename);
extern LIBTYPE void hdhomerun_debug_enable(struct hdhomerun_debug_t *dbg);
extern LIBTYPE void hdhomerun_debug_disable(struct hdhomerun_debug_t *dbg);
extern LIBTYPE bool_t hdhomerun_debug_enabled(struct hdhomerun_debug_t *dbg);

extern LIBTYPE void hdhomerun_debug_flush(struct hdhomerun_debug_t *dbg, uint64_t timeout);

extern LIBTYPE void hdhomerun_debug_printf(struct hdhomerun_debug_t *dbg, const char *fmt, ...);
extern LIBTYPE void hdhomerun_debug_vprintf(struct hdhomerun_debug_t *dbg, const char *fmt, va_list args);

#ifdef __cplusplus
}
#endif
