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

#include<boost/iostreams/filter/gzip.hpp>
#include<boost/iostreams/filtering_stream.hpp>

#include<gnuradio/top_block.h>
#include<gnuradio/uhd/usrp_source.h>

#include"wifispace.h"

static const std::string version = VERSION; /* from autoconf */

// Defaults for flags.
const std::string default_opt_A = "TX/RX";
const std::string default_opt_d = "uhd";
const float default_opt_g = 30;
const unsigned int default_opt_s = 62500;

std::string argv0;

// Flags.
std::chrono::seconds staytime{2};
int verbose = 0;

void
mainloop(gr::uhd::usrp_source::sptr src, gr::msg_queue::sptr msgq, const std::vector<Channel>& all_channels, std::ostream& measurements)
{
        auto last_switch = std::chrono::steady_clock::now();
        for(;;) {
                for (const auto& channel : all_channels) {
                        if (verbose) {
                                std::cout << boost::format("Switching to channel %d, frequency %f\n") % channel.channel % channel.frequency;
                        }
                        src->set_center_freq(channel.frequency);
                        for (;;) {
                                const auto now = std::chrono::steady_clock::now();
                                const auto system_now = std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count();
                                const auto msg = msgq->delete_head();
                                const auto strength = meanstd(parse_msg(*msg));

                                measurements << boost::format("%.3f %d %.9f\n") % system_now % channel.frequency % strength.first;

                                if ((now - last_switch) > staytime) {
                                        last_switch = now;
                                        break;
                                }
                        }
                }
        }
}

void
usage(int err)
{
        std::cout << boost::format("wifispace %s, by Thomas Habets <thomas@habets.se>\n") % version
                  << boost::format("Usage: %s [options] -o <output>\n") % argv0
                  << boost::format("  -A <antenna>      Antenna to use. Default: %s\n") % default_opt_A
                  << boost::format("  -d <device>       Device address. Default: %s\n") % default_opt_d
                  << boost::format("  -g <gain>         Input gain. Default: %f\n") % default_opt_g
                  <<               "  -h                Show this help text.\n"
                  <<               "  -o <output file>  Output filename. Mandatory.\n"
                  << boost::format("  -s <sample rate>  Sample rate to use. Default: %f\n") % default_opt_s
                  <<               "  -v                Increase verbosity.\n"
                  <<               "  -Z                Disable gzip on output file.\n"
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
        std::string output_filename;
        bool gzip = true;

        {
                int opt;
                while ((opt = getopt(argc, argv, "A:d:hg:o:s:vZ")) != -1) {
                        switch (opt) {
                        case 'A':
                                antenna = optarg;
                                break;
                        case 'd':
                                device_addr = optarg;
                                break;
                        case 'g':
                                gain = strtof(optarg, NULL);
                                break;
                        case 'h':
                                usage(0);
                        case 'o':
                                output_filename = optarg;
                                break;
                        case 's':
                                sample_rate = strtoul(optarg, NULL, 0);
                                break;
                        case 'v':
                                verbose++;
                                break;
                        case 'Z':
                                gzip = false;
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
        if (output_filename == "") {
                std::cerr << boost::format("%s: Need to specify output file (-o)\n") % argv0;
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

        auto tb = make_dsp(src, msgq);
        tb->start();
        std::cout << "Running...\n";

        std::vector<Channel> all_channels;
        all_channels.insert(all_channels.end(), channels_2_4GHz.begin(), channels_2_4GHz.end());
        all_channels.insert(all_channels.end(), channels_5GHz.begin(), channels_5GHz.end());

        std::ofstream fo(output_filename, std::ofstream::binary);
        if (!fo.is_open()) {
                throw std::runtime_error((boost::format("Failed to open measurement file %s: %s") % output_filename % strerror(errno)).str());
        }
        std::ostream* measurements = &fo;
        boost::iostreams::filtering_ostream z;
        if (gzip) {
                z.push(boost::iostreams::gzip_compressor(boost::iostreams::gzip_params(boost::iostreams::gzip::best_compression)));
                z.push(fo);
                measurements = &z;
        }
        mainloop(src, msgq, all_channels, *measurements);
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
