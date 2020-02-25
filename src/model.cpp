/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/


#include "model.hpp"

using namespace Solace;
using namespace kasofs;
using namespace mjstyxfs;


uint32 nodeEpochTime() noexcept {
	return time(nullptr);
}


kasofs::Result<void>
setValueNode(kasofs::Vfs& fs, kasofs::User user, kasofs::INode::Id nodeId, uint64 value) {
	auto maybeEntry = fs.open(user, nodeId, kasofs::Permissions::WRITE);
	if (!maybeEntry)
		return maybeEntry.moveError();

	char buf[64];
	uint const count = snprintf(buf, 64, "%ld", value);

	auto& entry = *maybeEntry;
	auto writeOk = entry.write(wrapMemory(buf, count));
	if (!writeOk)
		return writeOk.moveError();

	return none;
}


kasofs::Result<void>
setValueNode(kasofs::Vfs& fs, kasofs::User user, kasofs::INode::Id nodeId, int64 value) {
	auto maybeEntry = fs.open(user, nodeId, kasofs::Permissions::WRITE);
	if (!maybeEntry)
		return maybeEntry.moveError();

	char buf[64];
	uint const count = snprintf(buf, 64, "%ld", value);

	auto& entry = *maybeEntry;
	auto writeOk = entry.write(wrapMemory(buf, count));
	if (!writeOk)
		return writeOk.moveError();

	return none;
}


kasofs::Result<void>
setValueNode(kasofs::Vfs& fs, kasofs::User user, kasofs::INode::Id nodeId, double value) {
	auto maybeEntry = fs.open(user, nodeId, kasofs::Permissions::WRITE);
	if (!maybeEntry)
		return maybeEntry.moveError();

	char buf[64];
	uint const count = snprintf(buf, 64, "%lf", value);

	auto& entry = *maybeEntry;
	auto writeOk = entry.write(wrapMemory(buf, count));
	if (!writeOk)
		return writeOk.moveError();

	return none;
}

kasofs::Result<void>
setValueNode(kasofs::Vfs& fs, kasofs::User user, kasofs::INode::Id nodeId, std::string value) {
	auto maybeEntry = fs.open(user, nodeId, kasofs::Permissions::WRITE);
	if (!maybeEntry)
		return maybeEntry.moveError();

	auto& file = *maybeEntry;
	auto writeOk = file.write(wrapMemory(value.data(), value.size()));
	if (!writeOk)
		return writeOk.moveError();

	return none;
}


kasofs::Result<void>
mjstyxfs::mapJsonToFs(kasofs::Vfs& vfs, kasofs::INode::Id dirId, kasofs::VfsId jsonFsId, StringView valueName, rapidjson::Value const& value) {
	kasofs::User user = vfs.nodeById(vfs.rootId()).get().owner;

	switch (value.GetType()) {
	case rapidjson::Type::kNullType: return none;
	case rapidjson::Type::kTrueType: return none;
	case rapidjson::Type::kFalseType: return none;
	case rapidjson::Type::kNumberType:
		return vfs.mknode(dirId, valueName, jsonFsId, rapidjson::Type::kStringType, user)
				.then([user, &vfs, &value](kasofs::INode::Id id) -> kasofs::Result<void> {

				if (value.IsInt())		return setValueNode(vfs, user, id, static_cast<int64>(value.GetInt()));
				if (value.IsUint())		return setValueNode(vfs, user, id, static_cast<uint64>(value.GetUint()));
				if (value.IsInt64())		return setValueNode(vfs, user, id, value.GetInt64());
				if (value.IsUint64())		return setValueNode(vfs, user, id, value.GetUint64());
				if (value.IsDouble())		return setValueNode(vfs, user, id, value.GetDouble());

				return makeError(BasicError::InvalidInput, "not-a-number");
				});
	case rapidjson::Type::kStringType:
		return vfs.mknode(dirId, valueName, jsonFsId, rapidjson::Type::kStringType, user)
				.then([user, &vfs, &value](kasofs::INode::Id id) -> kasofs::Result<void> {
					return setValueNode(vfs, user, id, value.GetString());
				});
	case rapidjson::Type::kArrayType: {
		auto maybeDir = vfs.createDirectory(dirId, valueName, user, 0777);
		if (!maybeDir) {
			return maybeDir.moveError();
		}

		auto const objectDirId = *maybeDir;
		rapidjson::SizeType count{0};
		for (auto i = value.Begin(); i != value.End(); ++i, ++count) {
			auto childName = std::to_string(count);
			auto ok = mapJsonToFs(vfs, objectDirId, jsonFsId, StringView(childName.data(), childName.length()), *i);
			if (!ok)
				return ok.moveError();
		}

	} return none;

	case rapidjson::Type::kObjectType: {
		auto maybeDir = vfs.createDirectory(dirId, valueName, user, 0777);
		if (!maybeDir) {
			return maybeDir.moveError();
		}

		auto const objectDirId = *maybeDir;
		for (auto i = value.MemberBegin(); i != value.MemberEnd(); ++i) {
			auto ok = mapJsonToFs(vfs, objectDirId, jsonFsId, i->name.GetString(), i->value);
			if (!ok)
				return ok.moveError();
		}

		return none;
	}
	}

	return none;
}


