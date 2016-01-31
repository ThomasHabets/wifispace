#include<cinttypes>
#include<vector>

struct Channel {
        int channel;
        uint64_t frequency;
        uint64_t bandwidth;

        static const Channel& get_by_channel(int channel);
};

extern const std::vector<Channel> channels_2_4GHz;
extern const std::vector<Channel> channels_5GHz;

const Channel& channel_by_number(const std::vector<Channel>& band, int channel);
