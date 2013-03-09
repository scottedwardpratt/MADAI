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

/**
trainEmulator
  Tune hyperparameters for a N-D Gaussian Process Model Emulator

ACKNOWLEDGMENTS:
  This software was written in 2012-2013 by Hal Canary
  <cs.unc.edu/~hal>, based off of the MADAIEmulator program (Copyright
  2009-2012 Duke University) by C.Coleman-Smith <cec24@phy.duke.edu>
  in 2010-2012 while working for the MADAI project <http://madai.us/>.

USE:
  For details on how to use trainEmulator, consult the manpage via:
    $ nroff -man < [PATH_TO/]trainEmulator.1 | less
  or, if the manual is installed:
    $ man 1 trainEmulator
*/

#include <getopt.h>
#include <iostream> // cout, cin
#include <fstream> // ifstream, ofstream
#include <cstring> // strcmp, strlen
#include "GaussianProcessModelEmulator.h"

#define starts_with(s1,s2) (std::strncmp((s1), (s2), std::strlen(s2)) == 0)

static const madai::GaussianProcessModelEmulator::CovarianceFunction DEFAULT_COVARIACE_FUNCTION
  = madai::GaussianProcessModelEmulator::SQUARE_EXP_FN;
static const int DEFAULT_REGRESSION_ORDER = 1;
static const double DEFAULT_PCA_FRACTION = 0.99;

static const char useage [] =
  "useage:\n"
  "  trainEmulator [options] InputModelFile [ModelSnapshotFile]\n"
  "\n"
  "InputModelFile can be \"-\" to read from standard input.\n"
  "\n"
  "ModelSnapshotFile can be \"-\" or left unspecified to write to\n"
  "standard output.\n"
  "\n"
  "Options:\n"
  "\n"
  "  --regression_order=0  (constant)\n"
  "  --regression_order=1  (linear, default)\n"
  "  --regression_order=2  (quadratic)\n"
  "  --regression_order=3  (cubic)\n"
  "\n"
  "  --covariance_fn=POWER_EXPONENTIAL (POWER EXPONENTIAL)\n"
  "  --covariance_fn=SQUARE_EXPONENTIAL (SQUARED EXPONENTIAL, default)\n"
  "  --covariance_fn=MATERN_32  (MATERN 3/2)\n"
  "  --covariance_fn=MATERN_52  (MATERN 5/2)\n"
  "\n"
  "  -v=FRAC\n"
  "  --pca_variance=FRAC\n"
  "            sets the pca decomp to keep cpts up to variance fraction frac\n"
  "\n"
  "  -q\n"
  "  --quiet   run without any extraneous output on standard error.\n"
  "\n"
  "  -h -?     print this dialogue\n"
  "\n";


struct cmdLineOpts{
  int regressionOrder;
  madai::GaussianProcessModelEmulator::CovarianceFunction covarianceFunction;
  bool quietFlag;
  double pcaVariance;
  const char * inputFile; /* first non-flag argument  */
  const char * outputFile; /* second non-flag argument */
};

/**
 * option parsing using getoptlong.  If it fails, returns false.
 */
bool parseCommandLineOptions(int argc, char** argv, struct cmdLineOpts & opts)
{
  /* note: flags followed with a colon come with an argument */
  static const char optString[] = "r:c:qh?";

  // should add a variance option for the pca_decomp
  // and a flag to do return output in pca space
  static const struct option longOpts[] = {
    { "regression_order", required_argument , NULL, 'r'},
    { "covariance_fn", required_argument , NULL, 'c'},
    { "pca_variance", required_argument, NULL , 'v'},
    { "quiet", no_argument , NULL, 'q'},
    { "help", no_argument , NULL, 'h'},
    { NULL, no_argument, NULL, 0}
  };

  // init with default values
  opts.regressionOrder = DEFAULT_REGRESSION_ORDER;
  opts.covarianceFunction = DEFAULT_COVARIACE_FUNCTION;
  opts.quietFlag = 0;
  opts.pcaVariance = DEFAULT_PCA_FRACTION;
  opts.inputFile = NULL; // default to stdin
  opts.outputFile = "-"; // default to stdout

  int longIndex, opt;
  opt = getopt_long( argc, argv, optString, longOpts, &longIndex );
  while( opt != -1 ) {
    switch( opt ) {
    case 'r':
      opts.regressionOrder = atoi(optarg);
      if (((opts.regressionOrder) < 0) || ((opts.regressionOrder) > 3)) {
        std::cerr << "Error: regression_order given incorrect argument: \""
          << optarg <<"\"\n";
        return false;
      }
      break;
    case 'c':
      if (starts_with(optarg, "POWER_EXPONENTIAL")) {
        opts.covarianceFunction = madai::GaussianProcessModelEmulator::POWER_EXP_FN;
      } else if (starts_with(optarg, "SQUARE_EXPONENTIAL")) {
        opts.covarianceFunction = madai::GaussianProcessModelEmulator::SQUARE_EXP_FN;
      } else if (starts_with(optarg, "MATERN_32")) {
        opts.covarianceFunction = madai::GaussianProcessModelEmulator::MATERN_32_FN;
      } else if (starts_with(optarg, "MATERN_52")) {
        opts.covarianceFunction = madai::GaussianProcessModelEmulator::MATERN_52_FN;
      } else {
        std::cerr << "Error: covariance_fn given incorrect argument: "
          << optarg << "\n";
        return false;
      }
      break;
    case 'v':
      opts.pcaVariance = atof(optarg);
      /* expect the var to be a float in range [0,1] */
      if(opts.pcaVariance < 0.0 || opts.pcaVariance > 1.0){
        std::cerr << "Error: pca_variance given incorrect value: "
                    << optarg <<"\n";
          return false;
      }
      break;
    case 'q':
      opts.quietFlag = 1;
      break;
    case 'h':
      /* fall-through is intentional */
    case '?':
      std::cerr << useage << '\n';
      return false;
    default:
      /* You won't actually get here. */
      assert(false && "1152677029");
      return false;
    }
    opt = getopt_long(argc, argv, optString, longOpts, &longIndex);
  }

  // set the remaining fields
  if ((argc - optind) >= 2) {
    opts.outputFile = argv[optind + 1];
  }
  if ((argc - optind) >= 1) {
    opts.inputFile = argv[optind];
  }
  if (opts.inputFile == NULL) {
    std::cerr << useage << '\n';
    return false;
  }
  return true;
}

int main(int argc, char ** argv) {
  struct cmdLineOpts options;
  if (! parseCommandLineOptions(argc, argv, options))
    return EXIT_FAILURE;
  madai::GaussianProcessModelEmulator gpme;
  if (0 == std::strcmp(options.inputFile, "-")) {
    gpme.LoadTrainingData(std::cin);
  } else {
    std::ifstream is (options.inputFile);
    gpme.LoadTrainingData(is);
  }
  if (! gpme.Train(
          options.covarianceFunction,
          options.regressionOrder,
          options.pcaVariance)) {
    return EXIT_FAILURE;
  }
  if (0 == std::strcmp(options.outputFile, "-")) {
    gpme.Write(std::cout);
  } else {
    std::ofstream os(options.outputFile);
    gpme.Write(os);
  }
  return EXIT_SUCCESS;
}
