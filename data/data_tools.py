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
