# Third-Party Notices

PureLeaf incorporates the following third-party software, each under its own
license. The full license texts are reproduced below, and where applicable,
referenced to the bundled copy in the repository.

---

## Qt 6

```
URL:       https://www.qt.io/
Copyright: The Qt Company Ltd.
License:   GNU Lesser General Public License v3 (LGPLv3)

PureLeaf uses Qt under the LGPLv3. The dynamically-linked Qt libraries are not
modified. For details, see:
  https://www.qt.io/licensing/
```

---

## QWindowKit

```
URL:       https://github.com/stdware/qwindowkit
Copyright: (C) 2023-2025 Stdware Collections
           (C) 2021-2023 wangwenx190 (Yuhang Zhao)
License:   Apache License 2.0
Bundled:   third_party/qwindowkit/

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at:

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

A copy of the Apache License 2.0 is included at:
  third_party/qwindowkit/LICENSE
```

---

## SQLite3

```
URL:       https://www.sqlite.org/
License:   Public Domain

The SQLite amalgamation (sqlite3.c / sqlite3.h) is downloaded at build time
via CMake FetchContent from https://www.sqlite.org/download.html.

SQLite is in the Public Domain. The authors hereby grant permission to use,
copy, modify, and distribute the code for any purpose, with or without fee.
No warranty is provided.
```

---

## Lucide Icons

```
URL:       https://lucide.dev/
Copyright: (c) 2026 Lucide Icons and Contributors
License:   ISC License
Bundled:   apps/desktop-qt/resources/icons/lucide/

Copyright (c) 2026 Lucide Icons and Contributors

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

A copy of the ISC License is included at:
  apps/desktop-qt/resources/icons/lucide/LICENSE
```

---

## Feather Icons (subset via Lucide)

```
URL:       https://feathericons.com/
Copyright: (c) 2013-present Cole Bemis
License:   MIT License
Bundled:   apps/desktop-qt/resources/icons/lucide/ (arrow-left, minus,
           square, trash-2, x — adapted by Lucide)

The MIT License (MIT)

Copyright (c) 2013-present Cole Bemis

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

---

## GoogleTest

```
URL:       https://github.com/google/googletest
Copyright: (c) 2008 Google Inc.
License:   BSD 3-Clause License
Usage:     Build-time only (test framework); downloaded via CMake
           FetchContent. Not distributed with the application binary.
```

---

## CMake

```
URL:       https://cmake.org/
License:   BSD 3-Clause License
Usage:     Build-time only (build system). Not distributed.
```

---

## Ninja

```
URL:       https://ninja-build.org/
License:   Apache License 2.0
Usage:     Build-time only (build executor). Not distributed.
```
