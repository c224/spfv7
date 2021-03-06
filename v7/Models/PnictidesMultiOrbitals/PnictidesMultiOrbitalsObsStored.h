
/** \ingroup SPF */
/*@{*/

/*! \file PnictidesMultiOrbitalsObsStored.h
 *
 *  PnictidesMultiOrbitalsObsStored for pnictidesTwoOrbitals model
 *
 */

#ifndef OBS_STORED_PNICTIDES_MULTIORBITALS_H
#define OBS_STORED_PNICTIDES_MULTIORBITALS_H
#include "Vector.h"

namespace Spf {
	template<typename SpinOperationsType,typename ComplexType,
	typename ParametersModelType>
	class PnictidesMultiOrbitalsObsStored {

		typedef typename SpinOperationsType::DynVarsType DynVarsType;
		typedef typename DynVarsType::FieldType FieldType;
		typedef typename PsimagLite::Vector<FieldType>::Type VectorType;
		typedef typename PsimagLite::Vector<ComplexType>::Type ComplexVectorType;
		typedef typename SpinOperationsType::GeometryType GeometryType;
		typedef PsimagLite::Matrix<FieldType> MatrixType;

		enum {DIRECTION_X,DIRECTION_Y,DIRECTION_Z};
		enum {ORBITAL_XZ,ORBITAL_YZ};
		enum {SPIN_UP,SPIN_DOWN};
		static SizeType const DIRECTIONS  = 3;

	public:

		PnictidesMultiOrbitalsObsStored(
				SpinOperationsType& spinOperations,
				const GeometryType& geometry,
				const ParametersModelType& mp,
				SizeType dof) :
			spinOperations_(spinOperations),
			geometry_(geometry),
			mp_(mp),
			dof_(dof),
			lc_(dof*geometry.volume(),0),
			chargeCor_(geometry.volume(),0),
			mc_(geometry.volume(),DIRECTIONS),
			tc_(geometry.volume(),DIRECTIONS),
			cs_(geometry.volume(),DIRECTIONS),
			qs_(geometry.volume(),DIRECTIONS),
			counter_(0)
		{}

		template<typename GreenFunctionType>
		void operator()(const DynVarsType& spins,
				GreenFunctionType& greenFunction)
		{
			//if (!greenFunction.usesDiagonalization()) return;
			//spinOperations_.classicalCorrelations(cc_,spins);
			greenFunction.localCharge(lc_);
			chargeCorrelation(chargeCor_,greenFunction);
			PsimagLite::Matrix<FieldType> mi(spins.size,DIRECTIONS);
			calcMagSpins(mi,spins);
			PsimagLite::Matrix<ComplexType> qi(spins.size,DIRECTIONS);
			calcMagElectrons(qi,greenFunction);
			mCorrelation(mi,qi,greenFunction);
			tCorrelation(greenFunction);
			correlation(cs_,mi,greenFunction);
			correlation(qs_,qi,greenFunction);
			counter_++;
		}

		template<typename SomeOutputType>
		void finalize(SomeOutputType& fout)
		{
			if (counter_==0) return;
			divideAndPrint(fout,lc_,"#LocalCharge:");
			divideAndPrint(fout,chargeCor_,"#ChargeCorrelations:");
			divideAndPrint(fout,mc_,"#MCorrelations");
			divideAndPrint(fout,tc_,"#TCorrelations");
			divideAndPrint(fout,cs_,"#ClassicalSpinCorrelations");
			divideAndPrint(fout,qs_,"#ItinerantSpinCorrelations");
		}

	private:

		// Correlations of classical + itinerant spins
		template<typename GreenFunctionType>
		void mCorrelation(
				const PsimagLite::Matrix<FieldType>& mi,
				const PsimagLite::Matrix<ComplexType>& qi,
				GreenFunctionType& greenFunction)
		{
			PsimagLite::Matrix<ComplexType> v;
			v.resize(mi.n_row(),mi.n_col());
			for (SizeType i=0;i<v.n_row();i++)
				for (SizeType dir=0;dir<v.n_col();dir++)
					v(i,dir) = mi(i,dir) + qi(i,dir);
			correlation(mc_,v,greenFunction);
		}

