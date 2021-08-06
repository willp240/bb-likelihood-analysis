#ifndef __BBFIT__SystConfig__
#define __BBFIT__SystConfig__
#include <ParameterDict.h>
#include <string>

namespace bbfit{
class SystConfig{
public:

  ParameterDict GetNominal() const;
  ParameterDict GetMinima() const;
  ParameterDict GetMaxima() const;
  ParameterDict GetMass() const;
  ParameterDict GetNBins() const;
  ParameterDict GetConstrMean() const;
  ParameterDict GetConstrSigma() const;
  std::map<std::string, std::string> GetType() const;
  std::map<std::string, std::string> GetObs() const;

  const std::string& GetName() const;
  void SetName(const std::string& name_);

  void AddParameter(const std::string& name_, double nom_, double min_, double max_, double mass_, int nbins_, const std::string& obs_, const std::string& type_);
  void AddParameter(const std::string& name_, double nom_, double min_, double max_, double mass, int nbins_, double constrMean_, double constrSigma_, const std::string& obs_, const std::string& type_);

private:
  std::string fName;
  ParameterDict fNominal;
  ParameterDict fMinima;  
  ParameterDict fMaxima;
  ParameterDict fMass;
  ParameterDict fConstrMean;
  ParameterDict fConstrSigma;
  ParameterDict fNBins;
  std::map<std::string, std::string> fType;
  std::map<std::string, std::string> fObs;
};
}
#endif
