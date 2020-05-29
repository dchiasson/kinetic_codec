#!/usr/bin/env python3

import os
import subprocess

from machine_learning import optimal_fir

SENS = ['acc', 'gyro']
LOCS = ['rf','rs','rt','lf','ls','lt']
ACTS = ['bicycling','running','sitting','standing','walking']
DIMS = ['x','y','z']
SUBJ = range(1, 18+1)

def get_file_list(data_type):

    folder_loc = "/home/chiasson/Documents/David/research/HuGaDB/HuGaDB/Data.parsed.noVar/processed/files/"
    csv_loc = "/home/chiasson/Documents/David/research/HuGaDB/HuGaDB/Data.parsed.noVar/processed/"
    csv_folder = None
    all_folders = [os.path.join(folder_loc, f) for f in os.listdir(folder_loc)]
    all_folders = list(filter(lambda x: x.split('.')[-1] == 'txt', all_folders))
    file_list = []

    if data_type == "all":
        for folder in all_folders:
            for loc in LOCS:
                file_list.append(os.path.join(folder, loc))
        csv_folder = csv_loc
    elif data_type in SENS:
        pass
    elif data_type in LOCS:
        for folder in all_folders:
            file_list.append(os.path.join(folder, data_type))
        csv_folder = os.path.join(csv_loc, 'segment', data_type)
    elif data_type in ACTS:
        for folder in all_folders:
            if (folder.split('/')[-1].find(data_type) != -1):
                for loc in LOCS:
                    file_list.append(os.path.join(folder, loc))
        csv_folder = os.path.join(csv_loc, 'activity', data_type)
    elif data_type in DIMS:
        pass
    elif data_type in SUBJ:
        for folder in all_folders:
            if (int(folder.split('/')[-1].split('_')[-2]) == data_type):
                for loc in LOCS:
                    file_list.append(os.path.join(folder, loc))
        csv_folder = os.path.join(csv_loc, 'subject', str(data_type))
    else:
        print("Error: unknown data_type: {}".format(data_type))

    if not all([os.path.isdir(f) for f in file_list]):
        print("Error: non-existent files in list")
    if csv_folder is None:
        print("Error: could not find csv location for {}".format(data_type))

    return file_list, csv_folder

ACCEL_FIX=9.80665*2.0/32768
GYRO_FIX=2000/32768

def extract_size(output_text):
    return int(output_text.split(b',')[2].split(b' ')[-1])

def run_iteration(technique, k, filter_loc, data_loc, verbose=False):
    command = "./bin/humoco -t {} -k {} -f {} {}".format(technique, k, filter_loc, data_loc)
    result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, shell=True)
    if (result.returncode != 0):
        print(result.args)
        print(result.stdout)
        return 1e99
        raise Exception
    if verbose: 
        print(result.stdout.decode("utf-8"))
    size = extract_size(result.stdout)
    #print("{}".format(extract_size(result.stdout)))
    return size

def run_best_k(technique, filter_loc, data_loc, starting_k=12, verbose=False):
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

def process_data_list(technique, filter_loc, data_list, verbose=False):
    total_size = 0
    for data in data_list:
        #print(data)
        best_size, best_k = run_best_k(technique, filter_loc, data, 10, verbose)
        if verbose:
          print(best_size, best_k)
        csv_size = os.path.getsize(os.path.join(data,'acc_x')) * 6 * 8.32
        #print("K {}, CR{}".format(best_k, csv_size/best_size))
        print(csv_size/best_size)
        total_size += best_size
    print("total_size: {}".format(total_size))
    return total_size

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

def get_flac_compressed_size(data_loc):
    file_list = ["acc_x", "acc_y", "acc_z", "gyro_x", "gyro_y", "gyro_z"]
    total_size = 0
    for f in file_list:
        file_path = os.path.join(data_loc, f)
        #command = "flac --endian=little --channels=1 --bps=16 --sample-rate=60000 --sign=signed {} -f -r 0,0 -l 0 -b 16348".format(file_path)
        command = "flac --endian=little --channels=1 --bps=16 --sample-rate=60000 --sign=signed {} -f -r 0,0 -l 0".format(file_path)
        result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, shell=True)
        total_size += os.path.getsize(file_path + '.flac')
    print(total_size)
    return total_size

