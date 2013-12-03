/* 
 * File:   EUTelGBLFitter.cc
 * Contact: denys.lontkovskyi@desy.de
 * 
 * Created on January 25, 2013, 2:53 PM
 */

#ifdef USE_GBL

// eutelescope includes ".h"
#include "EUTelGeometryTelescopeGeoDescription.h"
#include "EUTelGBLFitter.h"
#include "EUTelTrackFitter.h"
#include "EUTELESCOPE.h"

// marlin util includes
#include "mille/Mille.h"

// ROOT
#if defined(USE_ROOT) || defined(MARLIN_USE_ROOT)
#include "TVector3.h"
#else
#error *** You need ROOT to compile this code.  *** 
#endif

// GBL
#include "include/GblTrajectory.h"
#include "include/GblPoint.h"
#include "include/GblData.h"
#include "include/BorderedBandMatrix.h"
#include "include/MilleBinary.h"
#include "include/VMatrix.h"

// LCIO includes <.h>
#include <EVENT/LCCollection.h>
#include <IMPL/LCCollectionVec.h>
#include <IMPL/TrackerHitImpl.h>
#include <IMPL/TrackImpl.h>
#include <IMPL/LCFlagImpl.h>
#include "LCIOTypes.h"
#include "lcio.h"

// system includes <>
#include <map>
#include <string>
#include <utility>
#include <vector>
#include <iomanip>
#include <iterator>
#include <algorithm>

namespace eutelescope {

    EUTelGBLFitter::EUTelGBLFitter() : EUTelTrackFitter("GBLTrackFitter"),
    _trackCandidates(),
    _gblTrackCandidates(),
    _fittrackvec(0),
    _parPropJac(5, 5),
    _eBeam(-1.),
    _hitId2GblPointLabel(),
    _alignmentMode(Utility::XYShiftXYRot),
    _mEstimatorType(),
    _mille(0),
    _paramterIdXShiftsMap(),
    _paramterIdYShiftsMap(),
    _paramterIdZShiftsMap(),
    _paramterIdXRotationsMap(),
    _paramterIdYRotationsMap(),
    _paramterIdZRotationsMap(),
    _excludeFromFit(),
    _chi2cut(1000.) {}

    EUTelGBLFitter::EUTelGBLFitter(std::string name) : EUTelTrackFitter(name),
    _trackCandidates(),
    _gblTrackCandidates(),
    _fittrackvec(0),
    _parPropJac(5, 5),
    _eBeam(-1.),
    _hitId2GblPointLabel(),
    _alignmentMode(Utility::XYShiftXYRot),
    _mEstimatorType(),
    _mille(0),
    _paramterIdXShiftsMap(),
    _paramterIdYShiftsMap(),
    _paramterIdZShiftsMap(),
    _paramterIdXRotationsMap(),
    _paramterIdYRotationsMap(),
    _paramterIdZRotationsMap(),
    _excludeFromFit(),
    _chi2cut(1000.) {}

    EUTelGBLFitter::~EUTelGBLFitter() {
    }
    
    void EUTelGBLFitter::setParamterIdPlaneVec( const std::vector<int>& vector)
    {
      _paramterIdPlaneVec = vector;
    }
 
    void EUTelGBLFitter::setParamterIdXResolutionVec( const std::vector<float>& vector)
    {
      _paramterIdXResolutionVec = vector;
    }
 
    void EUTelGBLFitter::setParamterIdYResolutionVec( const std::vector<float>& vector)
    {
      _paramterIdYResolutionVec = vector;
    }
       

    void EUTelGBLFitter::setParamterIdXRotationsMap( const std::map<int, int>& map ) {
        _paramterIdXRotationsMap = map;
    }
    
    void EUTelGBLFitter::setParamterIdYRotationsMap( const std::map<int, int>& map ) {
        _paramterIdYRotationsMap = map;
    }
    
    void EUTelGBLFitter::setParamterIdZRotationsMap( const std::map<int, int>& map ) {
        _paramterIdZRotationsMap = map;
    }

    void EUTelGBLFitter::setParamterIdZShiftsMap( const std::map<int, int>& map ) {
        _paramterIdZShiftsMap = map;
    }

    void EUTelGBLFitter::setParamterIdYShiftsMap( const std::map<int, int>& map ) {
        _paramterIdYShiftsMap = map;
    }

    void EUTelGBLFitter::setParamterIdXShiftsMap( const std::map<int, int>& map ) {
        _paramterIdXShiftsMap = map;
    }
    
    const std::map<int, int>& EUTelGBLFitter::getParamterIdXRotationsMap() const {
        return _paramterIdXRotationsMap;
    }
    
    const std::map<int, int>& EUTelGBLFitter::getParamterIdYRotationsMap() const {
        return _paramterIdYRotationsMap;
    }
    
    const std::map<int, int>& EUTelGBLFitter::getParamterIdZRotationsMap() const {
        return _paramterIdZRotationsMap;
    }

    const std::map<int, int>& EUTelGBLFitter::getParamterIdZShiftsMap() const {
        return _paramterIdZShiftsMap;
    }

    const std::map<int, int>& EUTelGBLFitter::getParamterIdYShiftsMap() const {
        return _paramterIdYShiftsMap;
    }

    const std::map<int, int>& EUTelGBLFitter::getParamterIdXShiftsMap() const {
        return _paramterIdXShiftsMap;
    }

