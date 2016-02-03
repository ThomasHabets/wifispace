#include"config.h"

#include<vector>

#include<boost/accumulators/accumulators.hpp>
#include<boost/accumulators/numeric/functional/vector.hpp>
#include<boost/accumulators/statistics/stats.hpp>
#include<boost/accumulators/statistics/variance.hpp>
#include<boost/bind/placeholders.hpp>

#include<gnuradio/blocks/message_sink.h>
#include<gnuradio/blocks/rms_cf.h>
#include<gnuradio/digital/mpsk_snr_est_cc.h>
#include<gnuradio/msg_queue.h>
#include<gnuradio/top_block.h>
#include<gnuradio/uhd/usrp_source.h>

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

// disclaimer:
//   I'm new to gnuradio and practical radio experimentation in general, so
//   most likely this pipeline is close to nonsense radio engineering wise.
//   But it does work. So it has that going for it, which is nice.
gr::top_block_sptr
make_dsp(gr::uhd::usrp_source::sptr src, gr::msg_queue::sptr msgq)
{
        auto tb = gr::make_top_block("wifispace");

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
