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

/*
 * GaussianProcessEmulator Class
 *
 * \author Hal Canary <cs.unc.edu/~hal/>
 * \author Christopher Coleman-Smith <cec24@phy.duke.edu>
 * \author Cory Quammen <cs.unc.edu/~cquammen>
 */

#ifndef madai_GaussianProcessEmulator_h_included
#define madai_GaussianProcessEmulator_h_included
#include <iostream> // std::istream std::ostream
#include <vector>   // std::vector
#include <Eigen/Dense> // Eigen::MatrixXd, Eigen::VectorXd
#include "Parameter.h" // madai::Parameter
namespace madai {
/**
   \class GaussianProcessEmulator
   \fixme describe the useage of this class. */
class GaussianProcessEmulator
{
public:
  // ENUMERATIONS
  /**
    CovarianceFunction describes which type of covariance function our
    GPME uses.*/
  typedef enum {
    SQUARE_EXPONENTIAL_FUNCTION,
    POWER_EXPONENTIAL_FUNCTION,
    MATERN_32_FUNCTION,
    MATERN_52_FUNCTION,
    UNKNOWN_FUNCTION
  } CovarianceFunctionType;
  /**
    is it trained and ready to go?  If an error occurs in a
    constructor, this will be set to .*/
  typedef enum {
    READY,
    UNCACHED,
    UNTRAINED,
    UNINITIALIZED,
    ERROR
  } StatusType;

  // METHODS
  /**
    Default constructor which makes an empty GPEM */
  GaussianProcessEmulator();

  /**
    Writes current state to file.  \returns true on sucess. */
  bool Write(std::ostream & output) const;
  /**
    This loads a fully-ready-to-go GPEM
    \returns true on success. */
  bool Load(std::istream & input);
  /**
    This takes an empty GPEM and loads training data.
    \returns true on success. */
  bool LoadTrainingData(std::istream & input);

  /**
    This takes an GPEM and sets default values for all of the
    hyperparameters. \returns true on success. */
  bool BasicTraining(
    double fractionResolvingPower,
    CovarianceFunctionType covarianceFunction,
    int regressionOrder,
    double defaultNugget,
    double amplitude,
    double scale);
  /**
    This takes an GPEM and trains it. \returns true on sucess. */
  bool Train(
    CovarianceFunctionType covarianceFunction,
    int regressionOrder, double fractionResolvingPower);

  /**
    Execute the model at an input point x.  Save a lot of time by not
    calculating the covaraince error.  */
  bool GetEmulatorOutputs (
    const std::vector< double > & x,
    std::vector< double > & y) const;

  /**
    Execute the model at an input point x.
    The covariance returned will be a flattened matrix */
  bool GetEmulatorOutputsAndCovariance (
    const std::vector< double > & x,
    std::vector< double > & y,
    std::vector< double > & ycov) const;

  /**
     Writes current state to file.  \returns true on sucess. */
  bool PrintThetas(std::ostream & output) const;

  /**
     \returns m_Status */
  StatusType GetStatus() const;

protected:
  /**
     Once Load(), Train(), or BasicTraining() finishes, calculate and
     cache some data to make calling GetEmulatorOutputsAndCovariance()
     faster. */
  bool MakeCache();

  /**
     Check status; make sure that the emulator is in a good state. */
  StatusType CheckStatus();

  /**
    Execute the model at an input point x.
    The covariance returned will be a flattened matrix */
  bool PrincipalComponentDecompose(double fractionResolvingPower);

public:
  // FIELDS
  /**
     Current status of the GPME   */
  StatusType m_Status;
  /**
     Comments */
  std::vector<std::string> m_Comments;
  /**
     A list of input parameters. Length should == m_numberParameters */
  std::vector<madai::Parameter> m_Parameters;
  /**
     A list of output names.  Length should == m_numberOutputs*/
  std::vector<std::string> m_OutputNames;
  /**
     Number of input parameters to the model. */
  int m_NumberParameters;
  /**
     Number of  model outputs */
  int m_NumberOutputs;
  /**
     Number of training points */
  int m_NumberTrainingPoints;
  /**
     Number of principle components in the output space.  This is the
     number of SingleModels we have */
  int m_NumberPCAOutputs;