    void EUTelGBLFitter::setMEstimatorType( const std::string& mEstimatorType ) {
        std::string mEstimatorTypeLowerCase = mEstimatorType;
        std::transform( mEstimatorType.begin(), mEstimatorType.end(), mEstimatorTypeLowerCase.begin(), ::tolower);
        
        if ( mEstimatorType.size() != 1 ) {
            streamlog_out( WARNING1 ) << "More than one character supplied as M-estimator option" << std::endl;
            streamlog_out( WARNING1 ) << "No M-estimator downweighting will be used" << std::endl;
            return;
        }
        
        if ( mEstimatorType.compare("t") == 0 ||
             mEstimatorType.compare("h") == 0 ||
             mEstimatorType.compare("c") == 0   ) this->_mEstimatorType = _mEstimatorType;
        else {
            streamlog_out( WARNING1 ) << "M-estimator option " << mEstimatorType << " was not recognized" << std::endl;
            streamlog_out( WARNING1 ) << "No M-estimator downweighting will be used" << std::endl;
        }
    }

    std::string EUTelGBLFitter::getMEstimatorType( ) const {
        return _mEstimatorType;
    }

    const std::map<long, int>& EUTelGBLFitter::getHitId2GblPointLabel( ) const {
        return _hitId2GblPointLabel;
    }

    void EUTelGBLFitter::setExcludeFromFitPlanes( const std::vector<int>& excludedPlanes ) {
        this->_excludeFromFit = excludedPlanes;
    }

    std::vector<int> EUTelGBLFitter::getExcludeFromFitPlanes( ) const {
        return _excludeFromFit;
    }

    TMatrixD EUTelGBLFitter::propagatePar(double ds) {
        /* for GBL:
           Jacobian for straight line track
           track = q/p, x', y', x, y
                    0,   1,  2, 3, 4
         */
        _parPropJac.UnitMatrix();
        _parPropJac[3][1] = ds; // x = x0 + xp * ds
        _parPropJac[4][2] = ds; // y = y0 + yp * ds
        return _parPropJac;
    }
    
    /** Propagation jacobian for solenoidal magnetic field
     * 
     *  This is C++ port of C. Kleinwort's python code b2thlx
     * 
     * @param ds        (3D) arc length to endpoint
     * @param phi       azimuthal direction at starting point
     * @param bfac      Magnetic field strength
     * @return 
     */
    /*
    TMatrixD EUTelGBLFitter::PropagatePar( double ds, double phi, double bfac ) {
        // for GBL:
        //   Jacobian for helical track
        //   track = q/p, 
        //            0,   1,  2, 3, 4
        //
        
        // TODO: undefined curvature. see klnwrt
        const double curvature          = 1.;
        // TODO: undefined dzds. see klnwrt
        const double dzds               = 1.;
        
        // at starting point
        const double cosLamStart        = 1./sqrt(1. + dzds*dzds);
        const double sinLamStart        = cosLamStart*dzds;
        
        // at end point
        const double cosLamEnd          = cosLamStart;
        const double sinLamEnd          = sinLamStart;
        const double invCosLamEnd       = 1./cosLamEnd;
        
        // direction of magnetic field
        TVector3 BFieldDir(0.,0.,1.);
        
        // track direction vectors
        const double phiEnd = phi * ds * cosLamStart * curvature;
        TVector3 t1(cosLamStart*cos(phi), cosLamStart*sin(phi), sinLamStart);
        TVector3 t2(cosLamEnd*cos(phiEnd), cosLamEnd*sin(phiEnd), sinLamEnd);
        
        
        const double q                  = curvature*cosLamStart;
        const double theta              = q * ds;
        const double sinTheta           = sin(theta);
        const double cosTheta           = cos(theta);
        const double gamma              = BFieldDir.Dot(t2);    // (B,T)
        
        TVector3 an1                    = BFieldDir.Cross(t1);  // B x T0
        TVector3 an2                    = BFieldDir.Cross(t2);  // B x T
        
        // U0, V0
        const double au1                = 1./t1.Perp2();
        TVector3 u1( -au1*t1[1], au1*t1[0], 0. );
        TVector3 v1( t1[2]*u1[2], t1[2]*u1[0], t1[0]*u1[1] - t1[1]*u1[0] );
        
        // U, V
        const double au2                = 1./t2.Perp2();
        TVector3 u2( -au2*t2[1], au1*t2[0], 0. );
        TVector3 v2( t2[2]*u2[2], t2[2]*u2[0], t2[0]*u2[1] - t2[1]*u2[0] );
        
        const double qp                 = -bfac;
        const double pav                = qp / cosLamStart / curvature;
        const double anv                = -BFieldDir.Dot(u2);
        const double anu                = -BFieldDir.Dot(v2);
        const double omCosTheta         = 1. - cosTheta;
        const double tmSinTheta         = theta - sinTheta;
        
        
        _parPropJac.Zero();
             // q/p
        _parPropJac[0][0] = 1.;
        _parPropJac[0][0] = 1.;     

        return _parPropJac;
    }
        */
    
    /** Propagation jacobian in inhomogeneous magnetic field
     * 
     * 
     * @param ds        Z - Z0 propagation distance
     * 
     * @return          transport matrix
     */
    
