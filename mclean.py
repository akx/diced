import argparse


def clean(data, start, end):
	try:
		istart = data.index(start)
		iend = data.index(end, istart)
	except ValueError:
		print('Not found', start, end)
		return data

	dl = len(data)
	data = data[:istart] + b'\x00' * (iend - istart) + data[iend:]
	assert len(data) == dl
	print('Fixed', start, end, (iend - istart))
	return data
	

ap = argparse.ArgumentParser()
ap.add_argument('file')
args = ap.parse_args()

with open(args.file, 'rb') as infp:
	data = infp.read()

data = clean(data, b'Illegal byte sequence', b'\x00' * 32)

with open(args.file, 'wb') as outfp:
	outfp.write(data)