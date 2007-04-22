
#ifndef JS_H
#define JS_H

/* js.c */

struct javascript_context *js_create_context(void *, long);
void js_destroy_context(struct javascript_context *);
void js_execute_code(struct javascript_context *, unsigned char *, int, void (*)(void *));

/* jsint.c */

#define JS_OBJ_MASK 255
#define JS_OBJ_MASK_SIZE 8

#define JS_OBJ_T_UNKNOWN 0
#define JS_OBJ_T_DOCUMENT 1
#define JS_OBJ_T_FRAME 2	/* document a frame se tvari pro mne stejne  --Brain */
#define JS_OBJ_T_LINK 3
#define JS_OBJ_T_FORM 4
#define JS_OBJ_T_ANCHOR 5
#define JS_OBJ_T_IMAGE 6
/* form elements */
#define JS_OBJ_T_TEXT 7
#define JS_OBJ_T_PASSWORD 8
#define JS_OBJ_T_TEXTAREA 9
#define JS_OBJ_T_CHECKBOX 10
#define JS_OBJ_T_RADIO 11
#define JS_OBJ_T_SELECT 12
#define JS_OBJ_T_SUBMIT 13
#define JS_OBJ_T_RESET 14
#define JS_OBJ_T_HIDDEN 15
#define JS_OBJ_T_BUTTON 16


extern struct history js_get_string_history;

struct js_state {
	struct javascript_context *ctx;	/* kontext beziciho javascriptu??? */
	struct list_head queue;		/* struct js_request - list of javascripts to run */
	struct js_request *active;	/* request is running */
	unsigned char *src;		/* zdrojak beziciho javascriptu??? */	/* mikulas: ne. to je zdrojak stranky */
	int srclen;
	int wrote;
};

struct js_document_description {
	/* Pro Martina: TADY pridat nejake polozky popisujici dokument
	- jako treba jake tam jsou polozky formulare, jake obrazky, jake
	linky apod. Neni tady obsah tech polozek, jenom popis, zda
	existuji.

	vyroba struktury je v js_upcall_get_document_description
	ruseni je v jsint_destroy_document_description */

	int prazdnapolozkaabytadynecobylo;
};


/* funkce js_get_select_options vraci pole s temito polozkami */
struct js_select_item{
	/* index je poradi v poli, ktere vratim, takze se tu nemusi skladovat */
	int default_selected;
	int selected;
	unsigned char *text; 	/* text, ktery se zobrazuje */
	unsigned char *value; 	/* value, ktera se posila */
};

struct fax_me_tender_string{
	void *ident;   /* struct f_data_c*, but JS doesn't know it ;-) */
	unsigned char *string;
};

struct fax_me_tender_int_string{
	void *ident;   /* struct f_data_c*, but JS doesn't know it ;-) */
	signed int num;
	unsigned char *string;
};

struct fax_me_tender_string_2_longy{
	void *ident;   /* struct f_data_c*, but JS doesn't know it ;-) */
	unsigned char *string;
	long doc_id,obj_id;
};

struct fax_me_tender_2_stringy{
	void *ident;   /* struct f_data_c*, but JS doesn't know it ;-) */
	unsigned char *string1;
	unsigned char *string2;
};

struct fax_me_tender_nothing{
	void *ident;   /* struct f_data_c*, but JS doesn't know it ;-) */
};

struct session;
struct f_data;
struct f_data_c;

void javascript_func(struct session *ses, unsigned char *code);
void jsint_execute_code(struct f_data_c *, unsigned char *, int, int, int, int);
void jsint_destroy(struct f_data_c *);
void jsint_run_queue(struct f_data_c *);
int jsint_get_source(struct f_data_c *, unsigned char **, unsigned char **);
void jsint_scan_script_tags(struct f_data_c *);
void jsint_destroy_document_description(struct f_data *);
long *jsint_resolve(void *context, long obj_id, char *takhle_tomu_u_nas_nadavame,int *n_items);
int jsint_object_type(long);
void jsint_set_cookies(struct f_data_c *fd, int final_flush);
struct f_data_c *jsint_find_document(long doc_id);
void jsint_kill_recursively(struct f_data_c *fd);

