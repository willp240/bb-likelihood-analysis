from rat import ROOT

def count_ev_indices(*filenames):

    total_ngen = 0

    
    for fn in filenames:

        f = ROOT.TFile(fn)
        meta = f.Get("meta")
        print sum(meta.GetEventsGeneratedCounts())
        total_ngen += sum(meta.GetEventsGeneratedCounts())
        
    print "Len: ", len(filenames)    
    return total_ngen
    
if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("infile_names", type=str, nargs="+")
    args = parser.parse_args()

    entries = count_ev_indices(*args.infile_names)
    
print "Number of generated events from metadata = ", entries

