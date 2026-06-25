#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

#include "pureleaf/database.h"
#include "pureleaf/file_repo.h"
#include "pureleaf/file_service.h"
#include "pureleaf/lock_manager.h"
#include "pureleaf/path_util.h"
#include "pureleaf/project_service.h"
#include "pureleaf/repo.h"
#include "pureleaf/storage.h"

namespace fs = std::filesystem;

// ── Test fixture with a real temp DB and temp dir ─────────────────

class ServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        base_ = (fs::temp_directory_path() / "pureleaf_service_tests").string();
        fs::create_directories(base_);

        // Unique subdirectory per test instance.
        testDir_ = (fs::path(base_) / std::to_string(counter_++)).string();
        fs::create_directories(testDir_);

        dbPath_ = (fs::path(testDir_) / "test.db").string();
        blobRoot_ = (fs::path(testDir_) / "blobs").string();
        fs::create_directories(blobRoot_);

        db_ = std::make_unique<pureleaf::Database>(dbPath_);
        lockMgr_ = std::make_unique<pureleaf::ProjectLockManager>();
        projectSvc_ = std::make_unique<pureleaf::ProjectService>(*db_, *lockMgr_);
        fileSvc_ = std::make_unique<pureleaf::FileService>(*db_, *lockMgr_);
        storage_ = std::make_unique<pureleaf::BlobStorage>(blobRoot_);
    }

    void TearDown() override {
        // Release services before DB to avoid dangling references.
        fileSvc_.reset();
        projectSvc_.reset();
        lockMgr_.reset();
        db_.reset();
        // Only remove our own test directory, not the shared base.
        std::error_code ec;
        fs::remove_all(testDir_, ec);
    }

    std::string base_;
    std::string testDir_;
    std::string dbPath_;
    std::string blobRoot_;
    static inline int counter_ = 0;

    std::unique_ptr<pureleaf::Database> db_;
    std::unique_ptr<pureleaf::ProjectLockManager> lockMgr_;
    std::unique_ptr<pureleaf::ProjectService> projectSvc_;
    std::unique_ptr<pureleaf::FileService> fileSvc_;
    std::unique_ptr<pureleaf::BlobStorage> storage_;
};

// ── Path utility tests ────────────────────────────────────────────

TEST(PathUtil, ValidNames) {
    EXPECT_TRUE(pureleaf::path_util::isValidName("main.tex"));
    EXPECT_TRUE(pureleaf::path_util::isValidName("chapter1"));
    EXPECT_TRUE(pureleaf::path_util::isValidName("my file.bib"));
    EXPECT_TRUE(pureleaf::path_util::isValidName("résumé.tex"));
}

TEST(PathUtil, InvalidNames) {
    EXPECT_FALSE(pureleaf::path_util::isValidName(""));
    EXPECT_FALSE(pureleaf::path_util::isValidName("."));
    EXPECT_FALSE(pureleaf::path_util::isValidName(".."));
    EXPECT_FALSE(pureleaf::path_util::isValidName("a/b"));
    EXPECT_FALSE(pureleaf::path_util::isValidName("a\\b"));
    EXPECT_FALSE(pureleaf::path_util::isValidName(std::string("a\0b", 3)));
}

TEST(PathUtil, SafeRelativePaths) {
    EXPECT_TRUE(pureleaf::path_util::isSafeRelativePath(""));
    EXPECT_TRUE(pureleaf::path_util::isSafeRelativePath("main.tex"));
    EXPECT_TRUE(pureleaf::path_util::isSafeRelativePath("chapters/intro.tex"));
    EXPECT_TRUE(pureleaf::path_util::isSafeRelativePath("a/b/c/d.tex"));
}

TEST(PathUtil, UnsafeRelativePaths) {
    EXPECT_FALSE(pureleaf::path_util::isSafeRelativePath("../main.tex"));
    EXPECT_FALSE(pureleaf::path_util::isSafeRelativePath("/etc/passwd"));
    EXPECT_FALSE(pureleaf::path_util::isSafeRelativePath("a/../../etc"));
    EXPECT_FALSE(pureleaf::path_util::isSafeRelativePath("\\\\server\\share"));
}

