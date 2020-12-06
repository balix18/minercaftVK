#pragma once
#include "pch.h"

struct Image
{
	std::string path;

	glm::ivec2 imageSize;
	int bitPerPixel = 0;
	std::unique_ptr<unsigned char> data;
};

struct ImageCache
{
	static ImageCache& Instance();

	ImageCache() = default;
	~ImageCache();

	ImageCache(ImageCache const&) = delete;
	ImageCache& operator=(ImageCache const&) = delete;
	ImageCache(ImageCache&&) = delete;
	ImageCache& operator=(ImageCache&&) = delete;

	Image* Load(std::string const& path);
	void Deflate();

private:

	std::vector<std::unique_ptr<Image>> loadedImages;
};

inline ImageCache& theImageCache = ImageCache::Instance();

