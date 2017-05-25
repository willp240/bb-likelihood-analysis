''' Take a folder full of files and separate them out into directories based
on the class of event
'''
import os
import glob
import re
import shutil

def read_event_type(path):
    ''' Get the smallest part of the string that defines what kind of 
    events these are
    :param str path:
    :returns str:
    '''
    match = re.search("TeLoaded(\S+)_r", path)
    if match is None:
        raise ValueError("{0} path don't make no sense to me".format(path))
    return match.group(1)

def separate_out_files(dirname, dry_run):
    '''
    '''
    dirname = os.path.abspath(dirname)
    for x in glob.glob(os.path.join(dirname, "*.root")):
        etype = read_event_type(x)

        subdir = os.path.join(dirname, etype)
        if not os.path.isdir(subdir):
            os.mkdir(subdir)
        
        print "{2}moving {0} to {1}".format(x, subdir, "(not) " if dry_run is True else "")
        if dry_run is False:
            shutil.move(x, subdir)

if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("directory", type=str)
    parser.add_argument("--dry_run", action="store_true")
    args = parser.parse_args()

    separate_out_files(args.directory, args.dry_run)
