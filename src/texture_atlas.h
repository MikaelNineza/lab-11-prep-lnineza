#ifndef TEXTURE_ATLAS_H
#define TEXTURE_ATLAS_H

#include <algorithm>
#include <cassert>
#include <iostream>

#include "vector_util.h"
#include <SFML/Graphics.hpp>


// RectPacker maintains two offsets into an overall bin size:
//  - A line y-offset, representing the current horizontal line on which to place new rectangles
//      horizontally along (the line corresponding to the rectangle tops)
//  - An x-offset into the line, representing the x-coordinate to align the next rectangle left-side
// Furthermore it maintains the maximum texture height encountered along the current line.
// The RectPacker places rectangles left to right along this line until it cannot fit the next
//  rectangle. Once this happens, the line is shifted down to meet the bottom of the tallest texture
//  placed on the line, and the x-offset is reset. This continues until the next rectangle
//  packed cannot be fit on the line within the bin space.
class RectPacker {
public:
    static constexpr float max_val = std::numeric_limits<float>::max();
    RectPacker(const sf::Vector2f& size): mSize(size) {};

    // pack returns { succeeded, rect id }. If not succeeded, id is irrelevant (but ideally should
    // be set to the maximum possible value).
    std::pair<bool, size_t> pack(const sf::Vector2f& rectSize) {
        if (rectSize.x > mSize.x || rectSize.y > mSize.y) {
            return {false, max_val};
        }

        
        if (rectSize.x + mXOffset > mSize.x) {
            mYOffset += mCurrentTexHeight;
            mCurrentTexHeight = 0.f;
            mXOffset = 0.f;
        }
        
        if (rectSize.y + mYOffset > mSize.y) {
            return {false, max_val};
        }

        sf::FloatRect newRect(sf::Vector2f{mXOffset, mYOffset}, rectSize);
        mRects.push_back(newRect);
        mXOffset += rectSize.x;
        mCurrentTexHeight = std::max(mCurrentTexHeight, rectSize.y);
        return {true, mRects.size() - 1};
    }

    // Returns the coordinates and size of a previously returned rectangle id.
    // Note that in this coordinate system, y points downwards from the origin at the top-left.
    sf::FloatRect getBounds(size_t rectID) const {
        return mRects.at(rectID);
    }

private:
    float mYOffset = 0.f;
    float mXOffset = 0.f;
    float mCurrentTexHeight = 0.f;
    sf::Vector2f mSize;
    std::vector<sf::FloatRect> mRects;
};

// Returns a new image which pages the given img with *pixels* pixels on each side.
// The colour of new border pixels are taken by sampling the closest valid pixel in the original
// image.
//
// NOTE: a production implementation might also support other edge wrapping behaviors, like cycling
// around to the other side of the texture. See https://learnopengl.com/Getting-started/Textures
// Our implementation corresponds to GL_CLAMP_TO_EDGE
inline sf::Image padImage(const sf::Image& img, uint pixels) {
    sf::Vector2u newDims(img.getSize().x + 2*pixels, img.getSize().y + 2*pixels);
    sf::Image newImage(newDims, sf::Color::Black);

    int maxX = static_cast<int>(img.getSize().x) - 1;
    int maxY = static_cast<int>(img.getSize().y) - 1;
    int p = static_cast<int>(pixels);
    for (int y = 0; y < newDims.y; ++y) {
        for (int x = 0; x < newDims.x; ++x) {
            int relX = std::clamp(x - p, 0, maxX);
            int relY = std::clamp(y - p, 0, maxY);

            sf::Color color = img.getPixel({static_cast<unsigned int>(relX), static_cast<unsigned int>(relY)});
            newImage.setPixel({static_cast<unsigned int>(x), static_cast<unsigned int>(y)}, color);
        }
    }
    return newImage;
};

using TextureID = size_t;

class TextureAtlas {
public:
    TextureAtlas(sf::Vector2u binSize, size_t padding)
        : mTexture(binSize), mPacker(sf::Vector2f(mTexture.getSize())), mPadding{padding} {}

    TextureID registerTexture(const std::string& filepath) {
        sf::Image temp;
        if (!temp.loadFromFile(filepath)) {
            throw;
        }

        sf::Image img = padImage(temp, mPadding);
        auto [canPack, id] = mPacker.pack(sf::Vector2f(img.getSize()));
        if (!canPack) {
            throw;
        }

        mTexture.update(img, sf::Vector2u(mPacker.getBounds(id).position));

        return id;
    }

    sf::FloatRect getBoundsUV(TextureID id) const { return {}; }

    sf::Vector2u getSizePixels(TextureID id) const { return {}; }

    const sf::Texture& getTexture() const { return mTexture; }

private:
    sf::Texture mTexture;
    RectPacker mPacker;
    size_t mPadding;
};

#endif  // TEXTURE_ATLAS_H
