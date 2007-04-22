#include "links.h"

#include "options_hooks.h"

void options_copy_item(void *in, void *out);

/* Just reject change - uneditable option */
OPTIONS_HOOK(reject_hook)
{
        return 0;
}

/* A bit hackish... I'm too lazy to implement in other way --karpov */

extern struct list_description bookmark_ld;

OPTIONS_HOOK(bookmarks_hook)
{
        if(!bookmark_ld.open){
                options_copy_item(current, changed);

                reinit_bookmarks();
        }

        return 1;
}

OPTIONS_HOOK(bfu_hook)
{
        struct session *s; /* we have 'ses' already */

#ifdef G
        if(F){
                options_copy_item(changed, current);

                shutdown_bfu();
                init_bfu();
                init_grview();

                foreach(s, sessions)
                        s->term->dev->resize_handler(s->term->dev);
        }
#endif
        return 1;
}

OPTIONS_HOOK(html_hook)
{
        options_copy_item(changed, current);

        html_interpret_recursive(ses->screen);
        draw_formatted(ses);

        cls_redraw_all_terminals();

        return 1;
}

OPTIONS_HOOK(video_hook)
{
#ifdef G
        struct rect r = {0, 0, 0, 0};
	struct terminal *term;

        if(!F) return 1;

        options_copy_item(changed, current);

        update_aspect();
        gamma_stamp++;
	
	/* Flush font cache */
	destroy_font_cache();

	/* Flush dip_get_color cache */
	gamma_cache_rgb = -2;

	/* Recompute dithering tables for the new gamma */
        init_dither(drv->depth);

	shutdown_bfu();
	init_bfu();
	init_grview();

	html_interpret_recursive(ses->screen);
	draw_formatted(ses);
	/* Redraw all terminals */
	foreach(term, terminals){
		memcpy(&r, &term->dev->size, sizeof r);
		t_redraw(term->dev, &r);
        }
#endif
        return 1;
}

OPTIONS_HOOK(cache_hook)
{
        options_copy_item(changed, current);

        shrink_memory(SH_CHECK_QUOTA);

        return 1;
}

OPTIONS_HOOK(js_hook)
{
#ifdef JS
        options_copy_item(changed, current);

        /* Find and execute all <stript>'s in document */
        if (ses->screen->f_data)
                jsint_scan_script_tags(ses->screen);

        if (!options_get_bool("js_enable")) {
                /* Kill all running scripts */
                if (ses->default_status) {
                        mem_free(ses->default_status);
                        ses->default_status=NULL;
                }
		jsint_kill_recursively(ses->screen);
        }

	/* reparse document  */
	html_interpret_recursive(ses->screen);
	draw_formatted(ses);

#endif
        return 1;
}

OPTIONS_HOOK(language_hook)
{
        int i;

        for (i = 0; i < n_languages(); i++)
                if (!(_stricmp(language_name(i),
                                 changed->value))) {
                        /* Language exists */
                        set_language(i);
                        return 1;
                }
        /* Incorrect language name, reject it */
        return 0;
}
