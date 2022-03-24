import os
import time
from shutil import copyfile

file_name = "test_coefs"
orig_time = os.path.getmtime(file_name)

while True:
    time.sleep(100)
    #print("iteration")
    if (os.path.getmtime(file_name) != orig_time):
        time_str = str(time.time())
        new_name = file_name+time_str
        copyfile(file_name, new_name)
        print("Copied file to: {}".format(new_name))
        orig_time = os.path.getmtime(file_name)


