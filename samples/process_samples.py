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
                   'rawfiles/'+base_filename+'.raw',
                   'bass',
                   '-5']
    subprocess.check_output(sox_command)
    sox_command_2 = ['sox',
                   'wavfiles/'+filename,
                   '--bits',
                   '8',
                   '-r'
                   '16384',
                   '--encoding',
                   'signed-integer',
                   '--endian',
                   'little',
                   'rawfiles/'+base_filename+'_reversed.raw',
                   'bass',
                   '-5',
                   'reverse']
    subprocess.check_output(sox_command_2)
    mozzi_command = ['python',
                     'char2mozzi.py',
                     'rawfiles/'+base_filename+'.raw',
                     '../arduino/drumkid/'+base_filename+'.h',
                     base_filename,
                     '16384']
    subprocess.check_output(mozzi_command)
    mozzi_command_2 = ['python',
                     'char2mozzi.py',
                     'rawfiles/'+base_filename+'_reversed.raw',
                     '../arduino/drumkid/'+base_filename+'_reversed.h',
                     base_filename+'_reversed',
                     '16384']
    subprocess.check_output(mozzi_command_2)
    