TEST(PathUtil, JoinParentBasename) {
    EXPECT_EQ(pureleaf::path_util::join("a", "b"), "a/b");
    EXPECT_EQ(pureleaf::path_util::join("", "b"), "b");
    EXPECT_EQ(pureleaf::path_util::join("a/", "b"), "a/b");
    EXPECT_EQ(pureleaf::path_util::parent("a/b/c"), "a/b");
    EXPECT_EQ(pureleaf::path_util::parent("a"), "");
    EXPECT_EQ(pureleaf::path_util::basename("a/b/c.tex"), "c.tex");
    EXPECT_EQ(pureleaf::path_util::basename("c.tex"), "c.tex");
}

// ── SHA-256 tests ─────────────────────────────────────────────────

TEST(Sha256, KnownVectors) {
    // Empty string.
    EXPECT_EQ(pureleaf::BlobStorage::sha256Hex(""),
              "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    // "abc".
    EXPECT_EQ(pureleaf::BlobStorage::sha256Hex("abc"),
              "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    // Longer message.
    EXPECT_EQ(pureleaf::BlobStorage::sha256Hex("The quick brown fox jumps over the lazy dog"),
              "d7a8fbb307d7809469ca9abcb0082e4f8d5651e46d3cdb762d02d0bf37c9e592");
}

TEST(Sha256, Consistency) {
    std::string content(1000, 'x');
    auto h1 = pureleaf::BlobStorage::sha256Hex(content);
    auto h2 = pureleaf::BlobStorage::sha256Hex(content);
    EXPECT_EQ(h1, h2);
    EXPECT_EQ(h1.size(), 64u);
}

// ── BlobStorage tests ─────────────────────────────────────────────

TEST_F(ServiceTest, BlobWriteReadDedup) {
    std::string data = "Hello, LaTeX!";
    auto hash1 = storage_->writeBlob(data);
    ASSERT_FALSE(hash1.empty());
    EXPECT_EQ(hash1.size(), 64u);

    // Write again — should dedup (same hash, no error).
    auto hash2 = storage_->writeBlob(data);
    EXPECT_EQ(hash1, hash2);

    // Exists.
    EXPECT_TRUE(storage_->exists(hash1));

    // Read back.
    auto readResult = storage_->readBlob(hash1);
    ASSERT_TRUE(readResult.ok());
    EXPECT_EQ(readResult.value(), data);

    // Read non-existent.
    auto miss = storage_->readBlob("nonexistenthash");
    EXPECT_FALSE(miss.ok());

    // Delete.
    EXPECT_TRUE(storage_->deleteBlob(hash1));
    EXPECT_FALSE(storage_->exists(hash1));
}

// ── ProjectService tests ──────────────────────────────────────────

TEST_F(ServiceTest, CreateProjectWithTemplate) {
    std::string projDir = (fs::path(testDir_) / "myproject").string();

    auto result = projectSvc_->createProject("My Project", projDir);
    ASSERT_TRUE(result.ok()) << "createProject failed";
    EXPECT_EQ(result.value().name, std::string("My Project"));
    EXPECT_EQ(result.value().mainTex, std::string("main.tex"));

    // Disk: directory + main.tex exist.
    EXPECT_TRUE(fs::exists(projDir));
    EXPECT_TRUE(fs::exists(fs::path(projDir) / "main.tex"));

    // main.tex content is non-empty.
    std::ifstream in((fs::path(projDir) / "main.tex").string());
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    EXPECT_TRUE(content.find("\\documentclass") != std::string::npos);
}

TEST_F(ServiceTest, DeleteProjectCleansDisk) {
    std::string projDir = (fs::path(testDir_) / "toDelete").string();

    auto created = projectSvc_->createProject("Delete Me", projDir);
    ASSERT_TRUE(created.ok());
    ASSERT_TRUE(fs::exists(projDir));

    auto deleted = projectSvc_->deleteProject(created.value().id);
    ASSERT_TRUE(deleted.ok());

    EXPECT_FALSE(fs::exists(projDir));

    // DB metadata gone.
    auto afterDelete = projectSvc_->getProject(created.value().id);
    EXPECT_FALSE(afterDelete.ok());
}

TEST_F(ServiceTest, RenameProject) {
    std::string projDir = (fs::path(testDir_) / "renameProj").string();
    auto created = projectSvc_->createProject("Old Name", projDir);
    ASSERT_TRUE(created.ok());

    auto renamed = projectSvc_->renameProject(created.value().id, "New Name");
    ASSERT_TRUE(renamed.ok());
    EXPECT_EQ(renamed.value().name, std::string("New Name"));
}

// ── FileService tests ─────────────────────────────────────────────

TEST_F(ServiceTest, FileCreateReadUpdate) {
    std::string projDir = (fs::path(testDir_) / "filetest").string();
    auto proj = projectSvc_->createProject("File Test", projDir);
    ASSERT_TRUE(proj.ok());
    auto projectId = proj.value().id;

    // Create a file.
    auto created = fileSvc_->createEntry(projectId, projDir, "", "intro.tex", false);
    ASSERT_TRUE(created.ok()) << "createEntry failed";
    EXPECT_EQ(created.value().name, std::string("intro.tex"));
    EXPECT_FALSE(created.value().isDir);

    // File exists on disk.
    EXPECT_TRUE(fs::exists(fs::path(projDir) / "intro.tex"));

    // Write content.
    auto writeResult = fileSvc_->updateContent(projDir, "intro.tex", "\\section{Intro}");
    ASSERT_TRUE(writeResult.ok());

    // Read content back.
    auto readResult = fileSvc_->getContent(projDir, "intro.tex");
    ASSERT_TRUE(readResult.ok());
    EXPECT_EQ(readResult.value(), std::string("\\section{Intro}"));

    // No .pureleaf_tmp left behind.
    EXPECT_FALSE(fs::exists(fs::path(projDir) / "intro.tex.pureleaf_tmp"));
}

TEST_F(ServiceTest, FileCreateInSubdirectory) {
    std::string projDir = (fs::path(testDir_) / "subtest").string();
    auto proj = projectSvc_->createProject("Sub Test", projDir);
    ASSERT_TRUE(proj.ok());
    auto projectId = proj.value().id;

    // Create a directory.
    auto dir = fileSvc_->createEntry(projectId, projDir, "", "chapters", true);
    ASSERT_TRUE(dir.ok());

    // Create a file inside it.
    auto file = fileSvc_->createEntry(projectId, projDir, "chapters", "ch1.tex", false);
    ASSERT_TRUE(file.ok());

    // Read via relative path.
    auto writeResult = fileSvc_->updateContent(projDir, "chapters/ch1.tex", "content");
    ASSERT_TRUE(writeResult.ok());
    auto readResult = fileSvc_->getContent(projDir, "chapters/ch1.tex");
    ASSERT_TRUE(readResult.ok());
    EXPECT_EQ(readResult.value(), std::string("content"));
}

TEST_F(ServiceTest, FileRenameAndDelete) {
    std::string projDir = (fs::path(testDir_) / "rndtest").string();
    auto proj = projectSvc_->createProject("RN Test", projDir);
    ASSERT_TRUE(proj.ok());
    auto projectId = proj.value().id;

    // Create file.
    auto file = fileSvc_->createEntry(projectId, projDir, "", "old.tex", false);
    ASSERT_TRUE(file.ok());
    ASSERT_TRUE(fileSvc_->updateContent(projDir, "old.tex", "data").ok());

    // Rename.
    auto renamed = fileSvc_->renameEntry(projectId, projDir, "old.tex", "new.tex");
    ASSERT_TRUE(renamed.ok());
    EXPECT_FALSE(fs::exists(fs::path(projDir) / "old.tex"));
    EXPECT_TRUE(fs::exists(fs::path(projDir) / "new.tex"));

    // Delete.
    auto deleted = fileSvc_->deleteEntry(projectId, projDir, "new.tex");
    ASSERT_TRUE(deleted.ok());
    EXPECT_FALSE(fs::exists(fs::path(projDir) / "new.tex"));
}

TEST_F(ServiceTest, DirectoryRecursiveDelete) {
    std::string projDir = (fs::path(testDir_) / "rectest").string();
    auto proj = projectSvc_->createProject("Recursive Test", projDir);
    ASSERT_TRUE(proj.ok());
    auto projectId = proj.value().id;

    // Create directory structure: chapters/ part1.tex, part2.tex
    fileSvc_->createEntry(projectId, projDir, "", "chapters", true);
    fileSvc_->createEntry(projectId, projDir, "chapters", "part1.tex", false);
    fileSvc_->createEntry(projectId, projDir, "chapters", "part2.tex", false);

    EXPECT_TRUE(fs::exists(fs::path(projDir) / "chapters" / "part1.tex"));

    // Delete directory recursively.
    auto result = fileSvc_->deleteEntry(projectId, projDir, "chapters");
    ASSERT_TRUE(result.ok());
    EXPECT_FALSE(fs::exists(fs::path(projDir) / "chapters"));

    // Verify all entries removed from DB via tree.
    auto tree = fileSvc_->getTree(projectId);
    ASSERT_TRUE(tree.ok());
    EXPECT_TRUE(tree.value().children.empty());
}

TEST_F(ServiceTest, FileTreeStructure) {
    std::string projDir = (fs::path(testDir_) / "treetest").string();
    auto proj = projectSvc_->createProject("Tree Test", projDir);
    ASSERT_TRUE(proj.ok());
    auto projectId = proj.value().id;

    // Create: chapters/ (dir), intro.tex, refs.bib
    fileSvc_->createEntry(projectId, projDir, "", "chapters", true);
    fileSvc_->createEntry(projectId, projDir, "chapters", "ch1.tex", false);
    fileSvc_->createEntry(projectId, projDir, "", "intro.tex", false);
    fileSvc_->createEntry(projectId, projDir, "", "refs.bib", false);

    auto tree = fileSvc_->getTree(projectId);
    ASSERT_TRUE(tree.ok());

    // Root should have 3 children: chapters (dir first), intro.tex, refs.bib.
    ASSERT_EQ(tree.value().children.size(), 3u);
    EXPECT_TRUE(tree.value().children[0].isDir);
    EXPECT_EQ(tree.value().children[0].name, std::string("chapters"));
    EXPECT_EQ(tree.value().children[0].children.size(), 1u);
    EXPECT_EQ(tree.value().children[0].children[0].name, std::string("ch1.tex"));
}

TEST_F(ServiceTest, RejectPathTraversal) {
    std::string projDir = (fs::path(testDir_) / "traversal").string();
    auto proj = projectSvc_->createProject("Traversal Test", projDir);
    ASSERT_TRUE(proj.ok());
    auto projectId = proj.value().id;

    // Creating a file named ".." should fail.
    auto bad1 = fileSvc_->createEntry(projectId, projDir, "", "..", false);
    EXPECT_FALSE(bad1.ok());

    // Reading content with path traversal should fail.
    auto bad2 = fileSvc_->getContent(projDir, "../../etc/passwd");
    EXPECT_FALSE(bad2.ok());

    // Writing content with path traversal should fail.
    auto bad3 = fileSvc_->updateContent(projDir, "../escape.tex", "evil");
    EXPECT_FALSE(bad3.ok());
    EXPECT_FALSE(fs::exists(fs::path(testDir_) / "escape.tex"));
}

TEST_F(ServiceTest, DuplicateNameRejected) {
    std::string projDir = (fs::path(testDir_) / "dupe").string();
    auto proj = projectSvc_->createProject("Dupe Test", projDir);
    ASSERT_TRUE(proj.ok());
    auto projectId = proj.value().id;

    auto first = fileSvc_->createEntry(projectId, projDir, "", "file.tex", false);
    ASSERT_TRUE(first.ok());

    auto second = fileSvc_->createEntry(projectId, projDir, "", "file.tex", false);
    EXPECT_FALSE(second.ok());
    EXPECT_EQ(second.error, pureleaf::Error::AlreadyExists);
}

// ── Lock manager tests ────────────────────────────────────────────

TEST(LockManager, BasicLockUnlock) {
    pureleaf::ProjectLockManager mgr;

    mgr.lock("p1");
    mgr.unlock("p1");

    // Should not deadlock.
    mgr.lock("p1");
    mgr.unlock("p1");
}

TEST(LockManager, DifferentProjectsParallel) {
    pureleaf::ProjectLockManager mgr;

    mgr.lock("p1");
    mgr.lock("p2");  // Different project, should not block.
    mgr.unlock("p2");
    mgr.unlock("p1");
}

TEST(LockManager, ScopedGuard) {
    pureleaf::ProjectLockManager mgr;

    {
        auto guard = mgr.acquire("p1");
        // Lock held.
    }
    // Lock released, can re-acquire.
    auto guard2 = mgr.acquire("p1");
}
