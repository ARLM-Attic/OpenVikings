#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <algorithm>

#include "vars.hpp"

// TODO
void hookKeyboard();
void unhookKeyboard();
void restoreVideo();
void restoreErrorInt();
uint32_t curSystemTime();

// TODO
void hookKeyboard() {}
void unhookKeyboard() {}
void restoreVideo() {}
void restoreErrorInt() {}
uint32_t curSystemTime() { return 0; }

inline uint16_t load16LE(uint8_t* d) {
	return d[0] | d[1] << 8;
}

void hookInts() {
	// init memory and segments();

	hookKeyboard();

	start_time = curSystemTime();
}

void errorQuit(const char* msg, uint16_t error_code) {
	unhookKeyboard();
	// TODO ??? call

	std::fclose(data_file);

	restoreVideo();
	restoreErrorInt();

	std::printf("\n*** An Error has occured while running PC Vikings ***\n");
	std::printf("%s", msg);
	if (error_code != 0) {
		std::printf("\nError code is: %04x\n", error_code);
	}

	std::exit(0);
}

void hwCheck() {
	struct Local {
		static void errReadData() {
			errorQuit("\nUnable to read data file.\n", errno);
		}

		static void errOpenData() {
			errorQuit("\nCannot find 'data.dat' file in the current directory.\n", errno);
		}

		static void errCopyprotect() {
			errorQuit("\nCopy protection failure. Please run setup.\n", 0);
		}

		static void errCorruptData() {
			errorQuit("\nData.dat has been corrupted. Please re-install.\n", 0);
		}

		static void errVgaCard() {
			errorQuit("\nPC Vikings must be run on a system with a VGA card.\n", 0);
		}

		static void errCpu386() {
			errorQuit("\nPC Vikings requires a 386 (or better) microprocessor to run.\n", 0);
		}
	};

	data_file = std::fopen("data.dat", "rb");
	if (!data_file)
		Local::errOpenData();

	if (std::fread(&chunk_offsets, sizeof(int32_t), 2, data_file) != 2)
		Local::errReadData();
	if (std::fseek(data_file, chunk_offsets[0], SEEK_SET) != 0)
		Local::errReadData();

	if (std::fread(&data_header1, sizeof(DataHeader1), 1, data_file) != 1)
		Local::errReadData();
	
	if (data_header1.magic != 0x6969)
		Local::errCorruptData();

	// insert funky system/BIOS checksum here
	
	if (false /* systemChecksum != data_header1.copy_checksum */)
		Local::errCopyprotect();

	data_header1_snd1 = data_header1.snd1;
	data_header1_snd2 = data_header1.snd2;

	if (false /* check video here */)
		Local::errVgaCard();

	if (false /* check cpu here */)
		Local::errCpu386();

	// calibrate joysticks here
}

void roundToNextSeg(uint8_t** seg, uint16_t* off) {
	// What am I supposed to do here?
	*seg += *off;
	*off = 0;
}

// Note: Originally this function returned es:di on the end of the buffer. Now I just return the pointer to it
uint8_t* decompressChunk(uint16_t chunk_id, uint8_t* dst) {
	struct Local {
		static void errBufOverrun() {
			errorQuit("\nGlobal buffer overrun on file read.\n", 0);
		}
		static void errReadData() {
			errorQuit("\nUnable to read data file.\n", errno);
		}
	};

	uint8_t* dst_copy = dst;

	if (chunk_id == 0xFFFA)
		return dst;

	if (std::fseek(data_file, chunk_id * 4, SEEK_SET) != 0)
		Local::errReadData();

	if (std::fread(&chunk_offsets, sizeof(int32_t), 2, data_file) != 2)
		Local::errReadData();
	if (std::fseek(data_file, chunk_offsets[0], SEEK_SET) != 0)
		Local::errReadData();
	if (std::fread(&decoded_data_len, sizeof(uint16_t), 1, data_file) != 1)
		Local::errReadData();

	int read_size = chunk_offsets[1] - chunk_offsets[0];
	if (read_size >= 0xB080)
		Local::errBufOverrun();

	if (std::fread(alloc_seg6 + 0x1000, read_size, 1, data_file) != 1)
		Local::errReadData();

	std::memset(alloc_seg6, 0, 0x1000);

	uint8_t* src = alloc_seg6;
	uint16_t bx = 0;
	uint16_t dx = decoded_data_len;
	uint16_t src_off = 0x1000;

	while (true) {
		uint8_t al = src[src_off++];

		for (int i = 0; i < 8; ++i) {
			if (al >> i & 1) {
				src[bx] = src[src_off];
				++bx &= 0x0FFF;
				*dst++ = src[src_off];
				if (--dx >= decoded_data_len) {
#if 0
					char tmpfn[30];
					sprintf(tmpfn, "chunks/chunk%03x.bin", chunk_id);
					std::FILE* df = std::fopen(tmpfn, "wb");
					std::fwrite(dst_copy, decoded_data_len+1, 1, df);
					std::fclose(df);
#endif
					return dst;
				}
				++src_off;
			} else {
				uint16_t new_src_off = load16LE(&src[src_off]);
				src_off += 2;

				uint16_t cx = (new_src_off >> 12) + 3;
				new_src_off &= 0x0FFF;

				do {
					src[bx] = src[new_src_off];
					++bx &= 0x0FFF;
					*dst++ = src[new_src_off];
					if (--dx >= decoded_data_len) {
#if 0
						char tmpfn[30];
						sprintf(tmpfn, "chunks/chunk%03x.bin", chunk_id);
						std::FILE* df = std::fopen(tmpfn, "wb");
						std::fwrite(dst_copy, decoded_data_len+1, 1, df);
						std::fclose(df);
#endif
						return dst;
					}
					++new_src_off &= 0x0FFF;
				} while (--cx != 0);
			}
		}
	}
}

