#include <TChain.h>
#include <TNtuple.h>
#include <TFile.h>
#include <string>
#include <sstream>
#include <sys/stat.h>
#include <EventConfigLoader.hh>
#include <EventConfig.hh>
#include <iostream>
using namespace bbfit;

void 
SplitInTwo(const std::string& filename, const std::string& outdir1, const std::string& outdir2, double frac){
    TFile f(filename.c_str());
    TNtuple* c = (TNtuple*)f.Get("pruned");

    Float_t energy;
    Float_t fitValid;
    Float_t reff;
    Float_t qmcdep;
    Float_t bipoCumul;
    Float_t bipolh;
    Float_t itr;

    c->SetBranchAddress("energy", &energy);
    c->SetBranchAddress("fitValid", &fitValid);
    c->SetBranchAddress("reff", &reff);
    c->SetBranchAddress("qmcdep", &qmcdep);
    c->SetBranchAddress("bipoCumul", &bipoCumul);
    c->SetBranchAddress("biPoLikelihood214", &bipolh);
    c->SetBranchAddress("itr", &itr);

    // struct stat st = {0};
    // if (stat(outdir1.c_str(), &st) == 0) {
    //     return;
    // }
    std::cout << outdir1 << std::endl;
    TFile output1(outdir1.c_str(), "RECREATE");
    TNtuple* newTree1 = new TNtuple("pruned", "", "energy:fitValid:reff:qmcdep:bipoCumul:biPoLikelihood214:itr");


    TFile output2(outdir2.c_str(), "RECREATE");
    TNtuple* newTree2 = new TNtuple("pruned", "", "energy:fitValid:reff:qmcdep:bipoCumul:biPoLikelihood214:itr");

    for(int i = 0; i < c->GetEntries(); i++){
        c->GetEntry(i);
        Float_t fillVals[] = {energy, fitValid, reff, qmcdep, bipoCumul, bipolh, itr};
        if(1. * i/c->GetEntries() < frac){
            newTree1->Fill(fillVals);
        }
        else{
            newTree2->Fill(fillVals);
        }
    }
        
    output1.cd();
    newTree1->Write();
    output1.Close();

    output2.cd();
    newTree2->Write();
    output2.Close();
}

void CreateFolder(const std::string& dirname){
    struct stat st = {0};
    if (stat(dirname.c_str(), &st) == -1) {
        mkdir(dirname.c_str(), 0700);
    }
}

int main(int argc, char* argv[]){
    if(argc != 5){
        std::cout << "Usage: ./split_half <event_config> <split_dir1> <split_dir2> frac" << std::endl;
    }
    std::string eventC(argv[1]);
    std::string splitDir1(argv[2]);
    std::string splitDir2(argv[3]);

    CreateFolder(splitDir1);
    CreateFolder(splitDir2);

    double frac;
    std::istringstream(argv[4]) >> frac;
    
    EventConfigLoader loader(eventC);
    
    typedef std::map<std::string, EventConfig>  EvMap;
    EvMap active = loader.LoadActive();
    for(EvMap::iterator it = active.begin(); it != active.end(); ++it){
        std::cout << it->first << std::endl;
        std::string output1 = splitDir1 + "/" + it->first + ".root";
        std::string output2 = splitDir2 + "/" + it->first + ".root";
        SplitInTwo(it->second.GetPrunedPath(), output1, output2, frac);
    }
    
    return 0;
}
