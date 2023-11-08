#! /usr/bin/python

import os, sys, string

if sys.stdout.isatty():
	OKBLUE = '\033[94m'
	OKGREEN = '\033[92m'
	WARNING = '\033[93m'
	FAIL = '\033[91m'
	ENDC = '\033[0m'
	BOLD = "\033[1m"
else:
	OKBLUE = ''
	OKGREEN = ''
	WARNING = ''
	FAIL = ''
	ENDC = ''
	BOLD = ''


def hex_conv(s):
	return int(s, 16)

def parse_objdump(objdump_cmd):
	syms = {}
	for line in os.popen(objdump_cmd):
		fields = line.split()
		if len(fields) == 6:
			if fields[3] == fields[5]:
				start = hex_conv(fields[0])
			else:
				symbol = fields[5].replace("class_", "")
				symbol = symbol.replace("util_", "")
				symbol = symbol.replace("tmu_", "")
				symbol = symbol.replace("mailbox", "mbox")
				syms[symbol] = (hex_conv(fields[0]), hex_conv(fields[4]))
	for sym in syms.keys():
		(addr, size) = syms[sym]
		syms[sym] = (addr-start, size)

	return syms

def one_way_compare(filename1, syms1, filename2, syms2):
	list1 = syms1.keys()
	list2 = syms2.keys()
	err = 0

	for sym in list1:
		if sym not in list2:
			print(WARNING + "ERROR: symbol " + sym + " present in " + filename1 + " but not in " +filename2 + ENDC)
			err += 1
		else:
			(addr1, size1) = syms1[sym]
			(addr2, size2) = syms2[sym]
			if addr1 != addr2:
				print(WARNING + "ERROR: symbol "+ sym + " at address " + str(addr1) + " in "  + filename1 + ", at address " + str(addr2) + " in "  + filename2 + ENDC)
				syms2.pop(sym)
				err += 1
			if size1 != size2:
				print(WARNING + "ERROR: symbol "+ sym + " has size " + str(size1) + " in "  + filename1 + ", but size " + str(size2) + " in "  + filename2 + ENDC)
				syms2.pop(sym)
				err += 1
	return err



def compare_shared_mem(ctrl_filename, pe, filename):

	print("Comparing memory for "+ pe + " shared memory...")
	sym_ctrl =  parse_objdump("objdump -j ." + pe + "_dmem_sh -t " + sys.argv[1])

	sym_pfe =  parse_objdump("objdump -j .dmem_sh -t "+ filename)

	err1 = one_way_compare(ctrl_filename, sym_ctrl, filename, sym_pfe)
	err2 = one_way_compare(filename, sym_pfe, ctrl_filename, sym_ctrl)

	if (err1 + err2) != 0:
		print(FAIL + BOLD + pe + " ERROR: " + ctrl_filename + " and " + filename + " don't match." + ENDC)
	else:
		print(OKGREEN + BOLD + pe + " OK." + ENDC)
	print("")

	return err1 + err2

# Main
if __name__=="__main__":
	if len(sys.argv)<3:
		print("Usage: " + sys.argv[0] + " <pfe.ko file> <class ELF file> <tmu ELF file> <util ELF file>")
		sys.exit(1)

	compare_list = []
	compare_list.append(("class", sys.argv[2]))	
	compare_list.append(("tmu", sys.argv[3]))	
	compare_list.append(("util", sys.argv[4]))	

	ret = 0
	for (pe, filename) in compare_list:
		ret += compare_shared_mem(sys.argv[1], pe, filename)

	exit(ret)
