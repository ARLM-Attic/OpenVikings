#include "main.hpp"

#include "vga_emu.hpp"
#include "input.hpp"
#include "timer.hpp"
#include "data.hpp"
#include "decoding.hpp"
#include "draw.hpp"
#include <array>
#include <vector>

static const ChunkId FONT_CHUNK = 2;
std::array<uint8_t, 0x1680> font_graphics;

static void drawTextTile(int c, const DrawSurface& surface, int x, int y) {
	assert(c >= 16 && c < 112);
	drawMasked8x8Tile(&font_graphics[(c - 16) * 8 * 9], surface, x, y);
}

static void deinitializeGame() {
	closeDataFiles();
	vga_deinitialize();
}

void errorQuit(const char* message, unsigned int code) {
	std::printf("*** OpenVikings encoutered a runtime error:\n\n%s\nError code: %ud\n", message, code);

	deinitializeGame();
	exit(1);
}

int main() {
	vga_initialize();
	openDataFiles();

	{
		std::vector<uint8_t> font_buffer = decompressChunk(FONT_CHUNK);
		assert(font_graphics.size() == font_buffer.size());

		for (size_t i = 0; i < font_buffer.size(); i += 9*8) {
			deplaneMasked8x8Tile(&font_buffer[i], &font_graphics[i]);
		}
	}

	vga_setPaletteColor(0, 0x000000);
	vga_setPaletteColor(1, 0x000000);
	vga_setPaletteColor(2, 0xFBFBFB);
	vga_setPaletteColor(3, 0x920000);

	while (true) {
		input_handleSDLEvents();
		if (input_quit_requested) {
			break;
		}

		drawTextTile('A', vga_surface, 0, 0);

		vga_present();
	}

	while (true) {
		input_handleSDLEvents();
		if (input_quit_requested) {
			break;
		}

		vga_present();
	}

	deinitializeGame();
}
