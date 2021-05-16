// Compile with something like
//
// g++ `root-config --cflags` -g -o MakePlots MakePlots.cpp -I`root-config --incdir` `root-config --glibs --libs`
//
//
// ./MakePlots will tell you how
//
// Should probably prettify this into something managable like classes, or at least some structs
#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <TMath.h>
#include <string>

#include "TObjArray.h"
#include "TChain.h"
#include "TFile.h"
#include "TBranch.h"
#include "TCanvas.h"
#include "TLine.h"
#include "TLegend.h"
#include "TString.h"
#include "TStyle.h"
#include "TH1.h"
#include "TF1.h"
#include "TH2.h"
#include "TGraphErrors.h"
#include "TVectorD.h"
#include "TColor.h"
#include "TKey.h"

#include <FitConfigLoader.hh>
#include <FitConfig.hh>

// Helper to plot the prefit limits

std::vector<std::string> *parNames;
std::vector<double> *parRates;
typedef std::map<std::string, double> ParMap;
ParMap constrMeans;
ParMap constrSigmas;
ParMap mins;
ParMap maxs;
ParMap asimovRates;
std::string asmvRatesFileName;
std::string mcmcConfigFile;
std::string asmvDistsFileName;
std::string scaledPostfitDistFileName;

TH1D* MakePrefit(int npar);

// Get the highest posterior density numbers from a 1D posterior
void GetHPD(TH1D * const post, double &central, double &error, double &error_pos, double &error_neg);
void GetArithmetic(TH1D * const hpost, double &mean, double &error);
void GetGaussian(TH1D *& hpost, TF1 *& gauss, double &central, double &error);

void MakePlots(std::string inputfile, bool correlations = false);
void GetParLimits(std::string paramName, double &central, double &prior, double &down_error, double &up_error);
void LoadInputVals();
void BlueRedPalette();

int BurnInCut = 5;

using namespace bbfit;

int main(int argc, char *argv[]) {
  
  if (argc != 6 && argc != 7 ) {
    std::cerr << "./MakePlots root_file_to_analyse.root asimovRatesFile asimovDistFile scaledPostfitDistFile mcmcConfigFile correlations " << std::endl;
    exit(-1);
  }

  std::string filename = argv[1];
  asmvRatesFileName = argv[2];
  asmvDistsFileName = argv[3];
  scaledPostfitDistFileName = argv[4];
  mcmcConfigFile = argv[5];

  bool correlations = false;
  if (argc == 7) {
      correlations = true;
  }

  MakePlots(filename, correlations);

  // If we want to compare two fits (e.g. binning changes or introducing new params/priors) 
  return 0;
}


