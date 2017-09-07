import json

def format_e(n):
    a = '%E' % n
    return a.split('E')[0].rstrip('0').rstrip('.') + 'E' + a.split('E')[1]

def load_files(*files):
    return [json.loads(open(fl).read()) for fl in files]

def lt_equivs(rates, mc_map, counts, overrides):
    lts = {}
    for k in rates.keys():
        mc_types = mc_map[k]
        try:            
            mc_counts = []
            for t in mc_types:
                try:
                    mc_counts.append(overrides[t])
                except KeyError:
                    mc_counts.append(counts[t][1])
                    pass
            tot_counts = sum(mc_counts)
            lts[k] = 1.* tot_counts/rates[k]
            print k, format_e(tot_counts), format_e(lts[k])
        except KeyError:
            pass
        
        except ZeroDivisionError:
            lts[k] = 0

    return lts

if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("counts_file")
    parser.add_argument("rates_file")
    parser.add_argument("-to_json", type=str, default=None, dest = "jsfile")

    args = parser.parse_args()
    cs, mcm, rts, ovrds = load_files(args.counts_file, args.mc_map_file, args.rates_file, args.ngen_override_file)

    ltes =  lt_equivs(rts, mcm, cs, ovrds)
    if args.jsfile is not None:
        with open(args.jsfile, "w") as f:
            f.write(json.dumps(sorted(ltes.items(), key = lambda x : x[1], reverse = True), indent=4, separators=(',', ': ')))

    print ltes
