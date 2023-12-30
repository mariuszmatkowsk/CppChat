#include <gtest/gtest.h>

#include <string>
#include <vector>

#include <Channel/channel.hpp>

TEST(channel, canCreateSenderAndReceiverWithBuiltInType) {
    auto [sender, receiver] = mpsc::make_channel<int>();
}

TEST(channel, canCreateSenderAndReceiverWithCustomType) {
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

TEST(channel, senderClose) {
    auto [sender, receiver] = mpsc::make_channel<int>();

    {
        auto _ = std::move(sender);
    }

    EXPECT_EQ(receiver.recv(), std::nullopt);
}

TEST(channel, receiverClose) {
    auto [sender, receiver] = mpsc::make_channel<int>();

    {
        auto _ = std::move(receiver);
    }

    sender.send(10);
}

