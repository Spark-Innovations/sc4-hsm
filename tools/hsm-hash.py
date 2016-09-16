#!/usr/bin/env python

import fcntl
import glob
import select
import os
import sys
import hashlib
from time import sleep

if bytes == str:  # python2
  iseq = lambda s: map(ord, s)
  bseq = lambda s: ''.join(map(chr, s))
  buffer = lambda s: s
else:  # python3
  iseq = lambda s: s
  bseq = bytes
  buffer = lambda s: s.buffer
  pass

b58_alphabet = '123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz'

def b58encode(v):
  origlen = len(v)
  v = v.lstrip(b'\0')
  newlen = len(v)
  p, acc = 1, 0
  for c in iseq(v[::-1]):
    acc += p * c
    p = p << 8
    pass
  result = ''
  while acc > 0:
    acc, mod = divmod(acc, 58)
    result += b58_alphabet[mod]
    pass
  return (result + b58_alphabet[0] * (origlen - newlen))[::-1]

def b58decode(v):
  if not isinstance(v, str): v = v.decode('ascii')
  origlen = len(v)
  v = v.lstrip(b58_alphabet[0])
  newlen = len(v)
  p, acc = 1, 0
  for c in v[::-1]:
    acc += p * b58_alphabet.index(c)
    p *= 58
    pass
  result = []
  while acc > 0:
    acc, mod = divmod(acc, 256)
    result.append(mod)
    pass
  return (bseq(result) + b'\0' * (origlen - newlen))[::-1]

def slices(s, n):
  for i in range(len(s)/n): yield(s[i*n: (i+1)*n])
  i = len(s)%n
  if (i>0): yield(s[-i:])

def unhex(hex):
  return ''.join(chr(int(x, 16)) for x in slices(hex, 2))

def hash(path):

  with open(sys.argv[1], 'r') as f: s = f.read()

  hsm_path = glob.glob("/dev/cu.usbmodem*") or glob.glob("/dev/ttyACM*")
  if not hsm_path:
    print "HSM not found"
    exit(-1)
    return
  
  with open(hsm_path[0], 'r+') as hsm:
    hsm_fd = hsm.fileno()

    hsm.write('h%s\n' % len(s))
    for slice in slices(s, 128):
      hsm.read(1)
      hsm.write(slice)
      pass

    c = hsm.read(1)
    sleep(0.1) # Just to be sure
    flag = fcntl.fcntl(hsm_fd, fcntl.F_GETFL)
    fcntl.fcntl(hsm_fd, fcntl.F_SETFL, flag | os.O_NONBLOCK)
    h1 = c + hsm.read().split('\n')[0]
    h2 = b58encode(hashlib.sha512(s).digest())
    with os.popen("shasum -a512 %s" % path) as f: s = f.read()
    h3 = b58encode(unhex(s.split(' ')[0]))
    if h1==h2 and h2==h3:
      for s in slices(h1, 21): print s
    else:
      print "Hash mismatch!"
      print "HSM:"
      print h1
      print "Hashlib:"
      print h2
      print "Shasum:"
      print h3
      pass
    pass
  return

if __name__=='__main__':
  if len(sys.argv) < 2:
    print 'Usage: %s [file]' % sys.argv[0]
    exit(-1)
  hash(sys.argv[1])
  pass
