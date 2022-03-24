#!/usr/bin/env python3
"""
Precompute FIR filter coefficents from computing polynomial regression based prediction

This vector will be saved in binary format and can be directly read into a c program.
"""
import os
import numpy as np
import data_tools

fixed_point = True

for m in range(0, 7): # polynomial order
    if fixed_point:
        dir_name = 'fixed_poly_pred/{}_deg_poly_reg'.format(m)
    else:
        dir_name = 'poly_pred/{}_deg_poly_reg'.format(m)

    try:
        os.makedirs(dir_name)
    except OSError:
        pass
    for n in list(range(m, 20)) + [2**i for i in range(5, 10)]: # FIR filter length
    #for n in range(m, 20):
        print("computing m={}, n={}".format(m, n))
        x = np.matrix([[b**a for a in range(m)] for b in range(n)], np.double)
        n_p1 = np.matrix([(n) ** a for a in range(m)], np.double)
        factor = n_p1 *((x.T * x)**-1)*x.T
        data_tools.save_array(factor, os.path.join(dir_name, str(n)), fixed_point)
