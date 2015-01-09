#!/usr/bin/env python

import os
import sqlite3
import matplotlib as mpl
import matplotlib.pyplot as plt
import numpy as np


BIN_SIZE = 10  # for 2d histogram

def main(argv):
	import argparse
	parser = argparse.ArgumentParser()
	parser.add_argument('path', help='path to dataset folder',
		default='.', nargs='?', metavar='path')
	parser.add_argument('-p', '--predicate', help='SQL predicate for objects to plot',
		type=str)
	plot_group = parser.add_mutually_exclusive_group(required=True)
	plot_group.add_argument('-x', help='only plot x coordinate',
		action='store_true')
	plot_group.add_argument('-y', help='only plot y coordinate',
		action='store_true')
	plot_group.add_argument('-b', '--both', help='plot both coordinates (2D histogramm)',
		action='store_true')


	args = parser.parse_args()

	path = args.path
	filename = os.path.join(path, 'index.sqlite3')
	if not os.path.exists(filename):
		print "No dataset found in directory '" + path + "'"
		return 2

	conn = sqlite3.connect(filename)
	cur = conn.cursor()
	data = {}
	query = 'select x, y from objects'
	if args.predicate is not None:
		query += ' where ' + args.predicate
	if args.both:
		X = []
		Y = []
		for row in cur.execute(query):
			x = row[0]
			y = row[1]
			X.append(x)
			Y.append(y)
		plt.hist2d(X, Y, bins=(max(X)/BIN_SIZE, max(Y)/BIN_SIZE),
				norm=mpl.colors.SymLogNorm(1))
		plt.gca().invert_yaxis()
		plt.show()
	else:
		for row in cur.execute(query):
			x = row[0]
			y = row[1]
			if args.x:
				val = x
			elif args.y:
				val = y

			if val not in data:
				data[val] =  1
			else:
				data[val] += 1

		plt.bar(data.keys(), data.values())
#		plt.axes().set_xscale('log')
		plt.show()


if __name__ == '__main__':
	import sys
	exit(main(sys.argv))
