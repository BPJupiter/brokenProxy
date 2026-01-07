
# The Broken Proxy

I'm Frances Telfar, and this is my University of Auckland Summer Research Project supervised by Ulrich Speidel, in collaboration with the Catalyst project. This proxy is built to simulate macro-scale internet outages, currently at the cable level. Very much still in development.

## Settings Page
Here you can hotload proxy configuration.

`http://127.0.0.1:13406`

## Build
This project is written to the gnu89 standard, and as such is Linux-only for now. Future cross-platform c89 integration is tentatively planned.

Build with
`$ make release` in the project root.

## Curl commandline args
For testing with curl, use the following cmdline arguments:

`$ curl -x "http://127.0.0.1:13406" <url>`

## Chromium commandline args
For testing on Chromium, use the following cmdline arguments:

`$ chromium --proxy-server="127.0.0.1:13406" --user-data-dir="/tmp/chromium_proxy_session/"`
