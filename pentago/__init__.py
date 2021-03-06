from __future__ import absolute_import

from . import pentago_core
from .pentago_core import *
from numpy import asarray,fromstring,int64
import glob
import gzip
import os
import re

def large(n):
  s = str(n)
  return ''.join((',' if i and i%3==0 else '')+c for i,c in enumerate(reversed(s)))[::-1]

def report_thread_times(times,name=''):
  return pentago_core.report_thread_times(times,name)

def open_supertensors(path,io=IO):
  return open_supertensors_py(path,io)

def factorial(n):
  assert n>=0
  f = 1
  for i in xrange(2,n+1):
    f *= i
  return f

def binom(n,*k):
  k = asarray(k)
  sk = k.sum()
  if k.min()<0 or sk>n: return 0
  f = factorial(n)//factorial(n-sk)
  for i in k:
    f //= factorial(i)
  return f

def flip_board(board,turn=1):
  return pentago_core.flip_board_py(board,turn)

def rotate(board,*args):
  '''Usage: either rotate(board,qx,qy,count) or rotate(board,tuple)'''
  if len(args)==1:
    for x in 0,1:
      for y in 0,1:
        board = rotate(board,x,y,args[0][2*x+y])
    return board
  else:
    qx,qy,count = args
    count = (count%4+4)%4
    table = to_table(board)
    for i in xrange(count):
      copy = table.copy()
      for x in xrange(3):
        for y in xrange(3):
          copy[3*qx+x,3*qy+y] = table[3*qx+y,3*qy+2-x]
      table = copy
    return from_table(table)

def show_board(board,brief=False):
  if brief:
    return str(board)
  else:
    table = to_table(board)
    return '\n'.join('abcdef'[i]+'  '+''.join('_01'[table[j,5-i]] for j in xrange(6)) for i in xrange(6)) + '\n\n   123456'

def show_side(side):
  return '\n'.join('abcdef'[5-y]+'  '+''.join('_0'[bool(side&1<<16*(x//3*2+y//3)+x%3*3+y%3)] for x in xrange(6)) for y in reversed(xrange(6))) + '\n\n   123456'

def read_rank_history(path):
  '''Read history data for one rank'''
  if 'history-r' not in os.path.basename(path):
    raise RuntimeError('weird history file: %s'%path)
  _,extension = os.path.splitext(path)
  if not extension:
    data = fromfile(path,dtype=int64)
  elif extension == '.gz':
    data = fromstring(gzip.open(path).read(),dtype=int64)
  else:
    raise RuntimeError('unknown extension %s'%extension)
  data = data.reshape(-1,3)
  if not (data[0][0]&0xffffffff): # Check for bad endianness
    data = data.view(data.dtype.newbyteorder('<')).astype(data.dtype.newbyteorder('>')).view(int64)
  threads,kinds,_ = data[0]

  # Unpack thread and kind history
  rank_history = []
  for thread in xrange(threads):
    thread_history = []
    for kind in xrange(kinds):
      lo,hi,_ = data[1+thread*kinds+kind]
      thread_history.append(data[lo:hi])
    rank_history.append(thread_history)
  return rank_history

def read_all_history(dir,max_ranks=1<<30):
  '''Read the history for all ranks in a job'''
  names = glob.glob(dir+'/history-r*')
  ranks = [None]*min(len(names),max_ranks)
  assert ranks
  for name in names:
    m = re.search(r'history-r(\d+)((?:\.gz)?)$',name)
    if not m:
      raise RuntimeError('weird file: '+name)
    rank = int(m.group(1))
    if rank < len(ranks):
      ranks[rank] = read_rank_history(name)
  return [thread for rank in ranks for thread in rank],len(ranks)
