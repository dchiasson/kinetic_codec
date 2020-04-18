import os
import numpy as np

def save_array(data_array, file_name, is_fixed_point=True):
    if is_fixed_point:
        mmap_file = np.memmap(file_name, dtype=np.int32, mode='w+', shape=np.shape(data_array))
        # conversion to libfixmath fix16_t type
        # see `fix16_t_from_float` in fix16.h
        data_array = (data_array * 0x10000)
        data_array = np.multiply((data_array >= 0), data_array + 0.5) + np.multiply((data_array < 0), data_array - 0.5)
        data_array = data_array.astype(np.int32)
    else:
        mmap_file = np.memmap(file_name, dtype='f4', mode='w+', shape=np.shape(data_array))
    mmap_file[:] = data_array
    mmap_file.flush()

def fix_to_float(fixed):
    return fixed / 65536.0

def float_to_fix(float_num):
    if (float_num >= 0):
        return int(float_num * 0x10000 + .5)
    else:
        return int(float_num * 0x10000 - .5)

def print_fixed_point_coefs(file_name):
    mmap_file = np.memmap(file_name, dtype=np.int32, mode='r')
    length = len(mmap_file)
    group = False
    if ((length % 36) == 0):
        hist = length // 36
        group = True
    elif ((length % 6) == 0):
        hist = length // 6
    else:
        hist = length

    place = 0
    line_count = 0
    while place < length:
        if (group and (line_count % 6) == 0):
            print("====== Batch {} ========".format(line_count // 6))
        line = ""
        for _ in range(hist):
            line += "{:8.2f}".format(fix_to_float(mmap_file[place]))
            place += 1
        print(line)
        line_count+=1


if __name__ == "__main__":
    file_name = "/home/chiasson/Documents/David/research/kinetic_codec/data/fixed_poly_pred/2_deg_poly_reg/8"
    print_fixed_point_coefs(file_name)