  /**
     Original training parameter values: the "design"
     (rows:numberTrainingPoints by cols:numberParameters) */
  Eigen::MatrixXd m_ParameterValues;
  /**
     Original training output values.
     (rows:numberTrainingPoints by cols:numberOutputs) */
  Eigen::MatrixXd m_OutputValues;

  //@{
  /**
     The mean values and standard deviations of the columns of
     outputValues (size:numberOutputs) */
  Eigen::VectorXd m_OutputMeans;
  Eigen::VectorXd m_OutputStandardDeviations;
  //@}

  //@{
  /**
     The eigenvalues and vectors from the PCA decomposition of
     outputValues.  Only the first numberPCAOutputs of them are kept.
     Consequently, PCAEigenvalues is a vector of length
     numberPCAOutputs and PCAEigenvectors is a matrix of size

     numberPCAOutputs-by-numberOutputs. */
  Eigen::VectorXd m_PCAEigenvalues;
  Eigen::MatrixXd m_PCAEigenvectors;
  //@}
  /**
     This represents one PCA-decomposed model */
  struct SingleModel {
    //FUNCTIONS
    SingleModel();
    /**
       Calulate the covariance between two points in parameter space
       using m_theteas and m_covarianceFunction. */
    double CovarianceCalc(
        const Eigen::VectorXd &,
        const Eigen::VectorXd &) const;
    // /**
    //    Initialize a model and set hyperparameter values to default.* */
    // bool Initialize(
    //   GaussianProcessEmulator * parent,
    //   CovarianceFunctionType covarianceFunction,
    //   int regressionOrder);

    /**
       Called by GaussianProcessEmulator::MakeCache().
       Given a set of (thetas, covarianceFunction, regressionOrder),
       populate m_cInverse, m_betaVector, m_hMatrix, m_cInverseZ,
       m_cInverseHtrans, and m_IHTCIH;  */
    bool MakeCache();

    /**
       Sets default values for all of the hyperparameters. \returns
       true on success. */
    bool BasicTraining(
        CovarianceFunctionType covarianceFunction,
        int regressionOrder,
        double defaultNugget,
        double amplitude,
        double scale);

    /**
       Trains the submodel. \returns true on sucess. */
    bool Train(
        CovarianceFunctionType covarianceFunction,
        int regressionOrder);

    /**
       Execute the model at an input point x. */
    bool GetEmulatorOutputsAndCovariance(
        const std::vector< double > & x,
        double & mean,
        double & variance) const;
    /**
       Execute the model at an input point x.  Save a lot of time by
       not calculating the error. */
    bool GetEmulatorOutputs(
        const std::vector< double > & x,
        double & mean) const;

    // FIELDS
    /**
       A pointer to the parent */
    GaussianProcessEmulator * m_Parent;
    /**
       Which covariance function is used? */
    CovarianceFunctionType m_CovarianceFunction;
    /**
       Which order polynomial is used as a regression function? */
    int m_RegressionOrder;
    /**
       zMAtrix = (pca_evecs_r)^T (outputValues_standatd_score)/
       Each SingleModel gets one column (of length numberTrainingPoints)
       from the zMatrix.  This is the vector zValues.
    */
    Eigen::VectorXd m_ZValues;
    /**
       These are the hyperparameter. */
    Eigen::VectorXd m_Thetas;

    //@{
    /**
       Things we cached; values to carry from one calulation to the next.
    */
    Eigen::MatrixXd m_CInverse;
    // [NxN] m_CInverse = CMatrix.inverse();
    Eigen::MatrixXd m_RegressionMatrix1;
    // [FxF] m_RegressionMatrix1
    //     = (HMatrix.transpose() * m_CInverse * HMatrix).inverse();
    Eigen::MatrixXd m_RegressionMatrix2;
    // [FxN] m_RegressionMatrix2
    //     = (m_CInverse * HMatrix).transpose();
    Eigen::VectorXd m_BetaVector;
    //  [F]  m_BetaVector
    //   = m_RegressionMatrix * HMatrix.transpose() * m_CInverse * m_ZValues;
    Eigen::VectorXd m_GammaVector;
    //  [N]  m_GammaVector
    //        = m_CInverse * (m_ZValues - (HMatrix * m_BetaVector));
    //@}
  };
  /**
    An array of length numberPCAOutputs/ */
  std::vector< SingleModel > m_PCADecomposedModels;
};
} // end namespace madai


#endif /* madai_GaussianProcessEmulator_h_included */