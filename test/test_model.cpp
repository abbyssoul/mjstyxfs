/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/
/*******************************************************************************
 * Unit Test Suit
 *	@file test/test_model.cpp
 *	@brief		Test suit for mjstyxfs::Model
 ******************************************************************************/
#include "model.hpp"    // Class being tested.

#include <solace/output_utils.hpp>
#include <rapidjson/error/error.h>
#include <rapidjson/error/en.h>
#include <gtest/gtest.h>

using namespace Solace;
using namespace mjstyxfs;


struct TestJsonFS : public ::testing::Test {

	void SetUp() override {
		auto maybeFsId = _vfs.registerFilesystem<JsonFS>();
		ASSERT_TRUE(maybeFsId.isOk());

		_jsonFsId = *maybeFsId;
	}

	void TearDown() override {
		_vfs.unregisterFileSystem(_jsonFsId);
	}

protected:
	kasofs::User	_owner{0, 0};
	kasofs::Vfs		_vfs{_owner, kasofs::FilePermissions{0666}};
	kasofs::VfsId	_jsonFsId{0};

	rapidjson::Document _doc;
};



TEST_F(TestJsonFS, modelPopulation) {
	rapidjson::ParseResult ok = _doc.Parse("{\"test\": \"value-x\", \"other\": 32}");
	if (!ok) {
		FAIL() << rapidjson::GetParseError_En(ok.Code());
	}
	ASSERT_FALSE(_doc.HasParseError());

	auto mappedOk = mapJsonToFs(_vfs, _vfs.rootId(), _jsonFsId, "doc", _doc);
	if (!mappedOk) {
		FAIL() << mappedOk.getError();
	}
	ASSERT_TRUE(mappedOk.isOk());

	auto enumerator = _vfs.enumerateDirectory(_owner, _vfs.rootId());
	ASSERT_TRUE(enumerator.isOk());

	uint32 count = 0;
	for (auto e : *enumerator) {
		EXPECT_EQ("doc", e.name);
		count += 1;
	}
	EXPECT_EQ(1, count);

	char buf[32];
	{
		auto maybeX = _vfs.walk(_owner, *makePath("doc", "test"));
		ASSERT_TRUE(maybeX.isOk());

		auto entry = *maybeX;
		EXPECT_EQ("test", entry.name);

		auto maybeFile_test = _vfs.open(_owner, entry.nodeId, kasofs::Permissions::READ);
		ASSERT_TRUE(maybeFile_test.isOk());

		memset(buf, 0, sizeof(buf));
		auto readResult = (*maybeFile_test).read(wrapMemory(buf));
		ASSERT_TRUE(readResult.isOk());
		EXPECT_STREQ("value-x", buf);
	}

	{
		auto maybeY = _vfs.walk(_owner, *makePath("doc", "other"));
		ASSERT_TRUE(maybeY.isOk());

		auto entryY = *maybeY;
		EXPECT_EQ("other", entryY.name);

		auto maybeFile_other = _vfs.open(_owner, entryY.nodeId, kasofs::Permissions::READ);
		ASSERT_TRUE(maybeFile_other.isOk());

		memset(buf, 0, sizeof(buf));
		auto readResult2 = (*maybeFile_other).read(wrapMemory(buf));
		ASSERT_TRUE(readResult2.isOk());
		EXPECT_STREQ("32", buf);
	}
}

