/*=========================================================================
 *
 *  Copyright 2011-2013 The University of North Carolina at Chapel Hill
 *  All rights reserved.
 *
 *  Licensed under the MADAI Software License. You may obtain a copy of
 *  this license at
 *
 *         https://madai-public.cs.unc.edu/software/license/
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>

#include "ApplicationUtilities.h"
#include "MetropolisHastingsSampler.h"
#include "GaussianProcessEmulatedModel.h"
#include "RuntimeParameterFileReader.h"
#include "Paths.h"
#include "Trace.h"

#include "madaisys/SystemTools.hxx"

using madai::Paths;

/**
generateMCMCtrace
  Generate a trace of length N for a Gaussian Process Model Emulator
 */
static const int DEFAULT_NUMBER_ITERATIONS = 100;
static const int DEFAULT_BURN_IN = 0;
static const bool DEFAULT_USE_EMULATOR_COVARIANCE = false;
static const double DEFAULT_STEP_SIZE = 0.1;

struct GaussianProcessMCMCRuntimeParameters
{
  int numberIter;
  int numberBurnIn;
  bool UseEmulatedCovariance;
  double StepSize;
  std::string ModelOutputDirectory;
  std::string ExperimentalResultsDirectory;
};

bool parseMCMCRuntimeParameters(
    int argc, char** argv,
    struct GaussianProcessMCMCRuntimeParameters & Opts )
{
  // Initialize as defaults
  Opts.ModelOutputDirectory = madai::Paths::DEFAULT_MODEL_OUTPUT_DIRECTORY;
  Opts.ExperimentalResultsDirectory = madai::Paths::DEFAULT_EXPERIMENTAL_RESULTS_DIRECTORY;
  Opts.numberIter = DEFAULT_NUMBER_ITERATIONS;
  Opts.numberBurnIn = DEFAULT_BURN_IN;
  Opts.UseEmulatedCovariance = false;
  Opts.StepSize = DEFAULT_STEP_SIZE;

  for ( unsigned int i = 0; i < argc; i++ ) {
    std::string argString( argv[i] );

    if ( argString == "MCMC_NUMBER_ITERATIONS" ) {
      Opts.numberIter = atoi(argv[i+1]);
      i++;
    } else if ( argString == "MODEL_OUTPUT_DIRECTORY" ) {
      Opts.ModelOutputDirectory = std::string( argv[i+1] );
      i++;
    } else if ( argString == "EXPERIMENTAL_RESULTS_DIRECTORY" ) { 
      Opts.ExperimentalResultsDirectory = std::string( argv[i+1] );
      i++;
    } else if ( argString == "MCMC_NUMBER_BURN_IN" ) {
      Opts.numberBurnIn = atoi(argv[i+1]);
      i++;
    } else if ( argString == "MCMC_USE_EMULATOR_COVARIANCE" ) {
      std::string tstring(argv[i+1]);
      if ( tstring == "false" || tstring == "0" ) {
        Opts.UseEmulatedCovariance = false;
      } else if ( tstring == "true" || tstring == "1" ) {
        Opts.UseEmulatedCovariance = true;
      } else {
        std::cerr << "MCMC_USE_EMULATOR_COVARIANCE: " << tstring << " is invalid\n"
                  << "Setting to false\n";
        Opts.UseEmulatedCovariance = false;
      }
      i++;
    } else if ( argString == "MCMC_STEP_SIZE" ) {
      Opts.StepSize = atof(argv[i+1]);
      i++;
    }
  }
  return true;
}


