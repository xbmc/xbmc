/* New options system */

#include "links.h"

void free_options(void)
{
	struct options *opt;

	foreach(opt, options) {
		if(opt->name) mem_free(opt->name);
		if(opt->value) mem_free(opt->value);
        }

	free_list(options);
}

void finalize_options()
{
        free_options();
}


/* returns previous item in the same folder and with same the depth, or father if there's no previous item */
/* we suppose that previous items have correct pointer fotr */
struct options *options_previous_on_this_level(struct options *item)
{
	struct options *p;

	for (p=item->prev;p->depth>item->depth;p=p->fotr);
	return p;
}

/* add optgroup if name is empty */

void register_option(unsigned char *name, unsigned char *title, int opt_type, unsigned char *value,int depth)
{
        struct options *b,*p;

        if(!title) return;

        b=mem_alloc(sizeof(struct options));
        if(!b) return;

        b->title = title;
        b->opt_type=opt_type;
        if(name){
                b->name=stracpy(name);
                b->value=stracpy(value);
                b->type=0;
        } else {
                b->name=stracpy("");
                b->value=stracpy("");
                b->type=1;
        }
        b->depth=depth;

        p=options.prev;
        b->prev=p;
        b->next=(struct options *)(&options);
        p->next=b;
        options.prev=b;

        p=options_previous_on_this_level(b);
	if (p->depth<b->depth)b->fotr=p;   /* directory b belongs into */
	else b->fotr=p->fotr;

        b->change_hook = NULL; /* You must set change_hook by hand - use options_set_hook */
}

void *options_find(unsigned char *name)
{
        struct options *opt;
        if(name) {
                foreach(opt,options)
                        if(opt->name){
                                if(!strcmp(opt->name,name))
                                        return opt;
                        }
        }
        debug("Can't find option %s!\n",name);
        return NULL;
}

void options_set_hook(unsigned char *name,
                      int (*change_hook)(struct session *,
                                         struct options *current,
                                         struct options *changed))
{
        struct options *opt = options_find(name);

        if(opt)
                opt->change_hook = change_hook;
}

unsigned char *options_get(unsigned char *name)
{
        struct options *opt=(struct options *)options_find(name);
        if(opt){
                if(opt->value &&
                   *(opt->value))
                        return opt->value;
                else
                        return NULL;
        }
        return NULL;
}

int options_get_int(unsigned char *name)
{
        unsigned char *str_value=options_get(name);
        int value=0;
        if(str_value){
                sscanf(str_value,"%d",&value);
        }
        return value;
}

int options_get_bool(unsigned char *name)
{
        if(options_get_int(name))
                return 1;
        else
                return 0;
}

double options_get_double(unsigned char *name)
{
        unsigned char *str_value=options_get(name);
        double value=0;
        if(str_value){
                sscanf(str_value,"%lf",&value);
        }
        return value;
}

struct rgb options_get_rgb(unsigned char *name)
{
        struct rgb value= {0,0,0};
        unsigned char *str_value=options_get(name);
        if(str_value){
                decode_color(str_value,&value);
        }
        return value;
}

int options_get_rgb_int(unsigned char *name)
{
        struct rgb value= options_get_rgb(name);
        return (256*value.r+value.g)*256+value.b;
}

int options_get_cp(unsigned char *name)
{
        unsigned char *value=options_get(name);
        return get_cp_index(value);
}

void options_set(unsigned char *name, unsigned char *value)
{
        struct options *opt=(struct options *)options_find(name);

        if(!opt || opt->type)
                return;

        if(opt->value)
                mem_free(opt->value);

        opt->value=stracpy(value);
}

void options_set_int(unsigned char *name, int value)
{
        unsigned char *str=init_str();
        int l=0;
        add_num_to_str(&str,&l,value);
        options_set(name,str);
        if(str) mem_free(str);
}

void options_set_bool(unsigned char *name, int value)
{
        unsigned char *str=init_str();
        int l=0;
        add_num_to_str(&str,&l,value);
        options_set(name,str);
        if(str) mem_free(str);
}

void options_set_double(unsigned char *name, double value)
{
        /*
        unsigned char *str=init_str();
        int l=0;
        add_dblnum_to_str(&str,&l,value);
        options_set(name,str);
        if(str) mem_free(str);
        */
        /* ...just to handle local correctly */
        unsigned char *str=mem_alloc(15);
        snprintf(str,14,"%.13g",value);
        options_set(name,str);
        if(str) mem_free(str);
}

void options_set_rgb_int(unsigned char *name, int value)
{
        unsigned char *str=mem_alloc(7);
        snprintf(str,7,"%06x",value);
        options_set(name,str);
        if(str) mem_free(str);
}

void options_set_rgb(unsigned char *name, struct rgb value)
{
        options_set_rgb_int(name, (256*value.r+value.g)*256+value.b);
}

void load_options()
{
	FILE *f;
        unsigned char *options_file = stracpy (links_home);

        if (!options_file)
                goto load_failure;

        add_to_strn(&options_file, "options");

        f = fopen(options_file, "r");
        mem_free(options_file);

        if (!f){
                goto load_failure;
        }


        while(!feof(f)){
                unsigned char *tmp=mem_alloc(MAX_STR_LEN);
                unsigned char *name=mem_alloc(MAX_STR_LEN);
                unsigned char *value=tmp;
                unsigned char *vvv;

				if(fgets(tmp,MAX_STR_LEN,f)){
                        /* Very ugly realization of option string parsing --karpov */
                        //strcpy(name,tmp);
						sscanf(tmp,"%s",name);

                        while(*value!='"')
                                value++;
                        value++;

                        vvv=value;
#ifdef __XBOX__
						while(*vvv!='"') vvv++;
#else
						while((*vvv!='"')||
                              (*vvv=='"' &&
                               *(vvv-1)=='\\')) vvv++;
#endif
                        *vvv='\0';
                        options_set(name,value);
                }

				mem_free(tmp);
                mem_free(name);
        }
        fclose(f);

        return;

load_failure:
		OutputDebugString("Can't load options!\n");
}

void save_options()
{
        struct options *opt;
	FILE *f;
        unsigned char *options_file = stracpy(links_home);

        if (!options_file)
                goto save_failure;

        add_to_strn(&options_file, "options");

        f = fopen(options_file, "w");
        mem_free (options_file);

        if (!f){
                goto save_failure;
        }

        /* We assume here that options struct is sane - no null names */
        foreach(opt,options)
                if(!opt->type)
                        fprintf(f,
                                "%s \"%s\"\n",
                                opt->name,
                                (opt->value)
                                ? (opt->value)
                                : (unsigned char *)""
                               );
        fclose(f);

        return;

save_failure:
        internal("Can't save options!\n");
}

void init_options()
{
        register_options();
        load_options();
}
