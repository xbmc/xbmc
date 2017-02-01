/*
 * Copyright 2007 Johannes Lehtinen
 * This file is free software; Johannes Lehtinen gives unlimited
 * permission to copy, distribute and modify it.
 */

#include <stdlib.h>
#include <stdio.h>
#include <cpluff.h>
#include "core.h"


/* ------------------------------------------------------------------------
 * Data types
 * ----------------------------------------------------------------------*/

/** Type for plugin_data_t structure */
typedef struct plugin_data_t plugin_data_t;

/** Type for registered_classifier_t structure */
typedef struct registered_classifier_t registered_classifier_t;

/** Plug-in instance data */
struct plugin_data_t {
	
	/** The plug-in context */
	cp_context_t *ctx;
	
	/** Number of registered classifiers */
	int num_classifiers;
	
	/** An array of registered classifiers */
	registered_classifier_t *classifiers; 
};

/** Registered classifier information */
struct registered_classifier_t {
	
	/** The priority of the classifier */
	int priority;
	
	/** The classifier data */
	classifier_t *classifier;
};


/* ------------------------------------------------------------------------
 * Internal functions
 * ----------------------------------------------------------------------*/

/**
 * A run function for the core plug-in. In this case this function acts as
 * the application main function so there is no need for us to split the
 * execution into small steps. Rather, we execute the whole main loop at
 * once to make it simpler.
 */
static int run(void *d) {
	plugin_data_t *data = d;
	char **argv;
	int argc;
	int i;

	// Read arguments and print usage, if no arguments given
	argv = cp_get_context_args(data->ctx, &argc);
	if (argc < 2) {
		fputs("usage: cpfile <file> [<file>...]\n", stdout);
		return 0;
	}

	// Go through all files listed as command arguments
	for (i = 1; argv[i] != NULL; i++) {
		int j;
		int classified = 0;
		
		// Print file name
		printf("%s: ", argv[i]);
		
		// Try classifiers in order of descending priority
		for (j = 0; !classified && j < data->num_classifiers; j++) {
			classifier_t *cl
				= data->classifiers[j].classifier;
				
			classified = cl->classify(cl->data, argv[i]);
		}
		
		// Check if unknown file
		if (!classified) {
			fputs("unknown file type\n", stdout);
		}
	}
	
	// All done
	return 0;
} 

/**
 * Creates a new plug-in instance.
 */
static void *create(cp_context_t *ctx) {
	plugin_data_t *data = malloc(sizeof(plugin_data_t));
	if (data != NULL) {
		data->ctx = ctx;
		data->num_classifiers = 0;
		data->classifiers = NULL;
	} else {
		cp_log(ctx, CP_LOG_ERROR,
			"Insufficient memory for plug-in data.");
	}
	return data;
}

/**
 * Compares two registered classifiers according to priority.
 */
static int comp_classifiers(const registered_classifier_t *c1,
	const registered_classifier_t *c2) {
	return c2->priority - c1->priority;
}

/**
 * Initializes and starts the plug-in.
 */
static int start(void *d) {
	plugin_data_t *data = d;
	cp_extension_t **cl_exts;
	int num_cl_exts;
	cp_status_t status;
	int i;
	
	// Obtain list of registered classifiers
	cl_exts = cp_get_extensions_info(
		data->ctx,
		"org.c-pluff.examples.cpfile.core.classifiers",
		&status,
		&num_cl_exts
	);
	if (cl_exts == NULL) {
		
		// An error occurred and framework logged it
		return status;
	}
	
	// Allocate memory for classifier information, if any
	if (num_cl_exts > 0) {
		data->classifiers = malloc(
			num_cl_exts * sizeof(registered_classifier_t)
		);
		if (data->classifiers == NULL) {
			
			// Memory allocation failed
			cp_log(data->ctx, CP_LOG_ERROR,
				"Insufficient memory for classifier list.");
			return CP_ERR_RESOURCE;
		}
	} 
	
	/* Resolve classifier functions. This will implicitly start
	 * plug-ins providing the classifiers. */
	for (i = 0; i < num_cl_exts; i++) {
		const char *str;
		int pri;
		classifier_t *cl;
		
		// Get the classifier function priority
		str = cp_lookup_cfg_value(
			cl_exts[i]->configuration, "@priority"
		);
		if (str == NULL) {
			
			// Classifier is missing mandatory priority
			cp_log(data->ctx, CP_LOG_ERROR,
				"Ignoring classifier without priority.");
			continue;
		}
		pri = atoi(str);
		
		// Resolve classifier data pointer
		str = cp_lookup_cfg_value(
			cl_exts[i]->configuration, "@classifier");
		if (str == NULL) {
			
			// Classifier symbol name is missing
			cp_log(data->ctx, CP_LOG_ERROR,
				"Ignoring classifier without symbol name.");
			continue;
		}
		cl = cp_resolve_symbol(
			data->ctx,
			cl_exts[i]->plugin->identifier,
			str,
			NULL
		);
		if (cl == NULL) {
			
			// Could not resolve classifier symbol
			cp_log(data->ctx, CP_LOG_ERROR,
				"Ignoring classifier which could not be resolved.");
			continue;
		}
		
		// Add classifier to the list of registered classifiers
		data->classifiers[data->num_classifiers].priority = pri;
		data->classifiers[data->num_classifiers].classifier = cl;
		data->num_classifiers++;
	}
	
	// Release extension information
	cp_release_info(data->ctx, cl_exts);
	
	// Sort registered classifiers according to priority
	if (data->num_classifiers > 1) {
		qsort(data->classifiers,
			data->num_classifiers,
			sizeof(registered_classifier_t),
			(int (*)(const void *, const void *)) comp_classifiers);
	}
	
	// Register run function to do the real work
	cp_run_function(data->ctx, run);
	
	// Successfully started
	return CP_OK;
}

/**
 * Releases resources from other plug-ins.
 */
static void stop(void *d) {
	plugin_data_t *data = d;
	int i;
	
	// Release classifier data, if any
	if (data->classifiers != NULL) {
		
		// Release classifier pointers
		for (i = 0; i < data->num_classifiers; i++) {
			cp_release_symbol(
				data->ctx, data->classifiers[i].classifier
			);
		}
		
		// Free local data
		free(data->classifiers);
		data->classifiers = NULL;
		data->num_classifiers = 0;
	}
}

/**
 * Destroys a plug-in instance.
 */
static void destroy(void *d) {
	free(d);
}


/* ------------------------------------------------------------------------
 * Exported runtime information
 * ----------------------------------------------------------------------*/

/**
 * Plug-in runtime information for the framework. The name of this symbol
 * is stored in the plug-in descriptor.
 */
CP_EXPORT cp_plugin_runtime_t cp_ex_cpfile_core_funcs = {
	create,
	start,
	stop,
	destroy
};