void MakePlots(std::string inputFile, bool correlations) {

  // do we want to draw correlations or not
  std::cout << "File for study:       " << inputFile << std::endl;
  std::cout << "Draw correlations?    " << correlations << std::endl;

  LoadInputVals();

  // Open the chain
  TChain* chain = new TChain("posteriors","");
  chain->Add(inputFile.c_str());
  //MCMC Burn-in cut
  int cut = chain->GetMaximum("Step")/BurnInCut;
  std::stringstream ss;
  ss << "Step > " << cut;

  std::cout<<"MCMC has " << chain->GetMaximum("Step") << " steps, burn-in is set to "<< cut << std::endl;
  std::string stepcut = ss.str();

  // Get the list of branches
  TObjArray* brlis = (TObjArray*)chain->GetListOfBranches();
  // Get the number of branches
  int nbr = brlis->GetEntries();
  std::cout << "# of branches: " << nbr << std::endl;
  // Make an array of TStrings
  TString bnames[nbr];

  // Have a counter for how many of each parameter we have
  int npar = 0;

  // Loop over the number of branches
  // Find the name and how many of each systematic we have
  chain->SetBranchStatus("*", false);
  chain->SetBranchStatus("Step", true);
  for (int i = 0; i < nbr; i++) {

    // Get the TBranch and its name
    TBranch* br = (TBranch*)brlis->At(i);
    TString bname = br->GetName();

    if(bname.BeginsWith("LogL")) continue;
    if(bname.BeginsWith("Accepted")) continue;
    if(bname.BeginsWith("Step")) continue;

    chain->SetBranchStatus(bname, true);
    bnames[npar]=bname;
    npar++;
  }

  // Get first entry in chain
  chain->GetEntry(0);
 
  gStyle->SetOptFit(111);

  // Open a TCanvas to write the posterior onto
  TCanvas* c0 = new TCanvas("c0", "c0", 0, 0, 1600, 1024);
  c0->SetGrid();
  gStyle->SetOptStat(0);
  gStyle->SetOptTitle(0);
  c0->SetTickx();
  c0->SetTicky();
  c0->SetBottomMargin(0.1);
  c0->SetTopMargin(0.02);
  c0->SetRightMargin(0.1);
  c0->SetLeftMargin(0.10);

  // Make sure we can read files located anywhere and strip the .root ending
  inputFile = inputFile.substr(0, inputFile.find(".root"));

  TString canvasname = inputFile;
  // Append if we're drawing correlations
  // Open bracket means we want to make a pdf file
  if (correlations) {
    canvasname += "_plotCorrelations.pdf[";
  } else {
    canvasname += "_plotParameters.pdf[";
  }
  c0->Print(canvasname);

  // Once the pdf file is open no longer need to bracket
  canvasname.ReplaceAll("[","");

  // We fit with this Gaussian
  TF1 *gauss = new TF1("gauss","[0]/sqrt(2.0*TMath::Pi())/[2]*TMath::Exp(-0.5*pow(x-[1],2)/[2]/[2])",-5,5);
  gauss->SetLineWidth(2);
  gauss->SetLineColor(kGreen+3);

  // Some TVectors with the means, errors and Gaussian parameters of the PDFs
  TVectorD* mean_vec  = new TVectorD(npar);
  TVectorD* err_vec   = new TVectorD(npar); 
  TVectorD* gaus_mean_vec = new TVectorD(npar);
  TVectorD* gaus_err_vec  = new TVectorD(npar); 
  TVectorD* HPD_mean_vec  = new TVectorD(npar);
  TVectorD* HPD_err_p_vec   = new TVectorD(npar); 
  TVectorD* HPD_err_vec   = new TVectorD(npar); 
  TVectorD* HPD_err_m_vec   = new TVectorD(npar); 

  TMatrixT<double>* correlation = new TMatrixT<double>(npar,npar);
  for (int i = 0; i < npar; ++i) {
    for (int j = 0; j < npar; ++j) {
      (*correlation)(i,j) = 0.;
    }
  }

  // Output file to write to
  TString rootfilename = inputFile;
  if (correlations) {
    rootfilename += "_plotCorrelations.root";
  } else {
    rootfilename += "_plotParameters.root";
  }

  // The output file
  TFile* file = new TFile(rootfilename, "RECREATE");
  file->cd();
  // Make a directory with the parameters
  TDirectory *params = file->mkdir("params");
  TDirectory *corr = gDirectory;
  if(correlations) {
      corr = file->mkdir("corr");
  }

  //Now loop over all parameters
  for(int i = 0; i < npar; ++i) {

    file->cd();
    int nbins = 70;
    double asimovLine = 0.0;
    
    std::string tempString = std::string(bnames[i]);
    double central, prior, down, up;
    GetParLimits(tempString, central, prior, down, up);
    asimovLine = central;

    file->cd();
    
    // Get the maximum and minimum for the parameter
    chain->Draw(bnames[i],stepcut.c_str());
    TH1 *htemp = (TH1*)gPad->GetPrimitive("htemp");
    double maximum = 1.1*htemp->GetMaximumStored();
    double minimum = 0.9*htemp->GetMinimumStored();
    //double maximum = chain->GetMaximum(bnames[i]);
    //double minimum = chain->GetMinimum(bnames[i]);
    
    // This holds the posterior density
    TH1D *hpost = new TH1D(bnames[i], bnames[i], nbins, minimum, maximum);
    hpost->SetMinimum(0);
    hpost->GetYaxis()->SetTitle("Steps");
    hpost->GetYaxis()->SetNoExponent(false);
    hpost->SetTitle(bnames[i]);
    
    // Project bnames[i] onto hpost, applying stepcut
    chain->Project(bnames[i], bnames[i], stepcut.c_str());
    
    // Apply one smoothing
    hpost->Smooth();
    
    // Get the characteristics of the hpost
    double mean, rms;
    GetArithmetic(hpost, mean, rms);
    double peakval, sigma_p, sigma_m, sigma_hpd;
    GetHPD(hpost, peakval, sigma_hpd, sigma_p, sigma_m);
    double gauss_mean, gauss_rms;
    GetGaussian(hpost, gauss, gauss_mean, gauss_rms);
    
    std::cout << i << ": " << mean << " +/- " << rms << " (" << peakval << "+/-" << sigma_hpd << " + " << sigma_p << " - " << sigma_m << ")" << " (" << gauss_mean << "+/-" << gauss_rms << ")" << std::endl;
    
    TLine *hpd = new TLine(peakval, hpost->GetMinimum(), peakval, hpost->GetMaximum());
    hpd->SetLineColor(kRed);
    hpd->SetLineWidth(2);
    hpd->SetLineStyle(kSolid);

    // Make the legend
    TLegend *leg = new TLegend(0.12, 0.6, 0.6, 0.97);
    leg->SetTextSize(0.04);
    leg->AddEntry(hpost, Form("#splitline{PDF}{#mu = %.2f, #sigma = %.2f}", hpost->GetMean(), hpost->GetRMS()), "l");
    leg->AddEntry(gauss, Form("#splitline{Gauss}{#mu = %.2f, #sigma = %.2f}", gauss->GetParameter(1), gauss->GetParameter(2)), "l");
    leg->AddEntry(hpd, Form("#splitline{HPD}{#mu = %.2f, #sigma = %.2f (+%.2f-%.2f)}", peakval, sigma_hpd, sigma_p, sigma_m), "l");
    
    if (central != 0) {
      mean = mean / central;
      rms = rms / central;
      gauss_mean = gauss_mean / central;
      gauss_rms = gauss_rms / central;
      peakval = peakval / central;
      sigma_hpd = sigma_hpd / central;
      sigma_p = sigma_p / central;
      sigma_m = sigma_m / central;
    } else if (central == 0) {
      mean = mean + 1.0;
      gauss_mean = gauss_mean + 1.0;
      peakval = peakval + 1.0;
    }
    
    (*mean_vec)(i)      = mean;
    (*err_vec)(i)       = rms;
    (*gaus_mean_vec)(i) = gauss_mean;
    (*gaus_err_vec)(i)  = gauss_rms;
    (*HPD_mean_vec)(i)  = peakval;
    (*HPD_err_p_vec)(i) = sigma_p;
    (*HPD_err_vec)(i)   = sigma_hpd;
    (*HPD_err_m_vec)(i) = sigma_m;
    (*correlation)(i,i) = 1.0;
    
    hpost->SetLineWidth(2);
    hpost->SetLineColor(kBlack);
    hpost->SetMaximum(hpost->GetMaximum()*1.5);
    hpost->SetTitle(tempString.c_str());
    hpost->GetXaxis()->SetTitle(("Number of " + std::string(hpost->GetTitle())  + " Events").c_str());
    
    // Now make the TLine for the asimov
    TLine *asimov = new TLine(asimovLine, hpost->GetMinimum(), asimovLine, hpost->GetMaximum());
    asimov->SetLineColor(kBlue);
    asimov->SetLineWidth(2);
    asimov->SetLineStyle(kDashed);
    hpost->Draw();
    hpd->Draw("same");
    asimov->Draw("same");
    
    leg->AddEntry(asimov, Form("#splitline{Asimov}{x = %.2f}", asimovLine), "l");
    leg->SetLineColor(0);
    leg->SetLineStyle(0);
    leg->SetFillColor(0);
    leg->SetFillStyle(0);
    leg->Draw("same");
    
    // Write to file
    c0->SetName(hpost->GetName());
    c0->SetTitle(hpost->GetTitle());
    c0->Print(canvasname);
    
    // cd into params directory in root file
    file->cd();
    params->cd();
    c0->Write();
    
    // If we're interested in drawing the correlations need to invoke another for loop
    if (correlations) {

      // Loop over the other parameters to get the correlations
      for (int j = 0; j <= i; j++) {
	
	// Skip the diagonal elements which we've already done above
	if (j == i) continue;
	
	std::string tempString = std::string(bnames[j]);
	// Get the parameter name
	double central, prior, down, up;
	GetParLimits(tempString, central, prior, down, up);
	asimovLine = central;
	
	file->cd();
	
	int nbins = 70;
	
	TString drawcmd = bnames[j]+":"+bnames[i];
	std::cout << drawcmd << std::endl;
	chain->Draw(bnames[j],stepcut.c_str());
	TH1 *htemp2 = (TH1*)gPad->GetPrimitive("htemp");
	double maximum2 = 1.1*htemp2->GetMaximumStored();
	double minimum2 = 0.9*htemp2->GetMinimumStored();
	//double maximum2 = chain->GetMaximum(bnames[j]);
	//double minimum2 = chain->GetMinimum(bnames[j]);
	
	// TH2F to hold the correlation 
	TH2F *hpost2 = new TH2F(drawcmd, drawcmd, hpost->GetNbinsX(), hpost->GetBinLowEdge(0), hpost->GetBinLowEdge(hpost->GetNbinsX()+1), nbins, minimum2, maximum2);
	hpost2->SetMinimum(0);
	hpost2->GetXaxis()->SetTitle(hpost->GetXaxis()->GetTitle());
	hpost2->GetYaxis()->SetTitle( ("Number of " + std::string(bnames[j])  + " Events").c_str());
	hpost2->GetYaxis()->SetTitleOffset(1.2);
	hpost2->GetZaxis()->SetTitle("Steps");
	std::string plotTitle = hpost->GetXaxis()->GetTitle();
	plotTitle += " vs " + tempString;
	hpost2->SetTitle(plotTitle.c_str());
	
	// The draw command we want, i.e. draw param j vs param i
	chain->Project(drawcmd, drawcmd, stepcut.c_str());
	hpost2->Draw("colz");
	
	(*correlation)(i,j) = hpost2->GetCorrelationFactor();
	(*correlation)(j,i) = (*correlation)(i,j);
	std::cout << std::setw(10) << "correlation:" << (*correlation)(i,j) << std::endl;
	
	gStyle->SetPalette(51);
	c0->SetName(hpost2->GetName());
	c0->SetTitle(hpost2->GetTitle());
        c0->Print(canvasname);
	
	// Write it to root file
	file->cd();
	corr->cd();
	hpost2->Write();
	
	delete hpost2;
	
      } // End for (j = 0; j <= i; ++j)
    }	
    delete hpost;
    delete asimov;
    delete hpd;
    delete leg;
    
  } // End for npar

  // Make the prefit plot
  TH1D* prefit = MakePrefit(npar); 
  
  // cd into the output file
  file->cd();
  
  // Make a TH1D of the central values and the errors
  TH1D *paramPlot = new TH1D("paramPlot", "paramPlot", npar, 0, npar);
  paramPlot->SetName("postfitparams");
  paramPlot->SetTitle(stepcut.c_str());
  //paramPlot->SetFillStyle(3001);
  paramPlot->SetFillColor(kBlack);
  paramPlot->SetMarkerColor(paramPlot->GetFillColor());
  paramPlot->SetMarkerStyle(108);
  paramPlot->SetLineColor(paramPlot->GetFillColor());
  paramPlot->SetLineWidth(3);
  paramPlot->SetMarkerSize(prefit->GetMarkerSize());
  
  // Same but with Gaussian output
  TH1D *paramPlot_gauss = (TH1D*)(paramPlot->Clone());
  paramPlot_gauss->SetMarkerColor(kGreen+3);
  paramPlot_gauss->SetMarkerStyle(23);
  paramPlot_gauss->SetLineWidth(2);
  paramPlot_gauss->SetMarkerSize((prefit->GetMarkerSize())*0.75);
  paramPlot_gauss->SetFillColor(paramPlot_gauss->GetMarkerColor());
  paramPlot_gauss->SetFillStyle(3244);
  paramPlot_gauss->SetLineColor(paramPlot_gauss->GetMarkerColor());
  
  // Same but with Gaussian output
  TH1D *paramPlot_HPD = (TH1D*)(paramPlot->Clone());
  paramPlot_HPD->SetMarkerColor(kRed);
  paramPlot_HPD->SetMarkerStyle(25);
  paramPlot_HPD->SetLineWidth(2);
  paramPlot_HPD->SetMarkerSize((prefit->GetMarkerSize())*0.5);
  paramPlot_HPD->SetFillColor(kRed);
  paramPlot_HPD->SetFillStyle(3154);
  paramPlot_HPD->SetLineColor(paramPlot_HPD->GetMarkerColor());
  
  // Set labels and data
  for (int i = 0; i < npar; ++i) {
    paramPlot->SetBinContent(i+1, (*mean_vec)(i));
    paramPlot->SetBinError(i+1, (*err_vec)(i));
    
    paramPlot_gauss->SetBinContent(i+1, (*gaus_mean_vec)(i));
    paramPlot_gauss->SetBinError(i+1, (*gaus_err_vec)(i));
    
    paramPlot_HPD->SetBinContent(i+1, (*HPD_mean_vec)(i));
    double error = (*HPD_err_vec)(i);
    paramPlot_HPD->SetBinError(i+1, error);  
  }

  // Make a TLegend
  TLegend *CompLeg = new TLegend(0.12, 0.80, 0.5, 0.95);
  CompLeg->AddEntry(prefit, "Prior", "fp");
  CompLeg->AddEntry(paramPlot_HPD, "Postfit HPD", "fp");
  CompLeg->AddEntry(paramPlot, "Postfit PDF", "lep");
  CompLeg->AddEntry(paramPlot_gauss, "Postfit Gauss", "fp");
  CompLeg->SetFillColor(0);
  CompLeg->SetFillStyle(0);
  CompLeg->SetLineWidth(0);
  CompLeg->SetLineStyle(0);
  CompLeg->SetBorderSize(0);
  
  file->cd();
  c0->SetBottomMargin(0.2);
  
  file->cd();
  prefit->GetYaxis()->SetRangeUser(0.1, 1000);
  gPad->SetLogy();
  prefit->GetXaxis()->SetTitle("");
  prefit->GetXaxis()->SetRangeUser(0, npar);
  prefit->GetXaxis()->LabelsOption("v");
  
  paramPlot->GetXaxis()->SetRangeUser(0, npar);
  paramPlot_gauss->GetXaxis()->SetRangeUser(0, npar);
  paramPlot_HPD->GetXaxis()->SetRangeUser(0, npar);
  
  prefit->Write("param_prefit");
  paramPlot->Write("param");
  paramPlot_gauss->Write("param_gaus");
  paramPlot_HPD->Write("param_HPD");

  prefit->Draw("e2");
  paramPlot_gauss->Draw("e2, same");
  paramPlot_HPD->Draw("e2, same");
  paramPlot->Draw("same");

  CompLeg->SetX1NDC(0.33);
  CompLeg->SetX2NDC(0.80);
  CompLeg->SetY1NDC(0.20);
  CompLeg->SetY2NDC(0.50);
  
  CompLeg->Draw("same");
  c0->Write("param_canv");
  c0->Print(canvasname);
  c0->Clear();

  delete CompLeg;
  
  c0->SetLeftMargin(0.1);
  c0->SetBottomMargin(0.1);
  
  TH2D* hCorr = NULL;
  if (correlations) {
    gPad->SetLogy(0);
    // The correlation
    hCorr = new TH2D("hCorr", "hCorr", npar, 0, npar, npar, 0, npar);
    hCorr->GetZaxis()->SetTitle("Correlation");
    hCorr->SetMinimum(-1);
    hCorr->SetMaximum(1);
    hCorr->GetXaxis()->SetLabelSize(0.020);
    hCorr->GetYaxis()->SetLabelSize(0.020);
    
    // Loop over the correlation matrix entries
      for(int i=0; i < npar; i++) {

	hCorr->GetXaxis()->SetBinLabel(i+1, prefit->GetXaxis()->GetBinLabel(i+1));
	
	for (int j=0; j<npar; j++) {
	  
	  hCorr->GetYaxis()->SetBinLabel(j+1, prefit->GetXaxis()->GetBinLabel(j+1));

	  // The value of the correlation
	  double corr = (*correlation)(i,j);
	  
	  hCorr->SetBinContent(i+1, j+1, corr);
	}	
      }

      BlueRedPalette();
      /*
      // Take away the stat box
      gStyle->SetOptStat(0);
      // Make pretty correlation colors (red to blue)
      const int NRGBs = 5;
      TColor::InitializeColors();
      Double_t stops[NRGBs] = { 0.00, 0.25, 0.50, 0.75, 1.00 };
      Double_t red[NRGBs]   = { 0.00, 0.25, 1.00, 1.00, 0.50 };
      Double_t green[NRGBs] = { 0.00, 0.25, 1.00, 0.25, 0.00 };
      Double_t blue[NRGBs]  = { 0.50, 1.00, 1.00, 0.25, 0.00 };
      TColor::CreateGradientColorTable(5, stops, red, green, blue, 255);
      gStyle->SetNumberContours(255);
      */
      c0->cd();
      c0->Clear();
      hCorr->Draw("colz");
      c0->SetRightMargin(0.15);
      c0->Print(canvasname);

      file->cd();
      hCorr->Write("postfit_corr_plot");
  }

  TFile *asmvDistsFile = new TFile(asmvDistsFileName.c_str(), "READ");
  asmvDistsFile->cd();
  TH2D* prefitDist;
  TIter next(asmvDistsFile->GetListOfKeys());
  TKey *key;
  while ((key = (TKey*)next())) {
    std::string classname = std::string(key->GetClassName());
    if(classname == "TH2D")
      prefitDist = (TH2D*)key->ReadObj();
  }

  TFile *scaledPostfitDistFile = new TFile(scaledPostfitDistFileName.c_str(), "READ");
  scaledPostfitDistFile->cd();
  TH2D* postfitDist;
  TIter next2(scaledPostfitDistFile->GetListOfKeys());
  TKey *key2;

  while ((key2 = (TKey*)next2())) {
    std::string classname = std::string(key2->GetClassName());
    if(classname == "TH2D")
      postfitDist = (TH2D*)key2->ReadObj();
  }
  BlueRedPalette();
  prefitDist->Divide(postfitDist);
  prefitDist->GetZaxis()->SetRangeUser(0.7,1.3);
  prefitDist->Draw("colz");
  c0->Print(canvasname);

  // Then close the pdf file
  std::cout << "Closing pdf " << canvasname << std::endl;
  canvasname+="]";
  c0->Print(canvasname);

  // Write all the nice vectors
  file->cd();
  mean_vec->Write("postfit_params_arit");
  err_vec->Write("postfit_errors_arit");
  gaus_mean_vec->Write("postfit_params_gauss");
  gaus_err_vec->Write("postfit_errors_gauss");
  HPD_mean_vec->Write("postfit_params_HPD");
  HPD_err_vec->Write("postfit_errors_HPD");
  HPD_err_p_vec->Write("postfit_errors_HPD_pos");
  HPD_err_m_vec->Write("postfit_errors_HPD_neg");
  correlation->Write("postfit_corr");
  postfitDist->Write("postfit_dist");
  prefitDist->Write("prefit_dist");

  file->Close();
}

