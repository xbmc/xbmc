/*
 *
 * Hooks that will be called on options values change. Format is
 *
 * int change_hook(struct session *,
 *                 struct options *current,
 *                 struct options *changed);
 *
 * Zero return value means we reject this change.
 *
 */

#define OPTIONS_HOOK(name) int name(struct session *ses, struct options *current, struct options *changed)

OPTIONS_HOOK(reject_hook);
OPTIONS_HOOK(bookmarks_hook);
OPTIONS_HOOK(bfu_hook);
OPTIONS_HOOK(html_hook);
OPTIONS_HOOK(video_hook);
OPTIONS_HOOK(cache_hook);
OPTIONS_HOOK(js_hook);
OPTIONS_HOOK(language_hook);
#ifdef __XBOX__
OPTIONS_HOOK(linksboks_options_hook);
void register_options_xbox();
#endif
