/*
 *
 * Here we register all options we'll use, and also give them some
 * initial values.
 *
 */

#ifdef LINKSBOKS_STANDALONE
#define XBOX_DEFAULT_HORIZONTAL_MARGIN	40
#define XBOX_DEFAULT_VERTICAL_MARGIN	30
#else
#define XBOX_DEFAULT_HORIZONTAL_MARGIN	0
#define XBOX_DEFAULT_VERTICAL_MARGIN	0
#endif

#include "links.h"

#include "options_hooks.h"

/*
 * Hey, options are really unstructured (unlike ELinks ones),
 * it's just a scructure of options manager tree! --karpov
 *
 */


/*
 Document options
 */

void register_options_document()
{
        register_optgroup(TXT(T_DOCUMENT),0);
        {
#ifdef G
                register_optgroup(TXT(T_FONT_FAMILIES),1);
                {
                        register_option_font_with_hook("default_font_family_mono",TXT(T_MONOSPACED),G_HTML_DEFAULT_FAMILY_MONO,2,html_hook);
                        register_option_font_with_hook("default_font_family_vari",TXT(T_VARIABLE),G_HTML_DEFAULT_FAMILY,2,html_hook);
                }
                register_optgroup(TXT(T_DEFAULT_COLORS_GRAPHICS),1);
                {
                        struct rgb default_fg_g = { 0, 0, 0 };
//                        struct rgb default_bg_g = { 192, 192, 192 };		ysbox: white's better
                        struct rgb default_bg_g = { 255, 255, 255 };
                        struct rgb default_form_fg_g = { 0, 0, 0 };
                        struct rgb default_form_bg_g = { 192, 192, 192 };
						struct rgb default_link_g = { 0, 0, 255 };
                        struct rgb default_vlink_g = { 0, 0, 128 };

                        register_option_rgb_with_hook ("default_color_bg_g",TXT(T_BACKGROUND_COLOR),default_bg_g,2,html_hook);
                        register_option_rgb_with_hook ("default_color_fg_g",TXT(T_FOREGROUND_COLOR),default_fg_g,2,html_hook);
                        register_option_rgb_with_hook ("default_form_bg_g",TXT(T_FORM_BACKGROUND_COLOR),default_form_bg_g,2,html_hook);
                        register_option_rgb_with_hook ("default_form_fg_g",TXT(T_FORM_FOREGROUND_COLOR),default_form_fg_g,2,html_hook);
                        register_option_rgb_with_hook ("default_color_link_g",TXT(T_LINK_COLOR),default_link_g,2,html_hook);
                        register_option_rgb_with_hook ("default_color_vlink_g",TXT(T_VLINK_COLOR),default_vlink_g,2,html_hook);
                        register_option_bool_with_hook("use_color_separation",TXT(T_USE_COLOR_SEPARATION),1,2,html_hook);
                }
#endif
                register_optgroup(TXT(T_DEFAULT_COLORS_TEXTMODE),1);
                {
                        struct rgb default_fg = { 191, 191, 191 };
                        struct rgb default_bg = { 0, 0, 0 };
                        struct rgb default_link = { 255, 255, 255 };
                        struct rgb default_vlink = { 255, 255, 0 };

                        register_option_rgb_with_hook ("default_color_bg",TXT(T_BACKGROUND_COLOR),default_bg,2,html_hook);
                        register_option_rgb_with_hook ("default_color_fg",TXT(T_FOREGROUND_COLOR),default_fg,2,html_hook);
                        register_option_rgb_with_hook ("default_color_link",TXT(T_LINK_COLOR),default_link,2,html_hook);
                        register_option_rgb_with_hook ("default_color_vlink",TXT(T_VLINK_COLOR),default_vlink,2,html_hook);
                        register_option_bool_with_hook("transparency",TXT(T_TRANSPARENCY),1,2,html_hook);
                }

                register_optgroup(TXT(T_HTML_OPTIONS),1);
                {
                        register_option_cp_with_hook  ("html_assume_codepage",TXT(T_DEFAULT_CODEPAGE),"iso8859-1",2,html_hook);
                        register_option_bool_with_hook("html_hard_codepage",TXT(T_IGNORE_CHARSET_INFO_SENT_BY_SERVER),0,2,html_hook);
                        register_option_bool_with_hook("html_tables",TXT(T_DISPLAY_TABLES),1,2,html_hook);
                        register_option_bool_with_hook("html_frames",TXT(T_DISPLAY_FRAMES),1,2,html_hook);
                        register_option_bool_with_hook("html_images",TXT(T_DISPLAY_LINKS_TO_IMAGES),1,2,html_hook);
#ifdef G
                        register_option_bool_with_hook("html_images_display",TXT(T_DISPLAY_IMAGES),1,2,html_hook);
                        register_option_bool_with_hook("html_images_blocklist",TXT(T_USE_IMAGES_BLOCKLIST),0,2,html_hook);
                        register_option_int_with_hook ("html_images_scale",TXT(T_SCALE_ALL_IMAGES_BY),100,2,html_hook);
                        register_option_int_with_hook ("html_font_size",TXT(T_USER_FONT_SIZE),18,2,html_hook);
#endif
                        register_option_int_with_hook ("html_margin",TXT(T_TEXT_MARGIN),3,2,html_hook);
                        register_option_bool_with_hook("html_table_order",TXT(T_LINK_ORDER_BY_COLUMNS),0,2,html_hook);
                        register_option_bool_with_hook("html_links_numbered",TXT(T_NUMBERED_LINKS),0,2,html_hook);
                }
#ifdef JS
                register_optgroup(TXT(T_JAVASCRIPT_OPTIONS),1);
                {
                        register_option_bool_with_hook("js_enable",TXT(T_ENABLE_JAVASCRIPT),0,2,js_hook);
                        register_option_bool_with_hook("js_verbose_errors",TXT(T_VERBOSE_JS_ERRORS),0,2,js_hook);
                        register_option_bool_with_hook("js_verbose_warnings",TXT(T_VERBOSE_JS_WARNINGS),0,2,js_hook);
			register_option_bool_with_hook("js_all_conversions",TXT(T_ENABLE_ALL_CONVERSIONS),1,2,js_hook);
                        register_option_bool_with_hook("js_global_resolve",TXT(T_ENABLE_GLOBAL_NAME_RESOLUTION),1,2,js_hook);
                        register_option_int_with_hook ("js_fun_depth",TXT(T_JS_RECURSION_DEPTH),100,2,js_hook);
			register_option_int_with_hook ("js_memory_limit",TXT(T_JS_MEMORY_LIMIT),5*1024,2,js_hook);
                }
#endif
#ifdef GLOBHIST
                register_optgroup(TXT(T_GLOBAL_HISTORY),1);
                {
                        register_option_bool("document_history_global_enable",TXT(T_GLOBAL_HISTORY_ENABLE),1,2);
                        register_option_int ("document_history_global_max_items",TXT(T_MAX_NUMBER_OF_ENTRIES),100,2);
                }
#endif
                register_optgroup(TXT(T_SEARCH),1);
                {
			register_option_bool("search_everything_is_a_link","Everything-is-a-link technology",1,2);
                }
                register_optgroup(TXT(T_REFRESH),1);
                {
                        register_option_bool("refresh_enable",TXT(T_REFRESH_ENABLE),0,2);
                        register_option_int ("refresh_minimal",TXT(T_REFRESH_MINIMAL),1,2);
                }
#ifdef G
                register_optgroup(TXT(T_TEXT_SELECTION),1);
                {
                        register_option_bool("text_selection_rectangular_mode",TXT(T_RECTANGULAR_MODE),0,3);
                        register_option_cp  ("text_selection_clipboard_charset",TXT(T_SELECTION_ENCODING),"utf-8",3);
                }
                register_option_bool("keyboard_navigation",TXT(T_KEYBOARD_NAVIGATION),1,1);
#endif
        }
}

