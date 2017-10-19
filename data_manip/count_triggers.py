import ROOT

def count_ev_indices(br_name, *filenames):
    ch = ROOT.TChain(br_name)
    ch.Add(filenames)
    
    triggers = [0] * 10
    for x in ch:
        triggers[x.evIndex + 1] += 1
    return triggers
    
if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_arguments("infile_names", type=str, nargs="+")
    parser.add_arguments("--tree_name", type=str, default = "output")
    args = parser.parse_args()

    event_indices = count_ev_indices(args.tree_name, args.infile_names)
    for i, count in enumerate(event_indices):
        print i-1, count

    sum_ = 0
    for i, count in enumerate(event_indices):
        sum_ += count
        print i, sum_
