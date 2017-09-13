#ifndef __BBFIT__FitConfig__
#define __BBFIT__FitConfig__
#include <ParameterDict.h>
#include <string>
#include <set>

namespace bbfit{
class FitConfig{
public:
  ParameterDict GetMinima() const;
  ParameterDict GetMaxima() const;
  ParameterDict GetSigmas() const;
  ParameterDict GetNBins() const;
  
  int  GetIterations() const;
  void SetIterations(int);

  int  GetBurnIn() const;
  void SetBurnIn(int);

  void AddParameter(const std::string& name_, double min_, double max_, double sigma_, int nbins_);

  std::set<std::string> GetParamNames() const;
  
  const std::string& GetOutDir() const;
  void  SetOutDir(const std::string&);

private:
  std::string   fOutDir;
  ParameterDict fMinima;
  ParameterDict fMaxima;
  ParameterDict fSigmas;
  ParameterDict fNbins;
  int       fIterations;
  int       fBurnIn;
};
}
#endif
