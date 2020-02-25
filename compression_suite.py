#!/usr/bin/env python3

import os
import subprocess

def extract_size(output_text):
    return int(output_text.split(b',')[2].split(b' ')[-1])

def run_iteration(technique, k, filter_loc, data_loc):
    command = "./bin/humoco -t {} -k {} -f {} {}".format(technique, k, filter_loc, data_loc)
    result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, shell=True)
    if (result.returncode != 0):
        print(result.args)
        print(result.stdout)
        raise Exception
    print(result.stdout.decode("utf-8"))
    print("{}".format(extract_size(result.stdout)))


def cross_homo():
    #technique = 'cross-homo'
    technique = 'cross-hetero'
    subject_dir = "/home/chiasson/Documents/David/research/HuGaDB/HuGaDB/Data.parsed/processed_5_9_12_/subject/"
    for subject in os.listdir(subject_dir):
        data_loc = os.path.join(subject_dir, subject)
        data_loc += '/'
        print("Processing subject {}".format(subject))
        filter_loc = "~/Documents/David/research/kinetic_codec/machine_learning/cross_fir/test_coefs"
        run_iteration(technique, 7, filter_loc, data_loc)

def cross_homo_total():
    technique = 'auto-hetero'
    data_loc = "/home/chiasson/Documents/David/research/HuGaDB/HuGaDB/Data.parsed/processed_5_9_12_/"
    filter_loc = "~/Documents/David/research/kinetic_codec/machine_learning/cross_fir/test_coefs"
    run_iteration(technique, 7, filter_loc, data_loc)

def test_data():
    technique = 'auto-homo'
    data_loc = "/home/chiasson/Documents/David/research/HuGaDB/HuGaDB/Data.parsed/processed_5_9_12_/tmp/"
    filter_loc = "~/Documents/David/research/kinetic_codec/data/fixed_poly_pred/1_deg_poly_reg/1"
    run_iteration(technique, 5, filter_loc, data_loc)

def poly_reg():
    technique = "auto-homo"
    subject_dir = "/home/chiasson/Documents/David/research/HuGaDB/HuGaDB/Data.parsed/processed_5_9_12_/subject/"
    #data_loc = "/home/chiasson/Documents/David/research/HuGaDB/HuGaDB/Data.parsed/processed_5_9_12_/"
    #filter_loc = "~/Documents/David/research/kinetic_codec/data/fixed_poly_pred/{}_deg_poly_reg/{}"
    #run_iteration(technique, 12, filter_loc.format(0,0), data_loc)
    #for subject in os.listdir(subject_dir):
    for subject in [1]:
        #data_loc = os.path.join(subject_dir, subject)
        #data_loc += '/'
        #print("Processing subject {}".format(subject))
        data_loc = "/home/chiasson/Documents/David/research/HuGaDB/HuGaDB/Data.parsed/processed_5_9_12_/"

        filter_loc = "~/Documents/David/research/kinetic_codec/data/fixed_poly_pred/{}_deg_poly_reg/{}"
        run_iteration(technique, 7, filter_loc.format(6,19), data_loc)
        run_iteration(technique, 6, filter_loc.format(5,19), data_loc)
        run_iteration(technique, 6, filter_loc.format(4,19), data_loc)
        run_iteration(technique, 5, filter_loc.format(3,13), data_loc)
        run_iteration(technique, 5, filter_loc.format(2,8), data_loc)
        run_iteration(technique, 5, filter_loc.format(1,1), data_loc)
        run_iteration(technique, 12, filter_loc.format(0,0), data_loc)
        #for fil in [(6,19), (5,19), (4,19), (3,13), (2,8), (1,1), (0,0)]:
        #for fil in [(1,1)]:
        #    for k in range(2,7):
        #        print("{} for k= {}".format(fil, k))
        #        run_iteration(technique, k, filter_loc.format(*fil), data_loc)

if __name__ == '__main__':
    poly_reg()
    #cross_homo()
    #test_data()
