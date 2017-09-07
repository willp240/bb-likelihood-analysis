#ifndef __BBFIT__CutConfigLoader__
#define __BBFIT__CutConfigLoader__
#include <string>
#include <CutConfig.hh>
#include <map>

namespace bbfit{
class CutConfig;
class CutConfigLoader{
public:
  CutConfigLoader(const std::string& filePath_);
  ~CutConfigLoader();
  CutConfig LoadOne(const std::string& name_) const;
  std::map<std::string, CutConfig> LoadActive() const;
  std::map<std::string, CutConfig> LoadAll() const;

private:
  std::string fPath;
};
}
#endif
