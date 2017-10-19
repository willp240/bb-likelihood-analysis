#include <TTree.h>
#include <TChain.h>

void
Clean(const std::string& infiles_, const std::string& outfile_, double eMin_, double deltaT_){
    TChain c("output");
    c.Add(infiles_);
    
    Double_t sec;
    Double_t ns;
    Double_t days;
    Double_t energy;

    c.SetBranchAddress("uTDays", &days);
    c.SetBranchAddress("uTSecs", &secs);
    c.SetBranchAddress("uTNSecs", &nsecs);
    c.SetBranchAddress("energy", &energy);

    UniversalTime thisTime;
    UniversalTime lastTime(0, 0, 0);
    UniversalTime dt(0, 0, deltaT_); // in ns
    UniversalTime diff;

    double lastEnergy;

    bool flag1;
    bool flag2;
    for(int i = 0; i < c.GetEntries(); i++){                
        c.GetEntry(i);

        // save this time
        thisTime = UniversalTime(days, secs, nsecs);

        // work out time difference
        diff = thisTime - lastTime;
        
        if(!i){        
            flag1 = true;
        }
        else{
            flag1 = (diff < dt && energy > 1 && lastEnergy > 1);
            if(!flag1 && !flag2)
                std::cout << "passes" << std::endl;
            else
                std::cout << "fails" << std::endl;
        }
        
        // save for the next round
        lastTime = thisTime;
        lastEnergy = energy;
        flag2 = flag1;
    }

}


int main(){
    
    return 0;
}
