
#include <visp/vpCalibration.h>
#include <visp/vpMath.h>
#include <visp/vpPose.h>
#include <visp/vpPixelMeterConversion.h>
#undef MAX
#undef MIN
 
void
vpCalibration::calibLagrange(
  vpCameraParameters &cam, vpHomogeneousMatrix &cMo)
{

  vpMatrix A(2*npt,3) ;
  vpMatrix B(2*npt,9) ;


  LoX.front() ;
  LoY.front() ;
  LoZ.front() ;
  Lip.front() ;

  vpImagePoint ip;

  for (unsigned int i = 0 ; i < npt ; i++)
  {

    double x0 = LoX.value() ;
    double y0 = LoY.value() ;
    double z0 = LoZ.value() ;

    ip = Lip.value();
    
    double xi = ip.get_u()  ;
    double yi = ip.get_v()  ;

    A[2*i][0] = x0*xi;
    A[2*i][1] = y0*xi;
    A[2*i][2] = z0*xi;
    B[2*i][0] = -x0;
    B[2*i][1] = -y0;
    B[2*i][2] = -z0;
    B[2*i][3] = 0.0;
    B[2*i][4] = 0.0;
    B[2*i][5] = 0.0;
    B[2*i][6] = -1.0;
    B[2*i][7] = 0.0;
    B[2*i][8] = xi;
    A[2*i+1][0] = x0*yi;
    A[2*i+1][1] = y0*yi;
    A[2*i+1][2] = z0*yi;
    B[2*i+1][0] = 0.0;
    B[2*i+1][1] = 0.0;
    B[2*i+1][2] = 0.0;
    B[2*i+1][3] = -x0;
    B[2*i+1][4] = -y0;
    B[2*i+1][5] = -z0;
    B[2*i+1][6] = 0.0;
    B[2*i+1][7] = -1.0;
    B[2*i+1][8] = yi;

    LoX.next() ;
    LoY.next() ;
    LoZ.next() ;
    Lip.next() ;
  }


  vpMatrix BtB ;              /* compute B^T B  */
  BtB = B.t() * B ;

  /* compute (B^T B)^(-1)         */
  /* input : btb    (dimension 9 x 9) = (B^T B)     */
  /* output : btbinv  (dimension 9 x 9) = (B^T B)^(-1)  */

  vpMatrix BtBinv ;
  BtBinv = BtB.pseudoInverse(1e-16) ;

  vpMatrix BtA ;
  BtA = B.t()*A ;       /* compute B^T A  */


  vpMatrix r ;
  r = BtBinv*BtA ;  /* compute (B^T B)^(-1) B^T A */

  vpMatrix e  ;      /* compute - A^T B (B^T B)^(-1) B^T A*/
  e = -(A.t()*B)*r ;

  vpMatrix AtA ;     /* compute A^T A */
  AtA = A.AtA() ;

  e += AtA ;  /* compute E = A^T A - A^T B (B^T B)^(-1) B^T A */

  vpColVector x1(3) ;
  vpColVector x2 ;

  e.svd(x1,AtA) ;// destructive on e
  // eigenvector computation of E corresponding to the min eigenvalue.
  /* SVmax  computation*/
  double  svm = 0.0;
  int imin = 1;
  for (int i=0;i<x1.getRows();i++)
  {
    if (x1[i] > svm)
    {
      svm = x1[i];
      imin = i;
    }
  }

  svm *= 0.1; /* for the rank */

  for (int i=0;i<x1.getRows();i++)
  {
    if (x1[i] < x1[imin]) imin = i;
  }

  for (int i=0;i<x1.getRows();i++)
    x1[i] = AtA[i][imin];

  x2 = - (r*x1) ; // X_2 = - (B^T B)^(-1) B^T A X_1


  vpColVector sol(12) ;
  vpColVector resul(7) ;
  for (int i=0;i<3;i++) sol[i] = x1[i]; /* X_1  */
  for (int i=0;i<9;i++)       /* X_2 = - (B^T B)^(-1) B^T A X_1 */
  {
    sol[i+3] = x2[i];
  }

  if (sol[11] < 0.0) for (int i=0;i<12;i++) sol[i] = -sol[i];  /* since Z0 > 0 */

  resul[0] = sol[3]*sol[0]+sol[4]*sol[1]+sol[5]*sol[2];   /* u0 */

  resul[1] = sol[6]*sol[0]+sol[7]*sol[1]+sol[8]*sol[2];   /* v0 */

  resul[2] = sqrt(sol[3]*sol[3]+sol[4]*sol[4]+sol[5]*sol[5] /* px */
                  -resul[0]*resul[0]);
  resul[3] = sqrt(sol[6]*sol[6]+sol[7]*sol[7]+sol[8]*sol[8] /* py */
                  -resul[1]*resul[1]);

  cam.initPersProjWithoutDistortion(resul[2],resul[3],resul[0],resul[1]);

  resul[4] = (sol[9]-sol[11]*resul[0])/resul[2];  /* X0 */
  resul[5] = (sol[10]-sol[11]*resul[1])/resul[3]; /* Y0 */
  resul[6] = sol[11];         /* Z0 */

  vpMatrix rd(3,3) ;
  /* fill rotation matrix */
  for (int i=0;i<3;i++) rd[0][i] = (sol[i+3]-sol[i]*resul[0])/resul[2];
  for (int i=0;i<3;i++) rd[1][i] = (sol[i+6]-sol[i]*resul[1])/resul[3];
  for (int i=0;i<3;i++) rd[2][i] = sol[i];

  //  std::cout << "norme X1 " << x1.sumSquare() <<std::endl;
  //  std::cout << rd*rd.t() ;

  for (int i=0 ; i < 3 ; i++)
  {
    for (int j=0 ; j < 3 ; j++)
      cMo[i][j] = rd[i][j];
  }
  for (int i=0 ; i < 3 ; i++) cMo[i][3] = resul[i+4] ;

  this->cMo = cMo ;
  this->cMo_dist = cMo;

  double deviation,deviation_dist;
  this->computeStdDeviation(deviation,deviation_dist);

}


