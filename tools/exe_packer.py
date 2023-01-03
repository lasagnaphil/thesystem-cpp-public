# A python script that packs assets into the end of exes.

# How it works:
# file = | .... exe contents .... | ... data ... | data length (4 bytes) |

# Using (or abusing) the fact that windows doesn't load any data on memory after the end of executable, we can
# make single-file executables with all assets embedded in it, and mmap the region to hand it in to PhysFS.

import sys
import shutil

exe_path = sys.argv[1]
data_path = sys.argv[2]
exe_out_path = sys.argv[3]

f_exe = open(exe_path, 'rb')
f_data = open(data_path, 'rb')
shutil.copy(exe_path, exe_out_path)
f_exe_new = open(exe_out_path, 'ab')
f_exe_new.write(f_data.read())
data_size = f_data.tell() # f_data is seeked til the end
f_exe_new.write(data_size.to_bytes(4, 'little'))
f_exe_new.close()
f_data.close()