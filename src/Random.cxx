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
#include <time.h>

#include <madaisys/SystemInformation.hxx>

#include "Random.h"

namespace madai {

/**
 * Constructor.  Uses time() for seed.
 */
Random::Random() :
  m_UniformRealGenerator( m_BaseGenerator, m_UniformRealDistribution )
{
  this->Reseed();
}

/**
 * Constructor.
 */
Random::Random(unsigned long int seed) :
  m_UniformRealGenerator( m_BaseGenerator, m_UniformRealDistribution )
{
  this->Reseed(seed);
}

/**
 * Destructor.
 */
Random::~Random()
{
}

/**
 * reseed
 */
void Random::Reseed(unsigned long int seed)
{
  m_BaseGenerator.seed( seed );
}

/**
 * reseed with time() and current process id.
 */
void Random::Reseed()
{
  madaisys::SystemInformation systemInformation;
  this->Reseed(time(NULL) ^ (systemInformation.GetProcessId() << 16));
}

/**
 * Returns an integer < N and >= 0
 */
int Random::Integer( int N )
{
  return m_UniformIntDistribution( m_BaseGenerator, N );
}

/**
 * Returns an integer < N and >= 0
 */
int Random::operator()( int N )
{
  return m_UniformIntDistribution( m_BaseGenerator, N );
}

/**
 * Returns a long < N and >= 0
 */
long Random::Integer( long N )
{
  return m_UniformLongDistribution( m_BaseGenerator, N );
}

/**
 * Returns a long < N and >= 0
 */
long Random::operator()( long N )
{
  return m_UniformLongDistribution( m_BaseGenerator, N );
}

/**
 * min=0.0, max=1.0
 */
double Random::Uniform()
{
  return m_UniformRealGenerator();
}

/**
 *
 */
double Random::Uniform(double min, double max)
{
  return this->Uniform() * ( max - min ) + min;
}

/**
 * mean=0.0, var=1.0
 */
double Random::Gaussian()
{
  return m_NormalDistribution( m_UniformRealGenerator );
}

/**
 *
 */
double Random::Gaussian(double mean, double standardDeviation)
{
  return standardDeviation * this->Gaussian() + mean;
}

}
