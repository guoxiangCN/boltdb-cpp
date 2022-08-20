#include "gtest/gtest.h"
#include "boltdb/boltdb.h"

TEST(PageTest, TestPageHeader) {
    EXPECT_EQ(16, sizeof(boltdb::Page));
    boltdb::Page p;
    p.count = 1024;
    const auto &elements = p.GetBranchPageElements();
    EXPECT_EQ(1024, elements.size());
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}