import ROOT
import ConfigParser

class Grouper:
    def __init__(self):
        self.hists = {}

    def add(self, hist, group):
        try:
            self.hists[group].Add(hist)
        except KeyError:
            self.hists[group] = hist

def assign_group(parser, name):
    try:
        return parser.get(name, "plot_group")
    except ConfigParser.NoOptionError as e:
        return name

def grab_hist(filename):
    f = ROOT.TFile(filename)
    h = f.Get(f.GetListOfKeys()[0].GetName())
    h.SetDirectory(0)
    return h

def stack(order, hists, labels, colors):
    stack = ROOT.THStack("fit", "")
    leg   = ROOT.TLegend(0.75, 0.65, 0.95, 0.95)
    for name in order:
        try:
            stack.Add(hists[name])
        except KeyError as e:
            print "Warning: found no hists for group " + name
            continue
        t_col = ROOT.TColor.GetColor("#{0}".format(colors[name]))
        hists[name].SetLineColor(t_col)
        hists[name].SetFillColor(t_col)
        
    for name in reversed(order):
        try:
            leg.AddEntry(hists[name], labels[name], "FL")
        except KeyError as e:
            pass
    return stack, leg

if __name__ == "__main__":
    import argparse
    import glob
    import os
    parser = argparse.ArgumentParser()
    parser.add_argument("plot_config", type = str)
    parser.add_argument("event_config", type = str)
    parser.add_argument("result_dir", type=str)
    
    args = parser.parse_args()
    data  = grab_hist(os.path.join(args.result_dir, "scaled_dists", "data.root"))
    data.Sumw2()

    # read 
    scaled_dists_p = [ x for x in glob.glob(os.path.join(args.result_dir, "scaled_dists", "*.root")) if not x.endswith("data.root")]
    names = [os.path.basename(x).split(".root")[0] for x in scaled_dists_p]

    scaled_dists = {}
    for name, path in zip(names, scaled_dists_p):
        scaled_dists[name] = grab_hist(path)

    # read from the config file if these guys are in a group
    parser = ConfigParser.ConfigParser()
    parser.read(args.event_config)
    groups = dict((x, assign_group(parser, x)) for x in names)
    tex_labels = dict((x, parser.get(x, "tex_label")) for x in names)

    # filter them into their groups
    grouper = Grouper()
    for name in names:
        grouper.add(scaled_dists[name], groups[name])

    # read the plot config
    parser = ConfigParser.ConfigParser()
    parser.read(args.plot_config)

    for gp in grouper.hists.keys():
        try:
            tex_labels[gp] = parser.get(gp, "tex_label")
        except ConfigParser.NoOptionError as e:
            if not gp in tex_labels.keys():
                tex_labels[gp] = gp

        except ConfigParser.NoSectionError as e:
            pass

    colors = dict((name, parser.get(name, "color")) for name in grouper.hists.keys())
    order  = parser.get("summary", "plot_order").split(",")
    x_title  = parser.get("titles", "x_axis")
    y_title  = parser.get("titles", "y_axis")
    
    stack_, leg = stack(order, grouper.hists, tex_labels, colors)

    # draw it
    can =  ROOT.TCanvas()
    stack_.Draw()
    stack_.GetXaxis().SetTitle(x_title)
    stack_.GetYaxis().SetTitle(y_title)
    stack_.Draw()
    data.Draw("same PE")
    leg.AddEntry(data, "Data", "PE")
    leg.Draw("same")
    can.SaveAs(os.path.join(args.result_dir, "stacked_fit.root"))
   

    # now work out a chi square
    s_hist = stack_.GetStack().Last()
    chi_square = 0
    for i in xrange(1, s_hist.GetNbinsX() + 1):
        try:
            chi_square += (s_hist.GetBinContent(i) -  data.GetBinContent(i)) ** 2 / s_hist.GetBinContent(i)
        except ZeroDivisionError as e:
            print "zero probability bin! #", i, "  @ ", s_hist.GetXaxis().GetBinCenter(i)
    

    print "\n\nchisq/ndf = ", chi_square, " / ", len(scaled_dists)
    print "p-value = ", ROOT.TMath.Prob(chi_square, len(scaled_dists))