def run_all_on_data(data_type):

    data_list, csv_folder = get_file_list(data_type)

    """
    #####################
    # Reference formats #
    #####################
    print_csv_size(csv_folder)
    print_zip_size(csv_folder)
    print_binary_size(csv_folder)

    """
    print("##Rice")
    technique = "auto-homo"
    filter_loc = "~/Documents/David/research/kinetic_codec/data/fixed_poly_pred/{}_deg_poly_reg/{}"
    process_data_list(technique,  filter_loc.format(0,0), data_list)

    ##########
    # Spline #
    ##########
    print('##spline')
    technique = "auto-homo"
    filter_loc = "~/Documents/David/research/kinetic_codec/data/natural_spline_pred/condition0/2"
    process_data_list(technique, filter_loc, data_list)

    ###############################
    # Polynomial Regression Suite #
    ###############################
    technique = "auto-homo"
    filter_loc = "~/Documents/David/research/kinetic_codec/data/fixed_poly_pred/{}_deg_poly_reg/{}"
    print('##5 deg')
    process_data_list(technique, filter_loc.format(6,19), data_list)
    print('##4 deg')
    process_data_list(technique, filter_loc.format(5,19), data_list)
    print('##3 deg')
    process_data_list(technique, filter_loc.format(4,19), data_list)
    print('##2 deg')
    process_data_list(technique, filter_loc.format(3,13), data_list)
    print('##1 deg')
    process_data_list(technique, filter_loc.format(2,8), data_list)
    print('##0 deg')
    process_data_list(technique, filter_loc.format(1,1), data_list)

    print('##AR')
    filter_loc = optimal_fir.data_train(data_list, 3, is_cross=False, technique=optimal_fir.CVXPY, verbose=True)
    technique = "auto-hetero"
    #filter_loc = "~/Documents/David/research/kinetic_codec/machine_learning/cross_fir/auto_hetero_{}"
    #filter_loc = "~/Documents/David/research/kinetic_codec/machine_learning/cross_fir/built_auto_hetero_2_2"
    #process_data_list(technique, filter_loc.format(2), data_list)
    process_data_list(technique, filter_loc, data_list, verbose=False)


    print('##MVAR')
    filter_loc = optimal_fir.data_train(data_list, 2, is_cross=True, technique=optimal_fir.CVXPY, verbose=True)
    technique = 'cross-hetero'
    #filter_loc = "~/Documents/David/research/kinetic_codec/machine_learning/cross_fir_old/cross_hetero_{}"
    process_data_list(technique, filter_loc, data_list, verbose=False)


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

    run_all_on_data("all")
    """
    for person in SUBJ:
        if person == 2:
            continue
        if person == 1:
            continue
        print("Subject {}".format(person))
        run_all_on_data(person)
    """
    """

    filter_loc = "~/Documents/David/research/kinetic_codec/data/fixed_poly_pred/{}_deg_poly_reg/{}"
    for location in LOCS:
        print("Location {}".format(location))
        data_list, csv_folder = get_file_list(location)
        technique = "auto-homo"
        process_data_list(technique, filter_loc.format(1,1), data_list)
        #run_all_on_data(location)

    for action in ACTS:
        continue
        print("Action {}".format(action))
        data_list, csv_folder = get_file_list(action)
        technique = "auto-homo"
        process_data_list(technique, filter_loc.format(1,1), data_list)
        #run_all_on_data(action)
    """
    """
    filters = os.listdir("cross_fir")
    data_list, csv_folder = get_file_list("all")
    filter_dir = "/home/chiasson/Documents/David/research/kinetic_codec/cross_fir/"
    filters = filter(lambda x:os.path.getsize(x)==288, [os.path.join(filter_dir, x) for x in os.listdir(filter_dir)])
    #print(list(filters))

    for f in filters:
        technique = 'cross-hetero'
        process_data_list(technique, f, data_list, verbose=False)
    """
