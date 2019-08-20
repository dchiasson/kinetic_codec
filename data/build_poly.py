#!/usr/bin/env python3
"""
Precompute matrix from finding optimal polynomial regression coefficients

This matrix will be saved in binary format and can be directly read into a c program.
"""
import os
import numpy as np

fixed_point = True

for m in range(1, 7): # polynomial order
    if fixed_point:
        dir_name = 'fixed_poly_pred/{}_deg_poly_reg'.format(m)
    else:
        dir_name = 'poly_pred/{}_deg_poly_reg'.format(m)
    try:
        os.makedirs(dir_name)
    except OSError:
        pass
    #for n in list(range(m, 100)) + [2**i for i in range(4, 15)]:
    for n in range(m, 7):
        print("computing m={}, n={}".format(m, n))
        x = np.matrix([[b**a for a in range(m)] for b in range(n)], np.double)
        n_p1 = np.matrix([(n) ** a for a in range(m)], np.double)
        factor = n_p1 *((x.T * x)**-1)*x.T
        if fixed_point:
            mmap_file = np.memmap(os.path.join(dir_name, str(n)), dtype=np.int32, mode='w+', shape=np.shape(factor))
            # conversion to libfixmath fix16_t type
            # see `fix16_t_from_float` in fix16.h
            factor = (factor * 0x10000)
            factor = np.multiply((factor >= 0), factor + 0.5) + np.multiply((factor < 0), factor - 0.5)
            factor = factor.astype(np.int32)
        else:
            mmap_file = np.memmap(os.path.join(dir_name, str(n)), dtype='f4', mode='w+', shape=np.shape(factor))
        mmap_file[:] = factor
        mmap_file.flush()
