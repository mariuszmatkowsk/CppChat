#include <gtest/gtest.h>

#include <string>
#include <vector>

#include <Channel/channel.hpp>

TEST(make_channel, canCreateSenderAndReceiverWithBuilinType) {
    auto [sender, receiver] = mpsc::make_channel<int>();
}

TEST(make_channel, canCreateSenderAndReceiverWithCustomType) {
    struct Foo {
        int a;
        std::string msg;
        std::vector<int> v;
    };

    auto [sender, receiver] = mpsc::make_channel<Foo>();
}


TEST(channel, pingPong) {
    auto [sender, receiver] = mpsc::make_channel<int>();

    sender.send(5);

    EXPECT_EQ(receiver.recv(), 5);
}

TEST(channel, dropSender) {
    auto [sender, receiver] = mpsc::make_channel<double>();

    {
        auto _ = std::move(sender);
    }

    EXPECT_EQ(receiver.recv(), std::nullopt);
}