    TMatrixD EUTelGBLFitter::PropagatePar( double ds, double invP, double tx0, double ty0, double x0, double y0, double z0 ) {
        // for GBL:
        //   Jacobian for helical track
        //   track = q/p, x'  y'  x  y
        //            0,   1,  2, 3, 4
        //
        
        streamlog_out( DEBUG2 ) << "EUTelGBLFitter::PropagatePar()" << std::endl;
	// The formulas below are derived from equations of motion of the particle in
        // magnetic field under assumption |dz| small. Must be valid for |dz| < 20-30 cm

        // Get magnetic field vector
        gear::Vector3D vectorGlobal( x0, y0, z0 );        // assuming uniform magnetic field
	const gear::BField&   B = geo::gGeometry().getMagneticFiled();
        const double Bx         = B.at( vectorGlobal ).x();
        const double By         = B.at( vectorGlobal ).y();
        const double Bz         = B.at( vectorGlobal ).z();
	const double mm = 1000.;
	const double k = 0.299792458/mm;

        const double sqrtFactor = sqrt( 1. + tx0*tx0 + ty0*ty0 );

	const double Ax = sqrtFactor * (  ty0 * ( tx0 * Bx + Bz ) - ( 1. + tx0*tx0 ) * By );
	const double Ay = sqrtFactor * ( -tx0 * ( ty0 * By + Bz ) + ( 1. + ty0*ty0 ) * Bx );

	// Partial derivatives
	const double dAxdty0 = ty0 * Ax / (sqrtFactor*sqrtFactor) + sqrtFactor*( tx0*Bx + Bz );
	const double dAydtx0 = tx0 * Ay / (sqrtFactor*sqrtFactor) + sqrtFactor*( -ty0*By - Bz );

	const double dxdtx0 = ds;
	const double dxdty0 = 0.5 * invP * k * ds*ds * dAxdty0;

	const double dydtx0 = 0.5 * invP * k * ds*ds * dAydtx0;
	const double dydty0 = ds;

	const double dtxdty0 = invP * k * ds * dAxdty0;
	const double dtydtx0 = invP * k * ds * dAydtx0;

	const double dxdinvP0 = 0.5 * k * ds*ds * Ax;
	const double dydinvP0 = 0.5 * k * ds*ds * Ay;

	const double dtxdinvP0 = k * ds * Ax;
	const double dtydinvP0 = k * ds * Ay;

	// Fill-in matrix elements
	_parPropJac.UnitMatrix();
	_parPropJac[1][0] = dtxdinvP0;  _parPropJac[1][2] = dtxdty0;	
	_parPropJac[2][0] = dtydinvP0;  _parPropJac[2][1] = dtydtx0;	
	_parPropJac[3][0] = dxdinvP0;   _parPropJac[3][1] = dxdtx0;	_parPropJac[3][2] = dxdty0;	
	_parPropJac[4][0] = dydinvP0;   _parPropJac[4][1] = dydtx0;	_parPropJac[4][2] = dydty0;	
        
        if ( streamlog_level(DEBUG0) ){
             streamlog_out( DEBUG0 ) << "Propagation jacobian: " << std::endl;
            _parPropJac.Print();
        }
	
        streamlog_out( DEBUG2 ) << "-----------------------------EUTelGBLFitter::PropagatePar()-------------------------------" << std::endl;   

        return _parPropJac;
    }
        
    void EUTelGBLFitter::SetTrackCandidates( std::vector< EVENT::TrackerHitVec>& trackCandidates ) {
        this->_trackCandidates = trackCandidates;
        return;
    }

    double EUTelGBLFitter::interpolateTrackX(const EVENT::TrackerHitVec& trackCand, const double z) const {
        const int planeIDStart = Utility::GuessSensorID( static_cast< IMPL::TrackerHitImpl* >(trackCand.front()) );
        const double* hitPointLocalStart = trackCand.front()->getPosition();
        double hitposStart[] = {hitPointLocalStart[0],hitPointLocalStart[1],hitPointLocalStart[2]};
        double hitPointGlobalStart[] = {0.,0.,0.};
        geo::gGeometry().local2Master(planeIDStart,hitposStart,hitPointGlobalStart);
        
        double x0 = hitPointGlobalStart[0];
        double z0 = hitPointGlobalStart[2];

        double x = x0 - getTrackSlopeX(trackCand) * (z0 - z);

        return x;
    }
    
//    double EUTelGBLFitter::interpolateTrackX1(const EVENT::TrackerHitVec& trackCand, const double z) const {
//        streamlog_out(DEBUG2) << "EUTelGBLFitter::interpolateTrackX()" << std::endl;
//        
//        // Get starting track position
//        const float* x = ts->getReferencePoint();
//        const double x0 = x[0];
//        const double y0 = x[1];
//        const double z0 = x[2];
//        
//        // Get magnetic field vector
//        gear::Vector3D vectorGlobal( x0, y0, z0 );        // assuming uniform magnetic field running along X direction
//	const gear::BField&   B = geo::gGeometry().getMagneticFiled();
//        const double bx         = B.at( vectorGlobal ).x();
//        const double by         = B.at( vectorGlobal ).y();
//        const double bz         = B.at( vectorGlobal ).z();
//        TVector3 hVec(bx,by,bz);
//               
//        TVector3 pVec = getPfromCartesianParameters( ts );
//
//	const double H = hVec.Mag();
//        const double p = pVec.Mag();
//	const double mm = 1000.;
//        const double k = -0.299792458/mm*_beamQ*H;
//        const double rho = k/p;
//        
//        // Calculate end track position
//	TVector3 pos( x0, y0, z0 );
//	if ( fabs( k ) > 1.E-6  ) {
//		// Non-zero magnetic field case
//		TVector3 pCrossH = pVec.Cross(hVec.Unit());
//		TVector3 pCrossHCrossH = pCrossH.Cross(hVec.Unit());
//		const double pDotH = pVec.Dot(hVec.Unit());
//		TVector3 temp1 = pCrossHCrossH;	temp1 *= ( -1./k * sin( rho * s ) );
//		TVector3 temp2 = pCrossH;       temp2 *= ( -1./k * ( 1. - cos( rho * s ) ) );
//		TVector3 temp3 = hVec;          temp3 *= ( pDotH / p * s );
//		pos += temp1;
//		pos += temp2;
//		pos += temp3;
//        } else {
//		// Vanishing magnetic field case
//		const double cosA = cosAlpha( ts );
//		const double cosB = cosBeta( ts );
//		pos.SetX( x0 + cosA * s );
//		pos.SetY( y0 + cosB * s );
//		pos.SetZ( z0 + 1./p * pVec.Z() * s );
//	}
//        
//        streamlog_out(DEBUG2) << "---------------------------------EUTelGBLFitter::interpolateTrackX()------------------------------------" << std::endl;
//        
//        return pos;
//    }