FilePermissions
JsonFS::defaultFilePermissions(NodeType type) const noexcept {
	switch (type) {
	case rapidjson::Type::kObjectType: return FilePermissions(0777);
	case rapidjson::Type::kArrayType: return FilePermissions(0777);
	default:
		return FilePermissions(0644);
	}
}


JsonFS::size_type
JsonFS::nextId() noexcept {
	return _idBase++;
}


kasofs::Result<INode>
JsonFS::createNode(NodeType type, kasofs::User owner, kasofs::FilePermissions perms) {
	// Check node type:
	switch (type) {
	case rapidjson::Type::kNullType:
	case rapidjson::Type::kTrueType:
	case rapidjson::Type::kFalseType:
	case rapidjson::Type::kNumberType:
	case rapidjson::Type::kStringType:
		break;
	default:
		return makeError(SystemErrors::MEDIUMTYPE, "JsonFS::createNode");
	}


	INode node{type, owner, perms};
	node.dataSize = 0;
	node.vfsData = nextId();
	node.atime = nodeEpochTime();
	node.mtime = nodeEpochTime();
	_dataStore.emplace(node.vfsData, Buffer{});

	return mv(node);
}

kasofs::Result<void>
JsonFS::destroyNode(INode&) {
	return Ok();
}


kasofs::Result<Filesystem::OpenFID>
JsonFS::open(kasofs::INode&, kasofs::Permissions) {
	return Ok<OpenFID>(0);
}


kasofs::Result<JsonFS::size_type>
JsonFS::read(OpenFID, kasofs::INode& node, size_type offset, MutableMemoryView dest) {
	auto const id = node.vfsData;
	auto it = _dataStore.find(id);
	if (it == _dataStore.end())
		return makeError(GenericError::BADF, "JsonFs::read");

	auto& buffer = it->second;
	if (offset > buffer.size())
		return makeError(BasicError::Overflow, "JsonFs::read");

	auto data = wrapMemory(buffer.data() + offset, buffer.size() - offset).slice(0, dest.size());
	auto isOk = dest.write(data);
	if (!isOk) {
		return isOk.moveError();
	}

	return Ok(data.size());
}


kasofs::Result<Filesystem::size_type>
JsonFS::write(OpenFID, kasofs::INode& node, size_type offset, MemoryView src) {
	auto id = node.vfsData;
	auto it = _dataStore.find(id);
	if (it == _dataStore.end())
		return makeError(GenericError::BADF, "JsonFs::write");

	auto& buffer = it->second;
	if (offset > buffer.size())
		return makeError(BasicError::Overflow, "JsonFs::write");

	auto const newSize = offset + src.size();
	buffer.reserve(newSize);
	if (buffer.size() < newSize) {
		buffer.resize(newSize);
	}
	auto writeResult = wrapMemory(buffer.data(), buffer.size())
			.write(src, offset);

	if (!writeResult)
		return writeResult.moveError();

	node.dataSize = buffer.size();

	return Ok(src.size());
}


kasofs::Result<JsonFS::size_type>
JsonFS::seek(OpenFID, kasofs::INode&, size_type offset, SeekDirection) {
	return Ok(offset);
}


kasofs::Result<void>
JsonFS::close(OpenFID, kasofs::INode&) {
	return Ok();
}
