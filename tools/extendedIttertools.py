#!/usr/bin/env python3
from itertools import product
from fractions import Fraction
def gridKey(a):
	at=0
	for i in a:
		at+=smallest_denominator(i)
	return at
def smallest_denominator(decimal):
    fraction = Fraction(decimal).limit_denominator()
    return fraction.denominator
def interleave_iterators(a, b):
	for i in range(len(a) + 1):
		yield from zip(a[:i] + b[:i], a[i:] + b[i:])
        

def main():
	for i in [.25,.75,.5,0,1]:
		print(smallest_denominator(i))
	#for i in sorted(product([1/8,3/8,5/8,7/8,1/4,3/4,1/2,0,1],repeat=4),reverse=False,key=gridKey):
	#	print(i)
	for i in sorted(product([1/8,3/8,5/8,7/8,1/4,3/4,1/2,0,1],repeat=4),reverse=False,key=gridKey):
		print(i, end=' ')
		print(gridKey(i))
	
	
if __name__ == '__main__':
	try:
		main()
	except KeyboardInterrupt:
		pass
