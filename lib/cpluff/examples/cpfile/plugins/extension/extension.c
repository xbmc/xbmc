/*
 * Copyright 2007 Johannes Lehtinen
 * This file is free software; Johannes Lehtinen gives unlimited
 * permission to copy, distribute and modify it.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cpluff.h>
#include <core.h>


/* ------------------------------------------------------------------------
 * Internal functions
 * ----------------------------------------------------------------------*/

static int is_of_type(const char *path, const cp_cfg_element_t *type);

/**
 * Classifies a file based on file extension. This classifier uses extensions
 * installed at the file type extension point. Therefore we need pointer to
 * the plug-in context to access the extensions. A plug-in instance initializes
 * the classifier structure with the plug-in context pointer and registers a
 * virtual symbol pointing to the classifier.
 */
static int classify(void *d, const char *path) {
	cp_context_t *ctx = d;
	cp_extension_t **exts;
	const char *type = NULL;
	int i;
	
	// Go through all extensions registered at the extension point
	exts = cp_get_extensions_info(ctx, "org.c-pluff.examples.cpfile.extension.file-types", NULL, NULL);
	if (exts == NULL) {
		cp_log(ctx, CP_LOG_ERROR, "Could not resolve file type extensions.");
		return 0;
	}
	for (i = 0; type == NULL && exts[i] != NULL; i++) {
		int j;
		
		// Go through all file types provided by the extension
		for (j = 0; type == NULL && j < exts[i]->configuration->num_children; j++) {
			cp_cfg_element_t *elem = exts[i]->configuration->children + j;
			const char *desc = NULL;
			
			if (strcmp(elem->name, "file-type") == 0
				&& (desc = cp_lookup_cfg_value(elem, "@description")) != NULL
				&& (is_of_type(path, elem))) {
				type = desc;
			}
		}
	}
	
	// Release extension information
	cp_release_info(ctx, exts);
	
	// Print file type if recognized, otherwise try other classifiers
	if (type != NULL) {
		fputs(type, stdout);
		putchar('\n');
		return 1;
	} else {
		return 0;
	}
}

/**
 * Returns whether the specified file is of the type matching the specified
 * file-type element.
 */
static int is_of_type(const char *path, const cp_cfg_element_t *type) {
	int i;
	int iot = 0;
	
	/* Go through all extensions specified for the type */
	for (i = 0; !iot && i < type->num_children; i++) {
		cp_cfg_element_t *ee = type->children + i;
		const char *ext;
		
		iot = (strcmp(ee->name, "file-extension") == 0
			&& (ext = cp_lookup_cfg_value(ee, "@ext")) != NULL
			&& strlen(path) >= strlen(ext)
			&& strcmp(path + (strlen(path) - strlen(ext)), ext) == 0);
	}
	
	return iot;
}

/**
 * Creates a new plug-in instance. We use classifier instance as plug-in
 * instance because it includes all the data our plug-in instance needs.
 */
static void *create(cp_context_t *ctx) {
	classifier_t *cl;
	
	cl = malloc(sizeof(classifier_t));
	if (cl != NULL) {
		cl->data = ctx;
		cl->classify = classify;
	}
	return cl;
}

/**
 * Initializes and starts the plug-in.
 */
static int start(void *d) {
	classifier_t *cl = d;
	cp_context_t *ctx = cl->data;
	
	return cp_define_symbol(ctx, "cp_ex_cpfile_extension_classifier", cl);
}

/**
 * Destroys a plug-in instance.
 */
static void destroy(void *d) {
	if (d != NULL) {
		free(d);
	}
}


/* ------------------------------------------------------------------------
 * Exported classifier information
 * ----------------------------------------------------------------------*/

/**
 * Plug-in runtime information for the framework. The name of this symbol
 * is stored in the plug-in descriptor.
 */
CP_EXPORT cp_plugin_runtime_t cp_ex_cpfile_extension_funcs = {
	create,
	start,
	NULL,
	destroy
};
