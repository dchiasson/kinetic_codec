#!/usr/bin/env python3

#Depends on Python 3.5+ and NumPy 1.10+
#If you want to use older libraries, replace `@` with
#matrix multiplication functions

import sys
import csv
import matplotlib.pyplot as plt
import numpy as np
import quaternion
import scipy.fftpack

#file_name='HuGaDB_v1_walking_14_02.txt'
file_name='rot_only_trial.csv'
ACCEL_FIX = 100
GYRO_FIX = 16

[a_x, a_y, a_z, g_x, g_y, g_z] = [[] for i in range(6)]
with open(file_name, 'r') as csvfile:
    reader = csv.reader(csvfile, delimiter=',')
    _ = next(reader)
    _ = next(reader)
    _ = next(reader)
    header = next(reader)
    for row in reader:
        #print(row)
        n = 0
        #a_x.append(round(float(row[0+n])*ACCEL_FIX))
        #a_y.append(round(float(row[1+n])*ACCEL_FIX))
        #a_z.append(round(float(row[2+n])*ACCEL_FIX))
        #g_x.append(round(float(row[3+n])*GYRO_FIX))
        #g_y.append(round(float(row[4+n])*GYRO_FIX))
        #g_z.append(round(float(row[5+n])*GYRO_FIX))
        a_x.append(float(row[0+n]))
        a_y.append(float(row[1+n]))
        a_z.append(float(row[2+n]))
        g_x.append(float(row[3+n]))
        g_y.append(float(row[4+n]))
        g_z.append(float(row[5+n]))
        #a_x.append(int(row[0+n]))
        #a_y.append(int(row[1+n]))
        #a_z.append(int(row[2+n]))
        #g_x.append(int(row[3+n]))
        #g_y.append(int(row[4+n]))
        #g_z.append(int(row[5+n]))
        

def get_a(n):
    return np.r_[a_x[n], a_y[n], a_z[n]]
def get_g(n):
    return np.r_[g_x[n], g_y[n], g_z[n]]
def norm(vec):
    return np.sqrt((vec**2).sum())

#def get_sequential_matrix(

#Basic right-handed primary axis rotation matricies
def r_x(theta):
    return np.r_[[[1,0,0],
            [0, np.cos(theta), -np.sin(theta)],
            [0, np.sin(theta), np.cos(theta)]]]
def r_y(theta):
    return np.r_[[[np.cos(theta), 0, np.sin(theta)],
        [0,1,0],
        [-np.sin(theta), 0, np.cos(theta)]]]
def r_z(theta):
    return np.r_[[[np.cos(theta), -np.sin(theta), 0],
        [np.sin(theta), np.cos(theta), 0],
        [0, 0, 1]]]

# using the angle axis rotation matrix is equivalent to converting
# into a quaternion and using unit quaternion rotation matrix
def get_rot_matrix_from_angle_axis(phi, v):
    """rotation radians phi and unit axis of rotation v"""
    c = np.cos(phi)
    s = np.sin(phi)
    return np.r_[[[c + v[0]**2*(1-c), v[0]*v[1]*(1-c)-v[2]*s, v[0]*v[2]*(1-c) + v[1]*s],
                   [v[0]*v[1]*(1-c)+v[2]*s, c+v[1]**2*(1-c), v[1]*v[2]*(1-c)-v[0]*s],
                   [v[0]*v[2]*(1-c)-v[1]*s, v[2]*v[1]*(1-c)+v[0]*s, c+v[2]**2*(1-c)]]]

"""radians phi around unit vector v"""
def get_quat_from_ange_axis(phi, v):
    half_phi = phi / 2.0
    sin_half_pi = np.sin(half_phi)
    return np.r_[np.cos(half_phi), v[0]*sin_half_phi, v[1]*sin_half_phi, v[2]*sin_half_phi]

"""First order linear approximation of rotation which can be used for EKF"""
def first_order_linearized_quat(phi, v):
    return np.r_[1, 0.5*v[0]*phi, 0.5*v[1]*phi, 0.5*v[2]*phi]

def get_rot_matrix_from_unit_quat(q):
    """q is a 4 element array representing a unit quaternion"""
    # https://en.wikipedia.org/wiki/Quaternions_and_spatial_rotation
    # v` = qvq^1
    # v` = Rv
    # This equation is invalid for non-unit quaternions
    return np.r_[[[1-2*(q[2]**2 + q[3]**2), 2*(q[1]*q[2]-q[3]*q[0]), 2*(q[1]*q[3] + q[2]*q[0])],
                  [2*(q[1]*q[2] + q[3]*q[0]), 1-2*(q[1]**2 + q[3]**2), 2*(q[2]*q[3] - q[1]*q[0])],
                  [2*(q[1]*q[3] - q[2]*q[0]), 2*(q[2]*q[3] + q[1]*q[0]), 1-2*(q[1]**2 + q[2]**2)]]]

