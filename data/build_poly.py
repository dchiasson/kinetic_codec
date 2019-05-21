#!/usr/bin/env python3
"""
Precompute matrix from finding optimal polynomial regression coefficients

This matrix will be saved in binary format and can be directly read into a c program.
"""
import os
import numpy as np

for m in range(1, 7): # polynomial order
    dir_name = 'poly_reg/{}_deg_poly_reg'.format(m)
    try:
        os.makedirs(dir_name)
    except OSError:
        pass
    for i in range(3,15):
        n = 2**i
        x = np.matrix([[b**a for a in range(m)] for b in range(n)])
        factor = (x.T * x)**-1*x.T
        mmap_file = np.memmap(os.path.join(dir_name, str(n)), dtype='f4', mode='w+', shape=np.shape(factor))
        mmap_file[:] = factor
        mmap_file.flush()
