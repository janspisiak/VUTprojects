#!/usr/bin/env python3

#SYN:xspisi03

import sys
import argparse
import collections
import re

class AppError(Exception):
	def process():
		True;

def parseArgs():
	parser = argparse.ArgumentParser(description = 'Regex tag highlighter. Author: xspisi03@stud.fit.vutbr.cz', add_help = False)
	parser.add_argument('-h','--help', action = 'store_true', help = 'Show help message')
	parser.add_argument('--input', dest = "inFile", action = "store", help = 'Input file, default is stdin')
	parser.add_argument('--output', dest = "outFile", action = "store", help = 'Output file, default is stdout')
	parser.add_argument('--format', dest = "formatFile", action = "store", help = 'Format specification file')
	parser.add_argument('--br', dest = "br", action = "store_true", help = 'Replace newline with <br />')

	try:
		args = parser.parse_args()
	except:
		raise AppError("Unrecognised argument", 1)
	
	# check if some arg doesn't occur twice
	keys = {}
	for arg in sys.argv:
		key = arg.partition('=')[0]
		if key in keys:
			raise AppError("Argument \"" + key + "\" redefined", 1)
		else:
			keys[key] = 1;

	# resolve --help
	if args.help:
		if len(sys.argv) == 2:
			parser.print_help()
			sys.exit(0)
		else:
			raise AppError("Can't combine --help with other arguments.", 1)

	return args

formatOptions = {
	"bold":		["b"],
	"italic":	["i"],
	"underline":["u"],
	"teletype":	["tt"],
	"size":		["font size=", r"^[1-7]$"],
	"color":	["font color=#", r"^[0-9a-fA-F]{6}$"],
}

def formatToTag(format):
	key, data = (format.split(":", 1) + [""])[:2]
	if not key in formatOptions:
		raise AppError("Unknown formatting option: " + key, 2)
	tag = formatOptions[key][0]
	# if tag needs addtional data then parse it
	if len(formatOptions[key]) > 1:
		if not re.match(formatOptions[key][1], data):
			raise AppError("Data doesn't match " + formatOptions[key][1] + ": " + data, 2)
		tag += data

	return tag

negationTable = {
	"s": "\S",
	"a": "^$",
	"d": "\D",
	"l": "[^a-z]",
	"L": "[^A-Z]",
	"w": "[^a-zA-Z]",
	"W": "[^a-zA-Z0-9]",
	"t": "[^\t]",
	"n": "[^\n]",
}

escapeTable = {
	"s": "\s",
	"a": ".",
	"d": "\d",
	"l": "[a-z]",
	"L": "[A-Z]",
	"w": "[a-zA-Z]",
	"W": "[a-zA-Z0-9]",
	"t": "\t",
	"n": "\n",
}

pythonEscape = ['[', ']', '^', '$', '{', '}', '?', '\\']