void
vpCalibration::calibVVS(
  vpCameraParameters &cam,
  vpHomogeneousMatrix &cMo,
  bool verbose)
{
  std::cout.precision(10);
  int   n_points = npt ;

  vpColVector oX(n_points), cX(n_points)  ;
  vpColVector oY(n_points), cY(n_points) ;
  vpColVector oZ(n_points), cZ(n_points) ;
  vpColVector u(n_points) ;
  vpColVector v(n_points) ;

  vpColVector P(2*n_points) ;
  vpColVector Pd(2*n_points) ;

  vpImagePoint ip;


  LoX.front() ;
  LoY.front() ;
  LoZ.front() ;
  Lip.front() ;

  for (int i =0 ; i < n_points ; i++)
  {

    oX[i]  = LoX.value() ;
    oY[i]  = LoY.value() ;
    oZ[i]  = LoZ.value() ;

    ip = Lip.value();

    u[i] = ip.get_u()  ;
    v[i] = ip.get_v()  ;


    LoX.next() ;
    LoY.next() ;
    LoZ.next() ;
    Lip.next() ;
  }

  //  double lambda = 0.1 ;
  unsigned int iter = 0 ;

  double  residu_1 = 1e12 ;
  double r =1e12-1;
  while (vpMath::equal(residu_1,r,threshold) == false && iter < nbIterMax)
  {

    iter++ ;
    residu_1 = r ;

    double px = cam.get_px();
    double py = cam.get_py();
    double u0 = cam.get_u0();
    double v0 = cam.get_v0();

    r = 0 ;

    for (int i=0 ; i < n_points; i++)
    {
      cX[i] = oX[i]*cMo[0][0]+oY[i]*cMo[0][1]+oZ[i]*cMo[0][2] + cMo[0][3];
      cY[i] = oX[i]*cMo[1][0]+oY[i]*cMo[1][1]+oZ[i]*cMo[1][2] + cMo[1][3];
      cZ[i] = oX[i]*cMo[2][0]+oY[i]*cMo[2][1]+oZ[i]*cMo[2][2] + cMo[2][3];

      Pd[2*i] =   u[i] ;
      Pd[2*i+1] = v[i] ;

      P[2*i] =    cX[i]/cZ[i]*px + u0 ;
      P[2*i+1] =  cY[i]/cZ[i]*py + v0 ;

      r += ((vpMath::sqr(P[2*i]-Pd[2*i]) + vpMath::sqr(P[2*i+1]-Pd[2*i+1]))) ;
    }


    vpColVector error ;
    error = P-Pd ;
    //r = r/n_points ;

    vpMatrix L(n_points*2,10) ;
    for (int i=0 ; i < n_points; i++)
    {

      double x = cX[i] ;
      double y = cY[i] ;
      double z = cZ[i] ;
      double inv_z = 1/z;
      
      double X =   x*inv_z ;
      double Y =   y*inv_z ;

      //---------------

      {
        L[2*i][0] =  px * (-inv_z) ;
        L[2*i][1] =  0 ;
        L[2*i][2] =  px*X*inv_z ;
        L[2*i][3] =  px*X*Y ;
        L[2*i][4] =  -px*(1+X*X) ;
        L[2*i][5] =  px*Y ;
      }
      {
        L[2*i][6]= 1 ;
        L[2*i][7]= 0 ;
        L[2*i][8]= X ;
        L[2*i][9]= 0;
      }
      {
        L[2*i+1][0] = 0 ;
        L[2*i+1][1] = py*(-inv_z) ;
        L[2*i+1][2] = py*(Y*inv_z) ;
        L[2*i+1][3] = py* (1+Y*Y) ;
        L[2*i+1][4] = -py*X*Y ;
        L[2*i+1][5] = -py*X ;
      }
      {
        L[2*i+1][6]= 0 ;
        L[2*i+1][7]= 1 ;
        L[2*i+1][8]= 0;
        L[2*i+1][9]= Y ;
      }


    }    // end interaction
    vpMatrix Lp ;
    Lp = L.pseudoInverse(1e-10) ;

    vpColVector e ;
    e = Lp*error ;

    vpColVector Tc, Tc_v(6) ;
    Tc = -e*gain ;

    //   Tc_v =0 ;
    for (int i=0 ; i <6 ; i++)
      Tc_v[i] = Tc[i] ;

    cam.initPersProjWithoutDistortion(px+Tc[8],py+Tc[9],
                                      u0+Tc[6],v0+Tc[7]) ;

    cMo = vpExponentialMap::direct(Tc_v).inverse()*cMo ;
    if (verbose)
      std::cout <<  " std dev " << sqrt(r/n_points) << std::endl;

  }
  if (iter == nbIterMax)
  {
    vpERROR_TRACE("Iterations number exceed the maximum allowed (%d)",nbIterMax);
    throw(vpCalibrationException(vpCalibrationException::convergencyError,
                                 "Maximum number of iterations reached")) ;
  }
  this->cMo   = cMo;
  this->cMo_dist = cMo;
  this->residual = r;
  this->residual_dist = r;
  if (verbose)
    std::cout <<  " std dev " << sqrt(r/n_points) << std::endl;

}