    //! Predict track hit in Y direction using simplified model

    double EUTelGBLFitter::interpolateTrackY(const EVENT::TrackerHitVec& trackCand, const double z) const {
        const int planeIDStart = Utility::GuessSensorID( static_cast< IMPL::TrackerHitImpl* >(trackCand.front()) );
        const double* hitPointLocalStart = trackCand.front()->getPosition();
        double hitposStart[] = {hitPointLocalStart[0],hitPointLocalStart[1],hitPointLocalStart[2]};
        double hitPointGlobalStart[] = {0.,0.,0.};
        geo::gGeometry().local2Master(planeIDStart,hitposStart,hitPointGlobalStart);
        
        double y0 = hitPointGlobalStart[1];
        double z0 = hitPointGlobalStart[2];

        double y = y0 - getTrackSlopeY(trackCand) * (z0 - z);

        return y;
    }
    
//    double EUTelGBLFitter::interpolateTrackY1(const EVENT::TrackerHitVec& trackCand, const double z) const {
//        streamlog_out(DEBUG2) << "EUTelGBLFitter::interpolateTrackY()" << std::endl;
//        
//        // Get starting track position
//        const float* x = ts->getReferencePoint();
//        const double x0 = x[0];
//        const double y0 = x[1];
//        const double z0 = x[2];
//        
//        // Get magnetic field vector
//        gear::Vector3D vectorGlobal( x0, y0, z0 );        // assuming uniform magnetic field running along X direction
//	const gear::BField&   B = geo::gGeometry().getMagneticFiled();
//        const double bx         = B.at( vectorGlobal ).x();
//        const double by         = B.at( vectorGlobal ).y();
//        const double bz         = B.at( vectorGlobal ).z();
//        TVector3 hVec(bx,by,bz);
//               
//        TVector3 pVec = getPfromCartesianParameters( ts );
//
//	const double H = hVec.Mag();
//        const double p = pVec.Mag();
//	const double mm = 1000.;
//        const double k = -0.299792458/mm*_beamQ*H;
//        const double rho = k/p;
//        
//        // Calculate end track position
//	TVector3 pos( x0, y0, z0 );
//	if ( fabs( k ) > 1.E-6  ) {
//		// Non-zero magnetic field case
//		TVector3 pCrossH = pVec.Cross(hVec.Unit());
//		TVector3 pCrossHCrossH = pCrossH.Cross(hVec.Unit());
//		const double pDotH = pVec.Dot(hVec.Unit());
//		TVector3 temp1 = pCrossHCrossH;	temp1 *= ( -1./k * sin( rho * s ) );
//		TVector3 temp2 = pCrossH;       temp2 *= ( -1./k * ( 1. - cos( rho * s ) ) );
//		TVector3 temp3 = hVec;          temp3 *= ( pDotH / p * s );
//		pos += temp1;
//		pos += temp2;
//		pos += temp3;
//        } else {
//		// Vanishing magnetic field case
//		const double cosA = cosAlpha( ts );
//		const double cosB = cosBeta( ts );
//		pos.SetX( x0 + cosA * s );
//		pos.SetY( y0 + cosB * s );
//		pos.SetZ( z0 + 1./p * pVec.Z() * s );
//	}
//        
//        streamlog_out(DEBUG2) << "---------------------------------EUTelGBLFitter::interpolateTrackY()------------------------------------" << std::endl;
//        
//        return pos;
//    }

    double EUTelGBLFitter::getTrackSlopeX1(const EVENT::TrackerHitVec& trackCand) const {
        double x0 = trackCand.front()->getPosition()[0];
        double z0 = trackCand.front()->getPosition()[2];

        double xLast = trackCand.back()->getPosition()[0];
        double zLast = trackCand.back()->getPosition()[2];

        double kx = (x0 - xLast) / (z0 - zLast);

        return kx;
    }
    
    double EUTelGBLFitter::getTrackSlopeX(const EVENT::TrackerHitVec& trackCand) const {
        const int planeIDStart = Utility::GuessSensorID( static_cast< IMPL::TrackerHitImpl* >(trackCand.front()) );
        const double* hitPointLocalStart = trackCand.front()->getPosition();
        double hitposStart[] = {hitPointLocalStart[0],hitPointLocalStart[1],hitPointLocalStart[2]};
        double hitPointGlobalStart[] = {0.,0.,0.};
        geo::gGeometry().local2Master(planeIDStart,hitposStart,hitPointGlobalStart);
        
        double x0 = hitPointGlobalStart[0];
        double z0 = hitPointGlobalStart[2];

        const int planeIDFinish = Utility::GuessSensorID( static_cast< IMPL::TrackerHitImpl* >(trackCand.back()) );
        const double* hitPointLocalFinish = trackCand.back()->getPosition();
        double hitposFinish[] = {hitPointLocalFinish[0],hitPointLocalFinish[1],hitPointLocalFinish[2]};
        double hitPointGlobalFinish[] = {0.,0.,0.};
        geo::gGeometry().local2Master(planeIDFinish,hitposFinish,hitPointGlobalFinish);
        
        double xLast = hitPointGlobalFinish[0];
        double zLast = hitPointGlobalFinish[2];

        double kx = (x0 - xLast) / (z0 - zLast);

        return kx;
    }

