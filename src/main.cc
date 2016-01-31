#include<iostream>
#include<string>
#include<vector>

#include<boost/accumulators/accumulators.hpp>
#include<boost/accumulators/numeric/functional/vector.hpp>
#include<boost/accumulators/statistics/stats.hpp>
#include<boost/accumulators/statistics/variance.hpp>
#include<boost/bind/placeholders.hpp>
#include<boost/ref.hpp>

#include<gnuradio/blocks/message_sink.h>
#include<gnuradio/blocks/rms_cf.h>
#include<gnuradio/digital/mpsk_snr_est_cc.h>
#include<gnuradio/msg_queue.h>
#include<gnuradio/top_block.h>
#include<gnuradio/uhd/usrp_source.h>

gr::top_block_sptr
make(gr::msg_queue::sptr msgq)
{
        std::string device_addr = "uhd";
        float initial_frequency = 5.18e9;

        auto tb = gr::make_top_block("wifiscanner");

        auto src = gr::uhd::usrp_source
                ::make(device_addr,
                       uhd::stream_args_t("fc32"));
        src->set_samp_rate(32000);
        src->set_center_freq(initial_frequency);
        src->set_gain(30);
        src->set_antenna("TX/RX");

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

std::string
log_line(const std::vector<float>& v)
{
        if (v.empty()) {
                return "empty sample";
        }
        auto t = meanstd(v);
        return (boost::format("%f %f") % t.first % t.second).str();
}

int
main()
{
        auto msgq = gr::msg_queue::make(16);
        auto tb = make(msgq);
        tb->start();
        std::cout << "Running...\n";
        for(;;) {
                auto msg = msgq->delete_head();
                std::cout << log_line(parse_msg(*msg)) << std::endl;
        }
}
