#include <gtest/gtest.h>

#include "pureleaf/database.h"
#include "pureleaf/diff.h"
#include "pureleaf/repo.h"
#include "pureleaf/storage.h"
#include "pureleaf/synctex.h"
#include "pureleaf_c/pureleaf.h"
#include "pureleaf/desktop/platform.h"

#include <filesystem>

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

// ── Database & Repository Tests ──────────────────────────────────

namespace fs = std::filesystem;

class RepoTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Use a unique temp directory per test.
        auto base = fs::temp_directory_path() / "pureleaf_tests";
        fs::create_directories(base);
        dbPath_ = (base / ("test_" + std::to_string(counter_++) + ".db")).string();
        // Start clean.
        fs::remove(dbPath_);
    }

    void TearDown() override {
        fs::remove(dbPath_);
    }

    std::string dbPath_;
    static inline int counter_ = 0;
};

TEST_F(RepoTest, DatabaseOpensAndMigrates) {
    EXPECT_NO_THROW({
        pureleaf::Database db(dbPath_);
        EXPECT_TRUE(db.exec("SELECT 1;"));
    });
}

TEST_F(RepoTest, ProjectCRUD) {
    pureleaf::Database db(dbPath_);
    pureleaf::ProjectRepo repo(db);

    // Create
    auto created = repo.create("My Thesis", "/home/user/thesis");
    ASSERT_TRUE(created.ok()) << "create failed";
    EXPECT_EQ(created.value().name, std::string("My Thesis"));
    EXPECT_EQ(created.value().rootPath, std::string("/home/user/thesis"));
    EXPECT_TRUE(created.value().mainTex.empty());
    EXPECT_FALSE(created.value().id.empty());

    // Get
    auto got = repo.get(created.value().id);
    ASSERT_TRUE(got.ok());
    EXPECT_EQ(got.value().name, std::string("My Thesis"));

    // Get non-existent
    auto missing = repo.get("does-not-exist");
    EXPECT_FALSE(missing.ok());

    // Rename
    auto renamed = repo.rename(created.value().id, "Doctoral Thesis");
    ASSERT_TRUE(renamed.ok());
    EXPECT_EQ(renamed.value().name, std::string("Doctoral Thesis"));

    // Set main tex
    auto updated = repo.setMainTex(created.value().id, "main.tex");
    ASSERT_TRUE(updated.ok());
    EXPECT_EQ(updated.value().mainTex, std::string("main.tex"));

    // List
    repo.create("Second Project", "/home/user/second");
    auto list = repo.list();
    ASSERT_TRUE(list.ok());
    EXPECT_EQ(list.value().size(), 2u);

    // Delete
    EXPECT_TRUE(repo.remove(created.value().id));
    auto afterDelete = repo.get(created.value().id);
    EXPECT_FALSE(afterDelete.ok());
}

TEST_F(RepoTest, RevisionCRUD) {
    pureleaf::Database db(dbPath_);
    pureleaf::ProjectRepo projectRepo(db);

    // Create a project first (FK constraint requires it).
    auto proj = projectRepo.create("TestProj", "/tmp/proj");
    ASSERT_TRUE(proj.ok());

    // Insert a file row referencing the project (new schema: parent_id + name).
    std::string insertSql =
        "INSERT INTO files(id, project_id, parent_id, name, is_dir, created_at) "
        "VALUES('f1','" + proj.value().id + "',NULL,'main.tex',0,1000)";
    ASSERT_TRUE(db.exec(insertSql));

    pureleaf::RevisionRepo repo(db);

    auto r1 = repo.create("f1", "abc123hash", 1024);
    ASSERT_TRUE(r1.ok());
    EXPECT_EQ(r1.value().blobHash, "abc123hash");
    EXPECT_EQ(r1.value().size, 1024);

    auto r2 = repo.create("f1", "def456hash", 2048);

    // List by file
    auto list = repo.listByFile("f1");
    ASSERT_TRUE(list.ok());
    EXPECT_EQ(list.value().size(), 2u);
    // Newest first
    EXPECT_EQ(list.value()[0].id, r2.value().id);

    // Latest
    auto latest = repo.latest("f1");
    ASSERT_TRUE(latest.ok());
    EXPECT_EQ(latest.value().id, r2.value().id);

    // Latest for non-existent file
    auto noRev = repo.latest("no-such-file");
    EXPECT_FALSE(noRev.ok());
}