    double EUTelGBLFitter::getTrackSlopeY1(const EVENT::TrackerHitVec& trackCand) const {
        double y0 = trackCand.front()->getPosition()[1];
        double z0 = trackCand.front()->getPosition()[2];

        double yLast = trackCand.back()->getPosition()[1];
        double zLast = trackCand.back()->getPosition()[2];

        double ky = (y0 - yLast) / (z0 - zLast);

        return ky;
    }
    
    double EUTelGBLFitter::getTrackSlopeY(const EVENT::TrackerHitVec& trackCand) const {
        const int planeIDStart = Utility::GuessSensorID( static_cast< IMPL::TrackerHitImpl* >(trackCand.front()) );
        const double* hitPointLocalStart = trackCand.front()->getPosition();
        double hitposStart[] = {hitPointLocalStart[0],hitPointLocalStart[1],hitPointLocalStart[2]};
        double hitPointGlobalStart[] = {0.,0.,0.};
        geo::gGeometry().local2Master(planeIDStart,hitposStart,hitPointGlobalStart);
        
        double y0 = hitPointGlobalStart[1];
        double z0 = hitPointGlobalStart[2];

        const int planeIDFinish = Utility::GuessSensorID( static_cast< IMPL::TrackerHitImpl* >(trackCand.back()) );
        const double* hitPointLocalFinish = trackCand.back()->getPosition();
        double hitposFinish[] = {hitPointLocalFinish[0],hitPointLocalFinish[1],hitPointLocalFinish[2]};
        double hitPointGlobalFinish[] = {0.,0.,0.};
        geo::gGeometry().local2Master(planeIDFinish,hitposFinish,hitPointGlobalFinish);
        
        double yLast = hitPointGlobalFinish[1];
        double zLast = hitPointGlobalFinish[2];

        double ky = (y0 - yLast) / (z0 - zLast);

        return ky;
    }

    void EUTelGBLFitter::Reset() {
        if (!_gblTrackCandidates.empty()) {
            std::map< int, gbl::GblTrajectory* >::iterator it;
            for (it = _gblTrackCandidates.begin(); it != _gblTrackCandidates.begin(); ++it) delete it->second;
        }
        _gblTrackCandidates.clear();
//        _fittrackvec->clear();
        _hitId2GblPointLabel.clear();
    }

    /** Add a measurement to GBL point
     * 
     * @param point
     * @param meas measuremet vector (residuals) to be calculated in this routine
     * @param measPrec residuals weights (1/unc^2) to be calculated in this routine
     * @param hitpos hit position
     * @param xPred predicted by hit x-position (first approximation)
     * @param yPred predicted by hit y-position (first approximation)
     * @param hitcov hit covariance matrix
     * @param proL2m projection matrix from track coordinate system onto measurement system
     */
    void EUTelGBLFitter::addMeasurementsGBL(gbl::GblPoint& point, TVectorD& meas, TVectorD& measPrec, const double* hitpos,
            double xPred, double yPred, const EVENT::FloatVec& hitcov, TMatrixD& proL2m) {
        meas[0] = hitpos[0] - xPred;
        meas[1] = hitpos[1] - yPred;
        measPrec[0] = 1. / hitcov[0];
        measPrec[1] = 1. / hitcov[2];

        streamlog_out(DEBUG0) << "Residuals:" << std::endl;
        streamlog_out(DEBUG0) << "X:" << std::setw(20) << meas[0] << std::setw(20) << measPrec[0] << std::endl;
        streamlog_out(DEBUG0) << "Y:" << std::setw(20) << meas[1] << std::setw(20) << measPrec[1] << std::endl;

        point.addMeasurement(proL2m, meas, measPrec);
    }

    /** Add a acatterer to GBL point
     * Add Si + Kapton thin scatterer to a point
     * 
     * @param point
     * @param scat average scattering angle
     * @param scatPrecSensor 1/RMS^2 of the scattering angle (ususally determined from Highland's formula)
     * @param iPlane plane id
     * @param p momentum of the particle
     */
    void EUTelGBLFitter::addSiPlaneScattererGBL(gbl::GblPoint& point, TVectorD& scat, TVectorD& scatPrecSensor, int planeID, double p) {
        const int iPlane = geo::gGeometry().sensorIDtoZOrder(planeID);
        const double radlenSi           = geo::gGeometry()._siPlanesLayerLayout->getSensitiveRadLength(iPlane);
        const double radlenKap          = geo::gGeometry()._siPlanesLayerLayout->getLayerRadLength(iPlane);
        const double thicknessSi        = geo::gGeometry()._siPlanesLayerLayout->getSensitiveThickness(iPlane);
        const double thicknessKap       = geo::gGeometry()._siPlanesLayerLayout->getLayerThickness(iPlane);

        const double X0Si = thicknessSi / radlenSi; // Si 
        const double X0Kap = thicknessKap / radlenKap; // Kapton                

        const double tetSi = Utility::getThetaRMSHighland(p, X0Si);
        const double tetKap = Utility::getThetaRMSHighland(p, X0Kap);

        scatPrecSensor[0] = 1.0 / (tetSi * tetSi + tetKap * tetKap);
        scatPrecSensor[1] = 1.0 / (tetSi * tetSi + tetKap * tetKap);

        point.addScatterer(scat, scatPrecSensor);
    }

    // @TODO iplane, xPred, yPred must not be here. consider refactoring

