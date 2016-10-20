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
 * C-Pluff C API header file.
 * The elements declared here constitute the C-Pluff C API. To use the
 * API include this file and link the main program and plug-in runtime
 * libraries with the C-Pluff C library. In addition to local declarations,
 * this file also includes cpluffdef.h header file for defines common to C
 * and C++ API.
 */

#ifndef CPLUFF_H_
#define CPLUFF_H_

/**
 * @defgroup cDefines Defines
 * Preprocessor defines.
 */

#ifdef _WIN32
#include "win32/cpluffdef.h"
#else
#include "cpluffdef.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


/* ------------------------------------------------------------------------
 * Defines
 * ----------------------------------------------------------------------*/

/**
 * @def CP_C_API
 * @ingroup cDefines
 *
 * Marks a symbol declaration to be part of the C-Pluff C API.
 * This macro declares the symbol to be imported from the C-Pluff library.
 */

#ifndef CP_C_API
#define CP_C_API CP_IMPORT
#endif


/**
 * @defgroup cScanFlags Flags for plug-in scanning
 * @ingroup cDefines
 *
 * These constants can be orred together for the flags
 * parameter of ::cp_scan_plugins.
 */
/*@{*/

/** 
 * This flag enables upgrades of installed plug-ins by unloading
 * the old version and installing the new version.
 */
#define CP_SP_UPGRADE 0x01

/**
 * This flag causes all plug-ins to be stopped before any
 * plug-ins are to be upgraded.
 */
#define CP_SP_STOP_ALL_ON_UPGRADE 0x02

/**
 * This flag causes all plug-ins to be stopped before any
 * plugins are to be installed (also if new version is to be installed
 * as part of an upgrade).
 */
#define CP_SP_STOP_ALL_ON_INSTALL 0x04

/**
 * Setting this flag causes the currently active plug-ins to be restarted
 * after all changes to the plug-ins have been made (if they were stopped).
 */
#define CP_SP_RESTART_ACTIVE 0x08

/*@}*/


/* ------------------------------------------------------------------------
 * Data types
 * ----------------------------------------------------------------------*/

/**
 * @defgroup cEnums Enumerations
 * Constant value enumerations.
 */

/**
 * @defgroup cTypedefs Typedefs
 * Typedefs of various kind.
 */

/**
 * @defgroup cStructs Data structures
 * Data structure definitions.
 */
 

/* Enumerations */

/**
 * @ingroup cEnums
 *
 * An enumeration of status codes returned by API functions.
 * Most of the interface functions return a status code. The returned
 * status code either indicates successful completion of the operation
 * or some specific kind of error. Some functions do not return a status
 * code because they never fail.
 */
enum cp_status_t {

	/**
	 * Operation was performed successfully (equals to zero).
	 * @showinitializer
	 */
	CP_OK = 0,

	/** Not enough memory or other operating system resources available */
	CP_ERR_RESOURCE,

	/** The specified object is unknown to the framework */
	CP_ERR_UNKNOWN,

	/** An I/O error occurred */
	CP_ERR_IO,

	/** Malformed plug-in descriptor was encountered when loading a plug-in */
	CP_ERR_MALFORMED,

	/** Plug-in or symbol conflicts with another plug-in or symbol. */
	CP_ERR_CONFLICT,

	/** Plug-in dependencies could not be satisfied. */
	CP_ERR_DEPENDENCY,

	/** Plug-in runtime signaled an error. */
	CP_ERR_RUNTIME
	
};

/**
 * @ingroup cEnums
 * An enumeration of possible plug-in states. Plug-in states are controlled
 * by @ref cFuncsPlugin "plug-in management functions". Plug-in states can be
 * observed by @ref cp_register_plistener "registering" a
 * @ref cp_plugin_listener_func_t "plug-in listener function"
 * or by calling ::cp_get_plugin_state.
 *
 * @sa cp_plugin_listener_t
 * @sa cp_get_plugin_state
 */
enum cp_plugin_state_t {

	/**
	 * Plug-in is not installed. No plug-in information has been
	 * loaded.
	 */
	CP_PLUGIN_UNINSTALLED,
	
	/**
	 * Plug-in is installed. At this stage the plug-in information has
	 * been loaded but its dependencies to other plug-ins has not yet
	 * been resolved. The plug-in runtime has not been loaded yet.
	 * The extension points and extensions provided by the plug-in
	 * have been registered.
	 */
	CP_PLUGIN_INSTALLED,
	
	/**
	 * Plug-in dependencies have been resolved. At this stage it has
	 * been verified that the dependencies of the plug-in are satisfied
	 * and the plug-in runtime has been loaded but it is not active
	 * (it has not been started or it has been stopped).
	 * Plug-in is resolved when a dependent plug-in is being
	 * resolved or before the plug-in is started. Plug-in is put
	 * back to installed stage if its dependencies are being
	 * uninstalled.
	 */
	CP_PLUGIN_RESOLVED,
	
	/**
	 * Plug-in is starting. The plug-in has been resolved and the start
	 * function (if any) of the plug-in runtime is about to be called.
	 * A plug-in is started when explicitly requested by the main
	 * program or when a dependent plug-in is about to be started or when
	 * a dynamic symbol defined by the plug-in is being resolved. This state
	 * is omitted and the state changes directly from resolved to active
	 * if the plug-in runtime does not define a start function.
	 */
	CP_PLUGIN_STARTING,
	
	/**
	 * Plug-in is stopping. The stop function (if any) of the plug-in
	 * runtime is about to be called. A plug-in is stopped if the start
	 * function fails or when stopping is explicitly
	 * requested by the main program or when its dependencies are being
	 * stopped. This state is omitted and the state changes directly from
	 * active to resolved if the plug-in runtime does not define a stop
	 * function.
	 */
	CP_PLUGIN_STOPPING,
	
	/**
	 * Plug-in has been successfully started and it has not yet been
	 * stopped.
	 */
	CP_PLUGIN_ACTIVE
	
};

/**
 * @ingroup cEnums
 * An enumeration of possible message severities for framework logging. These
 * constants are used when passing a log message to a
 * @ref cp_logger_func_t "logger function" and when
 * @ref cp_register_logger "registering" a logger function.
 */
enum cp_log_severity_t {

	/** Used for detailed debug messages */
	CP_LOG_DEBUG,
	
	/** Used for informational messages such as plug-in state changes */
	CP_LOG_INFO,
	
	/** Used for messages warning about possible problems */
	CP_LOG_WARNING,
	
	/** Used for messages reporting errors */
	CP_LOG_ERROR
	
};

/*@}*/


/* Typedefs */

/**
 * @defgroup cTypedefsOpaque Opaque types
 * @ingroup cTypedefs
 * Opaque data type definitions.
 */
/*@{*/
 
