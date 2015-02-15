import re
import os
import pprint

re_seed =  re.compile(r'Random seed  : (.*)$')
re_query0 = re.compile(r'#0: (\d+.\d+)ms')
re_query1 = re.compile(r'#1: (\d+.\d+)ms')
re_match = re.compile(r'#0 \(neo.dll\) and #1 \(reference.dll\): (.*)$')

def process_log():
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
        print '{:6.1f}  {:10.2f} {:10.2f}  {}      {}'.format(ratio, q0, q1, match, seed)
        return ratio

if __name__ == "__main__":
    seeds = ('6B52EB4E-61DA851E-6B52EB4E-7109C3C8-409A049E', 
             '659942F4-6F112CA4-659942F4-7FC26A72-4E51AD24', 
             '9E5B9E32-94D3F062-9E5B9E32-8400B6B4-B59371E2', 
             '9551DCCB-9FD9B29B-9551DCCB-8F0AF44D-BE99331B', 
             '8E04EF27-848C8177-8E04EF27-945FC7A1-A5CC00F7', 
             '880BD0C9-8283BE99-880BD0C9-9250F84F-A3C33F19', 
             'BEDEA9F0-B456C7A0-BEDEA9F0-A4858176-95164620', 
             'B8D71A02-B25F7452-B8D71A02-A28C3284-931FF5D2', 
             'AF83A7F7-A50BC9A7-AF83A7F7-B5D88F71-844B4827', 
             'A8232733-A2AB4963-A8232733-B2780FB5-83EBC8E3', 
             '2969013A-23E16F6A-2969013A-333229BC-2A1EEEA', 
             '221A7848-28921618-221A7848-384150CE-9D29798')
    
    params = 'neo.dll reference.dll -p1000000 -q1000'
    with open("fulllog.txt", 'a+') as out:
        ratio = 0.0
        num_iter = len(seeds)
        for seed in seeds:
            os.system('point_search.exe %s -s%s' % (params, seed))
            ratio += process_log()

        print '\noverall ratio: {:6.1f}\n'.format(ratio / num_iter)
        
        
