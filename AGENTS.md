# AGENTS.md

Guidance for AI agents contributing to Kodi.

## Scope

Keep changes focused on the task that was requested.

* Make the smallest change that fully solves the stated problem.
* Treat the issue, pull request, or user request as the scope boundary.
* Do not fix unrelated problems discovered while working on the requested change.
* Do not expand a bug fix into a refactor unless the refactor is necessary to fix the bug.
* Do not add speculative hardening for hypothetical scenarios that are outside the stated scope.
* Do not preserve legacy behavior or compatibility when the task explicitly allows or requires a breaking change.
* Avoid unrelated cleanup, renaming, formatting, or modernization.
* Do not propose or perform additional work that is not required by the requested change unless explicitly asked.

If the implementation grows materially beyond the original task, stop expanding it and report why additional scope appears necessary.

## Comments

AI-generated comments should be concise and only explain information that is not obvious from the code.

* Do not restate the code in prose.
* Do not mention approaches that were tried, considered, replaced, or removed during implementation.
* Do not leave comments that only make sense in the context of an agent's iteration history.
* Prefer deleting an unnecessary comment over expanding or rewording it.
* Document an idea once, where it naturally belongs.
* Put implementation history, discarded alternatives, and review rationale in the commit message or PR description unless they are necessary to understand the finished code.
* Do not remove, rewrite, or shorten existing human-written comments unless the requested change makes them incorrect or explicitly asks for comment cleanup.

Before submitting, review only comments you added or changed and remove anything that does not help a future reader understand the code. Preserve existing comments outside the scope of the requested change.

## Commit messages

Keep commit messages concise and focused on information needed to understand the change.

* Prefer a short subject and a brief body explaining why the change is needed.
* Do not exhaustively describe implementation details that are already clear from the diff.
* In a multi-commit series, avoid repeating context covered by earlier commits or the pull request description.
* Do not turn commit messages into test reports, review histories, or inventories of every behavior covered.
* Wrap all commit message body text at 76 columns. Do not exceed 76 columns except for URLs or other unbreakable text.

## AI use and disclosure

AI assistance is welcome but optional. Human-written contributions
are equally valued, including work done without AI for educational,
ethical, or personal reasons. Contributors must not be pressured to
use AI or required to declare that they did not use it.

### Human authorship and project context

Respect deliberate human authorship. Writing code without AI can be a
principled decision about learning, ownership, and understanding the
result. Do not frame that effort as a problem to automate away.

[Vship](https://codeberg.org/Line-fr/Vship) is a useful example. Its
maintainer describes this GPU video-metrics library as human-written
and human-maintained, opposes AI, yet remains open to reviewing
useful AI-assisted contributions that can be maintained.

Style also serves a project's needs. In earlier Vship code, GPU
kernels live in headers, tiled Gaussian filters use shared memory,
and scratch planes are deliberately reused to control VRAM use.
Together with support for multiple GPU backends and configurable
concurrency, these choices address particular performance and
portability needs. They are not blanket best practices for Kodi.
Judge unusual code by correctness, actual constraints, and design
intent, not superficial conformity to generic style rules.

### Contributors

* Disclose AI models used to materially generate or modify code,
  tests, build files, or documentation.
* Identify each model and version, along with its reasoning effort
  setting when available.
* Report the actual configured effort, not an estimate inferred by
  the model. If unavailable, ask the human operator. If the setting
  is genuinely inaccessible, explicitly report that it is unknown
  or not exposed. Never guess.
* List multiple models when they materially contributed to the
  submitted changes.
* The human contributor remains responsible for the correctness,
  scope, and maintainability of all submitted work.

### Reviewers

* AI-assisted reviews must disclose their model/version and effort
  setting, even when no defects are found. The same rules for asking
  the operator, reporting unavailable effort, and never guessing
  apply to reviews.
* Report missing required disclosure on known AI-assisted
  contributions or reviews as a blocking finding, even when no code
  defects are found. Do not approve until disclosure is provided.
* Do not assume that a contribution used AI merely because it lacks
  disclosure. Do not infer AI authorship from coding style, change
  size, test coverage, or other unreliable indicators.
* When there is a concrete reason to question undisclosed AI use,
  ask the contributor neutrally rather than asserting AI authorship.
* Do not fail a review solely because a human-written contribution
  lacks a declaration of non-use.
* Apply the same correctness, security, scope, and maintainability
  standards to human-written and AI-assisted contributions.

AI disclosure provides context, not proof of quality. Evaluate the
submitted changes on their merits rather than trusting or distrusting
a particular model.

## Reviews

Review only for issues that should block the change.

* Report only regressions, correctness problems, build failures, security issues, or behavior that contradicts the stated intent.
* Do not suggest optional improvements, cleanup, alternative wording, refactoring, additional hardening, or follow-up work unless explicitly asked.
* Do not report speculative edge cases unless they are reasonably reachable in supported use and materially affect correctness.
* Do not require unrelated pre-existing problems to be fixed.
* Respect the stated scope and approved compatibility boundaries.
* Consider compiler, hardware, memory, and ABI constraints before
  labeling unconventional implementation choices as defects.
* If there are no blocking findings, say so and stop.
* Report spelling mistakes as review findings.

A review should not create additional work merely because additional work is possible.

## Change size

Large changes have a disproportionately high review and maintenance cost.

* Prefer small, independently reviewable changes.
* Do not combine a functional fix, unrelated cleanup, and architectural refactoring in one change.
* When a substantial refactor or feature is required, establish the architecture and intended scope before generating a large implementation.
* If implementation reveals that a substantially larger change is necessary, report that before continuing to expand the diff.
* Do not generate additional abstraction, infrastructure, or compatibility layers without a demonstrated need in the requested change.

Generated code is not free simply because it is easy to produce. Optimize for code that needs to exist and can be reviewed and maintained.

## Release stabilization

When working on a release branch or during Beta/RC stabilization, minimize change risk.

* Prefer targeted fixes over refactoring.
* Avoid architectural changes, API changes, and unrelated cleanup unless explicitly requested.
* Do not broaden a release-blocking fix to cover adjacent improvements that can wait for the next development cycle.
* Keep fixes easy to review, test, and revert.

## Before submitting

Review the final diff as a human reviewer would.

* Remove unrelated changes and formatting churn.
* Remove code that is not necessary for the requested behavior.
* Check that tests are proportional to the change and test the intended behavior.
* Do not introduce or substantially modify CI workflows, test
  infrastructure, or unrelated test suites unless necessary for
  the requested change. Keep testing proportional to the fix.
* Confirm that no speculative edge-case handling substantially expanded the implementation.
* Confirm that every changed file is necessary for the task.

## AI credit

This document was originally generated with Astra Pro.
