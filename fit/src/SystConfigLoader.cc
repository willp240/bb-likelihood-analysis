#include <SystConfigLoader.hh>
#include <SystConfig.hh>
#include <ConfigLoader.hh>
#include <map>
#include <algorithm>

namespace bbfit{

SystConfigLoader::SystConfigLoader(const std::string& filePath_){
    fPath = filePath_;    
}

SystConfig
SystConfigLoader::LoadActive() const{
  SystConfig ret;
  ConfigLoader::Open(fPath);
 
  typedef std::set<std::string> StringSet;
  StringSet toLoad;
  ConfigLoader::Load("summary", "active", toLoad);

  std::string name;
  double      nom;
  double      min;
  double      max;
  double      mass;
  double      constrMean;
  double      constrSigma;
  int         nbins;
  std::string obs;
  std::string type;

  if(std::find(toLoad.begin(), toLoad.end(), "all") != toLoad.end()){
    toLoad = ConfigLoader::ListSections();
    toLoad.erase("summary");
  }

  for(StringSet::iterator it = toLoad.begin(); it != toLoad.end(); ++it){
    name = *it;
    ConfigLoader::Load(name, "nominal", nom);
    ConfigLoader::Load(name, "minima", min);
    ConfigLoader::Load(name, "maxima", max);
    ConfigLoader::Load(name, "mass", mass);
    ConfigLoader::Load(name, "nbins", nbins);
    ConfigLoader::Load(name, "obs", obs);
    ConfigLoader::Load(name, "type", type);

    try{
      ConfigLoader::Load(name, "constraint_mean", constrMean);
      ConfigLoader::Load(name, "constraint_sigma", constrSigma);
      ret.AddParameter(name, nom, min, max, mass, nbins, constrMean, constrSigma, obs, type);
    }
    catch(const ConfigFieldMissing& e_){
      ret.AddParameter(name, nom, min, max, mass, nbins, obs, type);
    }
  }

  return ret;
}

SystConfigLoader::~SystConfigLoader(){ 
  ConfigLoader::Close();
}

}
