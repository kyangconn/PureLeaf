#include <gtest/gtest.h>

#include "pureleaf/diff.h"
#include "pureleaf/storage.h"
#include "pureleaf/synctex.h"
#include "pureleaf_c/pureleaf.h"
#include "pureleaf/desktop/platform.h"

/// Smoke tests that verify the architecture compiles and links across
/// core, capi, and platform layers. Feature implementations come later.

TEST(Storage, BlobRelativePathSharding) {
    EXPECT_EQ(pureleaf::BlobStorage::blobRelativePath("abcdef1234"), "ab/abcdef1234");
    // 2-char hash still shards: {hash[:2]}/{hash} = "ab/ab".
    EXPECT_EQ(pureleaf::BlobStorage::blobRelativePath("ab"), "ab/ab");
    // Too short to shard.
    EXPECT_EQ(pureleaf::BlobStorage::blobRelativePath("a"), "a");
}

TEST(SyncTex, ForwardConversionAppliesRatioAndScale) {
    auto r = pureleaf::synctexToPdfjs(/*page=*/1, /*x=*/100.0, /*y=*/200.0, /*scale=*/2.0);
    // 100 * (72/96) * 2 = 150
    EXPECT_DOUBLE_EQ(r.x, 150.0);
    // 200 * (72/96) * 2 = 300
    EXPECT_DOUBLE_EQ(r.y, 300.0);
    EXPECT_EQ(r.page, 1);
}

TEST(Diff, StubReturnsEmpty) {
    auto hunks = pureleaf::computeDiff("a", "b");
    EXPECT_TRUE(hunks.empty());  // TODO: real implementation
}

TEST(Capi, VersionAndSynctex) {
    EXPECT_STREQ(pl_version(), "0.1.0");

    auto r = pl_synctex_to_pdfjs(1, 100.0, 200.0, 1.0);
    EXPECT_EQ(r.page, 1);
    EXPECT_DOUBLE_EQ(r.x, 75.0);  // 100 * 0.75 * 1
}

TEST(Platform, DesktopPathsNonEmpty) {
    auto paths = pureleaf::desktop::getPaths();
    EXPECT_FALSE(paths.userDataDir.empty());
}
