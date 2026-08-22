# Copilot instructions

## Comments

Default to no comments. Code with clear names already says what it does — don't restate that.

Only add a comment when it captures something the code cannot express on its own:
- A non-obvious constraint or invariant ("must run before X or Y breaks")
- A workaround for a specific external bug/quirk, ideally with a reference
- Domain/external knowledge a competent developer wouldn't have without reading
  other source (e.g. "Kodi's NetworkServices::Start() disables the webserver
  when auth is on with an empty password")
- Behavior that would genuinely surprise a reader skimming the diff

Before writing a comment, ask: if I deleted this, would a competent reader be
confused or make a wrong assumption? If no, delete it.

Never write comments that:
- Describe what the next line does in plain English ("# loop over items")
- Reference the current task/ticket/PR ("added for the login fix")
- Restate the function/variable name in prose
- Span multiple paragraphs — one line, max two, per comment
- Use non-ASCII characters (no smart quotes, em dashes, arrows, etc.) — plain ASCII only
