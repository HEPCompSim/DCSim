#! /usr/bin/python3
import argparse

parser = argparse.ArgumentParser(description='Grid Search.')
parser.add_argument('-r', '--reference', type=str, help='Reference values file path', required=True)

parser.add_argument('-s', '--speed', nargs=3, help='compute speed option with 3 values range',metavar=('min', 'max', 'inciment'), required=True)
parser.add_argument('-rb', '--read-bandwidth', nargs=3, help='host read bandwidth range',metavar=('min', 'max', 'inciment'), required=True)
parser.add_argument('-wb', '--write-bandwidth', nargs=3, help='host write bandwidth range',metavar=('min', 'max', 'inciment'), required=True)
parser.add_argument('-ilb', '--internal-link-bandwidth', nargs=3, help='internal link bandwidth range',metavar=('min', 'max', 'inciment'), required=True)
parser.add_argument('-ill', '--internal-link-latency', nargs=3, help='internal link latency range',metavar=('min', 'max', 'inciment'), required=True)
parser.add_argument('-erb', '--external-read-bandwidth', nargs=3, help='external read bandwidth range',metavar=('min', 'max', 'inciment'), required=True)
parser.add_argument('-el', '--external-latency', nargs=3, help='external latency option range',metavar=('min', 'max', 'inciment'), required=True)
parser.add_argument('-rd', '--remote-bandwidth', nargs=3, help='remote bandwidth option range',metavar=('min', 'max', 'inciment'), required=True)


args = parser.parse_args()

print(args)
