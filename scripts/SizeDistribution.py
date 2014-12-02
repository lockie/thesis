#!/usr/bin/env python

import os
import sqlite3
import matplotlib.pyplot as plt


def main(argv):
	import argparse
	parser = argparse.ArgumentParser()
	parser.add_argument('path', help='path to dataset folder',
		default='.', nargs='?', metavar='path')
	parser.add_argument('-p', '--predicate', help='SQL predicate for objects to plot',
		type=str)
	plot_group = parser.add_mutually_exclusive_group()
	plot_group.add_argument('-W', '--width', help='only plot width',
		action='store_true')
	plot_group.add_argument('-H', '--height', help='only plot height',
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
	query = 'select width, height from objects'
	if args.predicate is not None:
		query += ' where ' + args.predicate
	for row in cur.execute(query):
		width  = row[0]
		height = row[1]
		if args.width:
			size = width
		elif args.height:
			size = height
		else:
			size = width * height

		if size not in data:
			data[size] =  1
		else:
			data[size] += 1

	plt.bar(data.keys(), data.values())
#	plt.axes().set_xscale('log')
	plt.show()


if __name__ == '__main__':
	import sys
	exit(main(sys.argv))

