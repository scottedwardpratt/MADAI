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

#include <cassert>
#include <cstdlib>
#include <iomanip>

#include "SamplerCSVWriter.h"
#include "Sample.h"
#include "Sampler.h"
#include "Model.h"

namespace madai {


int SamplerCSVWriter
::GenerateSamplesAndSaveToFile(
    Sampler & sampler,
    Model & model,
    std::ostream & outFile,
    int NumberOfSamples,
    int NumberOfBurnInSamples,
    bool UseEmulatorCovariance,
    bool WriteLogLikelihoodGradients,
    std::ostream * progress)
{
  model.SetUseModelCovarianceToCalulateLogLikelihood(UseEmulatorCovariance);
  sampler.SetModel( &model );
  int step = NumberOfBurnInSamples / 100, percent = 0;
  if ( step < 1 ) {
    step = 1; // avoid div-by-zero error
  }
  for ( int count = 0; count < NumberOfBurnInSamples; count++ ) {
    if (progress != NULL) {
      if ( count % step == 0 ) {
        (*progress) << '\r' << "Burn in percent done: " << std::setfill('0') << std::setw(2) << percent++ << "%  ";
        progress->flush();
      }
    }
    sampler.NextSample(); // Discard samples in the burn-in phase
  }
  step = NumberOfSamples / 100, percent = 0;
  if ( step < 1 ) {
    step = 1; // avoid div-by-zero error
  }

  WriteHeader( outFile, model.GetParameters(), model.GetScalarOutputNames(),
               WriteLogLikelihoodGradients);

  Sample oldSample;
  int successfulSteps = 0, failedSteps = 0;
  for (int count = 0; count < NumberOfSamples; count ++) {
    if (progress != NULL) {
      if (count % step == 0) {
        if( successfulSteps > 0 || failedSteps > 0 ) {
          (*progress) <<  '\r' << "Sampler percent done: " << std::setfill('0') << std::setw(2) << percent++ << "%";
          (*progress) << "\tSuccess rate: " << std::setfill('0') << std::setw(2) << 100*successfulSteps / (successfulSteps + failedSteps) << "%";
        }
      }
      progress->flush();
    }
    Sample sample = sampler.NextSample();
    if( sample == oldSample ) {
      failedSteps++;
    }
    else {
      successfulSteps++;
    }
    oldSample = sample;

    WriteSample( outFile, sample, WriteLogLikelihoodGradients );
  }
  if (progress != NULL) {
    // Leave the success rate percentage visible
    (*progress) << "\n";
    progress->flush();
  }

  return EXIT_SUCCESS;
}


void
SamplerCSVWriter
::WriteHeader( std::ostream & o,
               const std::vector< Parameter > & params,
               const std::vector< std::string > & outputs,
               bool WriteLogLikelihoodGradients)
{
  if ( !params.empty() ) {
    std::vector< Parameter >::const_iterator itr = params.begin();
    o << '"' << itr->m_Name << '"';
    for ( itr++; itr < params.end(); itr++ ) {
      o << ',' << '"' << itr->m_Name << '"';
    }
    if ( !outputs.empty() ) {
      o << ',';
    }
  }
  if ( !outputs.empty() ) {
    std::vector<std::string>::const_iterator itr = outputs.begin();
    //std::cout << "Output name: " << *itr << std::endl;
    o << '"' << *itr << '"';
    for ( itr++; itr < outputs.end(); itr++ ) {
      o << ',' << '"' << *itr << '"';
    }
  }
  o << ",\"LogLikelihood\"";
  if( WriteLogLikelihoodGradients ) {
    if ( !outputs.empty() ) {
      for ( std::vector<std::string>::const_iterator itr = outputs.begin();
            itr < outputs.end(); itr++ )
      {
        o << ',' << "\"dLL/d" << *itr << '"';
      }
      for ( std::vector<std::string>::const_iterator itr = outputs.begin();
            itr < outputs.end(); itr++ )
      {
        o << ',' << "\"sigma_" << *itr << "*dLL/dsigma_" << *itr << '"';
      }
    }
  }
  o << "\n";
}


/** Utility for writing a vector to an output stream
 *
 * The vector elements are delimited by the given delimiter */
template <class T>
void write_vector( std::ostream& o, std::vector< T > const & v, char delim ) {
  if ( !v.empty() ) {
    typename std::vector< T >::const_iterator itr = v.begin();
    o << *(itr++);
    while ( itr < v.end() ) {
      o << delim << *(itr++);
    }
  }
}


void
SamplerCSVWriter
::WriteSample( std::ostream & out, const Sample & sample,
            bool WriteLogLikelihoodGradients)
{
  write_vector( out, sample.m_ParameterValues, ',' );
  out << ',';

  if ( sample.m_OutputValues.size() > 0 ) {
    write_vector( out, sample.m_OutputValues, ',' );
    out << ',';
  }
  out << sample.m_LogLikelihood;
  if( WriteLogLikelihoodGradients ) {
    if ( sample.m_OutputValues.size() > 0 ) {
      out << ',';
      write_vector( out, sample.m_LogLikelihoodValueGradient, ',' );
      out << ',';
      write_vector( out, sample.m_LogLikelihoodErrorGradient, ',' );
    }
  }
  if ( sample.m_Comments.size() > 0 ) {
    out << ",\"";
    write_vector( out, sample.m_Comments, ';' );
    out << '"';
  }
  out << '\n';

  out.flush();
}


} // end namespace madai
