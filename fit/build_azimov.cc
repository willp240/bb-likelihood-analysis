// load up the PDFs and scale them to their expected rates, produces a histogram of the binned azimov data se
#include <EventConfigLoader.hh>
#include <DistConfigLoader.hh>
#include <DistConfig.hh>
#include <EventConfig.hh>
#include <ROOTNtuple.h>
#include <IO.h>
#include <BinnedED.h>
#include <AxisCollection.h>
#include <DistBuilder.hh>
#include <CutConfig.hh>
#include <CutConfigLoader.hh>
#include <CutCollection.h>
#include <CutFactory.hh>
#include <CutLog.h>
#include <string>
#include <sys/stat.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <TFile.h>

using namespace bbfit;

void
BuildAzimov(const std::string& evConfigFile_, 
            const std::string& pdfConfigFile_, 
            const std::string& cutConfigFile_,
            double liveTime_, const std::string& outName_, bool loadPDF_, 
            double nGenScale_, double loadingScale_){

    // load up the pdf configuration data
    DistConfigLoader dLoader(pdfConfigFile_);
    DistConfig pConfig = dLoader.Load();

    // load up all the event types we want to contribute
    typedef std::map<std::string, EventConfig> EvMap;
    EventConfigLoader loader(evConfigFile_);
    EvMap toGet = loader.LoadActive();

    // create the cuts
    typedef std::vector<CutConfig> CutVec;
    CutConfigLoader cutConfLoader(cutConfigFile_);
    CutVec cutConfs = cutConfLoader.LoadActive();

    CutCollection cutCol;
    for(CutVec::iterator it = cutConfs.begin(); it != cutConfs.end();
        ++it){
        std::string name = it->GetName();
        std::string type = it->GetType();
        std::string obs = it->GetObs();
        double val = it->GetValue();
        double val2 = it->GetValue2();
        Cut *cut = CutFactory::New(name, type, obs, val, val2);
        cutCol.AddCut(*cut);
        delete cut; // cut col takes its own copy
    }
    CutLog log(cutCol.GetCutNames());

    struct stat st = {0};
    if (stat(outName_.c_str(), &st) == -1) {
      mkdir(outName_.c_str(), 0700);
    }

    // create the empty dist
    BinnedED azimov;
    std::vector<BinnedED> indivAsmvDists;
    bool setAxes = false;
    if(!loadPDF_){
        AxisCollection axes = DistBuilder::BuildAxes(pConfig);
        azimov = BinnedED("azimov", axes);
        setAxes = true;
    }

    std::vector<std::string> names;
    std::vector<double> rates;

    // now build each of the PDFs, scale them to the correct size and add it to the azimov
    for(EvMap::iterator it = toGet.begin(); it != toGet.end(); ++it){
        DataSet* ds = new ROOTNtuple(it->second.GetSplitFakePath(), "pruned");
        BinnedED dist;
        dist = DistBuilder::Build(it->first, pConfig, ds, cutCol, log);
        unsigned long nGen = ds->GetNEntries();
        if(it->second.GetNGenerated()){
            nGen = it->second.GetNGenerated();
            // perhaps you have already spilt the data in half
            // so the number you want doesn't correpond to the number in the config file
            // in that case nGenScale should equal 0.5
            nGen *= nGenScale_;
        }

        if(!it->second.GetRate())
            continue;
        std::cout << "Integral before scale = " << dist.Integral() << std::endl;
        std::cout << "Efficiency  = " << dist.Integral()/nGen << std::endl;
        
	double rate = it->second.GetRate();
	if (it->second.GetLoadingScaling() == "true"){
	  std::cout<< "Scaling rate by "<<  loadingScale_ << " due to higher loading" <<std::endl;
	  rate *=loadingScale_;
	}
	dist.Scale(liveTime_ * rate/nGen);
        double nanCheck = rate/nGen;

        std::cout << liveTime_ << "\t" << rate << "\t" << nGen << std::endl;
        if(dist.Integral() == dist.Integral()){
	  if(!loadPDF_){
                azimov.Add(dist);
		indivAsmvDists.push_back(dist);
	  }
	  std::cout << "Added " << dist.Integral() << " of event type " << it->first << std::endl;
        }
        else{
            std::cout << "Skipped " << it->first << std::endl;
            continue;
        }
        if(!dist.Integral()){
            std::cout << "Skipped" << it->first << std::endl;
            continue;
        }

        
        if(loadPDF_){
            std::string distDir = pConfig.GetPDFDir();
            std::string distPath = distDir + "/" + it->first + ".h5";
            std::cout << "Loading histogram from "<< distPath << std::endl;
            BinnedED highDdist = BinnedED(it->first, IO::LoadHistogram(distPath));
            if(!setAxes){
                azimov = BinnedED("azimov", highDdist.GetAxes());
                setAxes = true;
            }
            highDdist.Scale(dist.Integral()/highDdist.Integral());
            if(highDdist.Integral() != highDdist.Integral()){
                std::cout << "ahhhh! " << it->first << "\t" 
                          << dist.Integral() << std::endl;
            }
            for(size_t is = 0; is < highDdist.GetNDims(); is++)
                std::cout << highDdist.GetAxes().GetAxis(is).GetNBins() << std::endl;
            azimov.Add(highDdist);
        }

        delete ds;
    }    
    
    IO::SaveHistogram(azimov.GetHistogram(), outName_ + ".h5");
    std::map < std::string, double > asimovmap;
    if(azimov.GetNDims() < 3){
        IO::SaveHistogram(azimov.GetHistogram(), outName_ + ".root");
	for(size_t i = 0; i < indivAsmvDists.size(); i++){
	  std::string name = indivAsmvDists.at(i).GetName();
	  IO::SaveHistogram(indivAsmvDists[i].GetHistogram(),
                            outName_ + "/" + name + ".root");
	  asimovmap[name] = indivAsmvDists[i].GetHistogram().Integral();
	}
	TFile *fRates = TFile::Open((outName_+"/asimovRates.root").c_str(), "RECREATE");
	fRates->WriteObject(&asimovmap, "AsimovRates");
	fRates->Close();
    }
    return;
}


int main(int argc, char* argv[]){
    if(argc != 7 && argc != 8 && argc != 9){
        std::cout << "Usage: ./build_azimov <event_config_file> <pdf_config_file> <cut_config_file> <live_time(yr)> <out_file(no ext)> <load_from_pdf(0 or 1)> <nGen scaling (optional)> <loading scale (optional)>"
                  << std::endl;
        return 1;
    }
    std::string evConfigFile(argv[1]);
    std::string pdfConfigFile(argv[2]);
    std::string cutConfigFile(argv[3]);
    std::string outName(argv[5]);
    double liveTime;
    bool  loadPDF;
    std::istringstream(argv[4]) >> liveTime;
    std::istringstream(argv[6]) >> loadPDF;
    
    double nGenScale = 1;
    if((argc == 8) || (argc == 9))
        std::istringstream(argv[7]) >> nGenScale;

    double loadingScale = 1;
    if(argc == 9)
      std::istringstream(argv[8]) >> loadingScale;

    BuildAzimov(evConfigFile, pdfConfigFile, cutConfigFile, liveTime, outName, loadPDF, nGenScale, loadingScale);
    return 0;
}
