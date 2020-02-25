/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/
#pragma once
#ifndef MJSTYXFS_MODEL_HPP
#define MJSTYXFS_MODEL_HPP

#include <kasofs/kasofs.hpp>
#include <rapidjson/document.h>

#include <unordered_map>

namespace mjstyxfs {

struct JsonFS final : public kasofs::Filesystem {
	// Filesystem interface
	kasofs::FilePermissions defaultFilePermissions(NodeType type) const noexcept override;

	kasofs::Result<kasofs::INode> createNode(NodeType type, kasofs::User owner, kasofs::FilePermissions perms) override;
	kasofs::Result<void> destroyNode(kasofs::INode& node) override;

	kasofs::Result<OpenFID> open(kasofs::INode& node, kasofs::Permissions op) override;
	kasofs::Result<size_type> read(OpenFID fid, kasofs::INode& node, size_type offset, Solace::MutableMemoryView dest) override;
	kasofs::Result<size_type> write(OpenFID fid, kasofs::INode& node, size_type offset, Solace::MemoryView src) override;
	kasofs::Result<size_type> seek(OpenFID fid, kasofs::INode& node, size_type offset, SeekDirection direction) override;
	kasofs::Result<void> close(OpenFID fid, kasofs::INode& node) override;

protected:
	using DataId = kasofs::INode::VfsData;

	DataId nextId() noexcept;
private:
	DataId _idBase{0};
	using Buffer = std::vector<Solace::byte>;
	std::unordered_map<DataId, Buffer> _dataStore;
};


kasofs::Result<void>
mapJsonToFs(kasofs::Vfs& vfs, kasofs::INode::Id dirId, kasofs::VfsId jsonFsId, Solace::StringView name, rapidjson::Value const& value);

}  // namespace mjstyxfs
#endif  // MJSTYXFS_MODEL_HPP
