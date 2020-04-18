#!/usr/bin/env python3

import os
import subprocess

def get_file_list(data_type):
    SENS = ['acc', 'gyro']
    LOCS = ['rf','rs','rt','lf','ls','lt']
    ACTS = ['bicycling','running','sitting','standing','walking']
    DIMS = ['x','y','z']
    SUBJ = range(1, 18+1)

    folder_loc = "/home/chiasson/Documents/David/research/HuGaDB/HuGaDB/Data.parsed/processed/files/"
    all_folders = [os.path.join(folder_loc, f) for f in os.listdir(folder_loc)]
    all_folders = list(filter(lambda x: x.split('.')[-1] == 'txt', all_folders))
    file_list = []

    if data_type == "all":
        for folder in all_folders:
            for loc in LOCS:
                file_list.append(os.path.join(folder, loc))
                #for data in os.listdir(os.path.join(folder, loc)):
                #    file_list.append(os.path.join(folder, loc, data))
    elif data_type in SENS:
        pass
    elif data_type in LOCS:
        pass
    elif data_type in ACTS:
        pass
    elif data_type in DIMS:
        pass
    elif data_type in SUBJ:
        pass
    else:
        print("Error: unknown data_type: {}".format(data_type))

    if not all([os.path.isfile(f) for f in file_list]):
        print("Error: non-existent files in list")

    return file_list

ACCEL_FIX=9.80665*2.0/32768
GYRO_FIX=2000/32768

def extract_size(output_text):
    return int(output_text.split(b',')[2].split(b' ')[-1])

def run_iteration(technique, k, filter_loc, data_loc, verbose=False):
    size = 0
    for data in data_loc: #NOOOOOO!! this should happen outside of K optimization!
        command = "./bin/humoco -t {} -k {} -f {} {}".format(technique, k, filter_loc, data)
        result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, shell=True)
        if (result.returncode != 0):
            print(result.args)
            print(result.stdout)
            return 1e99
            raise Exception
        if verbose: 
            print(result.stdout.decode("utf-8"))
        size += extract_size(result.stdout)
        #print("{}".format(extract_size(result.stdout)))
    return size

def run_best_k(technique, filter_loc, data_loc, starting_k=12, verbose=False):
    #    min_k = 4
    #    max_k = 14
    #    best_size = 1e99
    #    best_k = -1
    #    for k in range(min_k, max_k+1):
    #        size = run_iteration(technique, k, filter_loc, data_loc, verbose)
    #        if size < best_size:
    #            best_size = size
    #            best_k = k
    #    print("tech:{} == {} == {}".format(technique, filter_loc, data_loc))
    #    print("{}, {}".format(best_size, best_k))


    best_k = starting_k
    current_k = starting_k
    best_size = run_iteration(technique, current_k, filter_loc, data_loc, verbose)
    iterations = 0

    #increasing K
    while True:
        iterations += 1
        current_k += 1
        size = run_iteration(technique, current_k, filter_loc, data_loc, verbose)
        if size <= best_size:
            best_size = size
            best_k = current_k
        else:
            current_k -= 1
            break
        if iterations > 12:
            print("ERROR: k search failed during increase")
            return 1e99, -1

    iterations = 0
    #decreasing K
    while True:
        iterations += 1
        current_k -= 1
        size = run_iteration(technique, current_k, filter_loc, data_loc, verbose)
        if size < best_size:
            best_size = size
            best_k = current_k
        else:
            break
        if iterations > 12:
            print("ERROR: k search failed during decrease")
            return 1e99, -1

    return best_size, best_k
        

def print_csv_size(data_loc):
    print(os.path.getsize(os.path.join(data_loc, 'all.csv')))
    # If csv size is formatted to eight bytes per sample, instead use:
    #print(os.path.getsize(os.path.join(data_loc, 'acc_x')) * 6 * 8)

def print_binary_size(data_loc):
    # each of the six streams are the same size
    print(os.path.getsize(os.path.join(data_loc, 'acc_x')) * 6)

def print_zip_size(data_loc):
    csv_file = os.path.join(data_loc, 'all.csv')
    zip_csv = os.path.join(data_loc, 'all.csv.zip')
    try:
        os.remove(zip_csv)
    except FileNotFoundError:
        pass
    command = "zip -6 --compression-method deflate --quiet {} {}".format(zip_csv, csv_file)
    result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, shell=True)
    print(os.path.getsize(zip_csv))

