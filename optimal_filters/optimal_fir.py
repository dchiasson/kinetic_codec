#!/usr/bin/env python3

import os, sys
sys.path.append(os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'data'))
import data_tools

import pandas
#import tensorflow as tf
import numpy as np
import matplotlib.pyplot as plt
from matplotlib import patches
from sklearn.linear_model import LinearRegression, Lasso, Ridge, LogisticRegression
import cvxpy as cp

data_dir = '../../HuGaDB/HuGaDB/Data.parsed/'

techniques = [CVXPY, SK_RIDGE, SK_LASSO, SK_LINEAR] = range(4)

def plot_zplane(zeros, k, ax):
    if len(zeros) == 0:
        return
    uc = patches.Circle((0,0), radius=1, fill=False, color='black', ls='dashed')
    ax.add_patch(uc)
    p = plt.plot(zeros.real, zeros.imag, 'bo', ms=10)
    plt.setp( p, markersize=12.0, markeredgewidth=3.0,
              markeredgecolor='b', markerfacecolor='b')
    ax.spines['left'].set_position('center')
    ax.spines['bottom'].set_position('center')
    ax.spines['right'].set_visible(False)
    ax.spines['top'].set_visible(False)
    r = max(1.5, 1.3*max(abs(zeros))); plt.axis('scaled'); plt.axis([-r, r, -r, r])
    ticks = [-1, -.5, .5, 1]; plt.xticks(ticks); plt.yticks(ticks)
    ax.title.set_text("z={}, K={}".format(sum(zeros != 0.0), k))
            
def compute_zeros(b_vec):
    """Compute the zeros of the FIR filter given the coefficents b_vec
    y[n] = \summation b_k*x[n-k] 
    """

    if (b_vec==0.0).all():
        return np.array([]), 0
    zeros = np.roots(b_vec)
    poly = np.poly(zeros)
    print("computed zeros:")
    print(zeros)
    return zeros, b_vec[0]/poly[0]


FILES = [ACC_X, ACC_Y, ACC_Z, GYRO_X, GYRO_Y, GYRO_Z] = ['acc_x','acc_y','acc_z','gyro_x','gyro_y','gyro_z']
def build_features(dir_name, axis, history, is_cross=False):

    data_mat = []
    for data_axis in FILES:
        data_mat.append(np.memmap(os.path.join(dir_name, data_axis), dtype=np.int16, mode='r'))
    data_length = len(data_mat[0])
    features, labels = [], []

    #for start in range(0, 36, 6):
    col_set = [axis]
    #if axis < 3:
    #    col_set = [0,1,2]
    #else:
    #    col_set = [3,4,5]
    for start in col_set:
        if is_cross:
            cols = range(6)
        else:
            cols = [start]
        for time in range(history, data_length):
            example = []
            for col in cols:
                example.extend(data_mat[col][time-history:time])
            features.append(example)
            labels.append(data_mat[start][time])
    return np.asarray(features), np.asarray(labels)

def test():
    fil_coef = np.zeros((6,2))
    for thing in range(3):
        fil_coef[thing][1] = 1
    for thing in range(3,6):
        fil_coef[thing][1] = 1
        fil_coef[thing][0] = 0
    dir_name = 'cross_fir'
    data_tools.save_array(fil_coef, os.path.join(dir_name, 'newbuilt_auto_hetero_0'))
    print(fil_coef)

def do_cvxpy(X, y, verbose=False, guess=None):
    b = cp.Variable(len(X[0]))
    print("len b: {}".format(len(X[0])))
    K = len(y)
    l = 10 * K
    print("L: {}".format(l))
    objective = cp.Minimize(cp.sum((cp.abs(y-X*b))) + l*(cp.sum(cp.abs(b))))
    #objective = cp.Minimize(cp.sum_squares(y - X*b))
    #objective = cp.Minimize(cp.sum((cp.abs(y-X*b))))
    constraints = []
    #constraints = [cp.sum(b) == 1]
    prob = cp.Problem(objective, constraints)
    if guess is not None:
        b.value = guess
    #result = prob.solve(verbose=verbose)
    #result = prob.solve(solver='OSQP', warm_start=True, verbose=verbose)
    result = prob.solve(solver='SCS', normalize=True, warm_start=True, verbose=verbose, eps=1e-1, max_iters=50000)
    if verbose:
        print(b.value)
    return b.value

def compute_guess(stream, history, is_cross):
    if is_cross:
        guess = np.zeros(history * 6)
        stream_guess = compute_guess(stream, history, False)
        for index, value in enumerate(stream_guess):
            guess[history*stream + index] = value
    else:
        guess = np.zeros(history)
        if stream >= 3 and history > 1:
            guess[history-1] = 1.3
            guess[history-2] = -0.3
        else:
            guess[history-1] = 1.0
    return np.asarray(guess)

def data_train(folder_list, history=3, is_cross=False, technique=CVXPY, verbose=False):
    all_coeffs = []
    #for stream in [0, 3]:
    for stream in range(6):
        X, y = None, None
        for data_dir in folder_list:
            Xi, yi = build_features(data_dir, stream, history, is_cross)
            if X is None:
                X = Xi
                y = yi
            else:
                X = np.concatenate((X,Xi))
                y = np.concatenate((y,yi))
            #if verbose:
            #    print("features extracted from {} stream {}".format(data_dir, stream))
        if verbose:
            print("features built for stream {}".format(stream))

        guess = compute_guess(stream, history, is_cross)

        if technique == CVXPY:
            b = do_cvxpy(X, y, verbose, guess)
        if technique == SK_LASSO:
            reg = Lasso(fit_intercept=False, alpha=.1, max_iter=2000000)
            reg.fit(X,y)
            b = reg.coef_
        if technique == SK_LINEAR:
            reg = LinearRegression(fit_intercept=False).fit(X,y)
            reg.fit(X,y)
            b = reg.coef_

        all_coeffs.append(b)
    all_coeffs = np.asarray(all_coeffs)

    dir_name = 'cross_fir'
    try:
        os.makedirs(dir_name)
    except OSError:
        pass
    print("FINISHED")
    print(all_coeffs)
    coef_filename = os.path.join(dir_name, 'test_coefs')
    data_tools.save_array(all_coeffs, coef_filename)

    return os.path.abspath(coef_filename)
    # plot the poles and zeros!
    for var in range(6):
        b_vec = np.flip(reg.coef_[var*history:var*history+history])
        zeros, k = compute_zeros(b_vec)
        ax = plt.subplot(2,3,var+1)
        plot_zplane(zeros, k, ax)

    #plt.plot(x,y,'o')
    plt.show()

    #how to test this?
    # training accuracy should exceed any of my models
    # testing accuracy should exceed or converge to my model

    # Do I understand how to compute poles and zeros?
    # Are there any bugs in my code?
    # How do I interpret the results?
    
    # would this algorithm learn different coefficients for each the different body parts?

if __name__ == '__main__':
    data_train()
    #test()
