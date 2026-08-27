# AGENTS.md

Guidance for AI agents contributing to Kodi.

## Comments

AI-generated comments should be concise and only explain information that is not obvious from the code.

* Do not restate the code in prose.
* Do not mention approaches that were tried, considered, replaced, or removed during implementation.
* Do not leave comments that only make sense in the context of an agent's iteration history.
* Prefer deleting an unnecessary comment over expanding or rewording it.
* Document an idea once, where it naturally belongs.
* Put implementation history, discarded alternatives, and review rationale in the commit message or PR description unless they are necessary to understand the finished code.

Before submitting, review every comment you added and remove anything that does not help a future reader understand the code.
