# WiFiSpace

Copyright Thomas Habets <thomas@habets.se> 2016

GNURadio program for measuring how busy wifi channels are.

https://github.com/ThomasHabets/wifispace
git clone https://github.com/ThomasHabets/wifispace.git

## Installing

For most users you just need to install the dependencies, and then do
the normal `./configure && make && make install` dance.

```shell
apt-get install gnuradio-dev libuhd-dev libboost-all-dev
./configure
make -j8
make install
```

If GNURadio is installed in a non-standard location, try:

```shell
PKG_CONFIG_PATH=$HOME/opt/sdr/lib/pkgconfig ./configure --prefix=$HOME/opt/sdr CPPFLAGS=-I$HOME/opt/sdr/include LDFLAGS=-L$HOME/opt/sdr/lib
```

## Running

```shell
./wifispace -o file-$(date +%s).txt.gz
```

## Visualizations

### Density graph of how busyness is distributed over your samples

```shell
zcat file-1454279879.txt.gz | awk '$2 == 2412000000 { print $3}' > channel-data.txt
./tools/density.R  --args channel-data.txt
display wifispace.png
```

Also you can experiment with parameters to `density` and `plot`, like:

```
plot(
  density(scan('channel-data2.txt'), 0.0000001),
  xlim=c(0.00015,0.0002)
)
```
