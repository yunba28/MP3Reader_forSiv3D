#pragma once

#include <Siv3D.hpp>

inline uint32 ToSyncsafe(uint32 val)
{
	uint8* p = reinterpret_cast<uint8*>(&val);
	return (p[0] << 21) + (p[1] << 14) + (p[2] << 7) + p[3];
}

class MP3 final
{
private:

	enum class HeaderSize
	{
		Tag = 3,
		Version = 2,
		Flag = 1,
		Size = 4
	};

	enum class ExHeaderSize
	{
		Size = 4
	};

	enum class FrameSize
	{
		ID = 4,
		Size = 4,
		Flag = 2,
		Encording = 1
	};

public:

	MP3();

	MP3(FilePathView _path);

	~MP3();

	void play();

	void stop();

	const String& title()const noexcept;

	const String& artist()const noexcept;

	const String& album()const noexcept;

private:

	void readTags(FilePathView _path);

	void readFrameISO88591(MemoryReader& _reader, const String& _id, uint64 _offset, uint64 _size);

	void readFrameUTF16_BOM(MemoryReader& _reader, const String& _id, uint64 _offset, uint64 _size);

	void readFrameUTF16_UNBOM(MemoryReader& _reader, const String& _id, uint64 _offset, uint64 _size);

	void readFrameUTF8(MemoryReader& _reader, const String& _id, uint64 _offset, uint64 _size);

private:

	Audio mAudio;

	String mTitle;

	String mArtist;

	String mAlbum;

};