struct js_document_description *js_upcall_get_document_description(void *, long);
void js_upcall_document_write(void *p, unsigned char *str, int len);
void js_upcall_alert(void * struct_fax_me_tender_string);
unsigned char *js_upcall_get_title(void *data);
void js_upcall_set_title(void *data, unsigned char *title);
unsigned char *js_upcall_get_location(void *data);
unsigned char *js_upcall_get_useragent(void *data);
void js_upcall_confirm(void *struct_fax_me_tender_string);
void js_upcall_get_string(void *data);
unsigned char *js_upcall_get_referrer(void *data);
unsigned char *js_upcall_get_appname(void);
unsigned char *js_upcall_get_appcodename(void);
unsigned char *js_upcall_get_appversion(void);
long js_upcall_get_document_id(void *data);
long js_upcall_get_window_id(void *data);
void js_upcall_close_window(void *struct_fax_me_tender_nothing);
unsigned char *js_upcall_document_last_modified(void *data, long document_id);
unsigned char *js_upcall_get_window_name(void *data);
void js_upcall_clear_window(void *);
long *js_upcall_get_links(void *data, long document_id, int *len);
unsigned char *js_upcall_get_link_target(void *data, long document_id, long link_id);
long *js_upcall_get_forms(void *data, long document_id, int *len);
unsigned char *js_upcall_get_form_action(void *data, long document_id, long form_id);
unsigned char *js_upcall_get_form_target(void *data, long document_id, long form_id);
unsigned char *js_upcall_get_form_method(void *data, long document_id, long form_id);
unsigned char *js_upcall_get_form_encoding(void *data, long document_id, long form_id);
unsigned char *js_upcall_get_location_protocol(void *data);
unsigned char *js_upcall_get_location_port(void *data);
unsigned char *js_upcall_get_location_hostname(void *data);
unsigned char *js_upcall_get_location_host(void *data);
unsigned char *js_upcall_get_location_pathname(void *data);
unsigned char *js_upcall_get_location_search(void *data);
unsigned char *js_upcall_get_location_hash(void *data);
long *js_upcall_get_form_elements(void *data, long document_id, long form_id, int *len);
long *js_upcall_get_anchors(void *hej_Hombre, long document_id, int *len);
int js_upcall_get_checkbox_radio_checked(void *smirak, long document_id, long radio_tv_id);
void js_upcall_set_checkbox_radio_checked(void *smirak, long document_id, long radio_tv_id, int value);
int js_upcall_get_checkbox_radio_default_checked(void *smirak, long document_id, long radio_tv_id);
void js_upcall_set_checkbox_radio_default_checked(void *smirak, long document_id, long radio_tv_id, int value);
unsigned char *js_upcall_get_form_element_name(void *smirak, long document_id, long ksunt_id);
void js_upcall_set_form_element_name(void *smirak, long document_id, long ksunt_id, unsigned char *name);
unsigned char *js_upcall_get_form_element_default_value(void *smirak, long document_id, long ksunt_id);
void js_upcall_set_form_element_default_value(void *smirak, long document_id, long ksunt_id, unsigned char *name);
unsigned char *js_upcall_get_form_element_value(void *smirak, long document_id, long ksunt_id);
void js_upcall_set_form_element_value(void *smirak, long document_id, long ksunt_id, unsigned char *name);
void js_upcall_click(void *smirak, long document_id, long elem_id);
void js_upcall_focus(void *smirak, long document_id, long elem_id);
void js_upcall_blur(void *smirak, long document_id, long elem_id);
void js_upcall_submit(void *bidak, long document_id, long form_id);
void js_upcall_reset(void *bidak, long document_id, long form_id);
int js_upcall_get_radio_length(void *smirak, long document_id, long radio_id); /* radio.length */
int js_upcall_get_select_length(void *smirak, long document_id, long select_id); /* select.length */
int js_upcall_get_select_index(void *smirak, long document_id, long select_id); /* select.selectedIndex */
struct js_select_item* js_upcall_get_select_options(void *smirak, long document_id, long select_id, int *n);
void js_upcall_goto_url(void* struct_fax_me_tender_string);
int js_upcall_get_history_length(void *context);
void js_upcall_goto_history(void * data);
void js_upcall_set_default_status(void *context, unsigned char *tak_se_ukaz_Kolbene);
unsigned char* js_upcall_get_default_status(void *context);
void js_upcall_set_status(void *context, unsigned char *tak_se_ukaz_Kolbene);
unsigned char* js_upcall_get_status(void *context);
unsigned char * js_upcall_get_cookies(void *context);
long *js_upcall_get_images(void *smirak, long document_id, int *len);
long * js_upcall_get_all(void *context, long document_id, int *len);
int js_upcall_get_image_width(void *smirak, long document_id, long image_id);
int js_upcall_get_image_height(void *smirak, long document_id, long image_id);
int js_upcall_get_image_border(void *smirak, long document_id, long image_id);
int js_upcall_get_image_vspace(void *smirak, long document_id, long image_id);
int js_upcall_get_image_hspace(void *smirak, long document_id, long image_id);
unsigned char * js_upcall_get_image_name(void *smirak, long document_id, long image_id);
unsigned char * js_upcall_get_image_alt(void *smirak, long document_id, long image_id);
void js_upcall_set_image_name(void *smirak, long document_id, long image_id, unsigned char *name);
void js_upcall_set_image_alt(void *smirak, long document_id, long image_id, unsigned char *alt);
unsigned char * js_upcall_get_image_src(void *smirak, long document_id, long image_id);
void js_upcall_set_image_src(void *chuligane);
int js_upcall_image_complete(void *smirak, long document_id, long image_id);
long js_upcall_get_parent(void *smirak, long frame_id);
long js_upcall_get_frame_top(void *smirak, long frame_id);
long * js_upcall_get_subframes(void *smirak, long frame_id, int *count);


void js_downcall_vezmi_true(void *context);
void js_downcall_vezmi_false(void *context);
void js_downcall_vezmi_null(void *context);
void js_downcall_game_over(void *context);
void js_downcall_quiet_game_over(void *context);
void js_downcall_vezmi_int(void *context, int i);
void js_downcall_vezmi_float(void*context,double f);
/*void js_downcall_vezmi_float(void *context, float f);*/
void js_downcall_vezmi_string(void *context, unsigned char *string);

#endif
