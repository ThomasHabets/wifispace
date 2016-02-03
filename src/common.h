#include<cinttypes>
#include<vector>

#include<gnuradio/msg_queue.h>
#include<gnuradio/uhd/usrp_source.h>

struct Channel {
        int channel;
        uint64_t frequency;
        uint64_t bandwidth;

        static const Channel& get_by_channel(int channel);
};

extern const std::vector<Channel> channels_2_4GHz;
extern const std::vector<Channel> channels_5GHz;

const Channel& channel_by_number(const std::vector<Channel>& band, int channel);
gr::top_block_sptr make_dsp(gr::uhd::usrp_source::sptr src, gr::msg_queue::sptr msgq);
std::pair<float, float> meanstd(const std::vector<float>& v);
std::vector<float> parse_msg(const gr::message& msg);