def run_all_on_data(data_loc):
    #####################
    # Reference formats #
    #####################
    print_csv_size(data_loc)
    print_zip_size(data_loc)
    print_binary_size(data_loc)

    technique = "auto-homo"
    filter_loc = "~/Documents/David/research/kinetic_codec/data/fixed_poly_pred/{}_deg_poly_reg/{}"
    run_best_k(technique,  filter_loc.format(0,0), data_loc)

    ##########
    # Spline #
    ##########
    technique = "auto-homo"
    filter_loc = "~/Documents/David/research/kinetic_codec/data/natural_spline_pred/condition0/2"
    run_best_k(technique, filter_loc, data_loc)

    ###############################
    # Polynomial Regression Suite #
    ###############################
    technique = "auto-homo"
    filter_loc = "~/Documents/David/research/kinetic_codec/data/fixed_poly_pred/{}_deg_poly_reg/{}"
    run_best_k(technique, filter_loc.format(6,19), data_loc)
    run_best_k(technique, filter_loc.format(5,19), data_loc)
    run_best_k(technique, filter_loc.format(4,19), data_loc)
    run_best_k(technique, filter_loc.format(3,13), data_loc)
    run_best_k(technique, filter_loc.format(2,8), data_loc)
    run_best_k(technique, filter_loc.format(1,1), data_loc)

    technique = "auto-hetero"
    #filter_loc = "~/Documents/David/research/kinetic_codec/machine_learning/cross_fir/auto_hetero_{}"
    filter_loc = "~/Documents/David/research/kinetic_codec/machine_learning/cross_fir/built_auto_hetero_2_2"
    #run_best_k(technique, filter_loc.format(2), data_loc)
    run_best_k(technique, filter_loc, data_loc)


    technique = 'cross-hetero'
    filter_loc = "~/Documents/David/research/kinetic_codec/machine_learning/cross_fir_old/cross_hetero_{}"
    run_best_k(technique, filter_loc.format(3), data_loc)


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
    technique = 'cross-hetero'
    #technique = 'auto-homo'
    data_loc = "/home/chiasson/Documents/David/research/HuGaDB/HuGaDB/Data.parsed/processed/"
    filter_loc = "~/Documents/David/research/kinetic_codec/machine_learning/cross_fir/cross_hetero_{}"
    #filter_loc = "~/Documents/David/research/kinetic_codec/machine_learning/cross_fir/test_coefs"
    #filter_loc = "~/Documents/David/research/kinetic_codec/data/fixed_poly_pred/1_deg_poly_reg/1"
    #for k in range(3,10):
    for k in [6]:
        for fil in range(1,7):
            run_iteration(technique, k, filter_loc.format(fil), data_loc)

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

def test_2():

    data_loc = "/home/chiasson/Documents/David/research/HuGaDB/HuGaDB/Data.parsed/processed/activity/standing/"
    technique = 'auto-hetero'
    filter_loc = "~/Documents/David/research/kinetic_codec/machine_learning/cross_fir/cvx_solver_1"
    run_iteration(technique, 5, filter_loc, data_loc, True)

    technique = "cross-hetero"
    for i in range(1,8):
        filter_loc = "~/Documents/David/research/kinetic_codec/machine_learning/cross_fir/built_auto_hetero_2_{}".format(i)
        run_iteration(technique, 5, filter_loc, data_loc, True)

    filter_loc = "~/Documents/David/research/kinetic_codec/machine_learning/cross_fir/built_auto_hetero_1"
    run_iteration(technique, 5, filter_loc, data_loc, True)
    
    technique = "auto-homo"
    filter_loc = "~/Documents/David/research/kinetic_codec/data/fixed_poly_pred/{}_deg_poly_reg/{}"
    run_iteration(technique, 5, filter_loc.format(1,1), data_loc, True)

    technique = "cross-hetero"
    filter_loc = "~/Documents/David/research/kinetic_codec/machine_learning/cross_fir/built_auto_hetero_0"
    run_iteration(technique, 5, filter_loc, data_loc, True)

    technique = 'auto-hetero'
    filter_loc = "~/Documents/David/research/kinetic_codec/machine_learning/cross_fir/auto_hetero_2"
    run_iteration(technique, 5, filter_loc, data_loc, True)

    filter_loc = "~/Documents/David/research/kinetic_codec/machine_learning/cross_fir/newbuilt_auto_hetero_0"
    run_iteration(technique, 5, filter_loc, data_loc, True)

    filter_loc = "~/Documents/David/research/kinetic_codec/machine_learning/cross_fir/newbuilt_auto_hetero_1"
    run_iteration(technique, 5, filter_loc, data_loc, True)

if __name__ == '__main__':
    #test_2()
    #data_loc = "/home/chiasson/Documents/David/research/HuGaDB/HuGaDB/Data.parsed/processed_5_9_12_/training/"
    data_loc = "/home/chiasson/Documents/David/research/HuGaDB/HuGaDB/Data.parsed/processed.new/"
    run_all_on_data(data_loc)

    #poly_reg()
    #cross_homo_total()
    #test_data()
    
    #data_loc = "/home/chiasson/Documents/David/research/HuGaDB/HuGaDB/Data.parsed/processed_5_9_12_/training/"
    #technique = "auto-hetero"
    #filter_loc = "~/Documents/David/research/kinetic_codec/machine_learning/cross_fir/auto_hetero_{}"
    #run_iteration(technique, 5, filter_loc.format(2), data_loc)