void
vpCalibration::calibVVSMulti(
  unsigned int nbPose,
  vpCalibration table_cal[],
  vpCameraParameters &cam,
  bool verbose
)
{
  std::cout.precision(10);
  int nbPoint[256]; //number of points by image
  int nbPointTotal = 0; //total number of points
  
  unsigned int nbPose6 = 6*nbPose;

  for (unsigned int i=0; i<nbPose ; i++)
  {
    nbPoint[i] = table_cal[i].npt;
    nbPointTotal += nbPoint[i];
  }

  vpColVector oX(nbPointTotal), cX(nbPointTotal)  ;
  vpColVector oY(nbPointTotal), cY(nbPointTotal) ;
  vpColVector oZ(nbPointTotal), cZ(nbPointTotal) ;
  vpColVector u(nbPointTotal) ;
  vpColVector v(nbPointTotal) ;

  vpColVector P(2*nbPointTotal) ;
  vpColVector Pd(2*nbPointTotal) ;
  vpImagePoint ip;

  int curPoint = 0 ; //current point indice
  for (unsigned int p=0; p<nbPose ; p++)
  {
      
    table_cal[p].LoX.front() ;
    table_cal[p].LoY.front() ;
    table_cal[p].LoZ.front() ;
    table_cal[p].Lip.front()  ;
    
    for (int i =0 ; i < nbPoint[p] ; i++)
    {
      oX[curPoint]  = table_cal[p].LoX.value() ;
      oY[curPoint]  = table_cal[p].LoY.value() ;
      oZ[curPoint]  = table_cal[p].LoZ.value() ;
    
      ip = table_cal[p].Lip.value();
      u[curPoint] = ip.get_u()  ;
      v[curPoint] = ip.get_v()  ;

      table_cal[p].LoX.next() ;
      table_cal[p].LoY.next() ;
      table_cal[p].LoZ.next() ;
      table_cal[p].Lip.next() ;
 
      curPoint++;
    }
  }
  //  double lambda = 0.1 ;
  unsigned int iter = 0 ;

  double  residu_1 = 1e12 ;
  double r =1e12-1;
  while (vpMath::equal(residu_1,r,threshold) == false && iter < nbIterMax)
  {

    iter++ ;
    residu_1 = r ;
    
    double px = cam.get_px();
    double py = cam.get_py();
    double u0 = cam.get_u0();
    double v0 = cam.get_v0();
   
    r = 0 ;
    curPoint = 0 ; //current point indice
    for (unsigned int p=0; p<nbPose ; p++)
    {
      vpHomogeneousMatrix cMoTmp = table_cal[p].cMo;
      for (int i=0 ; i < nbPoint[p]; i++)
      {
        unsigned int curPoint2 = 2*curPoint;    
        
        cX[curPoint] = oX[curPoint]*cMoTmp[0][0]+oY[curPoint]*cMoTmp[0][1]
                       +oZ[curPoint]*cMoTmp[0][2] + cMoTmp[0][3];
        cY[curPoint] = oX[curPoint]*cMoTmp[1][0]+oY[curPoint]*cMoTmp[1][1]
                       +oZ[curPoint]*cMoTmp[1][2] + cMoTmp[1][3];
        cZ[curPoint] = oX[curPoint]*cMoTmp[2][0]+oY[curPoint]*cMoTmp[2][1]
                       +oZ[curPoint]*cMoTmp[2][2] + cMoTmp[2][3];

        Pd[curPoint2] =   u[curPoint] ;
        Pd[curPoint2+1] = v[curPoint] ;

        P[curPoint2] =    cX[curPoint]/cZ[curPoint]*px + u0 ;
        P[curPoint2+1] =  cY[curPoint]/cZ[curPoint]*py + v0 ;

        r += (vpMath::sqr(P[curPoint2]-Pd[curPoint2])
               + vpMath::sqr(P[curPoint2+1]-Pd[curPoint2+1])) ;
        curPoint++;
      }
    }

    vpColVector error ;
    error = P-Pd ;
    //r = r/nbPointTotal ;

    vpMatrix L(nbPointTotal*2,nbPose6+4) ;
    curPoint = 0 ; //current point indice
    for (unsigned int p=0; p<nbPose ; p++)
    {
      unsigned int q = 6*p;   
      for (int i=0 ; i < nbPoint[p]; i++)
      {
        unsigned int curPoint2 = 2*curPoint;
        unsigned int curPoint21 = curPoint2 + 1;

        double x = cX[curPoint] ;
        double y = cY[curPoint] ;
        double z = cZ[curPoint] ;

        double inv_z = 1/z;
            
        double X =   x*inv_z ;
        double Y =   y*inv_z ;

        //---------------
        {
          {
            L[curPoint2][q] =  px * (-inv_z) ;
            L[curPoint2][q+1] =  0 ;
            L[curPoint2][q+2] =  px*(X*inv_z) ;
            L[curPoint2][q+3] =  px*X*Y ;
            L[curPoint2][q+4] =  -px*(1+X*X) ;
            L[curPoint2][q+5] =  px*Y ;
          }
          {
            L[curPoint2][nbPose6]= 1 ;
            L[curPoint2][nbPose6+1]= 0 ;
            L[curPoint2][nbPose6+2]= X ;
            L[curPoint2][nbPose6+3]= 0;
          }
          {
            L[curPoint21][q] = 0 ;
            L[curPoint21][q+1] = py*(-inv_z) ;
            L[curPoint21][q+2] = py*(Y*inv_z) ;
            L[curPoint21][q+3] = py* (1+Y*Y) ;
            L[curPoint21][q+4] = -py*X*Y ;
            L[curPoint21][q+5] = -py*X ;
          }
          {
            L[curPoint21][nbPose6]= 0 ;
            L[curPoint21][nbPose6+1]= 1 ;
            L[curPoint21][nbPose6+2]= 0;
            L[curPoint21][nbPose6+3]= Y ;
          }

        }
        curPoint++;
      }    // end interaction
    }
    vpMatrix Lp ;
    Lp = L.pseudoInverse(1e-10) ;

    vpColVector e ;
    e = Lp*error ;

    vpColVector Tc, Tc_v(nbPose6) ;
    Tc = -e*gain ;

    //   Tc_v =0 ;
    for (unsigned int i = 0 ; i < nbPose6 ; i++)
      Tc_v[i] = Tc[i] ;

    cam.initPersProjWithoutDistortion(px+Tc[nbPose6+2],
                                      py+Tc[nbPose6+3],
                                      u0+Tc[nbPose6],
                                      v0+Tc[nbPose6+1]) ;

    //    cam.setKd(get_kd() + Tc[10]) ;
    vpColVector Tc_v_Tmp(6) ;

    for (unsigned int p = 0 ; p < nbPose ; p++)
    {
      for (unsigned int i = 0 ; i < 6 ; i++)
        Tc_v_Tmp[i] = Tc_v[6*p + i];

      table_cal[p].cMo = vpExponentialMap::direct(Tc_v_Tmp,1).inverse()
                         * table_cal[p].cMo;
    }
    if (verbose)
      std::cout <<  " std dev " << sqrt(r/nbPointTotal) << std::endl;

  }
  if (iter == nbIterMax)
  {
    vpERROR_TRACE("Iterations number exceed the maximum allowed (%d)",nbIterMax);
    throw(vpCalibrationException(vpCalibrationException::convergencyError,
                                 "Maximum number of iterations reached")) ;
  }
  for (unsigned int p = 0 ; p < nbPose ; p++)
  {
    table_cal[p].cMo_dist = table_cal[p].cMo ;
    table_cal[p].cam = cam;
    table_cal[p].cam_dist = cam;
    double deviation,deviation_dist;
    table_cal[p].computeStdDeviation(deviation,deviation_dist);
  }
  if (verbose)
    std::cout <<  " std dev " << sqrt(r/nbPointTotal) << std::endl;
}

