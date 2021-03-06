import sys

import lvtools.chunk as chunk
from lvtools.compression import decompress_data
from lvtools.level import LevelHeader
import lvtools.palette as palette
import lvtools.util as util

level_header_id = int(sys.argv[1], 0)
output_file = sys.argv[2]

header_data = decompress_data(chunk.read_chunk(level_header_id))
header = LevelHeader()
header.load(header_data)
pal = palette.assemble_palette(header.load_list.palettes)
util.dump_file(output_file, palette.pack_palette(pal))
