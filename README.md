## Installing

If GNURadio is installed in a non-standard location, run:

```shell
PKG_CONFIG_PATH=$HOME/opt/sdr/lib/pkgconfig ./configure --prefix=$HOME/opt/sdr CPPFLAGS=-I$HOME/opt/sdr/include LDFLAGS=-L$HOME/opt/sdr/lib
```
