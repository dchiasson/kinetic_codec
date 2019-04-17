#!/usr/bin/env python

"""
Conversion of a CSV file into fixed point, compact binary files

This process by itself will compress data size by about 2/3. This is because
CSV uses readable strings to encode data which is less compact than a binary
representation.

This process is not guaranteed to be lossless or reversable as guesses are made
about original data precision and quantization levels.
"""
import sys
import logging
import csv
import pickle
import os
import matplotlib.pyplot as plt
import numpy as np

def process_csv(file_name):
    """Convert a csv file to fixed point binary files"""
    if not os.path.isfile(file_name):
        logging.critical("cannot find csv filename: `{}`".format(file_name))
        sys.exit(1)
    dir_name = file_name + '_dat'
    if os.path.isdir(dir_name):
        os.rmdir(dir_name)
        #logging.critical("Binary data folder already exists at `{}`. Delete to recreate".format(dir_name))
        #sys.exit(1)

    os.mkdir(dir_name)
    with open(file_name, 'r') as csvfile:
        # @TODO(David): generalize to other formats of CSV and comments on top
        reader = csv.reader(csvfile, delimiter=',')
        header = next(reader)
        for i, field in enumerate(header):
            if field == '':
                continue
            csvfile.seek(0)
            next(reader)
            logging.info("Processing field `{}`".format(field))
            data_list = []
            for row in reader:
                data_list.append(float(row[i]))
            data_array = np.array(data_list, np.float64)
            mmap_file = np.memmap(os.path.join(dir_name, field), dtype='i2', mode='w+', shape=len(data_array))
            divisor = find_divisor(data_array)
            fixed_pt = np.round(data_array*divisor).astype(np.int16)
            print(len(fixed_pt))
            print("divisor {}, precision {}".format(divisor, np.log2(max(fixed_pt) - min(fixed_pt))))
            mmap_file[:] = fixed_pt
            mmap_file.flush()
            del mmap_file


def standard_field(input_name):
    """Returns a standard form of data field name"""
    return input_name.lower()


def find_divisor(array):
    """This function infers the constant for conversion of float to fixed point integer representation

    @arg[in]  array np_array of floating point sensor values
    @returns  scaling factor for converting floating point data into integers

    Sensors generally have an equidistant quantization factor which represent a small subset of
    possible floating point values. Thus, data can be represented much more compactly as an
    array of integers with a scaling factor. Integer representation is also necessary to avoid
    computational rounding errors inherently present in floating point operations.
    """
    unique = np.unique(array)
    if len(unique) <= 1:
        return 1.0
    diff_array = unique[1:] - unique[0:-1]
    min_array = np.ones_like(diff_array) * 1e99
    np.minimum(diff_array, 1e99, out=min_array, where=(diff_array > 1e-8))
    # @TODO(David): test for outliers
    # @TODO(David): test for equidistant quantization
    # @TODO(David): test for zero
    # @TODO(David): test for adequate variance
    return int(round(1.0 / np.min(min_array)))


if __name__ == "__main__":
    logging.getLogger().setLevel(logging.INFO)
    if len(sys.argv) > 2:
        logging.critical("USAGE: {} [FILE_NAME]".format(sys.argv[0]))
        sys.exit(1)
    elif len(sys.argv) == 1:
        #process_csv(raw_input("CSV input file name: "))
        process_csv('one_sensor_2019-04-02.csv')
    else:
        process_csv(sys.argv[1])
