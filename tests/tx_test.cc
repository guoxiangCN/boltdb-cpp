#include "boltdb/boltdb.h"
#include "gtest/gtest.h"

TEST(TxTest, TestTxStats) {
    boltdb::TxStats stat1, stat2;
    stat1.Add(stat2);
    stat1.Sub(stat2);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}