def parseRe(re):
	out = ""
	state = "expr"
	quant = ''
	# Convert state machine
	for index, char in enumerate(re):
		# flush the quantificator
		if quant and not char in ['+', '*']:
			out += quant
			quant = ''

		if state == "expr":
			if char in ['.', '|', '+', '*', ')']:
				raise AppError("Expecting expression at index " + str(index) + " in: " + re[min(index - 2, 0): max(index + 2, len(re))], 4)
			elif char == '%':
				state = "escape"
			elif char == '!':
				state = "negation"
			elif char == '(':
				out += char
			elif char in pythonEscape:
				out += '\\' + char
				state = "any"
			elif ord(char) >= 32:
				out += char
				state = "any"
		elif state == "any":
			if char == '+' and not quant:
				quant = '+'
			elif char == '*':
				quant = '*'
			elif char == '%':
				state = "escape"
			elif char == '!':
				state = "negation"
			elif char in ['(', '|']:
				out += char
				state = "expr"
			elif char == '.':
				state = "expr"
			elif char in pythonEscape:
				out += '\\' + char
			elif ord(char) >= 32:
				out += char
		elif state == "negation":
			if char == '%':
				state = "negationEscape"
			elif char in ['.', '|', '!', '*', '+', '(', ')']:
				raise AppError("Invalid negation at index " + str(index) + " in: " + re[min(index - 2, 0): max(index + 2, len(re))], 4)
			elif char in pythonEscape:
				out += "[^\\" + char + "]"
			elif ord(char) >= 32:
				out += "[^" + char + "]"
				state = "any"
			else:
				raise AppError("Invalid negation at index " + str(index) + " in: " + re[min(index - 2, 0): max(index + 2, len(re))], 4)
		elif state == "negationEscape":
			if char in negationTable:
				out += negationTable[char]
				state = "any"
			elif char in ['.', '|', '!', '*', '+', '(', ')', '%']:
				out += "[^\\" + char + "]"
				state = "any"
			else:
				raise AppError("Invalid negation at index " + str(index) + " in: " + re[min(index - 2, 0): max(index + 2, len(re))], 4)
		elif state == "escape":
			if char in escapeTable:
				out += escapeTable[char]
				state = "any"
			elif char in ['.', '|', '!', '*', '+', '(', ')', '%']:
				out += '\\' + char
				state = "any"
			else:
				raise AppError("Invalid escape at index " + str(index) + " in: " + re[min(index - 2, 0): max(index + 2, len(re))], 4)
	if state != "any":
		raise AppError("Unexpected end in regex: " + re, 4)

	# flush the quantificator
	if quant:
		out += quant

	return out

def main():
	try:
		params = parseArgs()

		if params.inFile:
			try:
				with open(params.inFile, 'r') as iH:
					source = iH.read()
			except EnvironmentError as e:
				raise AppError("Couldn't open input file " + e.filename + ": " + e.strerror, 2)
		else:
			source = sys.stdin.read()
		
		formatting = []
		if params.formatFile:
			try:
				with open(params.formatFile) as fH:
					for line in fH:
						key, tags = re.split(r"\t+", line, 1)

						tags = re.split(r"\s*,\s*", tags.strip())
						# parse each format option
						tags = [formatToTag(tag) for tag in tags]

						# try to convert IFJ RE to Python RE
						try:
							key = parseRe(key);
							re.compile(key)

							formatting.append((key, ("".join(["<" + tag + ">" for tag in tags]), "".join(["</" + tag.split(' ', 1)[0] + ">" for tag in reversed(tags)]))))
						except re.error:
							raise AppError("Invalid regex: " + key, 4)
			except EnvironmentError as e:
				# not a fatal error
				print("W: Something wrong with format file: " + e, file = sys.stderr)

		positions = collections.defaultdict(lambda: [[], []])
		# find matches and fill positions structure
		for p, (reg, tags) in enumerate(formatting):
			for match in re.finditer(reg, source, re.DOTALL):
				# ignore empty matches
				if match.end() - match.start() == 0:
					continue
				# Add end tags
				positions[match.end()][0] = [p] + positions[match.end()][0]
				# Add open tags
				positions[match.start()][1] += [p]

		# merge positions and source into result
		if len(positions) > 0:
			result = ""
			lastPos = 0
			for i, pos in enumerate(sorted(positions)):
				result += source[lastPos: pos]
				result += "".join([formatting[i][1][1] for i in positions[pos][0]] + [formatting[i][1][0] for i in positions[pos][1]])
				lastPos = pos
			result += source[pos:]
		else:
			result = source

		# if specified add br tags
		if params.br:
			result = result.replace("\n", "<br />\n")

		# write the result
		if params.outFile:
			try:
				with open(params.outFile, 'w') as oH:
					oH.write(result)
			except EnvironmentError as e:
				raise AppError("Couldn't open output file " + e.filename + ": " + e.strerror, 3)
		else:
			sys.stdout.write(result)
	except AppError as e:
		print("E: " + str(e.args[0]), file=sys.stderr)
		if len(e.args) > 1:
			sys.exit(e.args[1])

if __name__ == "__main__":
	main()