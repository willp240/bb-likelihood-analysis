#include <string>
#include <FitConfigLoader.hh>
#include <CutConfigLoader.hh>
#include <FitConfigLoader.hh>
#include <FitConfig.hh>
#include <CutFactory.hh>
#include <CutCollection.h>
#include <fstream>
#include <ROOTNtuple.h>
#include <BinnedNLLH.h>
#include <sys/stat.h>
#include <Rand.h>
#include <AxisCollection.h>
#include <IO.h>
#include <MetropolisHastings.h>

using namespace bbfit;

void
Fit(const std::string& mcmcConfigFile_, 
    const std::string& distDir_,
    const std::string& cutConfigFile_, 
    const std::string& dataPath_,
    const std::string& outDirOverride_){
  
  // Load up the configuration data
  FitConfig mcConfig;
  
  typedef std::map<std::string, CutConfig> CutMap;
  CutMap cutConfs;

  {
    FitConfigLoader mcLoader(mcmcConfigFile_);
    mcConfig = mcLoader.LoadActive();
    
    CutConfigLoader cutConfLoader(cutConfigFile_);
    cutConfs = cutConfLoader.LoadActive();
  }

  // Make the cuts
  CutCollection cutCol;
  for(CutMap::iterator it = cutConfs.begin(); it != cutConfs.end();
      ++it){
    std::string name = it->first;
    std::string type = it->second.GetType();
    std::string obs = it->second.GetObs();
    double val = it->second.GetValue();
    double val2 = it->second.GetValue2();
    Cut *cut = CutFactory::New(name, type, obs, val, val2);
    cutCol.AddCut(*cut);
    delete cut; // cut col takes its own copy
  }
  

  // Load up the dists
  std::vector<BinnedED> dists;
  
  // the ones you actually want to fit are those listed in mcmcconfig
  typedef std::set<std::string> StringSet;
  StringSet distsToFit = mcConfig.GetParamNames();

  for(StringSet::iterator it = distsToFit.begin(); it != distsToFit.end();
      ++it){
    std::string distPath = distDir_ + "/" + *it + ".h5";
    dists.push_back(BinnedED(*it, IO::LoadHistogram(distPath)));
  }
  
  // Load up the data set
  ROOTNtuple dataToFit(dataPath_, "oxsx_saved");
    
  // now build the likelihood
  BinnedNLLH lh;
  lh.AddPdfs(dists);
  lh.SetCuts(cutCol);
  lh.SetDataSet(&dataToFit);

  // and now the optimiser
  MetropolisHastings mh;

  mh.SetMaxIter(mcConfig.GetIterations());
  mh.SetBurnIn(mcConfig.GetBurnIn());
  mh.SetMinima(mcConfig.GetMinima());
  mh.SetMaxima(mcConfig.GetMaxima());
  mh.SetSigmas(mcConfig.GetSigmas());

  // we're going to minmise not maximise -log(lh) and the 
  // mc chain needs to know that the test stat is logged 
  // otherwise it will give us the distribution of the log(lh) not the lh

  mh.SetTestStatLogged(true);
  mh.SetFlipSign(true);
  
  // create some axes for the mc to fill
  AxisCollection lhAxes;
  for(StringSet::iterator it = distsToFit.begin(); it != distsToFit.end();
      ++it){
    lhAxes.AddAxis(BinAxis(*it, mcConfig.GetMinima()[*it], 
			   mcConfig.GetMaxima()[*it],
			   mcConfig.GetNBins()[*it]		   
			   )
		   );
  }

  mh.SetHistogramAxes(lhAxes);
  
  // go
  Rand::SetSeed(0);
  FitResult res = mh.Optimise(&lh);

  // Now save the results
  std::string outDir = mcConfig.GetOutDir();
  if(outDirOverride_ != "")
    outDir = outDirOverride_;

  std::string projDir1D = outDir + "/1dlhproj";
  std::string projDir2D = outDir + "/2dlhproj";
  std::string scaledDistDir = outDir + "/scaled_dists";

  struct stat st = {0};
  if (stat(outDir.c_str(), &st) == -1) {
    mkdir(outDir.c_str(), 0700);
  }

  st = {0};
  if (stat(projDir1D.c_str(), &st) == -1) {
    mkdir(projDir1D.c_str(), 0700);
  }

  st = {0};
  if (stat(projDir2D.c_str(), &st) == -1) {
    mkdir(projDir2D.c_str(), 0700);
  }

  st = {0};
  if (stat(scaledDistDir.c_str(), &st) == -1) {
    mkdir(scaledDistDir.c_str(), 0700);
  }

  res.SaveAs(outDir + "/fit_result.txt");
  
  // save the histograms
  HistMap proj1D = res.Get1DProjections();
  HistMap proj2D = res.Get2DProjections();

  for(HistMap::iterator it = proj1D.begin(); it != proj1D.end();
      ++it){
    IO::SaveHistogram(it->second, projDir1D + "/" + it->first + ".root");
  }

  for(HistMap::iterator it = proj2D.begin(); it != proj2D.end();
      ++it){
    IO::SaveHistogram(it->second, projDir2D + "/" + it->first + ".root");
  }

  // scale the distributions to the correct heights
  // they are named the same as their fit parameters
  ParameterDict bestFit = res.GetBestFit();
  for(size_t i = 0; i < dists.size(); i++){
    std::string name = dists.at(i).GetName();
    dists[i].Normalise();
    dists[i].Scale(bestFit[name]);
    IO::SaveHistogram(dists[i].GetHistogram(), 
		      scaledDistDir + "/" + name + ".root");
  }

  // and a copy of all of the configurations used
  std::ifstream if_a(mcmcConfigFile_.c_str(), std::ios_base::binary);
  std::ifstream if_b(cutConfigFile_.c_str(),  std::ios_base::binary);
  
  std::ofstream of("log.txt", std::ios_base::binary);
  
  of << "dists from : " << distDir_
     << "\n\n\n" << "data set fit : " << dataPath_
     << "\n\n\n" << if_a.rdbuf()
     << "\n\n\n" << if_b.rdbuf();
  
}

int main(int argc, char *argv[]){
  if (argc < 4 || argc > 5){
    std::cout << "\nUsage: fit_dataset <fit_config_file> <pdf_dir> <cut_config_file> <data_to_fit> <(opt) outdir_override>" << std::endl;
      return 1;
  }

  std::string fitConfigFile(argv[1]);
  std::string pdfPath(argv[2]);
  std::string cutConfigFile(argv[3]);
  std::string dataPath(argv[4]);
  std::string outDirOverride;
  if(argc == 5)
    outDirOverride = std::string(argv[5]);

  Fit(fitConfigFile, pdfPath, cutConfigFile, dataPath, outDirOverride);

  return 0;
}
