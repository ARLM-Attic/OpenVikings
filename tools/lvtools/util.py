from struct import Struct
from array import array
import os

word = Struct('<H')

def fread_word(f):
    return read_word(f.read(2))

def read_word(buf, offset=0):
    return word.unpack_from(buf, offset)[0]

def load_file(filename):
    data = array('B')
    with open(filename, 'rb') as f:
        data.fromfile(f, os.fstat(f.fileno()).st_size)
    return data

def dump_file(filename, data):
    with open(filename, 'wb') as f:
        f.write(data)

# From http://stackoverflow.com/a/312464/42029
def take_n(l, n):
    """ Yield successive n-sized chunks from l.
    """
    for i in xrange(0, len(l), n):
        yield l[i:i+n]

# From http://stackoverflow.com/a/8534391/42029
def find_if(pred, l):
    return next((x for x in l if pred(x)), None)
