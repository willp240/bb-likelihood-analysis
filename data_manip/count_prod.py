''' Count the files and events for the production data
'''
import glob
import ROOT
import os

def count_files(filestr):
    '''Count the files matching the string given (wildcards included)
    :param str filestr: pattern to match
    :returns int: the count
    '''
    return len(glob.glob(filestr))

def count_events(filestr, tree):
    ''' Count the events inside all the root files that match this pattern
    in this tree
    :param str filestr: pattern to match
    :param str tree: the name of the tree in the ROOT directory
    :returns int: the count
    '''
    ch = ROOT.TChain(tree)
    ch.Add(filestr)
    return ch.GetEntries()

def examine_directory(dirname, tree_name):
    ''' Walk down the directory 
    :param dirname:
    :param str tree: key of the tree in ROOT directory
    :returns list( ("name", int , int) ): dirname, nfiles, nevents
    '''
    returnlist = []
    for root, dirs, files in os.walk(dirname):
        print root
        root_files = os.path.join(root, "*.root")
        files = count_files(root_files)
        evs = 0
        if files:
            evs = count_events(root_files, tree_name)
        returnlist.append((os.path.basename(root), files, evs))
    return returnlist

def examine_files(path, tree):
    ''' Look at files matching this path
    :param str path:
    :param str tree: key of the tree in ROOT directory
    :returns list( ("name", int , int) ): dirname, nfiles, nevents
    '''
    return [(os.path.basename(path), count_files(path), count_events(path, tree))]

def format_result(result):
    '''Take the result and make it a nice string
    :param list( ("name", int, int) )
    :returns str:
    '''
    return "\n".join("{0: <40} | {1: <20} | {2: <20}".format(*x) for x in result)

def examine_path(path, tree):
    ''' If this is a directory look inside, if its a file string just match
    that string
    :param str path: to the files or directory
    :returns list( ("name", int, int) ):
    '''
    if os.path.isdir(path):
        return examine_directory(path, tree)
    else:
        return examine_files(path, tree)
    


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument("path", type = str)
    parser.add_argument("-tree", type = str, default = "output")
    args = parser.parse_args()
    print format_result(examine_path(args.path, args.tree))