    /** Add alignment derivative necessary for MILLIPEDE
     * 
     * @param point
     * @param alDer matrix of global parameter (alignment constants) derivatives 
     * @param globalLabels vector of alignment parameters ids
     * @param iPlane plane id
     * @param xPred predicted by hit x-position (first approximation)
     * @param yPred predicted by hit y-position (first approximation)
     * @param xSlope predicted x-slope
     * @param ySlope predicted y-slope
     */
    void EUTelGBLFitter::addGlobalParametersGBL(gbl::GblPoint& point, TMatrixD& alDer, std::vector<int>& globalLabels, int iPlane,
            double xPred, double yPred, double xSlope, double ySlope) {
        alDer[0][0] = 1.0; // dx/dx
        alDer[0][1] = 0.0; // dx/dy
        alDer[1][0] = 0.0; // dy/dx
        alDer[1][1] = 1.0; // dy/dy
        globalLabels[0] = _paramterIdXShiftsMap[iPlane]; // dx
        globalLabels[1] = _paramterIdYShiftsMap[iPlane]; // dy
        if (_alignmentMode == Utility::XYShiftXYRot
                || _alignmentMode == Utility::XYZShiftXYRot
                || _alignmentMode == Utility::XYShiftXZRotXYRot
                || _alignmentMode == Utility::XYShiftYZRotXYRot
                || _alignmentMode == Utility::XYShiftXZRotYZRotXYRot
                || _alignmentMode == Utility::XYZShiftXZRotYZRotXYRot) {
            alDer[0][2] = -yPred; // dx/rot
            alDer[1][2] = xPred; // dy/rot
            globalLabels[2] = _paramterIdZRotationsMap[iPlane]; // rot z
        }
        if (_alignmentMode == Utility::XYZShiftXYRot
                || _alignmentMode == Utility::XYZShiftXZRotYZRotXYRot) {
            alDer[0][3] = xSlope; // dx/dz
            alDer[1][3] = ySlope; // dy/dz
            globalLabels[3] = _paramterIdZShiftsMap[iPlane]; // dz
        }
        if (_alignmentMode == Utility::XYZShiftXZRotYZRotXYRot) {
            alDer[0][4] = xPred*xSlope; // dx/rot y
            alDer[1][4] = xPred*ySlope; // dy/rot y
            globalLabels[4] = _paramterIdYRotationsMap[iPlane]; // drot y
            alDer[0][5] = yPred*xSlope; // dx/rot x
            alDer[1][5] = yPred*ySlope; // dy/rot x
            globalLabels[5] = _paramterIdXRotationsMap[iPlane]; // drot x
        }
        if (_alignmentMode == Utility::XYShiftXZRotXYRot) {
            alDer[0][3] = xPred*xSlope; // dx/rot y
            alDer[1][3] = xPred*ySlope; // dy/rot y
            globalLabels[3] = _paramterIdYRotationsMap[iPlane]; // drot y
        }
        if (_alignmentMode == Utility::XYShiftYZRotXYRot) {
            alDer[0][3] = yPred*xSlope; // dx/rot x
            alDer[1][3] = yPred*ySlope; // dy/rot x
            globalLabels[3] = _paramterIdXRotationsMap[iPlane]; // drot x
        }
        if (_alignmentMode == Utility::XYShiftXZRotYZRotXYRot) {
            alDer[0][3] = xPred*xSlope; // dx/rot y
            alDer[1][3] = xPred*ySlope; // dy/rot y
            globalLabels[3] = _paramterIdYRotationsMap[iPlane]; // drot y
            alDer[0][4] = yPred*xSlope; // dx/rot x
            alDer[1][4] = yPred*ySlope; // dy/rot x
            globalLabels[4] = _paramterIdXRotationsMap[iPlane]; // drot x
        }
        if (_alignmentMode == Utility::XYShiftXZRotYZRotXYRot) {
            alDer[0][3] = xPred*xSlope; // dx/rot y
            alDer[1][3] = xPred*ySlope; // dy/rot y
            globalLabels[3] = _paramterIdYRotationsMap[iPlane]; // drot y
            alDer[0][4] = yPred*xSlope; // dx/rot x
            alDer[1][4] = yPred*ySlope; // dy/rot x
            globalLabels[4] = _paramterIdXRotationsMap[iPlane]; // drot x
        }

        point.addGlobals(globalLabels, alDer);
    }