def get_qaut_product(a, b):
    """Expects two length four iterables, and returns the quaternion rotation operation a*b*a^-1"""
    return


def ideal_rot(a_vec, g_vec):
    #g_vec_rad = g_vec * 2000/32768 * np.pi/180
    #g_vec_rad = g_vec/16*np.pi/180
    g_vec_rad = g_vec*np.pi/180
    T = 1.0/100 #measurement interval
    gnorm = norm(g_vec_rad)
    phi = T*gnorm
    v = g_vec_rad/gnorm
    #print(v)
    #print("rotated {} degrees/sec".format(phi*180/np.pi/T))
    R = get_rot_matrix_from_angle_axis(phi, v)
    return a_vec @ R
    #return R @ a_vec # same as inverting angle


def linearized_rot(a_vec, g_vec):
    g_vec_rad = g_vec * np.pi/180
    T = 1.0/100 #measurement interval
    gnorm = np.sqrt((g_vec_rad**2).sum())
    phi = T*gnorm
    axis = g_vec_rad/gnorm
    quat = first_order_linearized_quat(phi, axis)
    R = get_rot_matrix_from_unit_quat(quat)
    #return a_vec @ R
    return R @ a_vec

if __name__ == "__main__":
    N = len(a_x)
    T = 1.0/100.0
    xf = np.linspace(0.0, 1/(2.0*T), N/2)
    plt.figure(1)
    plt.subplot(6,1,1)
    plt.plot(np.abs(scipy.fftpack.fft(a_x)[:N//2]))
    plt.subplot(6,1,2)
    plt.plot(np.abs(scipy.fftpack.fft(a_y)[:N//2]))
    plt.subplot(6,1,3)
    plt.plot(np.abs(scipy.fftpack.fft(a_z)[:N//2]))
    plt.subplot(6,1,4)
    plt.plot(np.abs(scipy.fftpack.fft(g_x)[:N//2]))
    plt.subplot(6,1,5)
    plt.plot(np.abs(scipy.fftpack.fft(g_y)[:N//2]))
    plt.subplot(6,1,6)
    plt.plot(np.abs(scipy.fftpack.fft(g_z)[:N//2]))

    # predict 9.8m/s instead of just a rotation

    diff_error=[]
    pred_error=[]
    smooth_error=[]
    lin_error=[]
    a_norm=[]
    D = 0
    a_state = np.r_[0.0,0.0,0.0]
    for n in range(D, 1000 + D):
        #pred = ideal_rot(get_a(n), get_g(n-D)) #past gyro
        lin_pred =  linearized_rot(get_a(n), get_g(n-D))
        #pred = ideal_rot(get_a(n), get_g(n+1)) #current gryo
        pred = ideal_rot(get_a(n), (get_g(n) + get_g(n+1))/2) #average past and current (trapezoidal integration)
        p = .95
        a_state = ideal_rot((p*get_a(n)+(1-p)*a_state), (get_g(n) + get_g(n+1))/2)
        diff_error.append(norm(get_a(n+1) - get_a(n)))
        pred_error.append(norm(get_a(n+1) - pred))
        lin_error.append(norm(get_a(n+1) - lin_pred))
        smooth_error.append(norm(get_a(n+1) - a_state))
        a_norm.append(norm(get_a(n+1)))
        #print(" real {} = g {} = pred {} = lin {}".format(get_a(n+1), get_g(n), pred, lin_pred))


    print("diff {}".format(sum(diff_error)))
    print("pred {}".format(sum(pred_error)))
    print("smooth {}".format(sum(smooth_error)))
    print("lin {}".format(sum(lin_error)))
    print("norm {}".format(sum(a_norm)))

    plt.figure(2)
    x = range(1000)
    #print(len(a_x))
    #plt.subplot(2,1,1)
    plt.plot(x, diff_error, label="diff")
    #plt.subplot(2,1,2)
    plt.plot(x, pred_error, label="pred")
    plt.plot(x, smooth_error, label="smooth")
    plt.plot(x, a_norm, label="mag")
    #plt.plot(x, a_x[0:1000])
    #plt.subplot(3,1,2)
    #plt.plot(x, a_y[0:1000])
    #plt.subplot(3,1,3)
    #plt.plot(x, a_z[0:1000])
    plt.legend()
    plt.show()
