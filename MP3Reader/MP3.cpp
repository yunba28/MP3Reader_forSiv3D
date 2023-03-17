#include "MP3.hpp"

MP3::MP3()
	: mAudio()
	, mTitle()
	, mArtist()
	, mAlbum()
{
}

MP3::MP3(FilePathView _path)
	: mAudio(Audio::Stream, _path)
	, mTitle()
	, mArtist()
	, mAlbum()
{
	readTags(_path);
}

MP3::~MP3()
{
}

void MP3::play()
{
	mAudio.play();
}

void MP3::stop()
{
	mAudio.stop();
}

const String& MP3::title() const noexcept
{
	return mTitle;
}

const String& MP3::artist() const noexcept
{
	return mArtist;
}

const String& MP3::album() const noexcept
{
	return mAlbum;
}

void MP3::readTags(FilePathView _path)
{
	MemoryReader reader{ Blob{ _path } };

	if (not reader.isOpen())
		return;

	uint64 offset = 0;
	uint64 size = FromEnum(HeaderSize::Tag);

	String headerTag; // ヘッダ識別子
	std::string tmp{ "000"};
	reader.read(tmp.data(), offset, size);
	headerTag = Unicode::FromUTF8(tmp);

	offset += size;
	size = FromEnum(HeaderSize::Version);

	uint16 headerVersion; // MP3バージョン
	reader.read(&headerVersion, offset, size);
	uint32 major = headerVersion & (1 << 0);
	uint32 revision = headerVersion & (1 << 2);
	headerVersion = (major == 3 and revision == 0) ? 3 : 4;

	offset += size;
	size = FromEnum(HeaderSize::Flag);

	uint8 headerFlag; // MP3フラグ
	reader.read(&headerFlag, offset, size);

	[[maybe_unused]] bool unSync = headerFlag & (1 << 7); // 非同期か
	bool useExtend = headerFlag & (1 << 6); // 拡張ヘッダを使うか

	offset += size;
	size = FromEnum(HeaderSize::Size);

	uint32 headerSize; // ヘッダ以降のタグサイズ
	reader.read(&headerSize, offset, size);
	headerSize = ToSyncsafe(headerSize);

	offset += size;
	size = FromEnum(ExHeaderSize::Size);

	uint32 exheaderSize; // 拡張ヘッダのサイズ
	if (useExtend)
	{
		reader.read(&exheaderSize, offset, size);
		if (headerVersion != 3)
			exheaderSize = ToSyncsafe(exheaderSize);

		offset += size;
		size = exheaderSize - size;
	}
	else
	{
		size = 0;
	}

	offset += size;
	size = FromEnum(FrameSize::ID);

	uint64 readerSize = reader.size();

	while (readerSize > (offset + size))
	{
		String frameID; // フレームID
		std::string tmp{ "0000" };
		reader.read(tmp.data(), offset, size);
		frameID = Unicode::FromUTF8(tmp);

		offset += size;
		size = FromEnum(FrameSize::Size);

		uint32 frameSize; // フレームサイズ
		reader.read(&frameSize, offset, size);
		if (headerVersion != 3)
			frameSize = ToSyncsafe(frameSize);

		offset += size;
		size = FromEnum(FrameSize::Flag);

		uint16 frameFlag; // フレームフラグ
		reader.read(&frameFlag, offset, size);

		uint32 frameEncording = 0;
		if (frameID != U"TYER" and frameID != U"TRCK")
		{
			offset += size;
			size = FromEnum(FrameSize::Encording);

			reader.read(&frameEncording, offset, size);
			frameEncording = ToSyncsafe(frameEncording);

			// テキストエンコーディングのバイナリビット分を引く
			frameSize -= 1;
		}

		offset += size;
		size = frameSize;

		switch (frameEncording)
		{
		case 0: readFrameISO88591(reader, frameID, offset, size); break;
		case 1: readFrameUTF16_BOM(reader, frameID, offset, size); break;
		case 2: readFrameUTF16_UNBOM(reader, frameID, offset, size); break;
		case 3: readFrameUTF8(reader, frameID, offset, size); break;
		default:readFrameISO88591(reader, frameID, offset, size); break;
		}

		offset += size;
		size = FromEnum(FrameSize::ID);
	}
}

