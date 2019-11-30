#!/usr/bin/env python3
"""
This script computes FIR filter coefficients for
computing natural cubic spline based extrapolation.

Filter coefficients assume that the past n+1 samples
and the extrapolation are equidistantly spaced.

Coefficients are output in a 16.16 fixed point format
so that they can be used in embedded compression
applications.
"""
import matplotlib.pyplot as plt
import numpy as np
import os


TESTING = True
figure = 0

# Boundary conditions
conditions = [NATURAL, END_SLOPE_0, PERIODIC, CONSERVATIVE, NOT_A_KNOT] = range(5)
boundary_condition = NATURAL

for boundary_condition in conditions:
    print("condition {}:".format(boundary_condition))
    for n in range(2,10): # uses the past n+1 samples
        while boundary_condition == NOT_A_KNOT and n < 4:
            n +=1
        dir_name = 'natural_spline_pred/condition{}'.format(boundary_condition)
        try:
            os.makedirs(dir_name)
        except OSError:
            pass
        # Build our system of linear equations based on spline constraints
        # Ax = b where x is vector of spline coefficients
        A = np.zeros((4*n, 4*n), np.double)

        # These computations can probably be done with n less rows/columns in this
        # matrix. Not really worth my time to optimize it.
        for s in range(n):
            # Beginning equality constraint
            A[2*s][4*s] = 1
            # Ending equality constraint
            A[2*s+1][4*s] = 1
            A[2*s+1][4*s + 1] = 1
            A[2*s+1][4*s + 2] = 1
            A[2*s+1][4*s + 3] = 1
        for s in range(n-1):
            # First-order continuity constraint
            # b_i + 2*c_i + 3*d_i - b_(i-1) = 0
            A[2*n + s][4*s + 1] = 1
            A[2*n + s][4*s + 2] = 2
            A[2*n + s][4*s + 3] = 3
            A[2*n + s][4*s + 5] = -1
            # Second-order continuity constraint
            A[3*n - 1 + s][4*s + 2] = 2
            A[3*n - 1 + s][4*s + 3] = 6
            A[3*n - 1 + s][4*s + 6] = -2
        if boundary_condition == NATURAL:
            #c_0 = 0
            A[-2][2] = 1
            # c_n + 3*d_n = 0
            A[-1][-2] = 1
            A[-1][-1] = 3
        elif boundary_condition == END_SLOPE_0: # AKA clamped
            A[-2][1] = 1
            A[-1][-3] = 1
            A[-1][-2] = 2
            A[-1][-1] = 3
        elif boundary_condition == PERIODIC:
            A[-2][1] = 1
            A[-2][-3] = -1
            A[-2][-2] = -2
            A[-2][-1] = -3
            A[-1][2] = 1
            A[-1][-2] = -1
            A[-1][-1] = -3
        elif boundary_condition == NOT_A_KNOT:
            A[-2][3] = 1
            A[-2][7] = -1
            A[-1][-1] = 1
            A[-1][-5] = -1
        elif boundary_condition == CONSERVATIVE:
            A[-2][-2] = 1
            A[-2][-1] = 3
            A[-1][-3] = 1
            A[-1][-2] = 2
            A[-1][-1] = 3
        else:
            print("ERROR: boundary condition not yet supported")

        # rearranges our matrix so that it can be applied as an FIR
        Ry = np.zeros((4*n, n+1))
        for s in range(n):
            Ry[2*s][s] = 1
            Ry[2*s + 1][s+1] = 1

        # This vector applies our spline parameters to point n+1
        rx = np.zeros(4*n)
        rx[-4] = 1
        rx[-3] = 2
        rx[-2] = 4
        rx[-1] = 8

        A = np.matrix(A)
        Ry = np.matrix(Ry)
        rx = np.matrix(rx)

        fir = rx * A**-1 * Ry
        data_tools.save_array(fir, os.path.join(dir_name, str(n)))

        if TESTING:
            # Plot the whole spline to confirm that it has desired behavior
            y = [0, 1, 4, -1, 2, 2, 9, 7.1] * 3 # test points to fit a spline to
            y = y[-(n+1)::]
            y = np.matrix(y).T

            # Compute all spline parameters
            x = A**-1 * Ry * y
            x = x.T.tolist()[0]

            pred = fir * y
            print(pred)
    
            def spline_it(i):
                """Create one smoothed point using spline parameters"""
                s = int(i)
                s = min(n-1, s)
                rem = i - s
                # c_c = a_i + b_i*x + c_i*x**2 + d_i*x**3
                return float(x[s*4] + x[s*4+1]*rem + x[s*4+2]*rem**2 + x[s*4+3] *rem**3)
            t = np.arange(0, n + 3, .1)
            my = list(map(spline_it, t))

            #y = y.tolist()[0]

            figure += 1
            #plt.figure(figure)
            plt.title('hist {}, cond {}'.format(n, boundary_condition))
            plt.plot(y, marker="o")
            plt.plot(t, my)
            plt.plot(n+1, pred, marker="o")

#plt.show()
