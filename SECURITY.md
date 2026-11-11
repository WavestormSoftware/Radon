# Security

Radon parses untrusted data in two places: TrueType fonts (through the vendored stb_truetype) and UTF-8
text. Load fonts only from sources you trust; stb_truetype does not defend against malicious font files.

## Reporting a vulnerability

Please do not open a public issue. Use GitHub's **Report a vulnerability** button under the repository's
Security tab (private vulnerability reporting). Include the affected version, a description, and a way
to reproduce it. You will get a reply as soon as a maintainer has looked at it.

Only the latest release is supported with fixes.