		template<typename GreenFunctionType>
		void tCorrelation(GreenFunctionType& greenFunction)
		{

			PsimagLite::Matrix<ComplexType> ti(geometry_.volume(),DIRECTIONS);
			calcTi(ti,greenFunction);
			correlation(tc_,ti,greenFunction);
		}

		// C_x = \sum_i <M_i M_{i+x}>, where M_i is passed in
		template<typename GreenFunctionType>
		void correlation(
				MatrixType& cc,
				const PsimagLite::Matrix<ComplexType>& m,
				GreenFunctionType&)
		{
			for (SizeType x=0;x<cc.n_row();x++) {
				for (SizeType i=0;i<cc.n_row();i++) {
					SizeType j = geometry_.add(i,x);
					for (SizeType dir=0;dir<cc.n_col();dir++)
						cc(x,dir) += real(m(i,dir) * m(j,dir));
				}
			}
		}

		// Magnetization vector of the itinerant electrons
		template<typename GreenFunctionType>
		void calcMagElectrons(
				PsimagLite::Matrix<ComplexType>& me,
				GreenFunctionType& greenFunction)
		{

			ComplexType sqrtOfMinus1 = ComplexType(0,1);
			SizeType volume = me.n_row();
			for (SizeType i=0;i<volume;i++) {
				for (SizeType orb=0;orb<mp_.numberOfOrbitals;orb++) {
					SizeType x = i+(orb+SPIN_UP*mp_.numberOfOrbitals)*volume;
					SizeType y = i+(orb+SPIN_DOWN*mp_.numberOfOrbitals)*volume;

					me(i,DIRECTION_X) -= 0.5*(
							greenFunction(x,y) + greenFunction(y,x));
					me(i,DIRECTION_Y) -= 0.5*sqrtOfMinus1*(
							greenFunction(x,y) - greenFunction(y,x));
					me(i,DIRECTION_Z) -= 0.5*
							(greenFunction(x,x) - greenFunction(y,y));
				}
			}
		}

		// Magnetization of the classical spins
		void calcMagSpins(
				PsimagLite::Matrix<FieldType>& ms,
				const DynVarsType& spins)
		{
			SizeType volume = ms.n_row();
			for (SizeType i=0;i<volume;i++) {
				FieldType theta = spins.theta[i];
				FieldType phi = spins.phi[i];
				FieldType m = (mp_.modulus[i]) ? 1.0 : 0.0;
				ms(i,DIRECTION_X) = m*sin(theta) * cos(phi);
				ms(i,DIRECTION_Y) = m*sin(theta) * sin(phi);
				ms(i,DIRECTION_Z) = m*cos(theta);
			}
		}

		template<typename GreenFunctionType>
		void calcTi(PsimagLite::Matrix<ComplexType>& ti,
				GreenFunctionType& greenFunction)
		{
			ComplexType sqrtOfMinus1 = ComplexType(0,1);
			SizeType volume = ti.n_row();
			for (SizeType i=0;i<volume;i++) {
				for (SizeType spin=0;spin<2;spin++) {
					SizeType x = i+(ORBITAL_XZ+spin*mp_.numberOfOrbitals)*volume;
					SizeType y = i+(ORBITAL_YZ+spin*mp_.numberOfOrbitals)*volume;
					ti(i,DIRECTION_X) += (-0.5)*(
							greenFunction(x,y) + greenFunction(y,x));
					ti(i,DIRECTION_Y) -= sqrtOfMinus1*0.5*(
							greenFunction(x,y) - greenFunction(y,x));
					ti(i,DIRECTION_Z) += (-0.5)*(
							greenFunction(x,x) - greenFunction(y,y));
				}
			}
		}

		// C_x = \sum_i <n_i n_{i+x}>, where n_i = \sum_{dof} n_{i dof}
		template<typename GreenFunctionType>
		void chargeCorrelation(typename PsimagLite::Vector<FieldType>::Type& cc,
				       GreenFunctionType& greenFunction)
		{
			SizeType volume = geometry_.volume();
			//allChargeCorrelation(greenFunction); // for debugging only, comment out for production
			for (SizeType gamma=0;gamma<dof_;gamma++) {
				for (SizeType gamma2=0;gamma2<dof_;gamma2++) {
					typename PsimagLite::Vector<FieldType>::Type tmpV(dof_*dof_*volume,0);
					chargeCorrelation(tmpV,gamma,gamma2,greenFunction);
					for (SizeType x=0;x<volume;x++) {
						cc[x] += tmpV[x + gamma*volume+gamma2*volume*dof_];
					}
				}
			}
		}