// Get the highest posterior density from a TH1D
void GetHPD(TH1D * const hpost, double &central, double &error, double &error_pos, double &error_neg) {

  // Get the bin which has the largest posterior density
  int MaxBin = hpost->GetMaximumBin();
  // And it's value
  double peakval = hpost->GetBinCenter(MaxBin);

  // The total integral of the posterior
  double integral = hpost->Integral();

  // Keep count of how much area we're covering
  double sum = 0.0;

  // Counter for current bin
  int CurrBin = MaxBin;
  while (sum/integral < 0.6827/2.0 && CurrBin < hpost->GetNbinsX()+1) {
    sum += hpost->GetBinContent(CurrBin);
    CurrBin++;
  }
  double sigma_p = fabs(hpost->GetBinCenter(MaxBin)-hpost->GetBinCenter(CurrBin));
  // Reset the sum
  sum = 0.0;

  // Reset the bin counter
  CurrBin = MaxBin;
  // Counter for current bin
  while (sum/integral < 0.6827/2.0 && CurrBin >= 0) {
    sum += hpost->GetBinContent(CurrBin);
    CurrBin--;
  }
  double sigma_m = fabs(hpost->GetBinCenter(CurrBin)-hpost->GetBinCenter(MaxBin));

  // Now do the double sided HPD
  sum = 0.0;
  int LowBin = MaxBin-1;
  int HighBin = MaxBin+1;
  double LowCon = 0.0;
  double HighCon = 0.0;
  while (sum/integral < 0.6827 && LowBin >= 0 && HighBin < hpost->GetNbinsX()+1) {

    // Get the slice
    LowCon = hpost->GetBinContent(LowBin);
    HighCon = hpost->GetBinContent(HighBin);

    // If we're on the last slice and the lower contour is larger than the upper
    if ((sum+LowCon+HighCon)/integral > 0.6827 && LowCon > HighCon) {
      sum += LowCon;
      break;
      // If we're on the last slice and the upper contour is larger than the lower
    } else if ((sum+LowCon+HighCon)/integral > 0.6827 && HighCon >= LowCon) {
      sum += HighCon;
      break;
    } else {
      sum += LowCon + HighCon;
    }

    LowBin--;
    HighBin++;
  }

  double sigma_hpd = 0.0;
  if (LowCon > HighCon) {
    sigma_hpd = fabs(hpost->GetBinCenter(LowBin)-hpost->GetBinCenter(MaxBin));
  } else {
    sigma_hpd = fabs(hpost->GetBinCenter(HighBin)-hpost->GetBinCenter(MaxBin));
  }

  central = peakval;
  error = sigma_hpd;
  error_pos = sigma_p;
  error_neg = sigma_m;

}

