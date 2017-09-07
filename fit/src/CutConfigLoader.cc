#include <CutConfigLoader.hh>
#include <CutConfig.hh>
#include <ConfigLoader.hh>
#include <map>
#include <algorithm>

namespace bbfit{

CutConfigLoader::CutConfigLoader(const std::string& filePath_){
    fPath = filePath_;    
}

CutConfig
CutConfigLoader::LoadOne(const std::string& name_) const{
  ConfigLoader::Open(fPath);

  double value;
  std::string type;
  std::string obs;

  ConfigLoader::Load(name_, "value", value);
  ConfigLoader::Load(name_, "type", type);
  ConfigLoader::Load(name_, "obs", obs);

  CutConfig retVal;
  retVal.SetName(name_);
  retVal.SetValue(value);
  retVal.SetType(type);
  retVal.SetObs(obs);
  return retVal;
}

std::map<std::string, CutConfig>
CutConfigLoader::LoadActive() const{
  typedef std::vector<std::string> StringVec;
  typedef std::map<std::string, CutConfig> CutConfigMap;
  ConfigLoader::Open(fPath);

  StringVec toLoad;
  ConfigLoader::Load("summary", "order", toLoad);

  //  if all is in the list, just do all of them
  if(std::find(toLoad.begin(), toLoad.end(), "all") != toLoad.end())
    return LoadAll();

  CutConfigMap evMap;
  for(size_t i = 0; i < toLoad.size(); i++)
    evMap[toLoad.at(i)] = LoadOne(toLoad.at(i));

  return evMap;
}

CutConfigLoader::~CutConfigLoader(){ 
  ConfigLoader::Close();
}

std::map<std::string, CutConfig>
CutConfigLoader::LoadAll() const{
  typedef std::set<std::string> StringSet;
  StringSet toLoad = ConfigLoader::ListSections();
  toLoad.erase("summary");

  std::map<std::string, CutConfig> evMap;
  for(StringSet::iterator it = toLoad.begin(); it != toLoad.end();
      ++it)
    evMap[*it] = LoadOne(*it);

  return evMap;
}

}