/**
 * A plug-in context represents the co-operation environment of a set of
 * plug-ins from the perspective of a particular participating plug-in or
 * the perspective of the main program. It is used as an opaque handle to
 * the shared resources but the framework also uses the context to identify
 * the plug-in or the main program invoking framework functions. Therefore
 * a plug-in should not generally expose its context instance to other
 * plug-ins or the main program and neither should the main program
 * expose its context instance to plug-ins. The main program creates
 * plug-in contexts using ::cp_create_context and plug-ins receive their
 * plug-in contexts via @ref cp_plugin_runtime_t::create.
 */
typedef struct cp_context_t cp_context_t;

/*@}*/

 /**
  * @defgroup cTypedefsShorthand Shorthand type names
  * @ingroup cTypedefs
  * Shorthand type names for structs and enumerations.
  */
/*@{*/

/** A type for cp_plugin_info_t structure. */
typedef struct cp_plugin_info_t cp_plugin_info_t;

/** A type for cp_plugin_import_t structure. */
typedef struct cp_plugin_import_t cp_plugin_import_t;

/** A type for cp_ext_point_t structure. */
typedef struct cp_ext_point_t cp_ext_point_t;

/** A type for cp_extension_t structure. */
typedef struct cp_extension_t cp_extension_t;

/** A type for cp_cfg_element_t structure. */
typedef struct cp_cfg_element_t cp_cfg_element_t;

/** A type for cp_plugin_runtime_t structure. */
typedef struct cp_plugin_runtime_t cp_plugin_runtime_t;

/** A type for cp_status_t enumeration. */
typedef enum cp_status_t cp_status_t;

/** A type for cp_plugin_state_t enumeration. */
typedef enum cp_plugin_state_t cp_plugin_state_t;

/** A type for cp_log_severity_t enumeration. */
typedef enum cp_log_severity_t cp_log_severity_t;

/*@}*/

/**
 * @defgroup cTypedefsFuncs Callback function types
 * @ingroup cTypedefs
 * Typedefs for client supplied callback functions.
 */
/*@{*/

/**
 * A listener function called synchronously after a plugin state change.
 * The function should return promptly.
 * @ref cFuncsInit "Library initialization",
 * @ref cFuncsContext "plug-in context management",
 * @ref cFuncsPlugin "plug-in management",
 * listener registration (::cp_register_plistener and ::cp_unregister_plistener)
 * and @ref cFuncsSymbols "dynamic symbol" functions must not be called from
 * within a plug-in listener invocation. Listener functions are registered
 * using ::cp_register_plistener.
 * 
 * @param plugin_id the plug-in identifier
 * @param old_state the old plug-in state
 * @param new_state the new plug-in state
 * @param user_data the user data pointer supplied at listener registration
 */
typedef void (*cp_plugin_listener_func_t)(const char *plugin_id, cp_plugin_state_t old_state, cp_plugin_state_t new_state, void *user_data);

/**
 * A logger function called to log selected plug-in framework messages. The
 * messages may be localized. Plug-in framework API functions must not
 * be called from within a logger function invocation. In a multi-threaded
 * environment logger function invocations are serialized by the framework.
 * Logger functions are registered using ::cp_register_logger.
 *
 * @param severity the severity of the message
 * @param msg the message to be logged, possibly localized
 * @param apid the identifier of the activating plug-in or NULL for the main program
 * @param user_data the user data pointer given when the logger was registered
 */
typedef void (*cp_logger_func_t)(cp_log_severity_t severity, const char *msg, const char *apid, void *user_data);

/**
 * A fatal error handler for handling unrecoverable errors. If the error
 * handler returns then the framework aborts the program. Plug-in framework
 * API functions must not be called from within a fatal error handler
 * invocation. The fatal error handler function is set using
 * ::cp_set_fatal_error_handler.
 *
 * @param msg the possibly localized error message
 */
typedef void (*cp_fatal_error_func_t)(const char *msg);

/**
 * A run function registered by a plug-in to perform work.
 * The run function  should perform a finite chunk of work and it should
 * return a non-zero value if there is more work to be done. Run functions
 * are registered using ::cp_run_function and the usage is discussed in
 * more detail in the @ref cFuncsPluginExec "serial execution" section.
 * 
 * @param plugin_data the plug-in instance data pointer
 * @return non-zero if there is more work to be done or zero if finished
 */
typedef int (*cp_run_func_t)(void *plugin_data);

/*@}*/


/* Data structures */

/**
 * @ingroup cStructs
 * Plug-in information structure captures information about a plug-in. This
 * information can be loaded from a plug-in descriptor using
 * ::cp_load_plugin_descriptor. Information about installed plug-ins can
 * be obtained using ::cp_get_plugin_info and ::cp_get_plugins_info. This
 * structure corresponds to the @a plugin element in a plug-in descriptor.
 */
struct cp_plugin_info_t {
	
	/**
	 * The obligatory unique identifier of the plugin. A recommended way
	 * to generate identifiers is to use domain name service (DNS) prefixes
	 * (for example, org.cpluff.ExamplePlugin) to avoid naming conflicts. This
	 * corresponds to the @a id attribute of the @a plugin element in a plug-in
	 * descriptor.
	 */
	char *identifier;
	
	/**
	 * An optional plug-in name. NULL if not available. The plug-in name is
	 * intended only for display purposes and the value can be localized.
	 * This corresponds to the @a name attribute of the @a plugin element in
	 * a plug-in descriptor.
	 */
	char *name;
	
	/**
	 * An optional release version string. NULL if not available. This
	 * corresponds to the @a version attribute of the @a plugin element in
	 * a plug-in descriptor.
	 */
	char *version;
	
	/**
	 * An optional provider name. NULL if not available. This is the name of
	 * the author or the organization providing the plug-in. The
	 * provider name is intended only for display purposes and the value can
	 * be localized. This corresponds to the @a provider-name attribute of the
	 * @a plugin element in a plug-in descriptor.
	 */
	char *provider_name;
	
	/**
	 * Path of the plugin directory or NULL if not known. This is the
	 * (absolute or relative) path to the plug-in directory containing
	 * plug-in data and the plug-in runtime library. The value corresponds
	 * to the path specified to ::cp_load_plugin_descriptor when loading
	 * the plug-in.
	 */
	char *plugin_path;
	
	/**
	 * Optional ABI compatibility information. NULL if not available.
	 * This is the earliest version of the plug-in interface the current
	 * interface is backwards compatible with when it comes to the application
	 * binary interface (ABI) of the plug-in. That is, plug-in clients compiled against
	 * any plug-in interface version from @a abi_bw_compatibility to
	 * @ref version (inclusive) can use the current version of the plug-in
	 * binary. This describes binary or runtime compatibility.
	 * The value corresponds to the @a abi-compatibility
	 * attribute of the @a backwards-compatibility element in a plug-in descriptor.
	 */
	char *abi_bw_compatibility;
	
