// SPDX-License-Identifier: Apache-2.0
#include <dump_serialize.hpp>

#include <cstdlib>
#include <exception>
#include <filesystem>
#include <set>
#include <span>
#include <string>

#include <gtest/gtest.h>

class TestDumpSerial : public ::testing::Test
{
  public:
    TestDumpSerial() = default;

    void SetUp()
    {
        char tmpdir[] = "/tmp/dump.XXXXXX";
        std::span<char> tmpdirSpan(reinterpret_cast<char*>(tmpdir),
                                   sizeof(tmpdir));
        auto dirPtr = mkdtemp(tmpdirSpan.data());
        if (dirPtr == nullptr)
        {
            throw std::bad_alloc();
        }
        dumpDir = std::string(dirPtr);
        std::filesystem::create_directories(dumpDir);
        dumpFile = dumpDir;
        dumpFile /= "elogid";
    }
    void TearDown()
    {
        std::filesystem::remove_all(dumpDir);
    }

    std::string dumpDir;
    std::filesystem::path dumpFile;
};

TEST_F(TestDumpSerial, Serialization)
{
    using ElogList = std::set<uint32_t>;
    ElogList e;
    e.insert(1);
    e.insert(2);
    e.insert(3);
    phosphor::dump::elog::serialize(e, dumpFile.c_str());
    bool value = phosphor::dump::elog::deserialize(dumpFile.c_str(), e);
    EXPECT_EQ(value, true);
}

TEST_F(TestDumpSerial, DeserializationFalseCase)
{
    using ElogList = std::set<uint32_t>;
    ElogList e;
    e.insert(1);
    bool value = phosphor::dump::elog::deserialize(dumpFile.c_str(), e);
    EXPECT_EQ(value, false);
}

TEST(DumpDeSerialPath, DeserializationFalsePath)
{
    using ElogList = std::set<uint32_t>;
    ElogList e;
    e.insert(1);
    // Providing dummy path
    bool value = phosphor::dump::elog::deserialize("/tmp/Fake/serial", e);
    EXPECT_EQ(value, false);
}
