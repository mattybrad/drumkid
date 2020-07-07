
import subprocess, os
for filename in os.listdir('wavfiles'):
    base_filename = filename.split('.')[0]
    sox_command = ['sox',
                   'wavfiles/'+filename,
                   '--bits',
                   '8',
                   '-r'
                   '16384',
                   '--encoding',
                   'signed-integer',
                   '--endian',
                   'little',
                   'rawfiles/'+base_filename+'.raw']
    subprocess.check_output(sox_command)
    mozzi_command = ['python',
                     'char2mozzi.py',
                     'rawfiles/'+base_filename+'.raw',
                     '../arduino/1.1/drumkid/'+base_filename+'.h',
                     base_filename,
                     '16384']
    subprocess.check_output(mozzi_command)
