/* Global history - stolen from ELinks */

#include "links.h"

#ifdef GLOBHIST

struct global_history global_history = {
	0,
	{ &global_history.items, &global_history.items }
};

void free_global_history_item(struct global_history_item *item)
{
        if(item->title) mem_free(item->title);
	if(item->url) mem_free(item->url);
}

void delete_global_history_item(struct global_history_item *item)
{
	free_global_history_item(item);
	del_from_list(item);
	mem_free(item);
	global_history.n--;
}

/* Search global history for item matching url. */
struct global_history_item *get_global_history_item(unsigned char *url)
{
	struct global_history_item *item;

	if (!url) return NULL;

        if (!options_get_bool("document_history_global_enable"))
		return NULL;

        /* Search for matching entry. */
        foreach(item,global_history.items)
                if(!strcmp(url,item->url))
                        return item;
        return NULL;
}


/* Add a new entry in history list, take care of duplicate, respect history
 * size limit, and update any open history dialogs. */
void add_global_history_item(unsigned char *url, unsigned char *title, ttime time)
{
	struct global_history_item *item;
	int max_globhist_items;

	if (!options_get_bool("document_history_global_enable"))
		return;

	if (!title || !url)
		return;

	max_globhist_items = options_get_int("document_history_global_max_items");

	item = get_global_history_item(url);
        if (item) delete_global_history_item(item);

	while (global_history.n >= max_globhist_items) {
		item = global_history.items.prev;

		if ((void *) item == &global_history.items) {
			internal("global history is empty");
                        global_history.n = 0;
                        return;
                }

                delete_global_history_item(item);
        }

	item = mem_alloc(sizeof(struct global_history_item));
	if (!item)
		return;

	item->last_visit = time;
	item->title = stracpy(title);
	if (!item->title) {
		mem_free(item);
		return;
        }
	item->url = stracpy(url);
	if (!item->url) {
		mem_free(item->title);
		mem_free(item);
		return;
	}
	item->refcount = 0;

	add_to_list(global_history.items, item);
	global_history.n++;
}

int globhist_simple_search(unsigned char *search_url, unsigned char *search_title)
{
	return 0;
}


void read_global_history()
{
	unsigned char in_buffer[MAX_STR_LEN];
	unsigned char *file_name;
	unsigned char *title, *url, *last_visit;
	FILE *f;

	if (!options_get_bool("document_history_global_enable"))
		return;

	file_name = straconcat(links_home, "globhist", NULL);
	if (!file_name) return;

	f = fopen(file_name, "r");
	mem_free(file_name);
	if (f == NULL)
		return;

	title = in_buffer;

	while (fgets(in_buffer, MAX_STR_LEN, f)) {
		url = strchr(in_buffer, '\t');
		if (!url)
			continue;
		*url = '\0';
		url++; /* Now url points to the character after \t. */

		last_visit = strchr(url, '\t');
		if (!last_visit)
			continue;
		*last_visit = '\0';
		last_visit++;

		/* Is using atol() in this way acceptable? It seems
		 * non-portable to me; ttime might not be a long. -- Miciah */
		add_global_history_item(url, title, atol(last_visit));
	}

	fclose(f);
}

void write_global_history()
{
	struct global_history_item *item;
	unsigned char *file_name;
        FILE *f;

	if (!options_get_bool("document_history_global_enable"))
		return;

	file_name = straconcat(links_home, "globhist", NULL);
	if (!file_name) return;

	f=fopen(file_name, "w"); /* rw for user only */
	mem_free(file_name);
	if (!f) return;

	foreachback (item, global_history.items) {
		unsigned char *p;
		int i;
		int bad = 0;

		p = item->title;
		for (i = strlen(p) - 1; i >= 0; i--)
			if (p[i] < ' ')
				p[i] = ' ';

		p = item->url;
		for (i = strlen(p) - 1; i >= 0; i--)
			if (p[i] < ' ')
				bad = 1; /* Incorrect url, skip it. */

		if (bad) continue;

		if (fprintf(f, "%s\t%s\t%ld\n",
				   item->title,
				   item->url,
				   item->last_visit) < 0) break;
	}

	fclose(f);
}

static void free_global_history()
{
	struct global_history_item *item;

	foreach (item, global_history.items) {
		free_global_history_item(item);
	}

        free_list(global_history.items);
}

void finalize_global_history()
{
	write_global_history();
	free_global_history();
}


#endif
