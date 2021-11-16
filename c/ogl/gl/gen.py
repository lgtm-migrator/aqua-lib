import os
import requests

GL_XML_URL = "https://raw.githubusercontent.com/KhronosGroup/OpenGL-Registry/main/xml/gl.xml"
KHRPLATFORM_URL = "https://raw.githubusercontent.com/KhronosGroup/EGL-Registry/main/api/KHR/khrplatform.h"

# download 'khrplatform.h' if it doesn't exist

if not os.path.exists("khrplatform.h"):
	with open("khrplatform.h", "w") as f:
		f.write(requests.get(KHRPLATFORM_URL).text)

# attempt to open cached version of 'gl.xml'
# if it doesn't exist, download and cache it

xml = None

try:
	f = open("gl.xml")
	xml = f.read()
	f.close()

except FileNotFoundError:
	with open("gl.xml", "w") as f:
		xml = requests.get(GL_XML_URL).text
		f.write(xml)

# parse XML

import xml.etree.ElementTree as libxml
registry = libxml.ElementTree(libxml.fromstring(xml)).getroot()

header = """
// this file is automatically generated by 'c/ogl/gl/gen.py' from the Khronos Group OpenGL Registry
// if you need to update this (you probably won't ever), delete 'gl.xml' and run 'gen.py'

#if !defined(__AQUA_LIB__OGL_GL_GL)
#define __AQUA_LIB__OGL_GL_GL

#include "khrplatform.h"
"""

# utilities

def to_str(x):
	return "" if x is None else x

def comment(tag):
	global header

	if "comment" in tag.attrib:
		header += " // " + tag.attrib["comment"]

	header += '\n'

def enum_type(group):
	return f"GL_{group}_t"

# actual parsing functions

def parse_types(types):
	global header

	for _type in types:
		if _type.tag != "type":
			print(f"WARNING Non-type {_type.tag} in types")
			continue

		if _type.text:
			if "#include" in _type.text:
				continue

			header += _type.text

		for bit in _type:
			header += to_str(bit.text) + to_str(bit.tail)

		comment(_type)

known_groups = []

def parse_enums(enums):
	global header, known_groups

	count = 0

	for enum in enums:
		if enum.tag == "unused":
			return

		count += enum.tag == "enum"

	if not count:
		header += "/* empty enum */"
		comment(enums)

		return

	if "group" in enums.attrib:
		header += "typedef "

	header += "enum {\n"

	for enum in enums:
		if enum.tag != "enum":
			print(f"WARNING Non-enum {enum.tag} in enums")
			continue

		name = enum.attrib["name"]
		value = enum.attrib["value"]

		header += f"\t{name} = {value},"
		comment(enum)

	if "group" in enums.attrib:
		group = enums.attrib["group"]

		known_groups.append(group)
		header += f"}} {enum_type(group)};"

	else:
		header += "};"

	comment(enums)

def parse_funcs(funcs):
	global header, known_groups

	header += "typedef struct {\n"

	for func in funcs:
		if func.tag != "command":
			print(f"WARNING Non-command {func.tag} in commands")
			continue

		proto = func[0]

		if proto.tag != "proto":
			print(f"WARNING First child tag of command {proto.tag} not proto")
			continue

		header += "\t" + to_str(proto.text)

		for bit in proto:
			if bit.tag == "name":
				name = bit.text

				if name[:2] == "gl":
					name = name[2:]

				header += f"(*{name}) ("
				break

			header += to_str(bit.text) + to_str(bit.tail)

		first = True

		for param in func[1:]:
			if param.tag != "param":
				continue

			if not first:
				header += ", "

			first = False

			header += to_str(param.text)

			for bit in param:
				text = to_str(bit.text)

				if bit.tag == "ptype" and text == "GLenum" and "group" in param.attrib and param.attrib["group"] in known_groups:
					text = enum_type(param.attrib["group"])

				header += text + to_str(bit.tail)

		header += ");\n"

	header += "} gl_funcs_t;\n"

for tag in registry:
	if tag.tag == "comment":
		print(tag.text)

	elif tag.tag in ("feature", "extensions"):
		pass # ignore these

	elif tag.tag == "types":
		parse_types(tag)

	elif tag.tag == "enums":
		parse_enums(tag)

	elif tag.tag == "commands":
		parse_funcs(tag)

	else:
		print(f"WARNING Unrecognized tag {tag.tag}")

# write to output file

header += """
#endif
"""

with open("gl.h", "w") as f:
	f.write(header)
