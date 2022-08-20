#include "gtest/gtest.h"
#include "boltdb/boltdb.h"

TEST(PageTest, TestPageHeader) {
    EXPECT_EQ(16, sizeof(boltdb::Page));
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}