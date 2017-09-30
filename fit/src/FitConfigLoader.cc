#include <FitConfigLoader.hh>
#include <ConfigLoader.hh>
#include <set>
#include <cmath>
#include <algorithm>
#include <Exceptions.h>

namespace bbfit{

FitConfigLoader::FitConfigLoader(const std::string& filePath_){
    fPath = filePath_;    
}

FitConfigLoader::~FitConfigLoader(){ 
  ConfigLoader::Close();
}

FitConfig
FitConfigLoader::LoadActive() const{
  FitConfig ret;
  ConfigLoader::Open(fPath);
  int it;
  int burnIn;
  int nSteps;
  double epsilon;
  std::string outDir;
  std::string dataSet;

  ConfigLoader::Load("summary", "iterations", it);
  ConfigLoader::Load("summary", "burn_in", burnIn);
  ConfigLoader::Load("summary", "output_directory", outDir);
  ConfigLoader::Load("summary", "n_steps", nSteps);
  ConfigLoader::Load("summary", "epsilon", epsilon);

  ret.SetOutDir(outDir);
  ret.SetNSteps(nSteps);
  ret.SetEpsilon(epsilon);
  ret.SetIterations(it);
  ret.SetBurnIn(burnIn);
  
  typedef std::set<std::string> StringSet;
  StringSet toLoad;
  ConfigLoader::Load("summary", "fit_dists", toLoad);
 
  std::string name;
  double min;
  double max;
  double sig;
  double constrMean;
  double constrSigma;
  int    nbins;  

  if(std::find(toLoad.begin(), toLoad.end(), "all") != toLoad.end()){
      toLoad = ConfigLoader::ListSections();
      toLoad.erase("summary");
  }
  
  for(StringSet::iterator it = toLoad.begin(); it != toLoad.end(); ++it){
    name = *it;
    ConfigLoader::Load(name, "min", min);
    ConfigLoader::Load(name, "max", max);
    ConfigLoader::Load(name, "sig", sig);
    ConfigLoader::Load(name, "nbins", nbins);

    try{
        ConfigLoader::Load(name, "constraint_mean", constrMean);
        ConfigLoader::Load(name, "constraint_sigma", constrSigma);
        ret.AddParameter(name, min, max, sig, nbins, constrMean, constrSigma);
    }
    catch(const ConfigFieldMissing& e_){
        ret.AddParameter(name, min, max, sig, nbins);
    }
  }

  return ret;
}

}
