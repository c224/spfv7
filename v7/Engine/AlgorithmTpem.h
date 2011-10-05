
/** \ingroup SPF */
/*@{*/

/*! \file AlgorithmTpem.h
 *
 *  Diagonalization method for SPF
 *
 */
#ifndef ALGORITHM_TPEM_H
#define ALGORITHM_TPEM_H
#include <algorithm>
#include "ProgressIndicator.h" // in PsimagLite
#include "Matrix.h" // in PsimagLite
#include "Fermi.h" // in PsimagLite
#include "Complex.h" // in PsimagLite

namespace Spf {
	template<typename EngineParametersType,typename ModelType,typename RngType>
	class AlgorithmTpem {
	public:	
		typedef typename EngineParametersType::FieldType FieldType;
		typedef std::complex<FieldType> ComplexType;
		typedef PsimagLite::Matrix<ComplexType> MatrixType;

		AlgorithmTpem(const EngineParametersType& engineParams,ModelType& model)
		: engineParams_(engineParams),model_(model),
		  hilbertSize_(model_.hilbertSize()),
		{}

		void init()
		{
// 			model_.createHamiltonian(matrixOld_,ModelType::OLDFIELDS);
// 			diag(matrixOld_,eigOld_,'N');
// 			sort(eigOld_.begin(), eigOld_.end(), std::less<FieldType>());
		}

		size_t hilbertSize() const { return hilbertSize_; }

		bool isAccepted(size_t i,RngType& rng)
		{
			FieldType dsDirect = model_.deltaDirect(i);
							
			model_.createHsparse(matrixNew_,ModelType::NEWFIELDS);
			if (ether.isSet("adjusttpembounds")) {
				if (tpemAdjustBounds(aux.sparseTmp[0],ether,aux)!=0) {
					cerr<<"Cannot adjust bounds for tpem spectrum\n";
					exit(1);
				}
			}
			dS += calculate_dS (moment,aux.sparseMatrix[0], aux.sparseTmp[0],
								support,ether,aux,tpemOptions);
			
			dS -= ether.beta*dsDirect;

			
			//if (engineParams_.carriers>0) model_.adjustChemPot(eigNew_); //changes engineParams_.mu
			//FieldType integrationMeasure = model_.integrationMeasure(i);
				
			return doMetropolis(dsDirect,integrationMeasure,rng);
		}

		void accept(size_t i)
		{
			model_.accept(i);
			eigOld_ = eigNew_;
		}

		void prepare()
		{
// 			diagonalize(matrixNew_,eigNew_,'V',ModelType::OLDFIELDS);
		}
		
// 		const ComplexType& matrix(size_t lambda1,size_t lambda2) const
// 		{
// 			return matrixNew_(lambda1,lambda2);
// 		}
// 		
// 		const FieldType& e(size_t i) const
// 		{
// 			return eigNew_[i];
// 		}
// 
// 		void diagonalize(
// 				MatrixType& matrix,
// 				std::vector<FieldType>& eigs,
// 				char jobz='N',
// 				size_t fields=ModelType::NEWFIELDS) const
// 		{
// 			model_.createHamiltonian(matrix,fields);
// 			diag(matrix,eigs,jobz);
// 			if (jobz!='V')
// 				std::sort(eigs.begin(), eigs.end(), std::less<FieldType>());
// 		}
// 		
// 		void printMatrix(size_t mode) const
// 		{
// 			if (mode==ModelType::NEWFIELDS) {
// 				std::cerr<<matrixNew_;
// 				return;
// 			}
// 			MatrixType m(matrixNew_.n_row(),matrixNew_.n_col());
// 			if (!isHermitian(m)) throw std::runtime_error("Problem\n");
// 			model_.createHamiltonian(m,ModelType::OLDFIELDS);
// 			std::cerr<<m;
// 		}

		template<typename EngineParametersType2,typename ModelType2,
			typename RandomNumberGeneratorType2>
		friend std::ostream& operator<<
			(std::ostream& os,const AlgorithmTpem<EngineParametersType2,
					ModelType2,RandomNumberGeneratorType2>& a);

	private:
		bool doMetropolis(FieldType dsDirect,
		                  FieldType integrationMeasure,
		                  RngType& rng)
		{
			FieldType mu=engineParams_.mu;
			FieldType beta = engineParams_.beta;
			FieldType X =1.0;
			
			for (size_t i=0;i<eigNew_.size();i++) {
				FieldType temp = 0;
				if (eigNew_[i]>mu)
					temp = (1.0+exp(-beta*(eigNew_[i]-mu)))/
						(1.0+exp(-beta*(eigOld_[i]-mu)));
				else
				temp =(1.0+exp(beta*(eigNew_[i]-mu)))/
							(exp(beta*(eigNew_[i]-mu))+
									exp(-beta*(eigOld_[i]-eigNew_[i])));
			
				X *= temp;
			}
			//std::cerr<<"Xbefore="<<X<<" ";
			//if (ether.isSet("sineupdate")) X *= integrationMeasure;
			X *=  exp(-beta*dsDirect);
			X = X/(1.0+X);

			FieldType r=rng.random();
			if (X>r) return true;
			else return false;
		}
		
		void metropolisOrGlauber(const RealType& dS) const
		{
			if (DO_GLAUBER) {
				if (dS<0) {
					dS=exp(dS)/(1.0+exp(dS));
				} else {
					dS=1.0/(1.0+exp(-dS));
				}
				return (dS>myRandom());
			}
			// METROPOLIS PROPER 
			return (dS > 0.0 || myRandom () < exp (dS));
		}
		
		void testEigs() const
		{
			FieldType eps = 1e-6;
			for (size_t i=0;i<eigOld_.size();i++) {
				if (fabs(eigOld_[i]-eigNew_[i])>eps) return;
			}
			throw std::runtime_error("Eigs are equal!!\n");
		}
		
		void testMatrix() const
		{
			FieldType eps = 1e-6;
			for (size_t i=0;i<matrixOld_.n_row();i++) {
				for (size_t j=0;j<matrixOld_.n_col();j++) {
					if (fabs(real(matrixOld_(i,j)-matrixNew_(i,j)))>eps &&
					fabs(imag(matrixOld_(i,j)-matrixNew_(i,j)))>eps) return;
				}
			}
			throw std::runtime_error("Matrix are equal!!\n");
		}
		
		const EngineParametersType& engineParams_;
		ModelType& model_;
		std::vector<FieldType> eigNew_,eigOld_;
		size_t hilbertSize_;
		MatrixType matrixNew_,matrixOld_;
		
		
	}; // AlgorithmTpem
	
	template<typename EngineParametersType,typename ModelType,
		typename RandomNumberGeneratorType>
	std::ostream& operator<<(std::ostream& os,const AlgorithmTpem<
			EngineParametersType,ModelType,RandomNumberGeneratorType>& a)
	{
		
		typedef typename EngineParametersType::FieldType FieldType;
		std::vector<FieldType> eigNew(a.hilbertSize_);
		PsimagLite::Matrix<std::complex<FieldType> > matrix(a.hilbertSize_,a.hilbertSize_);
		a.diagonalize(matrix,eigNew,'V',ModelType::OLDFIELDS);
		os<<"Eigenvalues\n";
		os<<eigNew;
		return os;
	}
} // namespace Spf

/*@}*/
#endif // ALGORITHM_TPEM_H
