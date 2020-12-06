#include "image_cache.h"

ImageCache& ImageCache::Instance()
{
	static ImageCache instance;
	return instance;
}

Image* ImageCache::Load(std::string const& path)
{
	// Egyesével meg kell nézni, hogy már nem lett-e betöltve,
	// mert ha igen, akkor nem kell még egyszer
	for (auto const& imageCandidate : loadedImages) {
		if (imageCandidate->path == path) {
			return imageCandidate.get();
		}
	}

	// Még nincs betöltve az image, tehát be kell tölteni
	theLogger.LogInfo("Loading image (stbi): {}\n", path);

	auto image = std::make_unique<Image>();
	image->path = path;
	image->data.reset(stbi_load(image->path.c_str(), &image->imageSize.x, &image->imageSize.y, &image->bitPerPixel, STBI_rgb_alpha));

	if (!image->data)
		throw std::runtime_error("stb::stbi_load failed");

	loadedImages.push_back(std::move(image));

	return loadedImages[loadedImages.size() - 1].get();
}

void ImageCache::Deflate()
{
	for (auto& loadedImage : loadedImages) {
		stbi_image_free(loadedImage.get());
		loadedImage.release();
	}
}

ImageCache::~ImageCache()
{
	Deflate();
}