// **************************
// Get the mean and RMS of a 1D posterior
void GetArithmetic(TH1D * const hpost, double &mean, double &error) {
  // **************************
  mean = hpost->GetMean();
  error = hpost->GetRMS();
}

// **************************
// Get Gaussian characteristics
void GetGaussian(TH1D *& hpost, TF1 *& gauss, double &central, double &error) {
  // **************************

  double mean = hpost->GetMean();
  double err = hpost->GetRMS();
  double peakval = hpost->GetBinCenter(hpost->GetMaximumBin());

  // Set the range for the Gaussian fit
  gauss->SetRange(mean - 1.5*err , mean + 1.5*err);
  // Set the starting parameters close to RMS and peaks of the histograms
  gauss->SetParameters(hpost->GetMaximum()*err*sqrt(2*3.14), peakval, err);

  // Perform the fit
  hpost->Fit(gauss->GetName(),"Rq");
  hpost->SetStats(0);

  central = gauss->GetParameter(1);
  error = gauss->GetParameter(2);

}

// **************************
// Function to get limits for a parameter from the input
void LoadInputVals( ) {
  // **************************
  TFile *asmvRatesFile = new TFile(asmvRatesFileName.c_str(), "OPEN");
  asmvRatesFile->cd();
  std::map<std::string, double>* tempMap;
  asmvRatesFile->GetObject("AsimovRates",tempMap);
  asimovRates = (ParMap)*tempMap;
  //asmvRatesFile->GetObject("BackgroundRates",parRates);
  //for(int i=0; i<parNames->size(); i++){
  // std::string tempName = parNames->at(i);
  //if(tempName.find("-") != std::string::npos)
  //  tempName.replace(tempName.find("-"), 1, "_");
  //parNames->at(i) = tempName;
  //asimovRates[parNames->at(i)] = parRates->at(i);
  // }

  FitConfig mcConfig;
  FitConfigLoader mcLoader(mcmcConfigFile);
  mcConfig = mcLoader.LoadActive();

  constrMeans  = mcConfig.GetConstrMeans();
  constrSigmas = mcConfig.GetConstrSigmas();
  mins = mcConfig.GetMinima();
  maxs = mcConfig.GetMaxima();


  ParMap tempMeans;
  ParMap tempSigmas;
  ParMap tempMins;
  ParMap tempMaxs;
  ParMap tempRates;

  for(ParameterDict::iterator it = mins.begin(); it != mins.end(); ++it){
    std::string tempName = it->first;
    if(tempName.find("-") != std::string::npos)
      tempName.replace(tempName.find("-"), 1, "_");
    tempMins[tempName] = it->second;
    tempMaxs[tempName] = maxs[it->first];
    tempSigmas[tempName] = constrSigmas[it->first];
    tempMeans[tempName] = constrMeans[it->first];
    tempRates[tempName] = asimovRates[it->first];
  }

  constrMeans = tempMeans;
  constrSigmas = tempSigmas;
  mins = tempMins;
  maxs = tempMaxs;
  asimovRates = tempRates;
}