	/**
	 * Optional API compatibility information. NULL if not available.
	 * This is the earliest version of the plug-in interface the current
	 * interface is backwards compatible with when it comes to the
	 * application programming interface (API) of the plug-in. That is,
	 * plug-in clients written for any plug-in interface version from
	 * @a api_bw_compatibility to @ref version (inclusive) can be compiled
	 * against the current version of the plug-in API. This describes
	 * source or build time compatibility. The value corresponds to the
	 * @a api-compatibility attribute of the @a backwards-compatibility
	 * element in a plug-in descriptor. 
	 */
	char *api_bw_compatibility;
	
	/**
	 * Optional C-Pluff version requirement. NULL if not available.
	 * This is the version of the C-Pluff implementation the plug-in was
	 * compiled against. It is used to determine the compatibility of
	 * the plug-in runtime and the linked in C-Pluff implementation. Any
	 * C-Pluff version that is backwards compatible on binary level with the
	 * specified version fulfills the requirement.
	 */
	char *req_cpluff_version;
	
	/** Number of import entries in the @ref imports array. */
	unsigned int num_imports;
	
	/**
	 * An array of @ref num_imports import entries. These correspond to
	 * @a import elements in a plug-in descriptor.
	 */
	cp_plugin_import_t *imports;

    /**
     * The base name of the plug-in runtime library, or NULL if none.
     * A platform specific prefix (for example, "lib") and an extension
     * (for example, ".dll" or ".so") may be added to the base name.
     * This corresponds to the @a library attribute of the
     * @a runtime element in a plug-in descriptor.
     */
    char *runtime_lib_name;
    
    /**
     * The symbol pointing to the plug-in runtime function information or
     * NULL if none. The symbol with this name should point to an instance of
     * @ref cp_plugin_runtime_t structure. This corresponds to the
     * @a funcs attribute of the @a runtime element in a plug-in descriptor. 
     */
    char *runtime_funcs_symbol;
    
	/** Number of extension points in @ref ext_points array. */
	unsigned int num_ext_points;
	
	/**
	 * An array of @ref num_ext_points extension points provided by this
	 * plug-in. These correspond to @a extension-point elements in a
	 * plug-in descriptor.
	 */
	cp_ext_point_t *ext_points;
	
	/** Number of extensions in @ref extensions array. */
	unsigned int num_extensions;
	
	/**
	 * An array of @ref num_extensions extensions provided by this
	 * plug-in. These correspond to @a extension elements in a plug-in
	 * descriptor.
	 */
	cp_extension_t *extensions;

};

/**
 * @ingroup cStructs
 * Information about plug-in import. Plug-in import structures are
 * contained in @ref cp_plugin_info_t::imports.
 */
struct cp_plugin_import_t {
	
	/**
	 * The identifier of the imported plug-in. This corresponds to the
	 * @a plugin attribute of the @a import element in a plug-in descriptor.
	 */
	char *plugin_id;
	
	/**
	 * An optional version requirement. NULL if no version requirement.
	 * This is the version of the imported plug-in the importing plug-in was
	 * compiled against. Any version of the imported plug-in that is
	 * backwards compatible with this version fulfills the requirement.
	 * This corresponds to the @a if-version attribute of the @a import
	 * element in a plug-in descriptor.
	 */
	char *version;
	
	/**
	 * Is this import optional. 1 for optional and 0 for mandatory import.
	 * An optional import causes the imported plug-in to be started if it is
	 * available but does not stop the importing plug-in from starting if the
	 * imported plug-in is not available. If the imported plug-in is available
	 * but the API version conflicts with the API version requirement then the
	 * importing plug-in fails to start. This corresponds to the @a optional
	 * attribute of the @a import element in a plug-in descriptor.
	 */
	int optional;
};

/**
 * @ingroup cStructs
 * Extension point structure captures information about an extension
 * point. Extension point structures are contained in
 * @ref cp_plugin_info_t::ext_points.
 */
struct cp_ext_point_t {

	/**
	 * A pointer to plug-in information containing this extension point.
	 * This reverse pointer is provided to make it easy to get information
	 * about the plug-in which is hosting a particular extension point.
	 */
	cp_plugin_info_t *plugin;
	
	/**
	 * The local identifier uniquely identifying the extension point within the
	 * host plug-in. This corresponds to the @name id attribute of an
	 * @a extension-point element in a plug-in descriptor.
	 */
	char *local_id;
	
	/**
	 * The unique identifier of the extension point. This is automatically
	 * constructed by concatenating the identifier of the host plug-in and
	 * the local identifier of the extension point.
	 */
	char *identifier;

	/**
	 * An optional extension point name. NULL if not available. The extension
	 * point name is intended for display purposes only and the value can be
	 * localized. This corresponds to the @a name attribute of
	 * an @a extension-point element in a plug-in descriptor.
	 */
	char *name;
	
	/**
	 * An optional path to the extension schema definition.
	 * NULL if not available. The path is relative to the plug-in directory.
	 * This corresponds to the @a schema attribute
	 * of an @a extension-point element in a plug-in descriptor.
	 */
	char *schema_path;
};

/**
 * @ingroup cStructs
 * Extension structure captures information about an extension. Extension
 * structures are contained in @ref cp_plugin_info_t::extensions.
 */
struct cp_extension_t {

	/** 
	 * A pointer to plug-in information containing this extension.
	 * This reverse pointer is provided to make it easy to get information
	 * about the plug-in which is hosting a particular extension.
	 */
	cp_plugin_info_t *plugin;
	
	/**
	 * The unique identifier of the extension point this extension is
	 * attached to. This corresponds to the @a point attribute of an
	 * @a extension element in a plug-in descriptor.
	 */
	char *ext_point_id;
	
	/**
	 * An optional local identifier uniquely identifying the extension within
	 * the host plug-in. NULL if not available. This corresponds to the
	 * @a id attribute of an @a extension element in a plug-in descriptor.
	 */
	char *local_id;

    /**
     * An optional unique identifier of the extension. NULL if not available.
     * This is automatically constructed by concatenating the identifier
     * of the host plug-in and the local identifier of the extension.
     */
    char *identifier;
	 
	/** 
	 * An optional extension name. NULL if not available. The extension name
	 * is intended for display purposes only and the value can be localized.
	 * This corresponds to the @a name attribute
	 * of an @a extension element in a plug-in descriptor.
	 **/
	char *name;
	
	/**
	 * Extension configuration starting with the extension element.
	 * This includes extension configuration information as a tree of
	 * configuration elements. These correspond to the @a extension
	 * element and its contents in a plug-in descriptor.
	 */
	cp_cfg_element_t *configuration;
};

/**
 * @ingroup cStructs
 * A configuration element contains configuration information for an
 * extension. Utility functions ::cp_lookup_cfg_element and
 * ::cp_lookup_cfg_value can be used for traversing the tree of
 * configuration elements. Pointer to the root configuration element is
 * stored at @ref cp_extension_t::configuration and others are contained as
 * @ref cp_cfg_element_t::children "children" of parent elements.
 */
