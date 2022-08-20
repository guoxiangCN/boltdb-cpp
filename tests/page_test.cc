#include "boltdb/boltdb.h"
#include "gtest/gtest.h"

TEST(PageTest, TestPageHeader) {
  ASSERT_EQ(16, sizeof(boltdb::Page));
}

TEST(PageTest, TestBranchPageElement) {
  boltdb::Page p;
  p.count = 1024;
  const auto& elements = p.GetBranchPageElements();
  ASSERT_EQ(1024, elements.size());
  for (int i = 1; i <= elements.size() - 1; i++) {
    auto delta = reinterpret_cast<const char*>(elements[i]) -
                 reinterpret_cast<const char*>(elements[i - 1]);
    ASSERT_EQ(sizeof(boltdb::BranchPageElement), delta);
  }
}

TEST(PageTest, TestLeafPageElement) {
  boltdb::Page p;
  p.count = 1024;
  const auto& elements = p.GetLeafPageElements();
  ASSERT_EQ(1024, elements.size());
  for (int i = 1; i <= elements.size() - 1; i++) {
    auto delta = reinterpret_cast<const char*>(elements[i]) -
                 reinterpret_cast<const char*>(elements[i - 1]);
    ASSERT_EQ(sizeof(boltdb::LeafPageElement), delta);
  }
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}