void allocMemAndLoadData() {
	alloc_seg1 = new uint8_t[0x8B80];
	alloc_seg2 = new uint8_t[0x2000];
	alloc_seg0 = new uint8_t[0x3600];
	alloc_seg3 = new uint8_t[0x3130];
	alloc_seg6 = new uint8_t[0xC080];
	alloc_seg11 = new uint8_t[0x7000];

	// Load sound data if sound enabled
	if (!(data_header1_snd1 && data_header1_snd2 && 0x8000)) {
		uint8_t* buf = new uint8_t[0xE470];
		ptr2_seg = buf;
		ptr2_offset = 0;

		// Note: roundToNextSeg not needed because segments are for 16-bit losers
		// I'm not sure these are quite right, they seem to point to the end of the data
		// but looking at the original that's effectively what happens?
		sound_buffer0 = buf = decompressChunk(0x1C7 + data_header1.field_6, buf);
		sound_buffer1 = buf = decompressChunk(0x20C + data_header1.field_8, buf);
		sound_buffer2 = decompressChunk(0x207 + data_header1.field_8, buf);
	}

	ptr3_seg = new uint8_t[0x2ABA0];
	ptr3_offset = 0;

	alloc_seg5 = new uint8_t[0xC000];

	loaded_chunk0 = 0x1C6;
	decompressChunk(loaded_chunk0, alloc_seg5);

	decompressChunk(4, chunk_buffer0);
	decompressChunk(5, chunk_buffer1);
	decompressChunk(6, chunk_buffer2);
	decompressChunk(7, chunk_buffer3);
	decompressChunk(8, chunk_buffer4);
	decompressChunk(9, chunk_buffer5);
	decompressChunk(10, chunk_buffer6);
	decompressChunk(11, chunk_buffer7);
	decompressChunk(12, chunk_buffer8);
	decompressChunk(13, chunk_buffer9);
	decompressChunk(14, chunk_buffer10);
	decompressChunk(2, chunk_buffer11);

	current_password[0] = 'S';
	current_password[1] = 'T';
	current_password[2] = 'R';
	current_password[3] = 'T';
	// TODO bunch of variable sets
}

void freeMemAndQuit() {
	unhookKeyboard();
	// TODO one call

	delete[] alloc_seg1;
	delete[] alloc_seg0;
	delete[] alloc_seg2;
	delete[] alloc_seg3;
	delete[] alloc_seg5;
	delete[] ptr3_seg;

	std::fclose(data_file);

	restoreVideo();
	restoreErrorInt();

	std::exit(0);
}

void clearSoundBuffers() {}

void initSound() {
	// Major TODO

	did_init_sound = 0;
	// TODO word_32858
	clearSoundBuffers();

	if (!(data_header1_snd1 && data_header1_snd2 && 0x8000)) {
		// sound enabled
		did_init_sound = 1;
	} else {
		did_init_sound = 1;
	}
}

void initVideo() {
	// TODO I probably want to do some limited form of VGA emulation here at
	// least initially, but in a less convoluted manner, please.
}

void fadePal() {
	Color c_and;
	for (int c = 0; c < 3; ++c) {
		c_and.rgb[c] = color1.rgb[c] | color2.rgb[c];
	}

	for (int i = 0; i < 0x100; ++i) {
		for (int c = 0; c < 3; ++c) {
			int8_t x = palette1[i].rgb[c];

			x -= c_and.rgb[c];

			if (x < 0)
				x = 0;
			if ((x & 0x40) != 0)
				x = 0x3F;

			palette2[i].rgb[c] = x;
		}
	}
}