struct cp_cfg_element_t {
	
	/**
	 * The name of the configuration element. This corresponds to the name of
	 * the element in a plug-in descriptor.
	 */
	char *name;

	/** Number of attribute name, value pairs in the @ref atts array. */
	unsigned int num_atts;
	
	/**
	 * An array of pointers to alternating attribute names and values.
	 * Attribute values can be localized.
	 */
	char **atts;
	
	/**
	  * An optional value of this configuration element. NULL if not available.
	  * The value can be localized. This corresponds to the
	  * text contents of the element in a plug-in descriptor.
	  */
	char *value;
	
	/** A pointer to the parent element or NULL if this is a root element. */
 	cp_cfg_element_t *parent;
 	
 	/** The index of this element among its siblings (0-based). */
 	unsigned int index;
 	
 	/** Number of children in the @ref children array. */
 	unsigned int num_children;

	/**
	 * An array of @ref num_children childrens of this element. These
	 * correspond to child elements in a plug-in descriptor.
	 */
	cp_cfg_element_t *children;
};

/**
 * @ingroup cStructs
 * Container for plug-in runtime information. A plug-in runtime defines a
 * static instance of this structure to pass information to the plug-in
 * framework. The plug-in framework then uses the information
 * to create and control plug-in instances. The symbol pointing
 * to the runtime information instance is named by the @a funcs
 * attribute of the @a runtime element in a plug-in descriptor.
 * 
 * The following graph displays how these functions are used to control the
 * state of the plug-in instance. 
 * 
 * @dot
 * digraph lifecycle {
 *   rankdir=LR;
 *   node [shape=ellipse, fontname=Helvetica, fontsize=10];
 *   edge [fontname=Helvetica, fontsize=10];
 *   none [label="no instance"];
 *   inactive [label="inactive"];
 *   active [label="active"];
 *   none -> inactive [label="create", URL="\ref create"];
 *   inactive -> active [label="start", URL="\ref start"];
 *   active -> inactive [label="stop", URL="\ref stop"];
 *   inactive -> none [label="destroy", URL="\ref destroy"];
 * }
 * @enddot
 */
struct cp_plugin_runtime_t {

	/**
	 * An initialization function called to create a new plug-in
	 * runtime instance. The initialization function initializes and
	 * returns an opaque plug-in instance data pointer which is then
	 * passed on to other control functions. This data pointer should
	 * be used to access plug-in instance specific data. For example,
	 * the context reference must be stored as part of plug-in instance
	 * data if the plug-in runtime needs it. On failure, the function
	 * must return NULL.
	 * 
	 * C-pluff API functions must not be called from within a create
	 * function invocation and symbols from imported plug-ins must not be
	 * used because they may not available yet.
	 * 
	 * @param ctx the plug-in context of the new plug-in instance
	 * @return an opaque pointer to plug-in instance data or NULL on failure
	 */  
	void *(*create)(cp_context_t *ctx);

	/**
	 * A start function called to start a plug-in instance.
	 * The start function must return zero (CP_OK) on success and non-zero
	 * on failure. If the start fails then the stop function (if any) is
	 * called to clean up plug-in state. @ref cFuncsInit "Library initialization",
	 * @ref cFuncsContext "plug-in context management" and
	 * @ref cFuncsPlugin "plug-in management" functions must not be
	 * called from within a start function invocation. The start function
	 * pointer can be NULL if the plug-in runtime does not have a start
	 * function.
	 * 
	 * The start function implementation should set up plug-in and return
	 * promptly. If there is further work to be done then a plug-in can
	 * start a thread or register a run function using ::cp_run_function.
	 * Symbols from imported plug-ins are guaranteed to be available for
	 * the start function.
	 * 
	 * @param data an opaque pointer to plug-in instance data
	 * @return non-zero on success, or zero on failure
	 */
	int (*start)(void *data);
	
	/**
	 * A stop function called to stop a plugin instance.
	 * This function must cease all plug-in runtime activities.
	 * @ref cFuncsInit "Library initialization",
	 * @ref cFuncsContext "plug-in context management",
	 * @ref cFuncsPlugin "plug-in management"
	 * functions, ::cp_run_function and ::cp_resolve_symbol must not be called
	 * from within a stop function invocation. The stop function pointer can
	 * be NULL if the plug-in runtime does not have a stop function.
	 * It is guaranteed that no run functions registered by the plug-in are
	 * called simultaneously or after the call to the stop function.
	 * 
	 * The stop function should release any external resources hold by
	 * the plug-in. Dynamically resolved symbols are automatically released
	 * and dynamically defined symbols and registered run functions are
	 * automatically unregistered after the call to stop function.
	 * Resolved external symbols are still available for the stop function
	 * and symbols provided by the plug-in should remain available
	 * after the call to stop function (although functionality might be
	 * limited). Final cleanup can be safely done in the destroy function.
	 *
	 * @param data an opaque pointer to plug-in instance data
	 */
	void (*stop)(void *data);

	/**
 	 * A destroy function called to destroy a plug-in instance.
 	 * This function should release any plug-in instance data.
 	 * The plug-in is stopped before this function is called.
 	 * C-Pluff API functions must not be called from within a destroy
 	 * function invocation and symbols from imported plug-ins must not be
 	 * used because they may not be available anymore. Correspondingly,
 	 * it is guaranteed that the symbols provided by the plug-in are not
 	 * used by other plug-ins when destroy function has been called.
	 *
	 * @param data an opaque pointer to plug-in instance data
	 */
	void (*destroy)(void *data);

};

/*@}*/


/* ------------------------------------------------------------------------
 * Function declarations
 * ----------------------------------------------------------------------*/

/**
 * @defgroup cFuncs Functions
 *
 * C API functions. The C-Pluff C API functions and
 * any data exposed by them are generally thread-safe if the library has been
 * compiled with multi-threading support. The
 * @ref cFuncsInit "framework initialization functions"
 * are exceptions, they are not thread-safe.
 */

/**
 * @defgroup cFuncsFrameworkInfo Framework information
 * @ingroup cFuncs
 *
 * These functions can be used to query runtime information about the
 * linked in C-Pluff implementation. They may be used by the main program or
 * by a plug-in runtime.
 */
/*@{*/

/**
 * Returns the release version string of the linked in C-Pluff
 * implementation.
 * 
 * @return the C-Pluff release version string
 */
CP_C_API const char *cp_get_version(void) CP_GCC_PURE;

/**
 * Returns the canonical host type associated with the linked in C-Pluff implementation.
 * A multi-platform installation manager could use this information to
 * determine what plug-in versions to install.
 * 
 * @return the canonical host type
 */
CP_C_API const char *cp_get_host_type(void) CP_GCC_PURE;

