import re
import os
import pprint

re_seed =  re.compile(r'Random seed  : (.*)$')
re_query0 = re.compile(r'#0: (\d+.\d+)ms')
re_query1 = re.compile(r'#1: (\d+.\d+)ms')
re_match = re.compile(r'#0 \(neo.dll\) and #1 \(reference.dll\): (.*)$')

def process_log(out):
    seed, match, q0, q1 = "*********************", "******", 1.0, 1.0
    with open("log.txt", 'r') as f:
        for line in f:
            mo = re_seed.match(line)
            if mo:
                seed = mo.group(1)
                
            mo = re_match.match(line)
            if mo:
                match = mo.group(1)

            mo = re_query0.match(line)
            if mo:
                q0 = float(mo.group(1))

            mo = re_query1.match(line)
            if mo:
                q1 = float(mo.group(1))
        ratio = q1 / q0
        out.write('{:6.1f}  {:10.2f} {:10.2f}  {}      {}\n'.format(ratio, q0, q1, match, seed))
        return ratio

if __name__ == "__main__":
    params = 'neo.dll reference.dll -p10000000 -q1000'
    with open("fulllog.txt", 'a+') as out:
        out.write('arguments: %s\n==================================================\n\n' % params)

        ratio = 0.0
        num_iter = 2
        for i in range(0, num_iter):
            os.system('point_search.exe %s' % params)
            ratio += process_log(out)

        out.write('\noverall ratio: {:6.1f}\n\n'.format(ratio / num_iter))
        
        
