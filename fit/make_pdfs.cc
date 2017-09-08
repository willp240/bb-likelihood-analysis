#include <DistConfigLoader.hh>
#include <DistConfig.hh>
#include <DistBuilder.hh>
#include <EventConfig.hh>
#include <EventConfigLoader.hh>
#include <iostream>
#include <sys/stat.h>
#include <BinnedED.h>
#include <DistFiller.h>
#include <ROOTNtuple.h>
#include <IO.h>
#include <DistTools.h>
#include <TH1D.h>
#include <TH2D.h>
#include <CutConfig.hh>
#include <CutConfigLoader.hh>
#include <CutCollection.h>
#include <CutFactory.hh>
#include <CutLog.h>
#include <HistTools.h>

using namespace bbfit;

int main(int argc, char *argv[]){
  if (argc != 4){
    std::cout << "Usage: make_trees <event_config_file> <pdf_config_file> <cut_config_file>" << std::endl;
    return 1;
  }
    
  std::string evConfigFile(argv[1]);
  std::string pdfConfigFile(argv[2]);
  std::string cutConfigFile(argv[3]);


  std::cout << "Reading from config files "  
	    << evConfigFile << ", "  << pdfConfigFile << std::endl;

    
  // load up the pdf configuration data too
  DistConfigLoader pLoader(pdfConfigFile);
  DistConfig pConfig = pLoader.Load();
 
  // create the pdf directory if it doesn't already exist
  std::string pdfDir = pConfig.GetPDFDir();
  struct stat st = {0};
  if (stat(pdfDir.c_str(), &st) == -1) {
    mkdir(pdfDir.c_str(), 0700);
  }

  // and another one for the projections - there will be loads
  std::string projDir = pdfDir + "/projections";
  st = {0};
  if (stat(projDir.c_str(), &st) == -1) {
    mkdir(projDir.c_str(), 0700);
  }


  // load up all the event types we want pdfs for
  typedef std::map<std::string, EventConfig> EvMap;
  EventConfigLoader loader(evConfigFile);
  EvMap toGet = loader.LoadActive();


  // create the cuts
  typedef std::map<std::string, CutConfig> CutMap;
  CutConfigLoader cutConfLoader(cutConfigFile);
  CutMap cutConfs = cutConfLoader.LoadActive();
  
  CutCollection cutCol;
  for(CutMap::iterator it = cutConfs.begin(); it != cutConfs.end();
      ++it){
    std::string name = it->first;
    std::string type = it->second.GetType();
    std::string obs = it->second.GetObs();
    double val = it->second.GetValue();
    double val2 = it->second.GetValue();
    Cut *cut = CutFactory::New(name, type, obs, val, val2);
    cutCol.AddCut(*cut);
    delete cut; // cut col takes its own copy
  }
 
  // now make and fill the pdfs
  for(EvMap::iterator it = toGet.begin(); it != toGet.end(); ++it){    
    // monitor the effect of the cuts
    CutLog log(cutCol.GetCutNames());

    // find the dataset
    DataSet* dataSet = new ROOTNtuple(it->second.GetPrunedPath(), "pruned");
    
    // create and fill
    BinnedED dist = DistBuilder::Build(it->first, pConfig, dataSet);
    DistFiller::FillDist(dist, *dataSet, cutCol, log);
    
    delete dataSet;

    // normalise
    dist.Normalise();

    // save a copy of the cut log
    log.SaveAs(it->first, pdfDir + it->first + ".txt");

    // save as hdf5 obj and root 
    // 1D, just save them
    if(dist.GetNDims() == 1)
      DistTools::ToTH1D(dist).SaveAs((pdfDir + it->first + ".root").c_str());

    // HigherD save the projections
    else{
      std::vector<BinnedED> projs = HistTools::GetVisualisableProjections(dist);

      // save them as apropriate
      for(size_t i = 0; i < projs.size(); i++){
	const BinnedED& proj = projs.at(i);

	if(projs.at(i).GetNDims() == 1)
	  DistTools::ToTH1D(proj).SaveAs((projDir + proj.GetName() + ".root").c_str());
	else
	  DistTools::ToTH2D(proj).SaveAs((projDir + proj.GetName() + ".root").c_str());
      }
    }

    IO::SaveHistogram(dist.GetHistogram(), pdfDir + it->first + ".h5");
  }

  return 0;
}
