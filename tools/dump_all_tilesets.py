import sys
import dump_tileset, dump_metatileset, dump_tileset_mask
from lvtools import level_info

if __name__ == '__main__':
    dumper = {
        'tiles': dump_tileset,
        'tilemask': dump_tileset_mask,
        'metatiles': dump_metatileset,
    }[sys.argv[1]]

    filename = "tilesets/%02d_%s.png"
    for i, (chunk_id, name) in enumerate(level_info.level_info):
        if name in ('MainMenu', 'MainMenu2', 'Ending1'):
            # Don't have tilesets
            continue
        print "Dumping %s..." % (name,)

        dumper.main(level_info.load_level_header(chunk_id=chunk_id), filename % (i, name), 16)
