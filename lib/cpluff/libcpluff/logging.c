/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2007 Johannes Lehtinen
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *-----------------------------------------------------------------------*/

/** @file
 * Logging functions
 */ 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include "cpluff.h"
#include "defines.h"
#include "util.h"
#include "internal.h"


/* ------------------------------------------------------------------------
 * Data types
 * ----------------------------------------------------------------------*/

/// Contains information about installed loggers
typedef struct logger_t {
	
	/// Pointer to logger
	cp_logger_func_t logger;
	
	/// Pointer to registering plug-in or NULL for the main program
	cp_plugin_t *plugin;
	
	/// User data pointer
	void *user_data;
	
	/// Minimum severity
	cp_log_severity_t min_severity;
	
	/// Selected environment or NULL
	cp_plugin_env_t *env_selection;
} logger_t;


/* ------------------------------------------------------------------------
 * Function definitions
 * ----------------------------------------------------------------------*/

/**
 * Updates the context logging limits. The caller must have locked the
 * context.
 */
static void update_logging_limits(cp_context_t *context) {
	lnode_t *node;
	int nms = CP_LOG_NONE;
	
	node = list_first(context->env->loggers);
	while (node != NULL) {
		logger_t *lh = lnode_get(node);
		if (lh->min_severity < nms) {
			nms = lh->min_severity;
		}
		node = list_next(context->env->loggers, node);
	}
	context->env->log_min_severity = nms;
}

static int comp_logger(const void *p1, const void *p2) {
	const logger_t *l1 = p1;
	const logger_t *l2 = p2;
	return l1->logger != l2->logger;
}

CP_C_API cp_status_t cp_register_logger(cp_context_t *context, cp_logger_func_t logger, void *user_data, cp_log_severity_t min_severity) {
	logger_t l;
	logger_t *lh = NULL;
	lnode_t *node = NULL;
	cp_status_t status = CP_OK;

	CHECK_NOT_NULL(context);
	CHECK_NOT_NULL(logger);
	cpi_lock_context(context);
	cpi_check_invocation(context, CPI_CF_LOGGER, __func__);
	do {
	
		// Check if logger already exists and allocate new holder if necessary
		l.logger = logger;
		if ((node = list_find(context->env->loggers, &l, comp_logger)) == NULL) {
			lh = malloc(sizeof(logger_t));
			node = lnode_create(lh);
			if (lh == NULL || node == NULL) {
				status = CP_ERR_RESOURCE;
				break;
			}
			lh->logger = logger;
			lh->plugin = context->plugin;
			list_append(context->env->loggers, node);
		} else {
			lh = lnode_get(node);
		}
		
		// Initialize or update the logger holder
		lh->user_data = user_data;
		lh->min_severity = min_severity;
		
		// Update global limits
		update_logging_limits(context);
		
	} while (0);

	// Report error
	if (status == CP_ERR_RESOURCE) {
		cpi_error(context, N_("Logger could not be registered due to insufficient memory."));		
	} else if (cpi_is_logged(context, CP_LOG_DEBUG)) {
		char owner[64];
		/* TRANSLATORS: %s is the context owner */
		cpi_debugf(context, N_("%s registered a logger."), cpi_context_owner(context, owner, sizeof(owner)));
	}
	cpi_unlock_context(context);

	// Release resources on error
	if (status != CP_OK) {
		if (node != NULL) {
			lnode_destroy(node);
		}
		if (lh != NULL) {
			free(lh);
		}
	}

	return status;
}

CP_C_API void cp_unregister_logger(cp_context_t *context, cp_logger_func_t logger) {
	logger_t l;
	lnode_t *node;
	
	CHECK_NOT_NULL(context);
	CHECK_NOT_NULL(logger);
	cpi_lock_context(context);
	cpi_check_invocation(context, CPI_CF_LOGGER, __func__);
	
	l.logger = logger;
	if ((node = list_find(context->env->loggers, &l, comp_logger)) != NULL) {
		logger_t *lh = lnode_get(node);
		list_delete(context->env->loggers, node);
		lnode_destroy(node);
		free(lh);
		update_logging_limits(context);
	}
	if (cpi_is_logged(context, CP_LOG_DEBUG)) {
		char owner[64];
		/* TRANSLATORS: %s is the context owner */
		cpi_debugf(context, N_("%s unregistered a logger."), cpi_context_owner(context, owner, sizeof(owner)));
	}
	cpi_unlock_context(context);
}

static void do_log(cp_context_t *context, cp_log_severity_t severity, const char *msg) {
	lnode_t *node;
	const char *apid = NULL;

	assert(cpi_is_context_locked(context));	
	if (context->env->in_logger_invocation) {
		cpi_fatalf(_("Encountered a recursive logging request within a logger invocation."));
	}
	if (context->plugin != NULL) {
		apid = context->plugin->plugin->identifier;
	}
	context->env->in_logger_invocation++;
	node = list_first(context->env->loggers);
	while (node != NULL) {
		logger_t *lh = lnode_get(node);
		if (severity >= lh->min_severity) {
			lh->logger(severity, msg, apid, lh->user_data);
		}
		node = list_next(context->env->loggers, node);
	}
	context->env->in_logger_invocation--;
}

CP_HIDDEN void cpi_log(cp_context_t *context, cp_log_severity_t severity, const char *msg) {
	assert(context != NULL);
	assert(msg != NULL);
	assert(severity >= CP_LOG_DEBUG && severity <= CP_LOG_ERROR);
	do_log(context, severity, _(msg));
}

CP_HIDDEN void cpi_logf(cp_context_t *context, cp_log_severity_t severity, const char *msg, ...) {
	char buffer[256];
	va_list va;
	
	assert(context != NULL);
	assert(msg != NULL);
	assert(severity >= CP_LOG_DEBUG && severity <= CP_LOG_ERROR);
		
	va_start(va, msg);
	vsnprintf(buffer, sizeof(buffer), _(msg), va);
	va_end(va);
	strcpy(buffer + sizeof(buffer)/sizeof(char) - 4, "...");
	do_log(context, severity, buffer);
}

static void process_unregister_logger(list_t *list, lnode_t *node, void *plugin) {
	logger_t *lh = lnode_get(node);
	if (plugin == NULL || lh->plugin == plugin) {
		list_delete(list, node);
		lnode_destroy(node);
		free(lh);
	}
}

CP_HIDDEN void cpi_unregister_loggers(list_t *loggers, cp_plugin_t *plugin) {
	list_process(loggers, plugin, process_unregister_logger);
}

CP_C_API void cp_log(cp_context_t *context, cp_log_severity_t severity, const char *msg) {
	CHECK_NOT_NULL(context);
	CHECK_NOT_NULL(msg);
	cpi_lock_context(context);
	cpi_check_invocation(context, CPI_CF_LOGGER, __func__);
	if (severity < CP_LOG_DEBUG || severity > CP_LOG_ERROR) {
		cpi_fatalf(_("Illegal severity value in call to %s."), __func__);
	}
	if (cpi_is_logged(context, severity)) {
		do_log(context, severity, msg);
	}
	cpi_unlock_context(context);
}

CP_C_API int cp_is_logged(cp_context_t *context, cp_log_severity_t severity) {
	int is_logged;
	
	CHECK_NOT_NULL(context);
	cpi_lock_context(context);
	cpi_check_invocation(context, CPI_CF_LOGGER, __func__);
	is_logged = cpi_is_logged(context, severity);
	cpi_unlock_context(context);
	return is_logged;
}
