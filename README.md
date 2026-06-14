
# The Broken Proxy

This started as a University of Auckland Summer Research Project supervised by Ulrich Speidel, in collaboration with the VULGEO project. This proxy is built to simulate macro-scale internet outages.

## License

This project is licnsed under the **PolyForm Noncommercial License 1.0.0**.
You Are free to use, modify, and distribute this software for non-commercial, academic, and researched purposes. **Commercial use is strictly prohibited** without prior written permission.
For commercial licensing inquiries, please contact: [Frances Telfar: atel215@aucklanduni.ac.nz]
See the [LICENSE](LICENSE) for more details.

## Settings Page

Here you can hotload proxy configuration.

`http://127.0.0.1:13406`

## Build

This project is written to the C89 standard on Windows and Linux.

Windows: Build with MSVC

Linux: Build with

``cmake -S . -B build``

``cmake --build build``

## Curl commandline args

For testing with curl, use the following cmdline arguments:

```
curl -x socks5h://127.0.0.1:13407 <url>
```

## Chromium commandline args

For testing on Chromium, use the following cmdline arguments:

```
chromium --proxy-server="socks5://127.0.0.1:13407" --user-data-dir="/tmp/chromium_proxy_serssion" 
```
