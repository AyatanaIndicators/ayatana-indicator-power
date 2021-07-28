# Build and installation instructions

## Compile-time build dependencies

 - cmake (>= 3.13)
 - cmake-extras
 - libayatana-common (>= 0.9.3)
 - glib-2.0 (>= 2.36)
 - libnotify (>=0.7.6)
 - gettext (>= 0.18)
 - systemd
 - gcovr (>= 2.4)
 - lcov (>= 1.9)
 - gtest (>= 1.6.0)

## For end-users and packagers

```
cd ayatana-indicator-power-X.Y.Z
mkdir build
cd build
cmake ..
make
sudo make install
```

**The install prefix defaults to `/usr`, change it with `-DCMAKE_INSTALL_PREFIX=/some/path`**

## For testers - unit tests only

```
cd ayatana-indicator-power-X.Y.Z
mkdir build
cd build
cmake .. -DENABLE_TESTS=ON
make
make test
make cppcheck
```

## For testers - both unit tests and code coverage

```
cd ayatana-indicator-power-X.Y.Z
mkdir build-coverage
cd build-coverage
cmake .. -DENABLE_COVERAGE=ON
make
make coverage-html
```
