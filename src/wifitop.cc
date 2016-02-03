/** wifispace/src/main.c
 *  Copyright Thomas Habets <thomas@habets.se> 2016
 */
#include"config.h"

#include<chrono>
#include<fstream>
#include<iostream>
#include<string>
#include<unistd.h>
#include<vector>
#include<cmath>

#include<gnuradio/top_block.h>

#include"common.h"

static const std::string version = VERSION; /* from autoconf */

// Defaults for flags.
const std::string default_opt_A = "TX/RX";
const std::string default_opt_b = "2.4GHz";
const std::string default_opt_d = "uhd";
const float default_opt_g = 30;
const unsigned int default_opt_s = 62500;

std::string argv0;
int verbose = 0;

std::chrono::milliseconds staytime{1000};

void
mainloop(gr::uhd::usrp_source::sptr src, gr::msg_queue::sptr msgq, const std::vector<Channel>& all_channels)
{
        auto last_switch = std::chrono::steady_clock::now();
        for (size_t c = 0; c < all_channels.size(); c++) {
                if (c > 0) {
                        std::cout << " ";
                }
                std::cout << boost::format("%2d") % all_channels[c].channel;
        }
        std::cout << "\n------------------------------------\n";
        for(;;) {
                std::vector<float> strength;
                for (const auto& channel : all_channels) {
                        if (verbose) {
                                std::cerr << "Switching channel\n";
                        }
                        src->set_center_freq(channel.frequency);
                        std::vector<float> s2;
                        for (;;) {
                                const auto now = std::chrono::steady_clock::now();
                                const auto msg = msgq->delete_head();
                                const auto power = meanstd(parse_msg(*msg));

                                s2.push_back(power.first);

                                if ((now - last_switch) > staytime) {
                                        last_switch = now;
                                        break;
                                }
                        }
                        strength.push_back(10000*meanstd(s2).first);
                }
                for (size_t c = 0; c < strength.size(); c++) {
                        std::cout << boost::format("%2.0f ") % strength[c];
                }
                std::cout << std::endl;
        }
}

void
usage(int err)
{
        std::cout << boost::format("wifitop %s, by Thomas Habets <thomas@habets.se>\n") % version
                  << boost::format("Usage: %s [options] -o <output>\n") % argv0
                  << boost::format("  -A <antenna>      Antenna to use. Default: %s\n") % default_opt_A
                  << boost::format("  -d <device>       Device address. Default: %s\n") % default_opt_d
                  << boost::format("  -b <band>         Frequency range. Default: %s\n") % default_opt_b
                  << boost::format("  -g <gain>         Input gain. Default: %f\n") % default_opt_g
                  <<               "  -h                Show this help text.\n"
                  << boost::format("  -s <sample rate>  Sample rate to use. Default: %f\n") % default_opt_s
                  <<               "  -v                Increase verbosity.\n"
                ;
        exit(err);
}

int
wrapped_main(int argc, char** argv)
{
        argv0 = argv[0];
        const float initial_frequency = channel_by_number(channels_5GHz, 36).frequency;

        // Flags that don't need to be global variables.
        std::string device_addr = default_opt_d;
        float gain = default_opt_g;
        unsigned int sample_rate = default_opt_s;
        std::string antenna = default_opt_A;
        std::string band = default_opt_b;

        {
                int opt;
                while ((opt = getopt(argc, argv, "A:d:hg:s:vZ")) != -1) {
                        switch (opt) {
                        case 'A':
                                antenna = optarg;
                                break;
                        case 'b':
                                band = optarg;
                                break;
                        case 'd':
                                device_addr = optarg;
                                break;
                        case 'g':
                                gain = strtof(optarg, NULL);
                                break;
                        case 'h':
                                usage(0);
                        case 's':
                                sample_rate = strtoul(optarg, NULL, 0);
                                break;
                        case 'v':
                                verbose++;
                                break;
                        default:
                                usage(1);
                        }
                }
        }
        if (optind != argc) {
                std::cerr << boost::format("%s: Extra args on command line.\n") % argv0;
                exit(1);
        }

        auto msgq = gr::msg_queue::make(16);
        auto src = gr::uhd::usrp_source
                ::make(device_addr,
                       uhd::stream_args_t("fc32"));
        src->set_samp_rate(sample_rate);
        src->set_center_freq(initial_frequency);
        src->set_gain(gain);
        src->set_antenna(antenna);

        std::vector<Channel> all_channels;
        for (auto b : {"2.4GHz", "2.4"}) {
                if (band == b) {
                        all_channels = channels_2_4GHz;
                        break;
                }
        }
        for (auto b : {"5GHz", "5"}) {
                if (band == b) {
                        all_channels = channels_5GHz;
                        break;
                }
        }
        if (all_channels.empty()) {
                std::cerr << boost::format("%s: Unknown band: %s\n") % argv0 % band;
                exit(1);
        }

        auto tb = make_dsp(src, msgq);
        tb->start();
        std::cout << "Running...\n";

        mainloop(src, msgq, all_channels);
        return 0;
}

int
main(int argc, char** argv)
{
        try {
                return wrapped_main(argc, argv);
        } catch (const std::exception& e) {
                std::cerr << boost::format("Exception: %s\n") % e.what();
                return 1;
        }
}
