#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "../FSpart.h"
#include "../P2PFS.h"

class MockRunner {
 public:
  MOCK_METHOD(void, get, (dht::InfoHash id, dht::GetCallbackSimple cb, dht::DoneCallback donecb), ());
};

// Demonstrate some basic assertions.
TEST(MainTest, MapInsertion) {
  filesInodes.insert(make_pair("test", 1));
  // Expect two strings not to be equal.
  EXPECT_EQ(filesInodes["test"], 1);
}

TEST(FileSystemTest, GenInode) {
  int uuid = genInode();
  // Expect two strings not to be equal.
  EXPECT_LT(uuid, INT_MAX);
  EXPECT_GE(uuid, 0);
}
