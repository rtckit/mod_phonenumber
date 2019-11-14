# mod_phonenumber
> Exposes [libphonenumber](https://github.com/googlei18n/libphonenumber) to [FreeSWITCH](https://github.com/signalwire/freeswitch)

![Version](https://img.shields.io/badge/version-v1.0.0-green) ![License](https://img.shields.io/badge/license-MIT-blue)

## Installation

As with FreeSWITCH, Debian is reference platform for mod_phonenumber:

```sh
curl -sL \
  https://rtckit.io/mod_phonenumber/freeswitch-mod-phonenumber_`curl -s https://rtckit.io/mod_phonenumber/release.txt`_`dpkg --print-architecture`.deb \
  -o freeswitch-mod-phonenumber.deb
sudo dpkg -i freeswitch-mod-phonenumber.deb
```

Make sure the module is added to `modules.conf.xml` or loaded manually (e.g. `load mod_phonenumber`).

## Usage

mod_phonenumber has been developed against FreeSWITCH 1.10+; although it may work well with previous versions, they are not explicitly supported.

The module can be used in three modes: as a dialplan application, through the API interface or whenever a new channel is created through hooks. Regardless of the invocation mode, it expects:
 * one or mode actions to execute
 * a phone number to execute the actions against
 * a set of parameters

Please refer to [rtckit.io/mod_phonenumber/](https://rtckit.io/mod_phonenumber/) for the complete documentation.

## Build

In order to build the module from source, the environment requires the typical C/C++ build essentials for your platform (`build-essential`), the FreeSWITCH development files (`libfreeswitch-dev`) as well as the libicu and libphonenumber libraries and their respective development files (`libicu-dev` and `libphonenumber-dev`).

```sh
make
make install
```

## Tests

Before running the test suite, make sure sure the module is built and installed.

```sh
make check
```

## License

MIT, see [LICENSE file](LICENSE).

### Acknowledgments

* [FreeSWITCH](https://github.com/signalwire/freeswitch) core developers
* [libphonenumber](https://github.com/google/libphonenumber) by Google
* [ICU Project](https://github.com/unicode-org/icu)

### Contributing

Bug reports (and small patches) can be submitted via the [issue tracker](https://github.com/rtckit/mod_phonenumber/issues). Forking the repository and submitting a Pull Request is preferred for substantial patches.

If you believe the module returns incorrect results, please test the input against libphonenumber's [online demo](https://libphonenumber.appspot.com/) application and if the results are different, please raise an issue here. Otherwise, it could be an issue in the upstream and the bug should be reported in the [libphonenumber repository](https://github.com/google/libphonenumber).