void
vpCalibration::calibVVSWithDistortion(
  vpCameraParameters& cam,
  vpHomogeneousMatrix& cMo,
  bool verbose)
{
  std::cout.precision(10);
  unsigned int n_points =npt ;

  vpColVector oX(n_points), cX(n_points)  ;
  vpColVector oY(n_points), cY(n_points) ;
  vpColVector oZ(n_points), cZ(n_points) ;
  vpColVector u(n_points) ;
  vpColVector v(n_points) ;

  vpColVector P(4*n_points) ;
  vpColVector Pd(4*n_points) ;


  LoX.front() ;
  LoY.front() ;
  LoZ.front() ;
  Lip.front() ;
  
  vpImagePoint ip;

  for (unsigned int i =0 ; i < n_points ; i++)
  {

    oX[i]  = LoX.value() ;
    oY[i]  = LoY.value() ;
    oZ[i]  = LoZ.value() ;


    ip = Lip.value();
    u[i] = ip.get_u();
    v[i] = ip.get_v();

    LoX.next() ;
    LoY.next() ;
    LoZ.next() ;
    Lip.next() ;
  }

  //  double lambda = 0.1 ;
  unsigned int iter = 0 ;

  double  residu_1 = 1e12 ;
  double r =1e12-1;
  while (vpMath::equal(residu_1,r,threshold)  == false && iter < nbIterMax)
  {

    iter++ ;
    residu_1 = r ;


    r = 0 ;
    double u0 = cam.get_u0() ;
    double v0 = cam.get_v0() ;

    double px = cam.get_px() ;
    double py = cam.get_py() ;
    
    double inv_px = 1/px ;
    double inv_py = 1/py ;

    double kud = cam.get_kud() ;
    double kdu = cam.get_kdu() ;

    double k2ud = 2*kud;
    double k2du = 2*kdu;    
    vpMatrix L(n_points*4,12) ;

    for (unsigned int i=0 ; i < n_points; i++)
    {
      unsigned int i4 = 4*i;
      unsigned int i41 = 4*i+1;
      unsigned int i42 = 4*i+2;
      unsigned int i43 = 4*i+3;
         
      cX[i] = oX[i]*cMo[0][0]+oY[i]*cMo[0][1]+oZ[i]*cMo[0][2] + cMo[0][3];
      cY[i] = oX[i]*cMo[1][0]+oY[i]*cMo[1][1]+oZ[i]*cMo[1][2] + cMo[1][3];
      cZ[i] = oX[i]*cMo[2][0]+oY[i]*cMo[2][1]+oZ[i]*cMo[2][2] + cMo[2][3];

      double x = cX[i] ;
      double y = cY[i] ;
      double z = cZ[i] ;
      double inv_z = 1/z;
      
      double X =   x*inv_z ;
      double Y =   y*inv_z ;

      double X2 = X*X;
      double Y2 = Y*Y;
      double XY = X*Y;        
       
      double up = u[i] ;
      double vp = v[i] ;

      Pd[i4] =   up ;
      Pd[i41] = vp ;

      double up0 = up - u0;
      double vp0 = vp - v0;

      double xp0 = up0 * inv_px;
      double xp02 = xp0 *xp0 ;   
      
      double yp0 = vp0 * inv_py;     
      double yp02 = yp0 * yp0;
      
      double r2du = xp02 + yp02 ;
      double kr2du = kdu * r2du;   

      P[i4] =   u0 + px*X - kr2du *(up0) ;
      P[i41] = v0 + py*Y - kr2du *(vp0) ;

      double r2ud = X2 + Y2 ;
      double kr2ud = 1 + kud * r2ud;
      
      double Axx = px*(kr2ud+k2ud*X2);
      double Axy = px*k2ud*XY;
      double Ayy = py*(kr2ud+k2ud*Y2);
      double Ayx = py*k2ud*XY;

      Pd[i42] = up ;
      Pd[i43] = vp ;

      P[i42] = u0 + px*X*kr2ud ;
      P[i43] = v0 + py*Y*kr2ud ;


      r += (vpMath::sqr(P[i4]-Pd[i4]) +
          vpMath::sqr(P[i41]-Pd[i41]) +
          vpMath::sqr(P[i42]-Pd[i42]) +
          vpMath::sqr(P[i43]-Pd[i43]))*0.5;

      //--distorted to undistorted
      {
        {
          L[i4][0] =  px * (-inv_z) ;
          L[i4][1] =  0 ;
          L[i4][2] =  px*X*inv_z ;
          L[i4][3] =  px*X*Y ;
          L[i4][4] =  -px*(1+X2) ;
          L[i4][5] =  px*Y ;
        }
        {
          L[i4][6]= 1 + kr2du + k2du*xp02  ;
          L[i4][7]= k2du*up0*xp0*inv_px ;
          L[i4][8]= X + k2du*xp02*xp0 ;
          L[i4][9]= k2du*up0*xp02*inv_py ;
          L[i4][10] = -(up0)*(r2du) ;
          L[i4][11] = 0 ;
        }
        {
          L[i41][0] = 0 ;
          L[i41][1] = py*(-inv_z) ;
          L[i41][2] = py*Y*inv_z ;
          L[i41][3] = py* (1+Y2) ;
          L[i41][4] = -py*XY ;
          L[i41][5] = -py*X ;
        }
        {
          L[i41][6]= k2du*xp0*vp0*inv_px ;
          L[i41][7]= 1 + kr2du + k2du*yp02;
          L[i41][8]= k2du*vp0*xp02*inv_px;
          L[i41][9]= Y + k2du*yp02*yp0;
          L[i41][10] = -vp0*r2du ;
          L[i41][11] = 0 ;
        }
	//---undistorted to distorted
	      {
          L[i42][0] = Axx*(-inv_z) ;
          L[i42][1] = Axy*(-inv_z) ;
          L[i42][2] = Axx*(X*inv_z) + Axy*(Y*inv_z) ;
          L[i42][3] = Axx*X*Y +  Axy*(1+Y2);
          L[i42][4] = -Axx*(1+X2) - Axy*XY;
          L[i42][5] = Axx*Y -Axy*X;
	      }
	      {
          L[i42][6]= 1 ;
          L[i42][7]= 0 ;
          L[i42][8]= X*kr2ud ;
          L[i42][9]= 0;
          L[i42][10] = 0 ;
          L[i42][11] = px*X*r2ud ;
	      }
	      {
          L[i43][0] = Ayx*(-inv_z) ;
          L[i43][1] = Ayy*(-inv_z) ;
          L[i43][2] = Ayx*(X*inv_z) + Ayy*(Y*inv_z) ;
          L[i43][3] = Ayx*XY + Ayy*(1+Y2) ;
          L[i43][4] = -Ayx*(1+X2) -Ayy*XY ;
          L[i43][5] = Ayx*Y -Ayy*X;
	      }
	      {
          L[i43][6]= 0 ;
          L[i43][7]= 1;
          L[i43][8]= 0;
          L[i43][9]= Y*kr2ud ;
          L[i43][10] = 0 ;
          L[i43][11] = py*Y*r2ud ;
	      }
      }  // end interaction
    }    // end interaction

    vpColVector error ;
    error = P-Pd ;
    //r = r/n_points ;

    vpMatrix Lp ;
    Lp = L.pseudoInverse(1e-10) ;

    vpColVector e ;
    e = Lp*error ;

    vpColVector Tc, Tc_v(6) ;
    Tc = -e*gain ;

    for (int i=0 ; i <6 ; i++)
      Tc_v[i] = Tc[i] ;

    cam.initPersProjWithDistortion(px + Tc[8], py + Tc[9],
                                   u0 + Tc[6], v0 + Tc[7],
                                   kud + Tc[11],
                                   kdu + Tc[10]);

    cMo = vpExponentialMap::direct(Tc_v).inverse()*cMo ;
    if (verbose)
      std::cout <<  " std dev " << sqrt(r/n_points) << std::endl;

  }
  if (iter == nbIterMax)
  {
    vpERROR_TRACE("Iterations number exceed the maximum allowed (%d)",nbIterMax);
    throw(vpCalibrationException(vpCalibrationException::convergencyError,
                                 "Maximum number of iterations reached")) ;
  }
  this->residual_dist = r;
  this->cMo_dist = cMo ;
  this->cam_dist = cam ;

  if (verbose)
    std::cout <<  " std dev " << sqrt(r/n_points) << std::endl;
}