void MP3::readFrameISO88591(MemoryReader& _reader, const String& _id, uint64 _offset, uint64 _size)
{
	if (_id == U"TIT2") // 曲名
	{
		std::string tmp{ _size,'0',std::allocator<char>{} };
		_reader.read(tmp.data(), _offset, _size);
		mTitle = Unicode::Widen(tmp);
	}
	else if (_id == U"TPE1") // アーティスト名
	{
		std::string tmp{ _size,'0',std::allocator<char>{} };
		_reader.read(tmp.data(), _offset, _size);
		mArtist = Unicode::Widen(tmp);
	}
	else if (_id == U"TALB") // アルバム名
	{
		std::string tmp{ _size,'0',std::allocator<char>{} };
		_reader.read(tmp.data(), _offset, _size);
		mAlbum = Unicode::Widen(tmp);
	}
}

void MP3::readFrameUTF16_BOM(MemoryReader& _reader, const String& _id, uint64 _offset, uint64 _size)
{
	if (_id == U"TIT2") // 曲名
	{
		std::u16string tmp{ _size,'0',std::allocator<char>{} };
		_reader.read(tmp.data(), _offset, _size);
		mTitle = Unicode::FromUTF16(tmp);
	}
	else if (_id == U"TPE1") // アーティスト名
	{
		std::u16string tmp{ _size,'0',std::allocator<char>{} };
		_reader.read(tmp.data(), _offset, _size);
		mArtist = Unicode::FromUTF16(tmp);
	}
	else if (_id == U"TALB") // アルバム名
	{
		std::u16string tmp{ _size,'0',std::allocator<char>{} };
		_reader.read(tmp.data(), _offset, _size);
		mAlbum = Unicode::FromUTF16(tmp);
	}
}

void MP3::readFrameUTF16_UNBOM(MemoryReader& _reader, const String& _id, uint64 _offset, uint64 _size)
{
	if (_id == U"TIT2") // 曲名
	{
		std::u16string tmp{ _size,'0',std::allocator<char>{} };
		_reader.read(tmp.data(), _offset, _size);
		mTitle = Unicode::FromUTF16(tmp);
	}
	else if (_id == U"TPE1") // アーティスト名
	{
		std::u16string tmp{ _size,'0',std::allocator<char>{} };
		_reader.read(tmp.data(), _offset, _size);
		mArtist = Unicode::FromUTF16(tmp);
	}
	else if (_id == U"TALB") // アルバム名
	{
		std::u16string tmp{ _size,'0',std::allocator<char>{} };
		_reader.read(tmp.data(), _offset, _size);
		mAlbum = Unicode::FromUTF16(tmp);
	}
}

void MP3::readFrameUTF8(MemoryReader& _reader, const String& _id, uint64 _offset, uint64 _size)
{
	if (_id == U"TIT2") // 曲名
	{
		std::string tmp{ _size,'0',std::allocator<char>{} };
		_reader.read(tmp.data(), _offset, _size);
		mTitle = Unicode::Widen(tmp);
	}
	else if (_id == U"TPE1") // アーティスト名
	{
		std::string tmp{ _size,'0',std::allocator<char>{} };
		_reader.read(tmp.data(), _offset, _size);
		mArtist = Unicode::Widen(tmp);
	}
	else if (_id == U"TALB") // アルバム名
	{
		std::string tmp{ _size,'0',std::allocator<char>{} };
		_reader.read(tmp.data(), _offset, _size);
		mAlbum = Unicode::Widen(tmp);
	}
}
