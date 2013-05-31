/*=========================================================================
 *
 *  Copyright 2011-2013 The University of North Carolina at Chapel Hill
 *  All rights reserved.
 *
 *  Licensed under the MADAI Software License. You may obtain a copy of
 *  this license at
 *
 *         https://madai-public.cs.unc.edu/visualization/software-license/
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/

#ifndef madai_GaussianProcessEmulatorDirectoryReader_h_included
#define madai_GaussianProcessEmulatorDirectoryReader_h_included

#include <string>
#include <vector>


namespace madai {

// Forward declarations
class GaussianProcessEmulator;
class Parameter;

class GaussianProcessEmulatorDirectoryReader {
public:
  GaussianProcessEmulatorDirectoryReader();
  ~GaussianProcessEmulatorDirectoryReader();

  /**
   Enable verbose output when reading. */
  void SetVerbose( bool value );
  bool GetVerbose() const;

  /**
    This takes an empty GPEM and loads training data.
    \returns true on success. */
  bool LoadTrainingData(GaussianProcessEmulator * gpe,
                        std::string modelOutputDirectory,
                        std::string statisticalAnalysisDirectory,
                        std::string experimentalResultsFileName);

  /**
    This takes a GPEM and loads PCA data.
    \returns true on success. */
  bool LoadPCA( GaussianProcessEmulator * gpe,
                const std::string & statisticalAnalysisDirectory);

  /**
    This takes a GPEM and loads the emulator specific
    data (submodels with their thetas).
    \returns true on success. */
  bool LoadEmulator( GaussianProcessEmulator * gpe,
                     const std::string & statisticalAnalysisDirectory);

  /**
    Parses a file describing the parameter prior distributions. */
  static bool ParseParameters( std::vector< madai::Parameter > & parameters,
                               int & numberParameters,
                               const std::string & statisticalAnalysisDirectory,
                               bool verbose );

protected:
  bool m_Verbose;

};

} // end namespace madai

#endif // madai_GaussianProcessEmulatorDirectoryReader_h_included
