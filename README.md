# WiFiSpace

GNURadio program for measuring how busy wifi channels are.

## Installing

If GNURadio is installed in a non-standard location, run:

```shell
PKG_CONFIG_PATH=$HOME/opt/sdr/lib/pkgconfig ./configure --prefix=$HOME/opt/sdr CPPFLAGS=-I$HOME/opt/sdr/include LDFLAGS=-L$HOME/opt/sdr/lib
```

## Running

```shell
./wifispace -o file-$(date +%s).txt.gz
```

## Other visualizations

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
