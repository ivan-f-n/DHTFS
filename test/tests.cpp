#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "../src/support/globalFunctions.h"

class MockRunner {
 public:
  MOCK_METHOD(void, get, (dht::InfoHash id, dht::GetCallbackSimple cb, dht::DoneCallback donecb), ());
};

// Demonstrate some basic assertions.
TEST(MainTest, MapInsertion) {
  inodeMap.insert(make_pair("test", 1));
  // Expect two strings not to be equal.
  EXPECT_EQ(inodeMap["test"], 1);
}

TEST(FileSystemTest, GenInode) {
  int uuid = genInode();
  // Expect two strings not to be equal.
  EXPECT_LT(uuid, INT_MAX);
  EXPECT_GE(uuid, 0);
}
