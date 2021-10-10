# :hammer_and_wrench: cutils
[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE) ![Build status](https://github.com/abobija/cutils/actions/workflows/c-cpp.yml/badge.svg)

Set of compact and portable C utilities with the intention of being very fast, efficient and easy to use. Their main purpose is to be used in IoT and embedded systems.

## Components

List of components.

| Name          | Description   | Linux         | Windows       |
| ------------- | ------------- | ------------- | ------------- |
| `cutils` | Set of handy macros for object constructing, error checking, etc... | Yes | Yes |
| `estr` | String extension helpers | Yes | Yes |
| `xlist` | Doubly linked list (DLL) | Yes | Yes |
| `wxp` | String expander (similar to [wordexp](https://man7.org/linux/man-pages/man3/wordexp.3.html)) | Yes | Yes |
| `cmder` | Commander (wrapper around [getopt](https://man7.org/linux/man-pages/man3/getopt.3.html)) | Yes | Yes

## Build

[Make](https://www.gnu.org/software/make/manual/make.html#Overview) is used for controlling compilation process. Building supported on Linux and Windows (not tested on other platforms).

| Command  | Description |
| ------------- | ------------- |
| `make all` | Build all components |
| `make` | Same as `make all` |
| `make COMPONENT` | Build component, where `COMPONENT` is component name |
| `make test` | Run all tests |
| `make test.COMPONENT` | Run component test, where `COMPONENT` is component name |
| `make clean` | Clean the project |

## Author

GitHub: [abobija](https://github.com/abobija)<br>
Homepage: [abobija.com](https://abobija.com)

## License

[MIT](LICENSE)