void sub_16775() {
	uint16_t si = word_28526 + word_28880;
	if (si > word_2AA86)
		si = word_28526 - word_28880;

	uint16_t cx = word_30ED8[(si & 0xFFF8)/8 + word_317D9/2];
	uint16_t bx = word_31338[7 & si] + cx;

	uint16_t ax = word_28524 + word_2887E;
	if (ax > word_2AA84)
		ax = word_28524 - word_2887E;

	byte_317CE = (ax & 3) * 2;
	bx += (ax * 4) + 8;

	//vga_ports[0x3D4][0xD] = bx & 0xFF;
	//vga_ports[0x3D4][0xC] = (bx >> 8) & 0xFF;

	// TODO vars
}

void fadePalKeepFirst() {
	Color c_and;
	for (int c = 0; c < 3; ++c) {
		c_and.rgb[c] = color1.rgb[c] | color2.rgb[c];
	}

	for (int i = 0; i < 0x100; ++i) {
		for (int c = 0; c < 3; ++c) {
			int8_t x = palette1[i].rgb[c];

			x -= c_and.rgb[c];

			if (x < 0)
				x = 0;
			if ((x & 0x40) != 0)
				x = 0x3F;

			palette2[i].rgb[c] = x;
		}

		// Copy entries 1-15 unchanged
		for (; i < 0x10; ++i) {
			palette2[i] = palette1[i];
		}
	}
}

void zero_word_288C4() {
	// TODO
	//word_288F4 = 0;
	//word_288F6 = 0;
	//word_288F8 = 0;
	//std::memset(word_288C4, 0, 0x18);
	//word_28903 = 6;
	//word_28905 = 6;
	//word_28907 = 6;
	//word_28909 = 0;
	//word_2890B = 0;
	//word_2890D = 0;
}

void sub_108B8() {
	zero_word_288C4();
	data_load_list_struct.next_load_list_id = 0x27;
	// TODO word_288A2 = 0;
}

void loadLoadList(uint16_t list_id) {
	uint16_t chunk_id = loadListChunksB[list_id];
	if (chunk_id != 0xFFFF && chunk_id != loaded_chunk0) {
		loaded_chunk0 = chunk_id;
		decompressChunk(chunk_id, alloc_seg5);
	}
	decompressChunk(loadListChunksA[list_id], reinterpret_cast<uint8_t*>(&data_load_list_struct));
}

uint16_t seekLoadList() {
	uint16_t di;
	for (di = 0; load16LE(&data_load_list[di]) != 0xFFFF; di += 14);
	return di + 2;
}

uint16_t loadPaletteList(uint16_t di) {
	while (true) {
		uint16_t ax = load16LE(&data_load_list[di]);
		di += 2;

		if (ax == 0xFFFF)
			break;

		uint16_t offset = data_load_list[di++];
		decompressChunk(ax, reinterpret_cast<uint8_t*>(&palette1[offset]));
	}

	for (int i = 0; i < 16; ++i) {
		palette1[16*i].rgb[0] = 0;
		palette1[16*i].rgb[1] = 0;
		palette1[16*i].rgb[2] = 0;
	}

	byte_2AA88 = palette1[3];

	fadePalKeepFirst();

	return di;
}

uint16_t processLoadList(uint16_t di) {
	byte_2AA63 = data_load_list[di];
	di += 2;

	uint16_t si = 0;

	uint8_t al;
	while ((al = data_load_list[di++]) != 0) {
		byte_2AA64[si] = al;
		byte_2AA6C[si] = al;
		byte_2AA74[si] = data_load_list[di++ + 1];
		byte_2AA7C[si] = data_load_list[di++ + 2];

		for (; load16LE(&data_load_list[di]) != 0xFFFF; di += 2);
		di += 2;

		si += 1;
	}

	return di;
}

uint16_t loadChunkList(uint16_t di) {
	uint16_t si = 0;
	uint8_t* bx = alloc_seg11;

	while (true) {
		uint16_t ax = load16LE(&data_load_list[di]);
		di += 2;
		if (ax == 0xFFFF)
			break;
		loaded_chunks11[si] = ax;
		loaded_chunks_end11[si] = (bx - alloc_seg11) + 1;
		bx = decompressChunk(ax, bx);

		di += 4;
		si++;
	}

	return di;
}