int main(int argc, char ** argv) {

  if (argc < 3) {
    std::cerr << "Useage:\n"
              << "    generateMCMCtrace <StatisticsDirectory> <OutputFileName>\n"
              << "\n"
              << "This program produces a Markov Chain Monte Carlo trace from \n"
              << "a trained emulator. The programs PCADecompose and basicTrain \n"
              << "must have been run on <StatisticsDirectory> prior to running \n"
              << "this program."
              << "\n"
              << "<StatisticsDirectory>" << Paths::SEPARATOR
              << Paths::PCA_DECOMPOSITION_FILE << "\n"
              << "\n"
              << "<StatisticsDirectory> is the directory in which all \n"
              << "statistics data are stored. It contains the parameter file "
              << Paths::RUNTIME_PARAMETER_FILE << "\n"
              << "\n"
              << "<OutputFileName> is the name of the file the trace will be \n"
              << "stored in. This file will be written in the\n"
              << "directory <StatisticsDirectory>/trace/.\n"
              << "\n"
              << "Format of entries in " << Paths::RUNTIME_PARAMETER_FILE
              << ":\n\n"
              << "MODEL_OUTPUT_DIRECTORY <value> (default: "
              << Paths::DEFAULT_MODEL_OUTPUT_DIRECTORY << ")\n"
              << "EXPERIMENTAL_RESULTS_DIRECTORY <value> (default: "
              << Paths::DEFAULT_EXPERIMENTAL_RESULTS_DIRECTORY << ")\n"
              << "MCMC_NUMBER_ITERATIONS <value> (default: "
              << DEFAULT_NUMBER_ITERATIONS << ")\n"
              << "MCMC_NUMBER_BURN_IN <value> (default: "
              << DEFAULT_BURN_IN << ")\n"
              << "MCMC_USE_EMULATOR_COVARIANCE <value> (default: "
              << DEFAULT_USE_EMULATOR_COVARIANCE << ")\n"
              << "MCMC_STEP_SIZE <value> (default: "
              << DEFAULT_STEP_SIZE << ")\n";
    return EXIT_FAILURE;
  }
  std::string StatisticsDirectory( argv[1] );
  std::string OutputFileName( argv[2] );
  
  madai::EnsurePathSeparatorAtEnd( StatisticsDirectory );
  madai::RuntimeParameterFileReader RPFR;
  RPFR.ParseFile( StatisticsDirectory + madai::Paths::RUNTIME_PARAMETER_FILE );
  char** Args = RPFR.GetArguments();
  int NArgs = RPFR.GetNumberOfArguments();
  struct GaussianProcessMCMCRuntimeParameters Opts;
  if ( !parseMCMCRuntimeParameters( NArgs, Args, Opts ) ) {
    std::cerr << "Error: Parsing configuration file for gaussian process mcmc.\n";
    return EXIT_FAILURE;
  }
  std::string observationsFile = Opts.ExperimentalResultsDirectory +
    madai::Paths::SEPARATOR + madai::Paths::RESULTS_FILE;
  
  madai::GaussianProcessEmulatedModel gpem;
  std::string MOD = StatisticsDirectory + Opts.ModelOutputDirectory;
  std::string ERD = StatisticsDirectory + Opts.ExperimentalResultsDirectory;
  if ( gpem.LoadConfiguration( StatisticsDirectory, MOD, ERD ) != madai::Model::NO_ERROR ) {
    std::cerr << "Error in GaussianProcessEmulatedModel::LoadConfiguration\n";
    return EXIT_FAILURE;
  }

  gpem.SetUseModelCovarianceToCalulateLogLikelihood(Opts.UseEmulatedCovariance);

  std::ifstream observations(observationsFile.c_str());
  if (madai::Model::NO_ERROR != gpem.LoadObservations(observations)) {
    std::cerr << "error loading observations.\n";
    return EXIT_FAILURE;
  }
  observations.close();

  madai::MetropolisHastingsSampler mcmc;
  mcmc.SetModel( &gpem );
  mcmc.SetStepSize(Opts.StepSize);

  std::vector< madai::Parameter > const & parameters
    = gpem.GetParameters();

  int t = gpem.GetNumberOfScalarOutputs();

  int step = Opts.numberBurnIn / 100, percent = 0;
  if (step < 1)
    step = 1; // avoid div-by-zero error;
  for ( int count = 0; count < Opts.numberBurnIn; count++ ) {
    if ( count % step == 0 )
      std::cerr << '\r' << "Burn in done: " << percent++ << "%";
    mcmc.NextSample();
  }
  step = Opts.numberIter / 100, percent = 0;

  std::vector< madai::Sample> samples;
  for (int count = 0; count < Opts.numberIter; count ++) {
    if (count % step == 0)
      std::cerr <<  '\r' << "MCMC percent done: " << percent++ << "%";
    samples.push_back(mcmc.NextSample());
  }
  std::cerr << "\r" ;

  madai::Trace trace;
  for (std::vector< madai::Sample >::const_iterator it = samples.begin() ;
       it != samples.end(); ++it) {
    trace.Add( *it );
  }
  std::string traceDirectory = StatisticsDirectory + madai::Paths::TRACE_DIRECTORY;
  madaisys::SystemTools::MakeDirectory( traceDirectory.c_str() );
  std::string OutputFile = traceDirectory+OutputFileName;
  std::ofstream Out( OutputFile.c_str() );
  trace.WriteCSVOutput(
      Out,
      gpem.GetParameters(),
      gpem.GetScalarOutputNames() );
  return EXIT_SUCCESS;
}
