#!/usr/bin/env python

import struct
import gzip
import time

from gnuradio import gr
from gnuradio import uhd, digital, blocks


def channel_frequency_24GHz(n):
    if n < 1:
        raise ValueError("channel %d doesn't exist in 2.4GHz band" % n)
    if n < 14:
        return 2412000000 + (n-1) * 5000000
    if n == 14:
        return 2484000000
    raise ValueError("channel %d doesn't exist in 2.4GHz band" % n)


def channel_frequency_5GHz(n):
    if 36 <= n <= 64:
        return 5180000000 + (n-36)*10000000/2
    if r is None:
        raise ValueError("channel %d doesn't exist in 5GHz band" % n)
    return r


class my_top_block(gr.top_block):
    def __init__(self, frequency):
        gr.top_block.__init__(self)

        sample_rate = 32000
        ampl = 0.1

        # Source.
        addr = ''
        stream_args = uhd.stream_args('fc32')
        self.src = uhd.usrp_source(addr, stream_args)
        #src.set_subdev_spec(options.spec, 0)
        self.src.set_samp_rate(32000)
        self.src.set_center_freq(frequency)
        self.src.set_gain(30)
        self.src.set_antenna('TX/RX')

        # SNR
        snr = digital.mpsk_snr_est_cc(type=0)
        snr.set_alpha(0.001)
        snr.set_tag_nsample(1000000)

        rms = blocks.rms_cf()
        rms.set_alpha(0.0001)

        self.msgq = gr.msg_queue(16)

        sink = blocks.message_sink(4, self.msgq, 'bleh')
        self.connect(self.src, snr, rms, sink)

class parse_msg(object):
    def __init__(self, msg):
        self.center_freq = msg.arg1()
        self.vlen = int(msg.arg2())
        assert(msg.length() == self.vlen * gr.sizeof_float)
        # FIXME consider using NumPy array
        t = msg.to_string()
        self.raw_data = t
        self.data = struct.unpack('%df' % (self.vlen,), t)

def print_freq(out, g, f):
    g.src.set_center_freq(f)
    t = time.time()
    while t + 2.0 > time.time():
        m = parse_msg(g.msgq.delete_head())
        out.write("%.3f %d %.9f\n" % (time.time(), f, sum(m.data)/len(m.data)))

def channels_24GHz():
    return range(1,15)

def channels_5GHz():
    return range(36, 64+1, 2)

def main_loop():
    ofn = 'file-%d.txt.gz' % int(time.time())
    print "Scanning and logging to %s..." % ofn
    with gzip.open(ofn, 'wb') as out:
        g = my_top_block(channel_frequency_24GHz(1))
        g.start()
        while True:
            for channel in channels_24GHz():
                print_freq(out, g, channel_frequency_24GHz(channel))
            for channel in channels_5GHz():
                print_freq(out, g, channel_frequency_5GHz(channel))
        g.stop()

if __name__ == '__main__':
    try:
        main_loop()
    except [[KeyboardInterrupt]]:
        pass