/*@}*/


/**
 * @defgroup cFuncsInit Framework initialization
 * @ingroup cFuncs
 *
 * These functions are used for framework initialization.
 * They are intended to be used by the main program. These functions are
 * not thread safe.
 */
/*@{*/

/**
 * Sets the fatal error handler called on non-recoverable errors. The default
 * error handler prints the error message out to standard error and aborts
 * the program. If the user specified error handler returns then the framework
 * will abort the program. Setting NULL error handler will restore the default
 * handler. This function is not thread-safe and it should be called
 * before initializing the framework to catch all fatal errors.
 * 
 * @param error_handler the fatal error handler
 */
CP_C_API void cp_set_fatal_error_handler(cp_fatal_error_func_t error_handler);

/**
 * Initializes the plug-in framework. This function must be called
 * by the main program before calling any other plug-in framework
 * functions except @ref cFuncsFrameworkInfo "framework information" functions and
 * ::cp_set_fatal_error_handler. This function may be
 * called several times but it is not thread-safe. Library resources
 * should be released by calling ::cp_destroy when the framework is
 * not needed anymore.
 *
 * Additionally, to enable localization support, the main program should
 * set the current locale using @code setlocale(LC_ALL, "") @endcode
 * before calling this function.
 *
 * @return @ref CP_OK (zero) on success or error code on failure
 */
CP_C_API cp_status_t cp_init(void);

/**
 * Destroys the plug-in framework and releases the resources used by it.
 * The plug-in framework is only destroyed after this function has
 * been called as many times as ::cp_init. This function is not
 * thread-safe. Plug-in framework functions other than ::cp_init,
 * ::cp_get_framework_info and ::cp_set_fatal_error_handler
 * must not be called after the plug-in framework has been destroyed.
 * All contexts are destroyed and all data references returned by the
 * framework become invalid.
 */
CP_C_API void cp_destroy(void);

/*@}*/


/**
 * @defgroup cFuncsContext Plug-in context initialization
 * @ingroup cFuncs
 *
 * These functions are used to manage plug-in contexts from the main
 * program perspective. They are not intended to be used by a plug-in runtime.
 * From the main program perspective a plug-in context is a container for
 * installed plug-ins. There can be several plug-in context instances if there
 * are several independent sets of plug-ins. However, different plug-in
 * contexts are not very isolated from each other in practice because the
 * global symbols exported by a plug-in runtime in one context are visible to
 * all plug-ins in all context instances.
 */
/*@{*/

/**
 * Creates a new plug-in context which can be used as a container for plug-ins.
 * Plug-ins are loaded and installed into a specific context. The main
 * program may have more than one plug-in context but the plug-ins that
 * interact with each other should be placed in the same context. The
 * resources associated with the context are released by calling
 * ::cp_destroy_context when the context is not needed anymore. Remaining
 * contexts are automatically destroyed when the plug-in framework is
 * destroyed. 
 * 
 * @param status pointer to the location where status code is to be stored, or NULL
 * @return the newly created plugin context, or NULL on failure
 */
CP_C_API cp_context_t * cp_create_context(cp_status_t *status);

/**
 * Destroys the specified plug-in context and releases the associated resources.
 * Stops and uninstalls all plug-ins in the context. The context must not be
 * accessed after calling this function.
 * 
 * @param ctx the context to be destroyed
 */
CP_C_API void cp_destroy_context(cp_context_t *ctx) CP_GCC_NONNULL(1);

/**
 * Registers a plug-in collection with a plug-in context. A plug-in collection
 * is a directory that has plug-ins as its immediate subdirectories. The
 * plug-in context will scan the directory when ::cp_scan_plugins is called.
 * Returns @ref CP_OK if the directory has already been registered. A plug-in
 * collection can be unregistered using ::cp_unregister_pcollection or
 * ::cp_unregister_pcollections.
 * 
 * @param ctx the plug-in context
 * @param dir the directory
 * @return @ref CP_OK (zero) on success or @ref CP_ERR_RESOURCE if insufficient memory
 */
CP_C_API cp_status_t cp_register_pcollection(cp_context_t *ctx, const char *dir) CP_GCC_NONNULL(1, 2);

/**
 * Unregisters a previously registered plug-in collection from a
 * plug-in context. Plug-ins already loaded from the collection are not
 * affected. Does nothing if the directory has not been registered.
 * Plug-in collections can be registered using ::cp_register_pcollection.
 * 
 * @param ctx the plug-in context
 * @param dir the previously registered directory
 */
CP_C_API void cp_unregister_pcollection(cp_context_t *ctx, const char *dir) CP_GCC_NONNULL(1, 2);

/**
 * Unregisters all plug-in collections from a plug-in context.
 * Plug-ins already loaded are not affected. Plug-in collections can
 * be registered using ::cp_register_pcollection.
 * 
 * @param ctx the plug-in context
 */
CP_C_API void cp_unregister_pcollections(cp_context_t *ctx) CP_GCC_NONNULL(1);

/*@}*/


/**
 * @defgroup cFuncsLogging Logging
 * @ingroup cFuncs
 *
 * These functions can be used to receive and emit log messages related
 * to a particular plug-in context. They can be used by the main program
 * or by a plug-in runtime.
 */
/*@{*/

/**
 * Registers a logger with a plug-in context or updates the settings of a
 * registered logger. The logger will receive selected log messages.
 * If the specified logger is not yet known, a new logger registration
 * is made, otherwise the settings for the existing logger are updated.
 * The logger can be unregistered using ::cp_unregister_logger and it is
 * automatically unregistered when the registering plug-in is stopped or
 * when the context is destroyed. 
 *
 * @param ctx the plug-in context to log
 * @param logger the logger function to be called
 * @param user_data the user data pointer passed to the logger
 * @param min_severity the minimum severity of messages passed to logger
 * @return @ref CP_OK (zero) on success or @ref CP_ERR_RESOURCE if insufficient memory
 */
CP_C_API cp_status_t cp_register_logger(cp_context_t *ctx, cp_logger_func_t logger, void *user_data, cp_log_severity_t min_severity) CP_GCC_NONNULL(1, 2);

/**
 * Removes a logger registration.
 *
 * @param ctx the plug-in context
 * @param logger the logger function to be unregistered
 */
CP_C_API void cp_unregister_logger(cp_context_t *ctx, cp_logger_func_t logger) CP_GCC_NONNULL(1, 2);

/**
 * Emits a new log message.
 * 
 * @param ctx the plug-in context
 * @param severity the severity of the event
 * @param msg the log message (possibly localized)
 */
CP_C_API void cp_log(cp_context_t *ctx, cp_log_severity_t severity, const char *msg) CP_GCC_NONNULL(1, 3);

/**
 * Returns whether a message of the specified severity would get logged.
 * 
 * @param ctx the plug-in context
 * @param severity the target logging severity
 * @return whether a message of the specified severity would get logged
 */
