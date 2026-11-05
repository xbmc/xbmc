![Kodi Logo](resources/banner_slim.png)

# Security Policy

## Supported versions
Security fixes are made on the `master` branch and backported to the current stable release series, as well as to any upcoming release branch. Older release series do not receive security updates; please upgrade to the latest stable release.

## Reporting a vulnerability
**Please do not report or highlight security vulnerabilities through any public mechanism, including GitHub issues, pull requests, the forum, blog posts or videos.**

Report them privately through GitHub instead:

1. Go to the **[Security tab](https://github.com/xbmc/xbmc/security)** of this repository.
2. Click **Report a vulnerability** and fill in the form.

Please include as much of the following as you can:
* The affected Kodi version(s) and platform(s)
* The affected component (e.g. a demuxer, the web server, JSON-RPC, a subtitle parser)
* Steps to reproduce, and a proof-of-concept or sample file if available
* The impact you believe an attacker could achieve

## What to expect
Team Kodi is made up of volunteers, so please be patient. We will acknowledge your report and keep you updated in the private advisory as we triage it. If we confirm the issue, we will:
* Work on a fix in a private fork and coordinate the release timing with you.
* Ship the fix in a security release, announced on the **[Kodi news page](https://kodi.tv/blog/)** with advice to update.
* Publish a **[GitHub security advisory](https://github.com/xbmc/xbmc/security/advisories)** once the fixed release is available, request a CVE where appropriate and credit you unless you prefer to remain anonymous.

Please keep the details private until the advisory is published.

## Scope
This policy covers the code in this repository. Please report issues in:
* **Third-party add-ons and add-on repositories** to their respective authors.
* **Bundled third-party libraries** (e.g. FFmpeg) to their upstream projects. If Kodi is affected in a way specific to how it uses the library, report that here as well.
