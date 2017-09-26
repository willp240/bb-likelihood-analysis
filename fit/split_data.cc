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
#include <Rand.h>
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
  std::vector<bool> flags;

  for(EvMap::iterator it = active.begin(); it != active.end(); ++it){
    DataSet* ds = new ROOTNtuple(it->second.GetPrunedPath(), "pruned");
    // note the correction for the number of events generated e.g scintEdep cut
    double expectedCounts = it->second.GetRate() * liveTime_;
    
    if(it->second.GetNGenerated())
      expectedCounts *= ds->GetNEntries()/double(it->second.GetNGenerated());

    if(!expectedCounts)
      std::cout << "\n(" << ds->GetNEntries() << "\t" << it->second.GetNGenerated() << "\t" << liveTime_ << "\t" << it->second.GetRate() << ")\n" << std::endl;

    if(!ds->GetNEntries()){
      std::cout << "Warning:: skipping " << it->first << "  no events to choose from" << std::endl;
      continue;
    }

    Rand::SetSeed(0);

    dataSets.push_back(ds);
    names.push_back(it->first);
    rates.push_back(expectedCounts);
    flags.push_back(!it->second.GetRandomSplit());
    // 
    std::cout << "Loading data set " 
	      << it->second.GetPrunedPath() 
	      << " with expected counts "
	      << expectedCounts
	      << "\n";
    if(it->second.GetRandomSplit())
      std::cout << "random split ";
    else
      std::cout << "sequential split";    	
    std::cout << std::endl;
  }

  DataSetGenerator dsGen;
  dsGen.SetDataSets(dataSets);
  dsGen.SetExpectedRates(rates);
  dsGen.SetSequentialFlags(flags);
  std::string outDir;
  ConfigLoader::Open(configFile_);
  ConfigLoader::Load("summary", "split_ntup_dir", outDir);
  ConfigLoader::Close();

  struct stat st = {0};
  if (stat(outDir.c_str(), &st) == -1) {
    mkdir(outDir.c_str(), 0700);
  }

  std::cout << "Generating " << nDataSets_ << " data sets with livetime " << liveTime_
	    << "  including poisson fluctuations...\n" << std::endl;

  // actually generate the events
  std::vector<int> content;
  for(int iSet = 0; iSet < nDataSets_; iSet++){
    std::cout << "DataSet #" << iSet << std::endl;
    std::string outPath = Formatter() << outDir << "/fake_data_lt_" << liveTime_ << "__" << iSet;

    OXSXDataSet ds = dsGen.PoissonFluctuatedDataSet(&content);
    std::ofstream fs;
    fs.open((outPath + ".txt").c_str());
    for(size_t i = 0; i < names.size(); i++)
      fs << names.at(i) << "\t" << content.at(i) << "\n";
    fs.close();

    // save to a new ROOT tree
    IO::SaveDataSet(ds, outPath + ".root");

    std::cout << "\t .. written " << ds.GetNEntries() << " events to "  << outPath + ".root"
	      << "\t with logfile " << outPath + ".txt\n\n" << std::endl;
  }

  // save what's left over as independent data sets
  OXSXDataSet* remainder = NULL;
  int countsTaken = 0;
  content.clear();
  for(int iSet = 0; iSet < dataSets.size(); iSet++){
    std::cout << "Assembling the remainder for  " << names.at(iSet) << std::endl;

    remainder = dsGen.AllRemainingEvents(iSet, &countsTaken);
    content.push_back(countsTaken);
    
    std::cout << "\t.. and saving" << std::endl;
    IO::SaveDataSet(*remainder, Formatter() << outDir << "/" << names.at(iSet) << ".root");
    delete remainder;
  }

  std::ofstream fs;
  fs.open((outDir + "/mc_remainders.txt").c_str());
  for(size_t i = 0; i < names.size(); i++)
    fs << names.at(i) << "\t" << content.at(i) << "\n";
  fs.close();  
  
  std::cout << "\n\n Left over events written to  " << outDir
	    << "\t with logfile " << outDir + "mc_remainders.txt" << std::endl;    
}

int main(int argc, char *argv[]){
  if(argc != 4){
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