CP_C_API int cp_is_logged(cp_context_t *ctx, cp_log_severity_t severity) CP_GCC_NONNULL(1);

/*@}*/


/**
 * @defgroup cFuncsPlugin Plug-in management
 * @ingroup cFuncs
 *
 * These functions can be used to manage plug-ins. They are intended to be
 * used by the main program.
 */
/*@{*/

/**
 * Loads a plug-in descriptor from the specified plug-in installation
 * path and returns information about the plug-in. The plug-in descriptor
 * is validated during loading. Possible loading errors are reported via the
 * specified plug-in context. The plug-in is not installed to the context.
 * If operation fails or the descriptor
 * is invalid then NULL is returned. The caller must release the returned
 * information by calling ::cp_release_plugin_info when it does not
 * need the information anymore, typically after installing the plug-in.
 * The returned plug-in information must not be modified.
 * 
 * @param ctx the plug-in context
 * @param path the installation path of the plug-in
 * @param status a pointer to the location where status code is to be stored, or NULL
 * @return pointer to the information structure or NULL if error occurs
 */
CP_C_API cp_plugin_info_t * cp_load_plugin_descriptor(cp_context_t *ctx, const char *path, cp_status_t *status) CP_GCC_NONNULL(1, 2);

/**
 * Loads a plug-in descriptor from the specified block of memory and returns
 * information about the plug-in. The plug-in descriptor
 * is validated during loading. Possible loading errors are reported via the
 * specified plug-in context. The plug-in is not installed to the context.
 * If operation fails or the descriptor
 * is invalid then NULL is returned. The caller must release the returned
 * information by calling ::cp_release_info when it does not
 * need the information anymore, typically after installing the plug-in.
 * The returned plug-in information must not be modified.
 * 
 * @param ctx the plug-in context
 * @param buffer the buffer containing the plug-in descriptor.
 * @param buffer_len the length of the buffer.
 * @param status a pointer to the location where status code is to be stored, or NULL
 * @return pointer to the information structure or NULL if error occurs
 */
CP_C_API cp_plugin_info_t * cp_load_plugin_descriptor_from_memory(cp_context_t *ctx, const char *buffer, unsigned int buffer_len, cp_status_t *status) CP_GCC_NONNULL(1, 2);

/**
 * Installs the plug-in described by the specified plug-in information
 * structure to the specified plug-in context. The plug-in information
 * must have been loaded using ::cp_load_plugin_descriptor with the same
 * plug-in context.
 * The installation fails on #CP_ERR_CONFLICT if the context already
 * has an installed plug-in with the same plug-in identifier. Installation
 * also fails if the plug-in tries to install an extension point which
 * conflicts with an already installed extension point.
 * The plug-in information must not be modified but it is safe to call
 * ::cp_release_plugin_info after the plug-in has been installed.
 *
 * @param ctx the plug-in context
 * @param pi plug-in information structure
 * @return @ref CP_OK (zero) on success or an error code on failure
 */
CP_C_API cp_status_t cp_install_plugin(cp_context_t *ctx, cp_plugin_info_t *pi) CP_GCC_NONNULL(1, 2);

/**
 * Scans for plug-ins in the registered plug-in directories, installing
 * new plug-ins and upgrading installed plug-ins. This function can be used to
 * initially load the plug-ins and to later rescan for new plug-ins.
 * 
 * When several versions of the same plug-in is available the most recent
 * version will be installed. The upgrade behavior depends on the specified
 * @ref cScanFlags "flags". If #CP_SP_UPGRADE is set then upgrades to installed plug-ins are
 * allowed. The old version is unloaded and the new version installed instead.
 * If #CP_SP_STOP_ALL_ON_UPGRADE is set then all active plug-ins are stopped
 * if any plug-ins are to be upgraded. If #CP_SP_STOP_ALL_ON_INSTALL is set then
 * all active plug-ins are stopped if any plug-ins are to be installed or
 * upgraded. Finally, if #CP_SP_RESTART_ACTIVE is set all currently active
 * plug-ins will be restarted after the changes (if they were stopped).
 * 
 * When removing plug-in files from the plug-in directories, the
 * plug-ins to be removed must be first unloaded. Therefore this function
 * does not check for removed plug-ins.
 * 
 * @param ctx the plug-in context
 * @param flags the bitmask of flags
 * @return @ref CP_OK (zero) on success or an error code on failure
 */
CP_C_API cp_status_t cp_scan_plugins(cp_context_t *ctx, int flags) CP_GCC_NONNULL(1);

/**
 * Starts a plug-in. Also starts any imported plug-ins. If the plug-in is
 * already starting then
 * this function blocks until the plug-in has started or failed to start.
 * If the plug-in is already active then this function returns immediately.
 * If the plug-in is stopping then this function blocks until the plug-in
 * has stopped and then starts the plug-in.
 * 
 * @param ctx the plug-in context
 * @param id identifier of the plug-in to be started
 * @return @ref CP_OK (zero) on success or an error code on failure
 */
CP_C_API cp_status_t cp_start_plugin(cp_context_t *ctx, const char *id) CP_GCC_NONNULL(1, 2);

/**
 * Stops a plug-in. First stops any dependent plug-ins that are currently
 * active. Then stops the specified plug-in. If the plug-in is already
 * stopping then this function blocks until the plug-in has stopped. If the
 * plug-in is already stopped then this function returns immediately. If the
 * plug-in is starting then this function blocks until the plug-in has
 * started (or failed to start) and then stops the plug-in.
 * 
 * @param ctx the plug-in context
 * @param id identifier of the plug-in to be stopped
 * @return @ref CP_OK (zero) on success or @ref CP_ERR_UNKNOWN if unknown plug-in
 */
CP_C_API cp_status_t cp_stop_plugin(cp_context_t *ctx, const char *id) CP_GCC_NONNULL(1, 2);

/**
 * Stops all active plug-ins.
 * 
 * @param ctx the plug-in context
 */
CP_C_API void cp_stop_plugins(cp_context_t *ctx) CP_GCC_NONNULL(1);

/**
 * Uninstalls the specified plug-in. The plug-in is first stopped if it is active.
 * Then uninstalls the plug-in and any dependent plug-ins.
 * 
 * @param ctx the plug-in context
 * @param id identifier of the plug-in to be unloaded
 * @return @ref CP_OK (zero) on success or @ref CP_ERR_UNKNOWN if unknown plug-in
 */
CP_C_API cp_status_t cp_uninstall_plugin(cp_context_t *ctx, const char *id) CP_GCC_NONNULL(1, 2);

/**
 * Uninstalls all plug-ins. All plug-ins are first stopped and then
 * uninstalled.
 * 
 * @param ctx the plug-in context
 */
CP_C_API void cp_uninstall_plugins(cp_context_t *ctx) CP_GCC_NONNULL(1);

