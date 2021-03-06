#pragma once
#include <cstdint>

struct DrawSurface {
	uint8_t* pixels;
	int width, height;
	ptrdiff_t stride;
	// Number of offscreen pixels on each side that can be safely overwritten.
	int guard_band;

	uint8_t* at(int x, int y) const {
		return &pixels[(x + guard_band) + (y + guard_band) * stride];
	}
};

void fillSurface(uint8_t color, const DrawSurface& surface);
void drawMasked8x8Tile(uint8_t* tile_data, const DrawSurface& surface, int x, int y);
void loadFont();
void drawTextTile(int c, const DrawSurface& surface, int x, int y);
void drawTextTilePlus(int c, uint8_t color, const DrawSurface& surface, int x, int y);