void
vpCalibration::calibVVSWithDistortionMulti(
  unsigned int nbPose,
  vpCalibration table_cal[],
  vpCameraParameters &cam,
  bool verbose)
{
  std::cout.precision(10);
  unsigned int nbPoint[256]; //number of points by image
  unsigned int nbPointTotal = 0; //total number of points

  unsigned int nbPose6 = 6*nbPose;
  for (unsigned int i=0; i<nbPose ; i++)
  {
    nbPoint[i] = table_cal[i].npt;
    nbPointTotal += nbPoint[i];
  }

  vpColVector oX(nbPointTotal), cX(nbPointTotal)  ;
  vpColVector oY(nbPointTotal), cY(nbPointTotal) ;
  vpColVector oZ(nbPointTotal), cZ(nbPointTotal) ;
  vpColVector u(nbPointTotal) ;
  vpColVector v(nbPointTotal) ;

  vpColVector P(4*nbPointTotal) ;
  vpColVector Pd(4*nbPointTotal) ;
  vpImagePoint ip;

  int curPoint = 0 ; //current point indice
  for (unsigned int p=0; p<nbPose ; p++)
  {
    table_cal[p].LoX.front() ;
    table_cal[p].LoY.front() ;
    table_cal[p].LoZ.front() ;
    table_cal[p].Lip.front()  ;
 
    for (unsigned int i =0 ; i < nbPoint[p] ; i++)
    {

      oX[curPoint]  = table_cal[p].LoX.value() ;
      oY[curPoint]  = table_cal[p].LoY.value() ;
      oZ[curPoint]  = table_cal[p].LoZ.value() ;

      ip = table_cal[p].Lip.value();
      u[curPoint] = ip.get_u()  ;
      v[curPoint] = ip.get_v()  ;


      table_cal[p].LoX.next() ;
      table_cal[p].LoY.next() ;
      table_cal[p].LoZ.next() ;
      table_cal[p].Lip.next() ;
      curPoint++;
    }
  }
  //  double lambda = 0.1 ;
  unsigned int iter = 0 ;

  double  residu_1 = 1e12 ;
  double r =1e12-1;
  while (vpMath::equal(residu_1,r,threshold) == false && iter < nbIterMax)
  {
    iter++ ;
    residu_1 = r ;

    r = 0 ;
    curPoint = 0 ; //current point indice
    for (unsigned int p=0; p<nbPose ; p++)
    {
      vpHomogeneousMatrix cMoTmp = table_cal[p].cMo_dist;
      for (unsigned int i=0 ; i < nbPoint[p]; i++)
      {
        cX[curPoint] = oX[curPoint]*cMoTmp[0][0]+oY[curPoint]*cMoTmp[0][1]
                       +oZ[curPoint]*cMoTmp[0][2] + cMoTmp[0][3];
        cY[curPoint] = oX[curPoint]*cMoTmp[1][0]+oY[curPoint]*cMoTmp[1][1]
                       +oZ[curPoint]*cMoTmp[1][2] + cMoTmp[1][3];
        cZ[curPoint] = oX[curPoint]*cMoTmp[2][0]+oY[curPoint]*cMoTmp[2][1]
                       +oZ[curPoint]*cMoTmp[2][2] + cMoTmp[2][3];

        curPoint++;
      }
    }


    vpMatrix L(nbPointTotal*4,nbPose6+6) ;
    curPoint = 0 ; //current point indice
    double px = cam.get_px() ;
    double py = cam.get_py() ;
    double u0 = cam.get_u0() ;
    double v0 = cam.get_v0() ;

    double inv_px = 1/px ;
    double inv_py = 1/py ;

    double kud = cam.get_kud() ;
    double kdu = cam.get_kdu() ;

    double k2ud = 2*kud;
    double k2du = 2*kdu;
    
    for (unsigned int p=0; p<nbPose ; p++)
    {
      unsigned int q = 6*p;   
      for (unsigned int i=0 ; i < nbPoint[p]; i++)
      {
        unsigned int curPoint4 = 4*curPoint;
        double x = cX[curPoint] ;
        double y = cY[curPoint] ;
        double z = cZ[curPoint] ;

        double inv_z = 1/z;    
        double X =   x*inv_z ;
        double Y =   y*inv_z ;

        double X2 = X*X;
        double Y2 = Y*Y;
        double XY = X*Y;
       
        double up = u[curPoint] ;
        double vp = v[curPoint] ;

        Pd[curPoint4] =   up ;
        Pd[curPoint4+1] = vp ;

        double up0 = up - u0;
        double vp0 = vp - v0;

        double xp0 = up0 * inv_px;
        double xp02 = xp0 *xp0 ;
      
        double yp0 = vp0 * inv_py;
        double yp02 = yp0 * yp0;
      
        double r2du = xp02 + yp02 ;
        double kr2du = kdu * r2du;

        P[curPoint4] =   u0 + px*X - kr2du *(up0) ;
        P[curPoint4+1] = v0 + py*Y - kr2du *(vp0) ;

        double r2ud = X2 + Y2 ;
        double kr2ud = 1 + kud * r2ud;
      
        double Axx = px*(kr2ud+k2ud*X2);
        double Axy = px*k2ud*XY;
        double Ayy = py*(kr2ud+k2ud*Y2);
        double Ayx = py*k2ud*XY;

        Pd[curPoint4+2] = up ;
        Pd[curPoint4+3] = vp ;

        P[curPoint4+2] = u0 + px*X*kr2ud ;
        P[curPoint4+3] = v0 + py*Y*kr2ud ;

        r += (vpMath::sqr(P[curPoint4]-Pd[curPoint4]) +
             vpMath::sqr(P[curPoint4+1]-Pd[curPoint4+1]) +
             vpMath::sqr(P[curPoint4+2]-Pd[curPoint4+2]) +
             vpMath::sqr(P[curPoint4+3]-Pd[curPoint4+3]))*0.5 ;

        unsigned int curInd = curPoint4;
        //---------------
        {
          {
            L[curInd][q] =  px * (-inv_z) ;
            L[curInd][q+1] =  0 ;
            L[curInd][q+2] =  px*X*inv_z ;
            L[curInd][q+3] =  px*X*Y ;
            L[curInd][q+4] =  -px*(1+X2) ;
            L[curInd][q+5] =  px*Y ;
          }
          {
            L[curInd][nbPose6]= 1 + kr2du + k2du*xp02  ;
            L[curInd][nbPose6+1]= k2du*up0*yp0*inv_py ;
            L[curInd][nbPose6+2]= X + k2du*xp02*xp0 ;
            L[curInd][nbPose6+3]= k2du*up0*yp02*inv_py ;
            L[curInd][nbPose6+4] = -(up0)*(r2du) ;
            L[curInd][nbPose6+5] = 0 ;
          }
            curInd++;     
          {
            L[curInd][q] = 0 ;
            L[curInd][q+1] = py*(-inv_z) ;
            L[curInd][q+2] = py*Y*inv_z ;
            L[curInd][q+3] = py* (1+Y2) ;
            L[curInd][q+4] = -py*XY ;
            L[curInd][q+5] = -py*X ;
          }
          {
            L[curInd][nbPose6]= k2du*xp0*vp0*inv_px ;
            L[curInd][nbPose6+1]= 1 + kr2du + k2du*yp02;
            L[curInd][nbPose6+2]= k2du*vp0*xp02*inv_px;
            L[curInd][nbPose6+3]= Y + k2du*yp02*yp0;
            L[curInd][nbPose6+4] = -vp0*r2du ;
            L[curInd][nbPose6+5] = 0 ;
          }
            curInd++;
  //---undistorted to distorted
          {
            L[curInd][q] = Axx*(-inv_z) ;
            L[curInd][q+1] = Axy*(-inv_z) ;
            L[curInd][q+2] = Axx*(X*inv_z) + Axy*(Y*inv_z) ;
            L[curInd][q+3] = Axx*X*Y +  Axy*(1+Y2);
            L[curInd][q+4] = -Axx*(1+X2) - Axy*XY;
            L[curInd][q+5] = Axx*Y -Axy*X;
          }
          {
            L[curInd][nbPose6]= 1 ;
            L[curInd][nbPose6+1]= 0 ;
            L[curInd][nbPose6+2]= X*kr2ud ;
            L[curInd][nbPose6+3]= 0;
            L[curInd][nbPose6+4] = 0 ;
            L[curInd][nbPose6+5] = px*X*r2ud ;
          }
            curInd++;   
          {
            L[curInd][q] = Ayx*(-inv_z) ;
            L[curInd][q+1] = Ayy*(-inv_z) ;
            L[curInd][q+2] = Ayx*(X*inv_z) + Ayy*(Y*inv_z) ;
            L[curInd][q+3] = Ayx*XY + Ayy*(1+Y2) ;
            L[curInd][q+4] = -Ayx*(1+X2) -Ayy*XY ;
            L[curInd][q+5] = Ayx*Y -Ayy*X;
          }
          {
            L[curInd][nbPose6]= 0 ;
            L[curInd][nbPose6+1]= 1;
            L[curInd][nbPose6+2]= 0;
            L[curInd][nbPose6+3]= Y*kr2ud ;
            L[curInd][nbPose6+4] = 0 ;
            L[curInd][nbPose6+5] = py*Y*r2ud ;
          }
        }  // end interaction
        curPoint++;
      }    // end interaction
    }

    vpColVector error ;
    error = P-Pd ;
    //r = r/nbPointTotal ;

    vpMatrix Lp ;
    /*double rank =*/
    L.pseudoInverse(Lp,1e-10) ;
    vpColVector e ;
    e = Lp*error ;
    vpColVector Tc, Tc_v(6*nbPose) ;
    Tc = -e*gain ;
    for (unsigned int i = 0 ; i < 6*nbPose ; i++)
      Tc_v[i] = Tc[i] ;

    cam.initPersProjWithDistortion(  px+Tc[nbPose6+2], py+Tc[nbPose6+3],
                                     u0+Tc[nbPose6], v0+Tc[nbPose6+1],
                                     kud + Tc[nbPose6+5],
                                     kdu + Tc[nbPose6+4]);

    vpColVector Tc_v_Tmp(6) ;
    for (unsigned int p = 0 ; p < nbPose ; p++)
    {
      for (unsigned int i = 0 ; i < 6 ; i++)
        Tc_v_Tmp[i] = Tc_v[6*p + i];

      table_cal[p].cMo_dist = vpExponentialMap::direct(Tc_v_Tmp).inverse()
                            * table_cal[p].cMo_dist;
    }
    if (verbose)
      std::cout <<  " std dev: " << sqrt(r/nbPointTotal) << std::endl;
    //std::cout <<  "   residual: " << r << std::endl;

  }
  if (iter == nbIterMax)
  {
    vpERROR_TRACE("Iterations number exceed the maximum allowed (%d)",nbIterMax);
    throw(vpCalibrationException(vpCalibrationException::convergencyError,
                                 "Maximum number of iterations reached")) ;
  }
  for (unsigned int p = 0 ; p < nbPose ; p++)
  {
    table_cal[p].cam_dist = cam ;
    table_cal[p].computeStdDeviation_dist(table_cal[p].cMo_dist,
                                          cam);
  }
  if (verbose)
    std::cout <<" std dev " << sqrt(r/(nbPointTotal)) << std::endl;

}
/*!
  \brief calibration method of effector-camera from R. Tsai and R. Lorenz

  Compute extrinsic camera parameters : the constant transformation from
  the effector to the camera coordinates (eMc).

  R. Tsai, R. Lenz. -- A new technique for fully autonomous and efficient 3D
  robotics hand/eye calibration. -- IEEE Transactions on Robotics and
  Automation, 5(3):345--358, June 1989.

  \param nbPose : number of different positions (input)
  \param cMo : table of homogeneous matrices representing the transformation
  between the camera and the scene (input)
  \param rMe : table of homogeneous matrices representing the transformation
  between the effector (where the camera is fixed) and the reference coordinates
  (base of the manipulator) (input)
  \param eMc : homogeneous matrix representing the transformation
  between the effector and the camera (output)
*/
void
vpCalibration::calibrationTsai(unsigned int nbPose,
                               vpHomogeneousMatrix cMo[],
                               vpHomogeneousMatrix rMe[],
                               vpHomogeneousMatrix &eMc)
{

  vpColVector x ;
  {
    vpMatrix A ;
    vpColVector B ;
    unsigned int k = 0 ;
    // for all couples ij
    for (unsigned int i=0 ; i < nbPose ; i++)
    {
      vpRotationMatrix rRei, ciRo ;
      rMe[i].extract(rRei) ;
      cMo[i].extract(ciRo) ;
      //std::cout << "rMei: " << std::endl << rMe[i] << std::endl;

      for (unsigned int j=0 ; j < nbPose ; j++)
      {
        if (j>i) // we don't use two times same couples...
        {
          vpRotationMatrix rRej, cjRo ;
          rMe[j].extract(rRej) ;
          cMo[j].extract(cjRo) ;
	  //std::cout << "rMej: " << std::endl << rMe[j] << std::endl;

          vpRotationMatrix rReij = rRej.t() * rRei;

          vpRotationMatrix cijRo = cjRo * ciRo.t();

          vpThetaUVector rPeij(rReij);

          double theta = sqrt(rPeij[0]*rPeij[0] + rPeij[1]*rPeij[1]
                              + rPeij[2]*rPeij[2]);

          for (int m=0;m<3;m++) rPeij[m] = rPeij[m] * vpMath::sinc(theta/2);

          vpThetaUVector cijPo(cijRo) ;
          theta = sqrt(cijPo[0]*cijPo[0] + cijPo[1]*cijPo[1]
                       + cijPo[2]*cijPo[2]);
          for (int m=0;m<3;m++) cijPo[m] = cijPo[m] * vpMath::sinc(theta/2);

          vpMatrix As;
          vpColVector b(3) ;

          As = vpColVector::skew(vpColVector(rPeij) + vpColVector(cijPo)) ;

          b =  (vpColVector)cijPo - (vpColVector)rPeij ;           // A.40

          if (k==0)
          {
            A = As ;
            B = b ;
          }
          else
          {
            A = vpMatrix::stackMatrices(A,As) ;
            B = vpMatrix::stackMatrices(B,b) ;
          }
          k++ ;
        }
      }
    }
	
    // the linear system is defined
    // x = AtA^-1AtB is solved
    vpMatrix AtA = A.AtA() ;

    vpMatrix Ap ;
    AtA.pseudoInverse(Ap, 1e-6) ; // rank 3
    x = Ap*A.t()*B ;

//     {
//       // Residual
//       vpColVector residual;
//       residual = A*x-B;
//       std::cout << "Residual: " << std::endl << residual << std::endl;

//       double res = 0;
//       for (int i=0; i < residual.getRows(); i++)
// 	res += residual[i]*residual[i];
//       res = sqrt(res/residual.getRows());
//       printf("Mean residual = %lf\n",res);
//     }

    // extraction of theta and U
    double theta ;
    double   d=x.sumSquare() ;
    for (int i=0 ; i < 3 ; i++) x[i] = 2*x[i]/sqrt(1+d) ;
    theta = sqrt(x.sumSquare())/2 ;
    theta = 2*asin(theta) ;
    if (theta !=0)
    {
      for (int i=0 ; i < 3 ; i++) x[i] *= theta/(2*sin(theta/2)) ;
    }
    else
      x = 0 ;
  }

  // Building of the rotation matrix eRc
  vpThetaUVector xP(x[0],x[1],x[2]);
  vpRotationMatrix eRc(xP);

  {
    vpMatrix A ;
    vpColVector B ;
    // Building of the system for the translation estimation
    // for all couples ij
    vpRotationMatrix I3 ;
    I3.setIdentity() ;
    int k = 0 ;
    for (unsigned int i=0 ; i < nbPose ; i++)
    {
      vpRotationMatrix rRei, ciRo ;
      vpTranslationVector rTei, ciTo ;
      rMe[i].extract(rRei) ;
      cMo[i].extract(ciRo) ;
      rMe[i].extract(rTei) ;
      cMo[i].extract(ciTo) ;


      for (unsigned int j=0 ; j < nbPose ; j++)
      {
        if (j>i) // we don't use two times same couples...
        {

          vpRotationMatrix rRej, cjRo ;
          rMe[j].extract(rRej) ;
          cMo[j].extract(cjRo) ;

          vpTranslationVector rTej, cjTo ;
          rMe[j].extract(rTej) ;
          cMo[j].extract(cjTo) ;

          vpRotationMatrix rReij = rRej.t() * rRei ;

          vpTranslationVector rTeij = rTej+ (-rTei);

          rTeij = rRej.t()*rTeij ;

          vpMatrix a ;
          a = (vpMatrix)rReij - (vpMatrix)I3 ;

          vpTranslationVector b ;
          b =  eRc*cjTo - rReij*eRc*ciTo + rTeij ;

          if (k==0)
          {
            A = a ;
            B = b ;
          }
          else
          {
            A = vpMatrix::stackMatrices(A,a) ;
            B = vpMatrix::stackMatrices(B,b) ;
          }
          k++ ;
        }
      }
    }

    // the linear system is solved
    // x = AtA^-1AtB is solved
    vpMatrix AtA = A.AtA() ;
    vpMatrix Ap ;
    vpColVector AeTc ;
    AtA.pseudoInverse(Ap, 1e-6) ;
    AeTc = Ap*A.t()*B ;

//     {
//       // residual
//       vpColVector residual;
//       residual = A*AeTc-B;
//       std::cout << "Residual: " << std::endl << residual << std::endl;
//       double res = 0;
//       for (int i=0; i < residual.getRows(); i++)
// 	res += residual[i]*residual[i];
//       res = sqrt(res/residual.getRows());
//       printf("mean residual = %lf\n",res);
//     }

    vpTranslationVector eTc(AeTc[0],AeTc[1],AeTc[2]);

    eMc.insert(eTc) ;
    eMc.insert(eRc) ;
  }
}