/*
 Interface options
 */

void register_options_interface()
{
        register_optgroup(TXT(T_USER_INTERFACE),0);
        {
#ifdef G
                register_optgroup(TXT(T_TOOLBAR_BUTTONS),1);
                {
                        register_option_bool("toolbar_button_visibility_back",TXT(T_GO_BACK),1,2);
                        register_option_bool("toolbar_button_visibility_history",TXT(T_HISTORY),1,2);
                        register_option_bool("toolbar_button_visibility_forward",TXT(T_GO_FORWARD),1,2);
                        register_option_bool("toolbar_button_visibility_reload",TXT(T_RELOAD),1,2);
                        register_option_bool("toolbar_button_visibility_bookmarks",TXT(T_BOOKMARKS),1,2);
                        register_option_bool("toolbar_button_visibility_home",TXT(T_HOMEPAGE),0,2);
                        register_option_bool("toolbar_button_visibility_stop",TXT(T_STOP),1,2);
                }
                register_optgroup(TXT(T_MINI_STATUS),1);
                {
                        register_option_bool("ministatus_visibility_connecting",TXT(T_CONNECTIONG_CONNECTIONS),1,2);
                        register_option_bool("ministatus_visibility_running",TXT(T_RUNNING_CONNECTIONS),1,2);
                        register_option_bool("ministatus_visibility_images",TXT(T_IMAGES_LOADING),1,2);
                        register_option_bool("ministatus_visibility_encoding",TXT(T_CONTENT_ENCODING),1,2);
                        register_option_bool("ministatus_visibility_ssl",TXT(T_SSL),1,2);
                        register_option_bool("ministatus_visibility_keyboard",TXT(T_KEYBOARD_NAVIGATION),1,2);
                        register_option_bool("ministatus_visibility_refresh",TXT(T_REFRESH),0,2);
                }
#endif
                register_optgroup(TXT(T_TABBED_BROWSING),1);
                {
                        register_option_bool("tabs_new_on_middle_button",TXT(T_OPEN_IN_NEW_ON_MIDDLE_CLICK),1,2);
                        register_option_bool("tabs_new_on_ctrl_enter",TXT(T_OPEN_IN_NEW_ON_CTRL_ENTER),1,2);
                        register_option_bool("tabs_new_in_background",TXT(T_OPEN_LINKS_IN_BACKGROUND),0,2);
                        register_option_bool("tabs_cycle",TXT(T_CYCLE_THROUGH_TABS),1,2);
                        register_option_bool("tabs_close_switch_to_next",TXT(T_SWITCH_TO_NEXT_TAB_ON_CLOSE),1,2);
                        register_option_bool("tabs_close_last",TXT(T_ALLOW_TO_CLOSE_LAST_TAB),1,2);
                        register_option_bool("tabs_show",TXT(T_SHOW_TABBAR),1,2);
                        register_option_bool("tabs_show_if_single",TXT(T_SHOW_TABBAR_IF_SINGLE_TAB),1,2);
                }
                register_optgroup(TXT(T_FONTS_AND_COLORS),1);
                {
                        register_optgroup(TXT(T_MENU),2);
                        {
                                register_option_rgb_with_hook("menu_fg_color",TXT(T_MENU_FOREGROUND_COLOR),0x000000,3,bfu_hook);
                                register_option_rgb_with_hook("menu_bg_color",TXT(T_MENU_BACKGROUND_COLOR),0xdddddd,3,bfu_hook);
                                register_option_rgb_with_hook("menu_shadow_color",TXT(T_MENU_SHADOW_COLOR),0x000000,3,bfu_hook);
                                register_option_char_with_hook("menu_font",TXT(T_MENU_FONT),"default-medium-roman-serif-vari",3,bfu_hook);
                                register_option_char_with_hook("menu_bold_font",TXT(T_MENU_BOLD_FONT),"default-bold-roman-serif-vari",3,bfu_hook);
                                register_option_char_with_hook("menu_mono_font",TXT(T_MENU_MONO_FONT),"default-medium-roman-serif-mono",3,bfu_hook);
                                register_option_char_with_hook("menu_system_font",TXT(T_MENU_SYSTEM_FONT),"system-medium-roman-serif-vari",3,bfu_hook);
                                register_option_int_with_hook("menu_font_size",TXT(T_USER_FONT_SIZE),16,3,bfu_hook);
                        }
#ifdef G
                        register_optgroup(TXT(T_SCROLL_BAR),2);
                        {
                                register_option_rgb_with_hook("scrollbar_area_color",TXT(T_SCROLL_BAR_AREA_COLOR),0x888888,3,bfu_hook);
                                register_option_rgb_with_hook("scrollbar_bar_color",TXT(T_SCROLL_BAR_BAR_COLOR),0xdddddd,3,bfu_hook);
                                register_option_rgb_with_hook("scrollbar_frame_color",TXT(T_SCROLL_BAR_FRAME_COLOR),0xdddddd,3,bfu_hook);
                        }
#endif
                }
#ifdef G
                register_optgroup(TXT(T_VIDEO_OPTIONS),1);
                {
                        register_optgroup(TXT(T_GAMMA_CORRECTION),2);
                        {
                                register_option_double_with_hook("video_gamma_red",TXT(T_RED_DISPLAY_GAMMA),2.2,3,video_hook);
                                register_option_double_with_hook("video_gamma_green",TXT(T_GREEN_DISPLAY_GAMMA),2.2,3,video_hook);
                                register_option_double_with_hook("video_gamma_blue",TXT(T_BLUE_DISPLAY_GAMMA),2.2,3,video_hook);
                                register_option_double_with_hook("video_gamma_user",TXT(T_USER_GAMMA),1,3,video_hook);
                        }
                        register_optgroup(TXT(T_ASPECT_CORRECTION),2);
                        {
                                register_option_bool_with_hook("video_aspect_on",TXT(T_ASPECT_CORRECTION_ON),0,3,video_hook);
                                register_option_double_with_hook("video_aspect",TXT(T_ASPECT_RATIO),1,3,video_hook);
                        }
                        register_option_int_with_hook("video_display_optimize",TXT(T_DISPLAY_OPTIMIZATION) ,0,2,video_hook);
                        register_option_bool_with_hook("video_dither_letters",TXT(T_DITHER_LETTERS),1,2,video_hook);
                        register_option_bool_with_hook("video_dither_images",TXT(T_DITHER_IMAGES),1,2,video_hook);
                }
#endif
                register_option_char_with_hook("interface_language",TXT(T_LANGUAGE),"English",1,language_hook);
				register_option_bool("hide_menus","Hide menus",0,1);
        }
}

