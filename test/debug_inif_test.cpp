// SPDX-License-Identifier: Apache-2.0
#include <dump_serialize.hpp>
#include <filesystem>
#include <fstream>

#include <gtest/gtest.h>

namespace fs = std::filesystem;

class TestDumpSerial : public ::testing::Test
{
  public:
    TestDumpSerial()
    {
    }

    void SetUp()
    {
        char tmpdir[] = "/tmp/dump.XXXXXX";
        auto dirPtr = mkdtemp(tmpdir);
        if (dirPtr == NULL)
        {
            throw std::bad_alloc();
        }
        dumpDir = std::string(dirPtr);
        fs::create_directories(dumpDir);
        dumpFile = dumpDir;
        dumpFile /= "elogid";
    }
    void TearDown()
    {
        fs::remove_all(dumpDir);
    }

    std::string dumpDir;
    fs::path dumpFile;
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
