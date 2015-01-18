#!/usr/bin/env python

import sys
import os
from tempfile import mkdtemp
from shutil import make_archive
import numpy as np
from PIL import Image
import sqlite3


def unpickle(f):
    import cPickle
    fo = open(f, 'rb')
    d = cPickle.load(fo)
    fo.close()
    return d

def main():
    if len(sys.argv) != 2:
        print "Usage:", sys.argv[0], "<cifar batch file name>"
        return 1

    if os.path.exists("samples.tar"):
        print "samples.tar already exists in current directory!"
        return 1
    if os.path.exists("index.sqlite3"):
        print "index.sqlite3 already exists in current directory"
        return 1

    raw_data = unpickle(sys.argv[1])
    data = np.array(raw_data['data'])
    labels = raw_data['labels']

    tmpdir = mkdtemp()

    conn = sqlite3.connect("index.sqlite3")
    query = """create table objects(
        id integer primary key autoincrement not null,
        frame integer not null,
        x integer not null, y integer not null,
        width integer not null, height integer not null,
        descriptor blob,
        class int,
        filename text unique not null);"""
    conn.execute(query)
    query = """create table metadata(
        width integer not null, height integer not null
        );"""
    conn.execute(query)
    query = """insert or replace into metadata (width, height)
        values (32, 32);"""
    conn.execute(query)

    query = """insert into objects (frame, x, y, width, height, filename, class)
        values (?, 0, 0, 32, 32, ?, ?);"""
    for i in xrange(len(data)):
        array = np.zeros((32, 32, 3), dtype=np.uint8)
        cntr = 0
        for channel in xrange(3):
            for x in xrange(32):
                for y in xrange(32):
                    array[x, y, channel] = data[i][cntr]
                    cntr += 1
        image = Image.fromarray(array, mode='RGB')
        filename = "sample_"+str(i+1)+"_1.png"
        image.save(os.path.join(tmpdir, filename))
        conn.execute(query, (i+1, filename, labels[i]))

    make_archive("samples", "tar", tmpdir)
    conn.commit()
    conn.close()

    return 0


if __name__ == '__main__':
    sys.exit(main())

