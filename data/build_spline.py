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

y = [0, 1, 4, -1]
n = len(y) - 1 # uses past n + 1 samples

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
# Natural boundary conditions
#c_0 = 0
A[4*n - 2][2] = 1
# c_n + 3*d_n = 0
A[4*n - 1][4*n - 2] = 1
A[4*n - 1][4*n - 1] = 3

Ry = np.zeros((4*n, n+1))
for s in range(n):
    Ry[2*s][s] = 1
    Ry[2*s + 1][s+1] = 1

A = np.matrix(A)
Ry = np.matrix(Ry)
y = np.matrix(y)

x = A**-1 * Ry * y.T
x = x.T.tolist()[0]
print("this is x")
print(x)

def spline_it(i):
    s = int(i)
    s = min(n-1, s)
    rem = i - s
    print(i, s, rem)
    return float(x[s*4] + x[s*4+1]*rem + x[s*4+2]*rem**2 + x[s*4+3] *rem**3)
# c_c = a_i + b_i*x + c_i*x**2 + d_i*x**3
t = np.arange(0, 6, .1)
my = list(map(spline_it, t))
print(x)
print(t)
print(my)

y = y.tolist()[0]

plt.plot(y, marker="o")
plt.plot(t, my)

plt.show()
