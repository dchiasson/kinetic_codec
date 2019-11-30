#!/usr/bin/env python3

import os, sys
sys.path.append(os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'data'))
import data_tools

import pandas
#import tensorflow as tf
import numpy as np
import matplotlib.pyplot as plt
from matplotlib import patches
from sklearn.linear_model import LinearRegression, Lasso, Ridge

data_dir = '../../HuGaDB/HuGaDB/Data.parsed/'

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


HISTORY = 20
def build_features(file_name):
    file_name='HuGaDB_v1_walking_14_02.txt'
    ACCEL_FIX = 2.0/32765
    GYRO_FIX = 2000/32768
    start = 12
    cols = range(start, start+6)

    dataset = pandas.read_csv(file_name, sep='\t')
    data_length = np.shape(dataset)[0]
    data_mat = dataset.as_matrix()

    features, labels = [], []

    for start in range(0, 36, 6):
        cols = range(start, start+6)
        for time in range(HISTORY, data_length):
            example = []
            for col in cols:
                example.extend(data_mat[:,col][time-HISTORY:time])
            features.append(example)
            labels.append(data_mat[:,start][time]) # only train x_accel for now
    return np.asarray(features), np.asarray(labels)

if __name__ == '__main__':

    X, y = None, None
    iteration = 0
    for data_file in os.listdir(data_dir):
        iteration +=1
        if iteration < 20:
            continue
        if iteration > 21:
            break
        print("loading: {}".format(data_file))
        if X is not None:
            Xnew, ynew = build_features(data_file)
            X = np.concatenate((X, Xnew))
            y = np.concatenate((y, ynew))
        else:
            X, y = build_features(data_file)

    #reg = LinearRegression().fit(X,y)
    reg = Lasso(alpha=.1, max_iter=2000).fit(X,y)
    print(reg.score(X,y))
    reg.coef_[np.abs(reg.coef_) < (1.0/16)] = 0
    print(reg.coef_)
    print(np.linalg.norm(reg.coef_, ord=1))
    print(np.linalg.norm(reg.coef_, ord=2))

    dir_name = 'cross_fir'
    try:
        os.makedirs(dir_name)
    except OSError:
        pass
    data_tools.save_array(reg.coef_, os.path.join(dir_name, 'test_coefs'))

    # plot the poles and zeros!
    for var in range(6):
        b_vec = np.flip(reg.coef_[var*HISTORY:var*HISTORY+HISTORY])
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