/*
 Bookmarks
 */

void register_options_bookmarks()
{
        unsigned char *bookmarks_file = mem_alloc(MAX_STR_LEN);

        snprintf(bookmarks_file,MAX_STR_LEN,"%sbookmarks.html",links_home);

        register_optgroup(TXT(T_BOOKMARKS),0);
        {
                register_option_char_with_hook("bookmarks_file",TXT(T_BOOKMARKS_FILE),bookmarks_file,1,bookmarks_hook);
                register_option_char_with_hook("bookmarks_codepage",TXT(T_BOOKMARKS_ENCODING),"utf-8",1,bookmarks_hook);
        }
        mem_free(bookmarks_file);
}

/*
 Network options
 */

void register_options_network()
{
        register_optgroup(TXT(T_NETWORK_OPTIONS),0);
        {
				register_option_char("homepage",TXT(T_HOMEPAGE),"http://ysbox.online.fr/splash-0.99.html",1);
                register_optgroup(TXT(T_HTTP_OPTIONS),1);
                {
                        register_optgroup(TXT(T_HTTP_BUG_WORKAROUNDS),2);
                        {
                                register_option_bool("http_bugs_http10", TXT(T_USE_HTTP_10),0,3);
                                register_option_bool("http_bugs_allow_blacklist", TXT(T_ALLOW_SERVER_BLACKLIST),1,3);
                                register_option_bool("http_bugs_302_redirect", TXT(T_BROKEN_302_REDIRECT),1,3);
                                register_option_bool("http_bugs_post_no_keepalive", TXT(T_NO_KEEPALIVE_AFTER_POST_REQUEST),0,3);
                                register_option_bool("http_bugs_no_accept_charset", TXT(T_DO_NOT_SEND_ACCEPT_CHARSET),0,3);
                        }
                        register_optgroup(TXT(T_REFERER),2);
                        {
                                register_option_int("http_referer", TXT(T_REFERER_TYPE),1,3);
                                register_option_char("http_referer_fake_referer", TXT(T_FAKE_REFERER),NULL,3);
                        }
                        register_optgroup(TXT(T_HTTP_PROXY), 2);
                        {
                                register_option_char("http_proxy", TXT(T_HTTP_PROXY__HOST_PORT), NULL, 3);
                                register_option_char("http_proxy_user", TXT(T_HTTP_PROXY_USER), NULL, 3);
                                register_option_char("http_proxy_password", TXT(T_HTTP_PROXY_PASSWORD), NULL, 3);
                        }
                        register_option_char("http_fake_useragent", TXT(T_FAKE_USERAGENT),NULL,2);
                        {
                                /* List of all accepted charsets */
                                unsigned char *str=init_str();
                                unsigned char *cs;
                                int i,len=0;
                                for (i = 0; (cs = get_cp_mime_name(i)); i++) {
                                        if (len)
                                                add_to_str(&str, &len, ", ");
                                        add_to_str(&str, &len, cs);
                                }
                                register_option_char("http_accept_charset",TXT(T_ACCEPTED_CHARSETS),str,2);
                                if(str) mem_free(str);
                                if(cs) mem_free(cs);
                        }
                        /* Accept-Language: - default to NULL */
                        register_option_char("http_accept_language",TXT(T_ACCEPTED_LANGUAGES),NULL,2);
                }
                register_optgroup(TXT(T_FTP_OPTIONS),1);
                {
                        register_option_char("ftp_proxy", TXT(T_FTP_PROXY__HOST_PORT),NULL,2);
                        register_option_char("ftp_anonymous_password", TXT(T_PASSWORD_FOR_ANONYMOUS_LOGIN),"some@where.net",2);
                }
                register_optgroup(TXT(T_CACHE_OPTIONS),1);
                {
                        register_option_int_with_hook("cache_memory_size",TXT(T_MEMORY_CACHE_SIZE),1048576,2,cache_hook);
                        register_option_int_with_hook("cache_images_size",TXT(T_IMAGE_CACHE_SIZE),1048576,2,cache_hook);
                        register_option_int_with_hook("cache_formatted_entries",TXT(T_NUMBER_OF_FORMATTED_DOCUMENTS),5,2,cache_hook);
                        register_option_int_with_hook("cache_fonts_size","Font cache size",16000000,2,cache_hook);
                        register_option_bool("cache_aggressive",TXT(T_AGGRESSIVE_CACHE),1,2);
                }
                register_optgroup(TXT(T_CONNECTIONS),1);
                {
                        register_option_int ("network_max_connections",TXT(T_MAX_CONNECTIONS),16,2);
                        register_option_int ("network_max_connections_to_host",TXT(T_MAX_CONNECTIONS_TO_ONE_HOST),8,2);
                        register_option_int ("network_max_tries",TXT(T_RETRIES),3,2);
                        register_option_int ("network_receive_timeout",TXT(T_RECEIVE_TIMEOUT_SEC),120,2);
                        register_option_int ("network_unrestartable_receive_timeout",TXT(T_TIMEOUT_WHEN_UNRESTARTABLE),600,2);
                        register_option_bool("network_async_lookup",TXT(T_ASYNC_DNS_LOOKUP),1,2);
                }
                register_optgroup(TXT(T_DOWNLOADS),1);
                {
                        register_option_bool("network_download_utime",TXT(T_SET_TIME_OF_DOWNLOADED_FILES),0,2);
#ifdef __XBOX__
						register_option_char("network_download_directory",TXT(T_DOWNLOAD_DIRECTORY),"X:\\",2);
#else
						register_option_char("network_download_directory",TXT(T_DOWNLOAD_DIRECTORY),NULL,2);
#endif
						register_option_bool("network_download_prevent_overwriting",TXT(T_PREVENT_OVERWRITING_LOCAL_FILES),1,2);
                }
                register_optgroup(TXT(T_MAIL_AND_TELNET_PROGRAMS),1);
                {
                        register_option_char("network_program_mailto", TXT(T_MAILTO_PROG),NULL,2);
                        register_option_char("network_program_telnet", TXT(T_TELNET_PROG),NULL,2);
                        register_option_char("network_program_tn3270", TXT(T_TN3270_PROG),NULL,2);
                }
        }
}


/* And now lets put them together */

void register_options()
{
        register_options_document();
        register_options_interface();
        register_options_bookmarks();
        register_options_network();
#ifdef __XBOX__
		register_options_xbox();
#endif
}