/*@}*/


/**
 * @defgroup cFuncsPluginInfo Plug-in and extension information
 * @ingroup cFuncs
 *
 * These functions can be used to query information about the installed
 * plug-ins, extension points and extensions or to listen for plug-in state
 * changes. They may be used by the main program or by a plug-in runtime.
 */
/*@{*/

/**
 * Returns static information about the specified plug-in. The returned
 * information must not be modified and the caller must
 * release the information by calling ::cp_release_info when the
 * information is not needed anymore. When a plug-in runtime calls this
 * function it may pass NULL as the identifier to get information about the
 * plug-in itself.
 * 
 * @param ctx the plug-in context
 * @param id identifier of the plug-in to be examined or NULL for self
 * @param status a pointer to the location where status code is to be stored, or NULL
 * @return pointer to the information structure or NULL on failure
 */
CP_C_API cp_plugin_info_t * cp_get_plugin_info(cp_context_t *ctx, const char *id, cp_status_t *status) CP_GCC_NONNULL(1);

/**
 * Returns static information about the installed plug-ins. The returned
 * information must not be modified and the caller must
 * release the information by calling ::cp_release_info when the
 * information is not needed anymore.
 * 
 * @param ctx the plug-in context
 * @param status a pointer to the location where status code is to be stored, or NULL
 * @param num a pointer to the location where the number of returned plug-ins is stored, or NULL
 * @return pointer to a NULL-terminated list of pointers to plug-in information
 * 			or NULL on failure
 */
CP_C_API cp_plugin_info_t ** cp_get_plugins_info(cp_context_t *ctx, cp_status_t *status, int *num) CP_GCC_NONNULL(1);

/**
 * Returns static information about the currently installed extension points.
 * The returned information must not be modified and the caller must
 * release the information by calling ::cp_release_info when the
 * information is not needed anymore.
 *
 * @param ctx the plug-in context
 * @param status a pointer to the location where status code is to be stored, or NULL
 * @param num filled with the number of returned extension points, if non-NULL
 * @return pointer to a NULL-terminated list of pointers to extension point
 *			information or NULL on failure
 */
CP_C_API cp_ext_point_t ** cp_get_ext_points_info(cp_context_t *ctx, cp_status_t *status, int *num) CP_GCC_NONNULL(1);

/**
 * Returns static information about the currently installed extension points.
 * The returned information must not be modified and the caller must
 * release the information by calling ::cp_release_info when the
 * information is not needed anymore.
 *
 * @param ctx the plug-in context
 * @param extpt_id the extension point identifier or NULL for all extensions
 * @param status a pointer to the location where status code is to be stored, or NULL
 * @param num a pointer to the location where the number of returned extension points is to be stored, or NULL
 * @return pointer to a NULL-terminated list of pointers to extension
 *			information or NULL on failure
 */
CP_C_API cp_extension_t ** cp_get_extensions_info(cp_context_t *ctx, const char *extpt_id, cp_status_t *status, int *num) CP_GCC_NONNULL(1);

/**
 * Releases a previously obtained reference counted information object. The
 * documentation for functions returning such information refers
 * to this function. The information must not be accessed after it has
 * been released. The framework uses reference counting to deallocate
 * the information when it is not in use anymore.
 * 
 * @param ctx the plug-in context
 * @param info the information to be released
 */
CP_C_API void cp_release_info(cp_context_t *ctx, void *info) CP_GCC_NONNULL(1, 2);

/**
 * Returns the current state of the specified plug-in. Returns
 * #CP_PLUGIN_UNINSTALLED if the specified plug-in identifier is unknown.
 * 
 * @param ctx the plug-in context
 * @param id the plug-in identifier
 * @return the current state of the plug-in
 */
CP_C_API cp_plugin_state_t cp_get_plugin_state(cp_context_t *ctx, const char *id) CP_GCC_NONNULL(1, 2);

/**
 * Registers a plug-in listener with a plug-in context. The listener is called
 * synchronously immediately after a plug-in state change. There can be several
 * listeners registered with the same context. A plug-in listener can be
 * unregistered using ::cp_unregister_plistener and it is automatically
 * unregistered when the registering plug-in is stopped or when the context
 * is destroyed.
 * 
 * @param ctx the plug-in context
 * @param listener the plug-in listener to be added
 * @param user_data user data pointer supplied to the listener
 * @return @ref CP_OK (zero) on success or @ref CP_ERR_RESOURCE if out of resources
 */
CP_C_API cp_status_t cp_register_plistener(cp_context_t *ctx, cp_plugin_listener_func_t listener, void *user_data) CP_GCC_NONNULL(1, 2);

/**
 * Removes a plug-in listener from a plug-in context. Does nothing if the
 * specified listener was not registered.
 * 
 * @param ctx the plug-in context
 * @param listener the plug-in listener to be removed
 */
CP_C_API void cp_unregister_plistener(cp_context_t *ctx, cp_plugin_listener_func_t listener) CP_GCC_NONNULL(1, 2);

/**
 * Traverses a configuration element tree and returns the specified element.
 * The target element is specified by a base element and a relative path from
 * the base element to the target element. The path includes element names
 * separated by slash '/'. Two dots ".." can be used to designate a parent
 * element. Returns NULL if the specified element does not exist. If there are
 * several subelements with the same name, this function chooses the first one
 * when traversing the tree.
 *
 * @param base the base configuration element
 * @param path the path to the target element
 * @return the target element or NULL if nonexisting
 */
CP_C_API cp_cfg_element_t * cp_lookup_cfg_element(cp_cfg_element_t *base, const char *path) CP_GCC_PURE CP_GCC_NONNULL(1, 2);

/**
 * Traverses a configuration element tree and returns the value of the
 * specified element or attribute. The target element or attribute is specified
 * by a base element and a relative path from the base element to the target
 * element or attributes. The path includes element names
 * separated by slash '/'. Two dots ".." can be used to designate a parent
 * element. The path may end with '@' followed by an attribute name
 * to select an attribute. Returns NULL if the specified element or attribute
 * does not exist or does not have a value. If there are several subelements
 * with the same name, this function chooses the first one when traversing the
 * tree.
 *
 * @param base the base configuration element
 * @param path the path to the target element
 * @return the value of the target element or attribute or NULL
 */
CP_C_API char * cp_lookup_cfg_value(cp_cfg_element_t *base, const char *path) CP_GCC_PURE CP_GCC_NONNULL(1, 2);

/*@}*/