    void EUTelGBLFitter::pushBackPoint( std::vector< gbl::GblPoint >& pointList, const gbl::GblPoint& point, int hitid ) {
        pointList.push_back(point);
        
        // store point's GBL label for future reference
        _hitId2GblPointLabel[ hitid ] = static_cast<int>(pointList.size());
    }
    /**
     * Set track omega, D0, Z0, Phi, tan(Lambda), Chi2, NDF parameters
     * and add it to the LCIO collection of fitted tracks
     * 
     * @param fittrack pointer to track object to be stored
     * @param chi2     Chi2 of the track fit
     * @param ndf      NDF of the track fit
     */
    void EUTelGBLFitter::prepareLCIOTrack( IMPL::TrackImpl* fittrack, gbl::GblTrajectory* gblTraj, const EVENT::TrackerHitVec& trackCandidate, 
                                          double chi2, int ndf, double omega, double d0, double z0, double phi, double tanlam ) {
        // prepare output collection
        try {
            _fittrackvec = new IMPL::LCCollectionVec( EVENT::LCIO::TRACK );
            IMPL::LCFlagImpl flag( _fittrackvec->getFlag( ) );
            flag.setBit( lcio::LCIO::TRBIT_HITS );
            _fittrackvec->setFlag( flag.getFlag( ) );
        } catch ( ... ) {
            streamlog_out( WARNING2 ) << "Can't allocate output collection" << std::endl;
        }
        
        // prepare track
        float refpoint[3] = { 0., 0., 0. };
        fittrack->setReferencePoint( refpoint );
        
        fittrack->setChi2 ( chi2 );      // Chi2 of the fit (including penalties)
        fittrack->setNdf  ( ndf );        // Number of planes fired (!)
        fittrack->setOmega( omega );       // curvature of the track
        fittrack->setD0   ( d0 );          // impact paramter of the track in (r-phi)
        fittrack->setZ0   ( z0 );          // impact paramter of the track in (r-z)
        fittrack->setPhi  ( phi );         // phi of the track at reference point
        fittrack->setTanLambda( tanlam );   // dip angle of the track at reference point
        
        unsigned int numData;
        TVectorD residual(200);
        TVectorD measErr(200);
        TVectorD residualErr(200);
        TVectorD downWeight(200);

        EVENT::TrackerHitVec::const_iterator itrHit;
        for ( itrHit = trackCandidate.begin(); itrHit != trackCandidate.end(); ++itrHit ) {
            
            const int hitGblLabel = _hitId2GblPointLabel[ (*itrHit)->id() ];
            IMPL::TrackerHitImpl* hit = new IMPL::TrackerHitImpl();
                  
            gblTraj->getMeasResults( hitGblLabel, numData, residual, measErr, residualErr, downWeight);
            
            // retrieve original hit coordinates
            double pos[3] = { (*itrHit)->getPosition()[0], (*itrHit)->getPosition()[1], (*itrHit)->getPosition()[2] };
            
            // correct original values to the fitted ones
            pos[0] -= residual[0];
            pos[1] -= residual[1];
            
            hit -> setPosition(pos);
            
            // prepare covariance matrix
            float cov[TRKHITNCOVMATRIX] = {0.,0.,0.,0.,0.,0.};
            cov[0]=measErr[0]*measErr[0];
            cov[2]=measErr[1]*measErr[1];
            
            hit -> setCovMatrix(cov);
            
            fittrack->addHit( hit );
        } // loop over track's hits

        // add track to LCIO collection vector
        _fittrackvec->push_back( fittrack );
    }
    
