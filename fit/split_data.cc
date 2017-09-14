#include <EventConfigLoader.hh>
#include <EventConfig.hh>
#include <iostream>
#include <IO.h>
#include <ROOTNtuple.h>
#include <DataSetGenerator.h>
#include <OXSXDataSet.h>
#include <sstream>
#include <fstream>
#include <Formatter.hpp>
#include <ConfigLoader.hh>
#include <sys/stat.h>

using namespace bbfit;

void
MakeDataSets(const std::string& configFile_, double liveTime_, int nDataSets_){
  // load up the rates of the different event types
  EventConfigLoader loader(configFile_);

  typedef std::map<std::string, EventConfig>  EvMap;
  EvMap active = loader.LoadActive();
 
  // pull out the useful parts
  std::vector<std::string> names;
  std::vector<double> rates;
  std::vector<DataSet*> dataSets;

  for(EvMap::iterator it = active.begin(); it != active.end(); ++it){
    names.push_back(it->first);
    DataSet* ds = new ROOTNtuple(it->second.GetPrunedPath(), "pruned");
    dataSets.push_back(ds);
    // note the correction for the number of events generated e.g scintEdep cut
    rates.push_back(it->second.GetRate() * ds->GetNEntries()/1./it->second.GetNGenerated());
  }

  DataSetGenerator dsGen;
  dsGen.SetDataSets(dataSets);
  dsGen.SetExpectedRates(rates);
  
  std::string outDir;
  ConfigLoader::Open(configFile_);
  ConfigLoader::Load("summary", "split_ntup_dir", outDir);
  ConfigLoader::Close();

  struct stat st = {0};
  if (stat(outDir.c_str(), &st) == -1) {
    mkdir(outDir.c_str(), 0700);
  }

  // actually generate the events
  std::vector<int> content;
  for(int iSet = 0; iSet < nDataSets_; iSet++){
    std::string outPath = Formatter() << outDir << "/fake_data_lt_" << liveTime_ << "__" << iSet;

    OXSXDataSet ds = dsGen.PoissonFluctuatedDataSet(&content);
    std::ofstream fs;
    fs.open((outPath + ".txt").c_str());
    for(size_t i = 0; i < names.size(); i++)
      fs << names.at(i) << "\t" << content.at(i) << "\n";
    fs.close();
    
    // save to a new ROOT tree
    IO::SaveDataSet(ds, outPath + ".root");
  }

  // save what's left over as independent data sets
  std::vector<OXSXDataSet*> remainders = dsGen.AllRemainingEvents(&content);
  std::ofstream fs;
  fs.open((outDir + "/mc_remainder.txt").c_str());
  for(size_t i = 0; i < names.size(); i++)
    fs << names.at(i) << "\t" << content.at(i) << "\n";
  fs.close();
  
  for(size_t i = 0; i < remainders.size(); i++){
    IO::SaveDataSet(*remainders.at(i), Formatter() << outDir << "/" << names.at(i) << ".root");
  }

}

int main(int argc, char *argv[]){
  if(argc != 3){
    std::cout << "\nUsage: ./split_data <event_config_file> <livetime> <n_data_sets>" << std::endl;
    return 1;
  }

  std::string configFile(argv[1]);
  int nDataSets;
  std::istringstream(argv[3]) >> nDataSets;

  double liveTime;
  std::istringstream(argv[2]) >> liveTime;

  MakeDataSets(configFile, liveTime, nDataSets);
   
  return 0;
}
