#include<chrono>
#include<fstream>
#include<iostream>
#include<string>
#include<vector>

#include<boost/accumulators/accumulators.hpp>
#include<boost/accumulators/numeric/functional/vector.hpp>
#include<boost/accumulators/statistics/stats.hpp>
#include<boost/accumulators/statistics/variance.hpp>
#include<boost/bind/placeholders.hpp>
#include<boost/iostreams/filter/gzip.hpp>
#include<boost/iostreams/filtering_stream.hpp>
#include<boost/ref.hpp>

#include<gnuradio/blocks/message_sink.h>
#include<gnuradio/blocks/rms_cf.h>
#include<gnuradio/digital/mpsk_snr_est_cc.h>
#include<gnuradio/msg_queue.h>
#include<gnuradio/top_block.h>
#include<gnuradio/uhd/usrp_source.h>

#include"wifiscan.h"

// Flags.
std::chrono::seconds staytime{2};
int verbose = 0;

gr::top_block_sptr
make_dsp(gr::uhd::usrp_source::sptr src, gr::msg_queue::sptr msgq)
{
        auto tb = gr::make_top_block("wifiscanner");

        auto snr = gr::digital::mpsk_snr_est_cc
                ::make(gr::digital::SNR_EST_SIMPLE);
        snr->set_tag_nsample(1000000);
        snr->set_alpha(0.001);

        auto rms = gr::blocks::rms_cf::make();
        rms->set_alpha(0.0001);

        auto sink = gr::blocks::message_sink
                ::make(4, msgq, true);

        tb->connect(src, 0, snr, 0);
        tb->connect(snr, 0, rms, 0);
        tb->connect(rms, 0, sink, 0);
        return tb;
}

std::vector<float>
parse_msg(const gr::message& msg) {
        std::vector<float> ret;
        const int count = static_cast<int>(msg.arg2());
        if (msg.length() != count *  sizeof(float)) {
                std::cerr << msg.length() << " not multiple of floats";
                return ret;
        }
        float* p = reinterpret_cast<float*>(msg.msg());
        for (int c = 0; c < count; c++, p++) {
                ret.push_back(*p);
        }
        return ret;
}

std::pair<float, float>
meanstd(const std::vector<float>& v)
{
        boost::accumulators::accumulator_set
                <double,
                 boost::accumulators::stats<
                         boost::accumulators::tag::variance>> acc;
        std::for_each(v.begin(), v.end(),
                      boost::bind<void>(boost::ref(acc),
                                        _1));
        return std::make_pair(boost::accumulators::mean(acc),
                              sqrt(boost::accumulators::variance(acc)));
}

void
mainloop(gr::uhd::usrp_source::sptr src, gr::msg_queue::sptr msgq, const std::vector<Channel>& all_channels, std::ostream& measurements)
{
        auto last_switch = std::chrono::steady_clock::now();
        for(;;) {
                for (auto& channel : all_channels) {
                        if (verbose) {
                                std::cout << boost::format("Switching to channel %d\n") % channel.channel;
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

int
wrapped_main()
{
        auto msgq = gr::msg_queue::make(16);

        std::string device_addr = "uhd";
        const float initial_frequency = channel_by_number(channels_5GHz, 36).frequency;
        float gain = 30;
        unsigned int sample_rate = 62500;
        std::string antenna = "TX/RX";

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

        std::ofstream fo("hello.gz", std::ofstream::binary);
        if (!fo.is_open()) {
                throw std::runtime_error("Failed to open measurement file");
        }
        std::ostream* measurements = &fo;
        boost::iostreams::filtering_ostream z;
        if (true) {
                z.push(boost::iostreams::gzip_compressor(boost::iostreams::gzip_params(boost::iostreams::gzip::best_compression)));
                z.push(fo);
                measurements = &z;
        }
        mainloop(src, msgq, all_channels, *measurements);
        return 0;
}

int
main()
{
        try {
                return wrapped_main();
        } catch (const std::exception& e) {
                std::cerr << boost::format("Exception: %s\n") % e.what();
                return 1;
        }
}
