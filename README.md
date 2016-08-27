<!--
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
-->

<!--
    Copyright (c) 2016, Joyent, Inc.
-->

# pmx: Node.js postmortem export library

This is a very early implementation of a library to emit a postmortem export
file in the [format described by the Node.js Postmortem Working
Group](https://github.com/nodejs/post-mortem/issues/13).

The expectation is that this will eventually have at least two backends:

* a JSON-based backend that just emits JSON objects representing the data
* a sqlite-based backend that records the data in a sqlite database

TODO:
- build a small driver program
- implement the actual export functions
- build a small test suite