/**
 * @defgroup cFuncsPluginExec Plug-in execution
 * @ingroup cFuncs
 *
 * These functions support a plug-in controlled execution model. Started plug-ins can
 * use ::cp_run_function to register @ref cp_run_func_t "a run function" which is called when the
 * main program calls ::cp_run_plugins or ::cp_run_plugins_step. A run
 * function should do a finite chunk of work and then return telling whether
 * there is more work to be done. A run function is automatically unregistered
 * when the plug-in is stopped. Run functions make it possible for plug-ins
 * to control the flow of execution or they can be used as a coarse
 * way of task switching if there is no multi-threading support.
 *
 * The C-Pluff distribution includes a generic main program, cpluff-loader,
 * which only acts as a plug-in loader. It loads and starts up the
 * specified plug-ins, passing any additional startup arguments to them and
 * then just calls run functions of the plug-ins. This
 * makes it is possible to put all the application specific logic in
 * plug-ins. Application does not necessarily need a main program of its own.
 * 
 * It is also safe, from framework perspective, to call these functions from
 * multiple threads. Run functions may then be executed in parallel threads.
 */
/*@{*/

/**
 * Registers a new run function. The plug-in instance data pointer is given to
 * the run function as a parameter. The run function must return zero if it has
 * finished its work or non-zero if it should be called again later. The run
 * function is unregistered when it returns zero. Plug-in framework functions
 * stopping the registering plug-in must not be called from within a run
 * function. This function does nothing if the specified run
 * function is already registered for the calling plug-in instance.
 * 
 * @param ctx the plug-in context of the registering plug-in
 * @param runfunc the run function to be registered
 * @return @ref CP_OK (zero) on success or an error code on failure
 */
CP_C_API cp_status_t cp_run_function(cp_context_t *ctx, cp_run_func_t runfunc) CP_GCC_NONNULL(1, 2);

/**
 * Runs the started plug-ins as long as there is something to run.
 * This function calls repeatedly run functions registered by started plug-ins
 * until there are no more active run functions. This function is normally
 * called by a thin main proram, a loader, which loads plug-ins, starts some
 * plug-ins and then passes control over to the started plug-ins.
 * 
 * @param ctx the plug-in context containing the plug-ins
 */
CP_C_API void cp_run_plugins(cp_context_t *ctx) CP_GCC_NONNULL(1);

/**
 * Runs one registered run function. This function calls one
 * active run function registered by a started plug-in. When the run function
 * returns this function also returns and passes control back to the main
 * program. The return value can be used to determine whether there are any
 * active run functions left. This function does nothing if there are no active
 * registered run functions.
 * 
 * @param ctx the plug-in context containing the plug-ins
 * @return whether there are active run functions waiting to be run
 */
CP_C_API int cp_run_plugins_step(cp_context_t *ctx) CP_GCC_NONNULL(1);

/**
 * Sets startup arguments for the specified plug-in context. Like for usual
 * C main functions, the first argument is expected to be the name of the
 * program being executed or an empty string and the argument array should be
 * terminated by NULL entry. If the main program is
 * about to pass startup arguments to plug-ins it should call this function
 * before starting any plug-ins in the context. The arguments are not copied
 * and the caller is responsible for keeping the argument data available once
 * the arguments have been set until the context is destroyed. Plug-ins can
 * access the startup arguments using ::cp_get_context_args.
 * 
 * @param ctx the plug-in context
 * @param argv a NULL-terminated array of arguments
 */
CP_C_API void cp_set_context_args(cp_context_t *ctx, char **argv) CP_GCC_NONNULL(1, 2);

/**
 * Returns the startup arguments associated with the specified
 * plug-in context. This function is intended to be used by a plug-in runtime.
 * Startup arguments are set by the main program using ::cp_set_context_args.
 * The returned argument count is zero and the array pointer is NULL if no
 * arguments have been set.
 * 
 * @param ctx the plug-in context
 * @param argc a pointer to a location where the number of startup arguments is stored, or NULL for none
 * @return an argument array terminated by NULL or NULL if not set
 */
CP_C_API char **cp_get_context_args(cp_context_t *ctx, int *argc) CP_GCC_NONNULL(1);

/*@}*/


/**
 * @defgroup cFuncsSymbols Dynamic symbols
 * @ingroup cFuncs
 *
 * These functions can be used to dynamically access symbols exported by the
 * plug-ins. They are intended to be used by a plug-in runtime or by the main
 * program. 
 */
/*@{*/

/**
 * Defines a context specific symbol. If a plug-in has symbols which have
 * a plug-in instance specific value then the plug-in should define those
 * symbols when it is started. The defined symbols are cleared
 * automatically when the plug-in instance is stopped. Symbols can not be
 * redefined.
 * 
 * @param ctx the plug-in context
 * @param name the name of the symbol
 * @param ptr pointer value for the symbol
 * @return @ref CP_OK (zero) on success or a status code on failure
 */
CP_C_API cp_status_t cp_define_symbol(cp_context_t *ctx, const char *name, void *ptr) CP_GCC_NONNULL(1, 2, 3);

/**
 * Resolves a symbol provided by the specified plug-in. The plug-in is started
 * automatically if it is not already active. The symbol may be context
 * specific or global. The framework first looks for a context specific
 * symbol and then falls back to resolving a global symbol exported by the
 * plug-in runtime library. The symbol can be released using
 * ::cp_release_symbol when it is not needed anymore. Pointers obtained from
 * this function must not be passed on to other plug-ins or the main
 * program.
 * 
 * When a plug-in runtime calls this function the plug-in framework creates
 * a dynamic dependency from the symbol using plug-in to the symbol
 * defining plug-in. The symbol using plug-in is stopped automatically if the
 * symbol defining plug-in is about to be stopped. If the symbol using plug-in
 * does not explicitly release the symbol then it is automatically released
 * after a call to the stop function. It is not safe to refer to a dynamically
 * resolved symbol in the stop function except to release it using
 * ::cp_release_symbol.
 * 
 * When the main program calls this function it is the responsibility of the
 * main program to always release the symbol before the symbol defining plug-in
 * is stopped. It is a fatal error if the symbol is not released before the
 * symbol defining plug-in is stopped.
 *
 * @param ctx the plug-in context
 * @param id the identifier of the symbol defining plug-in
 * @param name the name of the symbol
 * @param status a pointer to the location where the status code is to be stored, or NULL
 * @return the pointer associated with the symbol or NULL on failure
 */
CP_C_API void *cp_resolve_symbol(cp_context_t *ctx, const char *id, const char *name, cp_status_t *status) CP_GCC_NONNULL(1, 2, 3);

/**
 * Releases a previously obtained symbol. The pointer must not be used after
 * the symbol has been released. The symbol is released
 * only after as many calls to this function as there have been for
 * ::cp_resolve_symbol for the same plug-in and symbol.
 *
 * @param ctx the plug-in context
 * @param ptr the pointer associated with the symbol
 */
CP_C_API void cp_release_symbol(cp_context_t *ctx, const void *ptr) CP_GCC_NONNULL(1, 2);

/*@}*/


#ifdef __cplusplus
}
#endif /*__cplusplus*/

#endif /*CPLUFF_H_*/