uint16_t loadChunkList2(uint16_t di) {
	uint16_t si = 0;

	while (true) {
		uint16_t chunk_id = load16LE(&data_load_list[di]);
		if (chunk_id == 0xFFFF)
			break;
		loaded_chunks2[si] = chunk_id;

		uint8_t* dest = ptr1;
		loaded_chunks2_ptr[si] = dest;
		ptr1 = decompressChunk(chunk_id, dest);
		di += 5;
		si++;
	}

	return di;
}

void sub_11784() {
	// TODO word_288A2 = 0;
	// TODO word_288A4 = 0;
}

void sub_11397() {
	// TODO word_28886 = byte_30A1E[word_2AAAB];
	// TODO word_28888 = byte_30A24[word_2AAAB];
}

void sub_137F1() {
	// TODO std::fill_n(word_29835, 20, 0);
	// TODO std::fill_n(word_29EED, 20, 0xFFFF);
}

void sub_12FB3() {
	// TODO std::fill_n(word_2892D, 128, 0);
}

// TODO
bool sub_13BBD(uint16_t si, uint16_t di) {
	return false;
}

void sub_13BA5() {
	// TODO word_28522 = 0xFFFF;
	uint16_t si = 0;
	uint16_t di = 0;

	while (sub_13BBD(si, di)) {
		si++;
		di += 14;
	}
}

void sub_13A0E() {
	// TODO sub_139EF();
	// TODO loc_13A94();
}

void sub_115D2() {
	// TODO
}

// addr seg00:6595
void loc_16595(uint16_t ax) {
	for (uint16_t si = 0; si < 0x100; ++si) {
		word_31448[si] = si*ax*2;
	}
}

// addr seg00:13B0
void sub_113B0() {
	uint16_t ax;
	
	ax = word_2AABC * 2;
	word_31648 = ax;
	video_levelWidth = ax * 8 - 320;

	ax = word_2AABE * 2;
	word_3164A = ax;
	video_levelHeight = ax * 8 - 176; // viewport height

	loc_16595(word_2AABC);
}

void sub_11080() {
	// TODO lotsa variables
	// TODO doPalFade1();
	// TODO sound_func1();
	// TODO zero_byte_31A4C();
	// TODO some vars
	// TODO loadChunks1();
	sub_11784();
	// TODO video_clearVRAM();
	// TODO zero_byte_2892D();
	// TODO zero_byte_28836();
	// TODO word_2AA8B = word_2AA8D;
	loadLoadList(data_load_list_struct.next_load_list_id);
	// TODO setColor1And2();
	if (word_2AA8D != 0x25)
		zero_word_288C4();
	// TODO sub_117AD();
	// TODO loadChunks3();
	uint16_t di = seekLoadList();
	di = loadPaletteList(di);
	di = processLoadList(di);
	di = loadChunkList(di);
	di = loadChunkList2(di);
	sub_11397();
	sub_137F1();
	sub_12FB3();
	// ***? TODO sub_11446();
	sub_113B0();
	// TODO sub_113D8();
	// TODO sub_17749();
	// *** TODO sub_173C7();
	// *** TODO sub_11439();
	sub_13BA5();
	sub_13A0E();
	sub_115D2();
	// TODO doPalFade2(); // Fade in
	// TODO sub_12345();
}

void test_read(uint16_t chunk_id) {
	struct Local {
		static void errReadData() {
			errorQuit("\nUnable to read data file.\n", errno);
		}
	};

	if (std::fseek(data_file, chunk_id * 4, SEEK_SET) != 0)
		Local::errReadData();

	if (std::fread(&chunk_offsets, sizeof(int32_t), 2, data_file) != 2)
		Local::errReadData();
	if (std::fseek(data_file, chunk_offsets[0], SEEK_SET) != 0)
		Local::errReadData();
	if (std::fread(&decoded_data_len, sizeof(uint16_t), 1, data_file) != 1)
		Local::errReadData();

	uint8_t planes[4][64*1024];
	std::memset(planes, 0, sizeof(planes));

	for (int i = 0; i < 4; ++i) {
		if (std::fread(planes[i], decoded_data_len, 1, data_file) != 1)
			Local::errReadData();
	}

	char tmpfn[30];
	sprintf(tmpfn, "image%03x.bin", chunk_id);
	std::FILE* df = std::fopen(tmpfn, "wb");
	for (int i = 0; i < decoded_data_len; ++i) {
		for (int j = 0; j < 4; ++j) {
			std::fputc(planes[j][i], df);
		}
	}
	std::fclose(df);
}


int main() {
	hookInts();
	hwCheck();
	allocMemAndLoadData();
	initSound();
	initVideo();
	// TODO setSomeGlobals(); sub_12CA3
	sub_108B8();
	sub_11080(); // Logo fade in
	// TODO
}