// **************************
// Function to get limits for a parameter from the input
void GetParLimits(std::string paramName, double &central, double &prior, double &down_error, double &up_error) {
  // **************************

  central = 0.0;
  double error = 0.0;
  double sigmas = 1.0;

  central = asimovRates[paramName];
  prior = constrMeans[paramName];
  error = constrSigmas[paramName];

  // We might be passed the valid range of parameter
  // Do a check to see if this is true
  if (central - error <  mins[paramName]) {
    down_error = mins[paramName];
  } else {
    down_error = central - error;
  }

  if (central + error > maxs[paramName]) {
    up_error = maxs[paramName];
  } else {
    up_error = central + error;
  }
}

// *****************************
// Make the prefit plots
TH1D* MakePrefit(int nPar) {
  // *****************************
  
  TH1D *PreFitPlot = new TH1D("Prefit", "Prefit", nPar, 0, nPar);
  for (int i = 0; i < PreFitPlot->GetNbinsX() + 1; ++i) {
    PreFitPlot->SetBinContent(i+1, 0);
    PreFitPlot->SetBinError(i+1, 0);
  }

  int count = 0;
  for(ParameterDict::iterator it = mins.begin(); it != mins.end(); ++it){
    if(asimovRates[it->first] != 0) {
      PreFitPlot->SetBinContent(count+1, asimovRates[it->first]/asimovRates[it->first]);
      PreFitPlot->GetXaxis()->SetBinLabel(count+1, it->first.c_str());
      if(constrMeans[it->first] != 0){
	PreFitPlot->SetBinContent(count+1, constrMeans[it->first]/asimovRates[it->first]);
	PreFitPlot->SetBinError(count+1, constrSigmas[it->first]/asimovRates[it->first]);
      }
    }

    else {
      PreFitPlot->SetBinContent(count+1, asimovRates[it->first]+1);
      PreFitPlot->GetXaxis()->SetBinLabel(count+1, it->first.c_str());
      if(constrMeans[it->first] != 0){
        PreFitPlot->SetBinContent(count+1, constrMeans[it->first]+1);
        PreFitPlot->SetBinError(count+1, constrSigmas[it->first]);
      }
    }
    //    PreFitPlot->SetBinContent(count+1, 1.0);
    //PreFitPlot->SetBinError(count+1, 0.01);
    count++;
  }
  
  PreFitPlot->GetYaxis()->SetTitle("Number of Events Relative to Asimov");

  PreFitPlot->SetDirectory(0);

  PreFitPlot->SetFillStyle(1001);
  PreFitPlot->SetFillColor(kBlue);
  PreFitPlot->SetMarkerStyle(21);
  PreFitPlot->SetMarkerSize(1.0);
  PreFitPlot->SetMarkerColor(kWhite);
  PreFitPlot->SetLineColor(kBlue);

  PreFitPlot->GetXaxis()->LabelsOption("v");

  return PreFitPlot;
}


void BlueRedPalette() {

  // Take away the stat box
  gStyle->SetOptStat(0);
  // Make pretty correlation colors (red to blue)
  const int NRGBs = 5;
  TColor::InitializeColors();
  Double_t stops[NRGBs] = { 0.00, 0.25, 0.50, 0.75, 1.00 };
  Double_t red[NRGBs]   = { 0.00, 0.25, 1.00, 1.00, 0.50 };
  Double_t green[NRGBs] = { 0.00, 0.25, 1.00, 0.25, 0.00 };
  Double_t blue[NRGBs]  = { 0.50, 1.00, 1.00, 0.25, 0.00 };
  TColor::CreateGradientColorTable(5, stops, red, green, blue, 255);
  gStyle->SetNumberContours(255);
}
