#include <FitConfigLoader.hh>
#include <ConfigLoader.hh>
#include <set>
#include <cmath>

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
  std::string outDir;

  ConfigLoader::Load("summary", "iterations", it);
  ConfigLoader::Load("summary", "burn_in", burnIn);
  ConfigLoader::Load("summary", "output_directory", outDir);
  
  ret.SetOutDir(outDir);
  ret.SetIterations(it);
  ret.SetBurnIn(burnIn);
  
  typedef std::set<std::string> StringSet;
  StringSet toLoad;
  ConfigLoader::Load("summary", "fit_dists", toLoad);
 
  std::string name;
  double min;
  double max;
  double sig;
  int    nbins;  
  for(StringSet::iterator it = toLoad.begin(); it != toLoad.end(); ++it){
    name = *it;
    ConfigLoader::Load(name, "min", min);
    ConfigLoader::Load(name, "max", max);
    ConfigLoader::Load(name, "sig", sig);
    ConfigLoader::Load(name, "nbins", nbins);
    
    ret.AddParameter(name, min, max, sig, nbins);
  }

  return ret;
}

}