    void EUTelGBLFitter::FitTracks() {
        Reset(); // 

        streamlog_out(DEBUG2) << " EUTelGBLFitter::FitTracks() " << std::endl;
        streamlog_out(DEBUG1) << " N track candidates:" << static_cast<int>(_trackCandidates.size()) << std::endl;

        TVectorD meas(2);
        TVectorD measPrec(2); // precision = 1/resolution^2

        TVectorD scat(2);
        scat.Zero();

        TVectorD scatPrecSensor(2);
        TVectorD scatPrec(2);

        TMatrixD alDer; // alignment derivatives
        std::vector<int> globalLabels;
        if (_alignmentMode == Utility::XYShift) {
            globalLabels.resize(2);
            alDer.ResizeTo(2, 2);
        } else if (_alignmentMode == Utility::XYShiftXYRot) {
            globalLabels.resize(3);
            alDer.ResizeTo(2, 3);
        } else if (_alignmentMode == Utility::XYZShiftXYRot) {
            globalLabels.resize(4);
            alDer.ResizeTo(2, 4);
        } else if (_alignmentMode == Utility::XYShiftYZRotXYRot) {
            globalLabels.resize(4);
            alDer.ResizeTo(2, 4);
        } else if (_alignmentMode == Utility::XYShiftXZRotXYRot) {
            globalLabels.resize(4);
            alDer.ResizeTo(2, 4);
        } else if (_alignmentMode == Utility::XYShiftXZRotYZRotXYRot) {
            globalLabels.resize(5);
            alDer.ResizeTo(2, 5);
        } else if (_alignmentMode == Utility::XYZShiftXZRotYZRotXYRot) {
            globalLabels.resize(6);
            alDer.ResizeTo(2, 6);
        }
        alDer.Zero();

        double p = _eBeam; // beam momentum
       
//        EVENT::TrackVec::const_iterator itTrkCand;
        std::vector< EVENT::TrackerHitVec >::const_iterator itTrkCand;
        EVENT::TrackerHitVec::const_iterator itHit;
        for ( itTrkCand = _trackCandidates.begin(); itTrkCand != _trackCandidates.end(); ++itTrkCand) {

            // sanity check. Mustn't happen in principle.
            if (itTrkCand->size() > geo::gGeometry().nPlanes()) continue;

            IMPL::TrackImpl * fittrack = new IMPL::TrackImpl();

            //GBL trajectory construction
            std::vector< gbl::GblPoint > pointList;

            TMatrixD jacPointToPoint(5, 5);
            jacPointToPoint.UnitMatrix();
            double step = 0.;
            
            for ( itHit = itTrkCand->begin(); itHit != itTrkCand->end(); ++itHit) {
                const int planeID = Utility::GuessSensorID( static_cast< IMPL::TrackerHitImpl* >(*itHit) );

                // Go to global coordinates
                const double* hitPointLocal = (*itHit)->getPosition();
                double hitPointGlobal[] = {0.,0.,0.};
                geo::gGeometry().local2Master(planeID,hitPointLocal,hitPointGlobal);

//                const EVENT::FloatVec hitcov = (*itHit)->getCovMatrix();
                EVENT::FloatVec hitcov(4);
                hitcov[0]=0.01;
                hitcov[1]=0.00;
                hitcov[2]=0.01;
                hitcov[3]=0.00;

                // check Processor Parameters for plane resolution // Denys
                if( _paramterIdXResolutionVec.size() > 0 && _paramterIdYResolutionVec.size() > 0 )
                {
                  for(int izPlane=0;izPlane<_paramterIdPlaneVec.size();izPlane++)
                  {
                    if( _paramterIdPlaneVec[izPlane] == planeID )
                    {  
                      hitcov[0] =  _paramterIdXResolutionVec[izPlane];
                      hitcov[2] =  _paramterIdYResolutionVec[izPlane];
 
                      hitcov[0] *= hitcov[0]; // squared !
                      hitcov[2] *= hitcov[2]; // squared !
                      break;
                    } 
                  }
                }
                else
                {  
                  hitcov = (*itHit)->getCovMatrix();            
                  
                }

                streamlog_out(DEBUG0) << "Hit covariance matrix: [0: "  << hitcov[0] << "] [1: "  << hitcov[1] << "] [2: "  << hitcov[2] << "] [3: " << hitcov[3]  << std::endl;

                double xPred = interpolateTrackX(*itTrkCand, hitPointGlobal[2]);
                double yPred = interpolateTrackY(*itTrkCand, hitPointGlobal[2]);

                double xSlope = getTrackSlopeX(*itTrkCand);
                double ySlope = getTrackSlopeY(*itTrkCand);

                gbl::GblPoint point(jacPointToPoint);
                TMatrixD proL2m(2, 2);
                proL2m.UnitMatrix();
                bool excludeFromFit = false;
                if ( std::find( _excludeFromFit.begin(), _excludeFromFit.end(), planeID ) != _excludeFromFit.end() ) excludeFromFit = true;
                if ( !excludeFromFit ) addMeasurementsGBL(point, meas, measPrec, hitPointGlobal, xPred, yPred, hitcov, proL2m);
                addSiPlaneScattererGBL(point, scat, scatPrecSensor, planeID, p);
                if (_alignmentMode != Utility::noAlignment) {
                    if ( !excludeFromFit ) addGlobalParametersGBL( point, alDer, globalLabels, planeID, xPred, yPred, xSlope, ySlope );
                }
                pushBackPoint( pointList, point, (*itHit)->id() );

                // construct effective scatters for air
                // the scatters must be at (Z(plane i) + Z(plane i+1))/2. +/- (Z(plane i) - Z(plane i+1))/sqrt(12)
                if ( itHit != (itTrkCand->end() - 1) ) {

                    // Go to global coordinates
                    const int nextPlaneID = Utility::GuessSensorID( static_cast< IMPL::TrackerHitImpl* >(*(itHit + 1)) );
                    const double* nextHitPoint = (*(itHit + 1))->getPosition();
                    double nextHitPointGlobal[] = { 0., 0., 0. };
                    geo::gGeometry().local2Master(nextPlaneID,nextHitPoint,nextHitPointGlobal);
                    
                    const double hitSpacing = sqrt( (nextHitPointGlobal[0]-hitPointGlobal[0])*(nextHitPointGlobal[0]-hitPointGlobal[0])+
                                                           (nextHitPointGlobal[1]-hitPointGlobal[1])*(nextHitPointGlobal[1]-hitPointGlobal[1])+
                                                           (nextHitPointGlobal[2]-hitPointGlobal[2])*(nextHitPointGlobal[2]-hitPointGlobal[2]) ); 
                    
		    double rad = geo::gGeometry().findRadLengthIntegral( hitPointGlobal, nextHitPointGlobal, true );
                    streamlog_out(DEBUG0) << "Rad length( " << planeID << "-" << nextPlaneID << "): " << rad << std::endl;
                    
                    double sigmaTheta = Utility::getThetaRMSHighland(p, rad);
                    scatPrec[0] = 1.0 / (sigmaTheta * sigmaTheta);
                    scatPrec[1] = 1.0 / (sigmaTheta * sigmaTheta);

                    // propagate parameters into the air gap between consecutive planes 

                    {   // downstream air scatterer
                        step = hitSpacing / 2. - hitSpacing / sqrt(12.);
                        jacPointToPoint = propagatePar(step);
                        gbl::GblPoint pointInAir1(jacPointToPoint);
                        pointInAir1.addScatterer(scat, scatPrec);
                        pointList.push_back(pointInAir1);
                    }
                    
                    {   // upstream air scatterer
                        step = 2. * hitSpacing / sqrt( 12. );
                        jacPointToPoint = propagatePar( step );
                        gbl::GblPoint pointInAir2( jacPointToPoint );
                        pointInAir2.addScatterer( scat, scatPrec );
                        pointList.push_back( pointInAir2 );
                    }
                    
                    // propagate to the next hit
                    {
                        step = hitSpacing / 2. - hitSpacing / sqrt( 12. );
                        jacPointToPoint = propagatePar( step );
                    }
                } // if not the last hit
            } // loop over hits

            double loss = 0.;
            double chi2 = 0.;
            int ndf = 0;
            // perform GBL fit
            {
                // check magnetic field at (0,0,0)
                const gear::BField&   B = geo::gGeometry().getMagneticFiled();
                const double Bmag       = B.at( TVector3(0.,0.,0.) ).r2();
                
                gbl::GblTrajectory* traj;
                if ( Bmag < 1.E-6 ) {
                   traj = new gbl::GblTrajectory( pointList, false );
                } else {
                   traj = new gbl::GblTrajectory( pointList, true );
                }
                int ierr = 0;
                if ( !_mEstimatorType.empty( ) ) ierr = traj->fit( chi2, ndf, loss, _mEstimatorType );
                else ierr = traj->fit( chi2, ndf, loss );

                if ( chi2 < _chi2cut ) {
                    if ( !ierr ) traj->milleOut( *_mille );
                }
                
                _gblTrackCandidates.insert( std::make_pair(0, traj ) );
                
                // Write fit result
                {
                    prepareLCIOTrack( fittrack, traj, (*itTrkCand), chi2, ndf, 0., 0., 0., 0., 0. );
                }
            }
        } // loop over supplied track candidates

        return;
    } // EUTelGBLFitter::FitTracks()
} // namespace eutelescope

#endif
