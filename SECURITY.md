# Security Policy

## Supported versions

Only the **latest release** of MicVST receives fixes. Please update to the newest version
(see [Releases](https://github.com/philipz794/MicVST/releases)) before reporting an issue.

## Scope

MicVST is a small **local** Windows app: it processes audio between local devices and a virtual
audio cable. It has **no telemetry and no auto-installer**. The only network access is an
**optional, opt-in update check** (off by default): when enabled, on startup it makes a single
request to the public GitHub releases API to see whether a newer version exists. The main areas of
interest are third-party VST3 plugins it loads and the config/log files it writes under
`%APPDATA%\MicVST\`.

## Reporting a vulnerability

Please report security issues **privately** via GitHub — do not open a public issue.

Use **"Report a vulnerability"** in the repository's **Security** tab
(*Security → Advisories → Report a vulnerability*). This opens a private security advisory that is
visible only to the maintainer.

I'll acknowledge your report as soon as I can and work on a fix. Thanks for helping keep
MicVST and its users safe.
