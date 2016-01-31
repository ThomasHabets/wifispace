#!/usr/bin/python
import sys
import matplotlib.pyplot as plt
import numpy
import wifiscan

from itertools import cycle

def main():
    print 'Reading data...'
    data = {}
    for line in sys.stdin:
        try:
            t,f,v = line.split()
        except ValueError:
            continue
        t,f,v = float(t), int(f), float(v)
        data.setdefault(f, {})[t] = v
    colours = "bgrcmyk"
    lines = [".","*", "+"]
    types = []
    for b in lines:
        for a in colours:
            types.append(a + b)
    typecycle = cycle(types)

    if False:
        for channel in wifiscan.channels_24GHz():
            d = data[wifiscan.channel_frequency_24GHz(channel)].values()
            print 'Channel %2d: %f %f' % (
                channel,
                numpy.mean(d),
                numpy.std(d))
            plt.plot(
                d,
                next(typecycle),
                label='2.4GHz channel %d' % channel)
    else:
        for channel in wifiscan.channels_5GHz():
            d = data[wifiscan.channel_frequency_5GHz(channel)].values()
            print 'Channel %2d: %f %f' % (
                channel,
                numpy.mean(d),
                numpy.std(d))
            plt.plot(
                d,
                next(typecycle),
                label='5GHz channel %d' % channel)
    #plt.yscale('log')
    plt.legend()
    plt.ylabel('Blaha')
    plt.show()

if __name__ == '__main__':
    main()
    