		template<typename GreenFunctionType>
		void chargeCorrelation(typename PsimagLite::Vector<FieldType>::Type& cc,
				       SizeType gamma,SizeType gamma2,GreenFunctionType& greenFunction)
		{
			SizeType volume = geometry_.volume();
			SizeType dof = dof_;
        		// Do for all x's
			FieldType slip = 0;
        		for (SizeType x=0;x<volume;x++) {     // Sum over all i's (or w's)
                		ComplexType tmp=0;
				//ComplexType test= ComplexType(0.,0.);
                		for (SizeType w=0;w<volume;w++) {
					SizeType v=geometry_.add(x,w);
                                        tmp += chargeCorrelation(w,gamma,v,gamma2,greenFunction);

					//test += (1.0-greenFunction(w+gamma*volume,
					//			w+gamma*volume));
				}
				slip += imag(tmp);
                		cc[x+gamma*volume+gamma2*volume*dof] += real(tmp)/volume;
			}

			if (fabs(slip)>1e-6) std::cerr<<"slipping="<<slip<<"\n";
		}

		template<typename GreenFunctionType>
		void allChargeCorrelation(GreenFunctionType& greenFunction)
		{
			SizeType volume = geometry_.volume();
			PsimagLite::Matrix<FieldType> m(volume*2,volume*2);
			for (SizeType i=0;i<volume;i++) {
				for (SizeType gamma=0;gamma<4;gamma++) {
					SizeType orb1 = (gamma & 1);
					for (SizeType j=0;j<volume;j++) {
						for (SizeType gamma2=0;gamma2<4;gamma2++) {
							SizeType orb2 = (gamma2 & 1);
							m(i+orb1*volume,j+orb2*volume) +=
								real(chargeCorrelation(i,gamma,j,gamma2,greenFunction));
						}
					}
				}
			}
			std::cerr<<m;
			throw PsimagLite::RuntimeError("testing\n");
		}

		template<typename GreenFunctionType>
		ComplexType chargeCorrelation(SizeType w, SizeType gamma,SizeType v, SizeType gamma2,
			GreenFunctionType& greenFunction)
		{
			SizeType volume = geometry_.volume();
			ComplexType tmp = 0;
			if (v==w && gamma==gamma2) {
				tmp += (1.0-greenFunction(w+gamma*volume,w+gamma*volume));
			} else {
				tmp +=(1.0-greenFunction(w+gamma*volume,w+gamma*volume))*
					(1.0-greenFunction(v+gamma2*volume,v+gamma2*volume));
				tmp -= greenFunction(v+gamma2*volume,w+gamma*volume)*
					greenFunction(w+gamma*volume,v+gamma2*volume);
			}
			return tmp;
		}

		template<typename SomeOutputType>
		void divideAndPrint(SomeOutputType& fout,
		                    VectorType& v,
		                    const PsimagLite::String& label)
		{
//			concurrency_.reduce(v,comm);
			v /= counter_;
			//! No comm below, only world root prints:
			if (!PsimagLite::Concurrency::root()) return;
			fout<<label<<"\n";
			fout<<v;
		}

		template<typename SomeOutputType>
		void divideAndPrint(SomeOutputType& fout,
		                    MatrixType& m,
		                    const PsimagLite::String& label)
		{
			//concurrency_.reduce(m);
			//if (!PsimagLite::Concurrency::root()) return;
			VectorType v(m.n_row(),0);
			for (SizeType dir=0;dir<m.n_col();dir++) {
				for (SizeType i=0;i<m.n_row();i++) v[i] =  m(i,dir);
				PsimagLite::String newlabel = label+ttos(dir);
				divideAndPrint(fout,v,newlabel);
			}
		}

		SpinOperationsType& spinOperations_;
		const GeometryType& geometry_;
		const ParametersModelType& mp_;
		SizeType dof_;
		VectorType lc_;
		VectorType chargeCor_;
		MatrixType mc_;
		MatrixType tc_;
		MatrixType cs_;
		MatrixType qs_;
		SizeType counter_;

	}; // PnictidesMultiOrbitalsObsStored

} // namespace Spf

/*@}*/
#endif // OBS_STORED_PNICTIDES_MULTIORBITALS_H

