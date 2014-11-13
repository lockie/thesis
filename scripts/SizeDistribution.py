#!/usr/bin/env python

import os
import sqlite3
import matplotlib.pyplot as plt


def usage(argv):
	print "USAGE: " + argv[0] + " [dataset path]"

def main(argv):
	if len(argv) > 2:
		usage(argv)
		return 1
	path = '.' if len(argv) == 1 else argv[1]
	filename = os.path.join(path, 'index.sqlite3')
	if not os.path.exists(filename):
		print "No dataset found in directory '" + path + "'"
		return 2

	conn = sqlite3.connect(filename)
	cur = conn.cursor()
	data = {}
	for row in cur.execute('select width, height from objects'):
		width = row[0]
		height = row[1]
		size = width*height
		if size not in data:
			data[size] =  1
		else:
			data[size] += 1

	plt.bar(data.keys(), data.values())
#	plt.axes().set_xscale('log')
	plt.show()


if __name__ == "__main__":
	import sys
	exit(main(sys.argv))

