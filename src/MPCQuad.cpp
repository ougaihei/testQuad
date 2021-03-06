/*
 *    This file is part of ACADO Toolkit.
 *
 *    ACADO Toolkit -- A Toolkit for Automatic Control and Dynamic Optimization.
 *    Copyright (C) 2008-2014 by Boris Houska, Hans Joachim Ferreau,
 *    Milan Vukov, Rien Quirynen, KU Leuven.
 *    Developed within the Optimization in Engineering Center (OPTEC)
 *    under supervision of Moritz Diehl. All rights reserved.
 *
 *    ACADO Toolkit is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU Lesser General Public
 *    License as published by the Free Software Foundation; either
 *    version 3 of the License, or (at your option) any later version.
 *
 *    ACADO Toolkit is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public
 *    License along with ACADO Toolkit; if not, write to the Free Software
 *    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */


 /**
  *    \file   examples/ocp/rocket.cpp
  *    \author Boris Houska, Hans Joachim Ferreau
  *    \date   2009
  */


#include <acado_optimal_control.hpp>
#include <acado_gnuplot.hpp>
#include <spline.h>
#include <robot.h>
#include <quadconf.h>
#include <locale>
#include <trajectory.h>
#include <acado_code_generation.hpp>
#include <acado/process/process.hpp>
#include <acado_toolkit.hpp>

#include <time.h>

/* >>> start tutorial code >>> */
clock_t t;


#define ALGO 0 // OCP; MPC; OCP then MPC; GENE
#define INIT 1 //libquadspline, quasistatic, quasistatic with avoidance or static
#define OBS 1
#define AVOID_SINGULARITIES 1
#define AVOID_SINGULARITIES_MPC 1
#define TIME_OPT 0
#define RELAX_END_CONST 0
#define PLOT 1 //gnuplot, file

int main( ){

    std::ofstream file;
    file.open("/tmp/log.txt",std::ios::out);
    std::ofstream file2;
    file2.open("/tmp/log_control.txt",std::ios::out);

    USING_NAMESPACE_ACADO

    // INTRODUCE THE VARIABLES:
    // -------------------------
    DifferentialState     x,y,z,vx,vy,vz,phi,theta,psi,p,q,r,u1,u2,u3,u4;
    Control               vu1,vu2,vu3,vu4;
    Parameter		  T;
#if TIME_OPT==1
    DifferentialEquation  f(0,T);
#else
    DifferentialEquation  f;
#endif

    // Quad constants
    const double c = 0.00001;
    const double Cf = 0.00065;
    const double d = 0.250;
    const double Jx = 0.018;
    const double Jy = 0.018;
    const double Jz = 0.026;
    const double Im = 0.0001;
    const double m = 0.9;
    const double g = 9.81;
    const double Cx = 0.1;

    // Minimization Weights
    double coeffU = 0.000000000000001;
    double coeffX = .00001;
    double coeffX2 = coeffX * 10.;
    double coeffX3 = coeffX * 0.00001;
    double coeffO = -coeffX * 0.1;

    // final values
    double xf = 0., yf = 0., zf = 20.;

    // Temps max
    double tps = 8.;
    int nb_it = 20.;
    double tmpc = 0.2;
    int mpc_nb_it = 1;
    int ocp_nb_it = 294;

    DVector x0(16);
    x0.setZero();
    x0(0) = 0.;
    x0(12) = 58.;
    x0(13) = 58.;
    x0(14) = 58.;
    x0(15) = 58.;

    // DEFINE A DIFFERENTIAL EQUATION:
    // -------------------------------
#if TIME_OPT==1
    OCP ocp( 0.0, T, nb_it ), ocpMPC( 0.0, tps, nb_it );
#else
    OCP ocp( 0.0, tps, nb_it ), ocpMPC( 0.0, tps, nb_it );
#endif

    //  ------------------------- LSQ MINIMISATION ------------------------ //
    Function h, hf;
    h << x;
    h << y;
    h << z;

    h << vu1;
    h << vu2;
    h << vu3;
    h << vu4;

    h << p;
    h << q;
    h << r;

//    h << phi;
//    h << theta;

//    h << exp((16 - (x*x+(z-5)*(z-5)))*0.01);

    hf << x;
    hf << y;
    hf << z;

    DMatrix Q(10,10), Qf(3,3);
    Q(0,0) = coeffX;
    Q(1,1) = coeffX;
    Q(2,2) = coeffX;

    Q(3,3) = coeffU;
    Q(4,4) = coeffU;
    Q(5,5) = coeffU;
    Q(6,6) = coeffU;

    Q(7,7) = coeffX2;
    Q(8,8) = coeffX2;
    Q(9,9) = coeffX2;


//    Q(10,10) = coeffX3;
//    Q(11,11) = coeffX3;

//    Q(12,12) = coeffX*0.03;

    Qf(0,0) = 1.;//coeffX;//*100;
    Qf(1,1) = 1.;//coeffX;//*100;
    Qf(2,2) = 1.;//coeffX;//*100;

    DVector reff(3), ref(10);
    ref(0) = xf;
    ref(1) = yf;
    ref(2) = zf;
    ref(3) = 0.;
    ref(4) = 0.;
    ref(5) = 0.;
    ref(6) = 0.;
    ref(7) = 0.;
    ref(8) = 0.;
    ref(9) = 0.;
//    ref(10) = 0.;
//    ref(11) = 0.;
//    ref(10) = 0.;

    reff(0) = xf;
    reff(1) = yf;
    reff(2) = zf;

#if ALGO==2
    ocpMPC.minimizeLSQ ( Q, h, ref);
#endif
#if TIME_OPT==1
        ocp.minimizeMayerTerm( T );
#else
    ocp.minimizeMayerTerm( 0 );
//    ocp.minimizeLSQ ( Q, h, ref);
//    ocp.minimizeLSQEndTerm(Qf, hf, reff);
//    ocp.minimizeMayerTerm( (x-xf)*(x-xf) + (y-yf)*(y-yf) + (z-zf)*(z-zf) );
//    ocp.minimizeLagrangeTerm(coeffX*(sqrt(1+(x-xf)*(x-xf)) + sqrt(1+(y-yf)*(y-yf)) + sqrt(1+(z-zf)*(z-zf))));
//    ocp.minimizeLagrangeTerm(coeffX*(sqrt(1+(x-ref(0))*(x-ref(0))) + sqrt(1+(y-ref(1))*(y-ref(1))) + sqrt(1+(z-ref(2))*(z-ref(2))) + (r-ref(7))*(r-ref(7)) + (p-ref(8))*(p-ref(8)) + (q-ref(9))*(q-ref(9)) ) + coeffU*( sqrt(1+(vu1-ref(3))*(vu1-ref(3)))+sqrt(1+(vu2-ref(4))*(vu2-ref(4)))+sqrt(1+(vu3-ref(5))*(vu3-ref(5)))+sqrt(1+(vu4-ref(6))*(vu4-ref(6)))));
//    ocp.minimizeLagrangeTerm(((x-xf)*(x-xf) + (y-yf)*(y-yf) + (z-zf)*(z-zf))*coeffX);
#endif

    // ------------------------------------------------------------------ //


    f << dot(x) == vx;
    f << dot(y) == vy;
    f << dot(z) == vz;
    f << dot(vx) == Cf*(u1*u1+u2*u2+u3*u3+u4*u4)*sin(theta)/m;
    f << dot(vy) == -Cf*(u1*u1+u2*u2+u3*u3+u4*u4)*sin(psi)*cos(theta)/m;
    f << dot(vz) == Cf*(u1*u1+u2*u2+u3*u3+u4*u4)*cos(psi)*cos(theta)/m - g;
    f << dot(phi) == -cos(phi)*tan(theta)*p+sin(phi)*tan(theta)*q+r;
    f << dot(theta) == sin(phi)*p+cos(phi)*q;
    f << dot(psi) == cos(phi)/cos(theta)*p-sin(phi)/cos(theta)*q;
    f << dot(p) == (d*Cf*(u1*u1-u2*u2)+(Jy-Jz)*q*r)/Jx;
    f << dot(q) == (d*Cf*(u4*u4-u3*u3)+(Jz-Jx)*p*r)/Jy;
    f << dot(r) == (c*(u1*u1+u2*u2-u3*u3-u4*u4)+(Jx-Jy)*p*q)/Jz;
    f << dot(u1) == vu1;
    f << dot(u2) == vu2;
    f << dot(u3) == vu3;
    f << dot(u4) == vu4;

    // DEFINE AN OPTIMAL CONTROL PROBLEM:
    // ----------------------------------
    
    //Dynamic
    ocp.subjectTo( f );

#if ALGO==2
    ocpMPC.subjectTo( f );
#endif
// ------------------------------------------------------------------------------------ //



    // ----------------------------- Set initial guess -------------------------- //



#if INIT==0 || INIT==1 || INIT==2 || INIT==3
#if TIME_OPT==1
    Grid timeGrid(0.0,1.,nb_it+1);
#else
    Grid timeGrid(0.0,tps,nb_it+1);
#endif
    VariablesGrid x_init(16, timeGrid);
#endif

#if INIT==0     // init with libquadspline
    double vmax=100;
    double amax=20;
    double jmax=40;
    double smax=320;

    Robot robot=Robot(1.0);

    robot.addDof(vmax,amax,jmax,smax);
    robot.addDof(vmax,amax,jmax,smax);
    robot.addDof(vmax,amax,jmax,smax);
    robot.addDof(1,2,1,1,true);

    LocalPath path(QuadConf(&robot, 0,0,0,0,0,0,0,0,0), QuadConf(&robot, xf,yf,zf,0,0,0,0,0,0));

    VariablesGrid acc(3,timeGrid);
    for (int i = 0 ; i<nb_it+1 ; i++ ) {
        QuadConf conf = path.getQuadConfAt(double(i)*tps/double(nb_it));
        x_init(i,0) = conf.getX();
        x_init(i,1) = conf.getY();
        x_init(i,2) = conf.getZ();
        x_init(i,3) = conf.getVx();
        x_init(i,4) = conf.getVy();
        x_init(i,5) = conf.getVz();
        x_init(i,6) = conf.getRoll();
        x_init(i,7) = -conf.getPitch();
        x_init(i,8) = conf.getYaw();
        x_init(i,9) = conf.getDroll();
        x_init(i,10) = -conf.getDpitch();
        x_init(i,11) = conf.getDyaw();
        x_init(i,12) = 58;
        x_init(i,13) = 58;
        x_init(i,14) = 58;
        x_init(i,15) = 58;
        acc(i,0) = conf.getAx();
        acc(i,1) = conf.getAy();
        acc(i,2) = conf.getAz();
    }
#endif

#if INIT==1      // init with quasistatic solution
    for (int i = 0 ; i<nb_it+1 ; i++ ) {
        x_init(i,0) = xf*double(i)/double(nb_it);
//        if (i<11)
            x_init(i,1) = yf*double(i)/double(nb_it);
//        else
//            x_init(i,1) = 20 - double(i);//yf*double(i)/double(nb_it);
        x_init(i,2) = zf*double(i)/double(nb_it);
        x_init(i,3) = 0.;
        x_init(i,4) = 0.;
        x_init(i,5) = 0.;
        x_init(i,6) = 0.;
        x_init(i,7) = 0.;
//        if ( i==17 || i==18 ) {
//        x_init(i,8) = 3.14  ;
//        }
//        else if ( i==19 || i==20 ) {
//        x_init(i,8) = 1.  ;
//        }
//        else
        x_init(i,8) = 0.;
        x_init(i,9) = 0.;
        x_init(i,10) = 0.;
        x_init(i,11) = 0.;
        x_init(i,12) = 58.;
        x_init(i,13) = 58.;
        x_init(i,14) = 58.;
        x_init(i,15) = 58.;
    }
#endif

#if INIT==2      // init with quasistatic solution and obtacle avoidance
    for (int i = 0 ; i<nb_it+1 ; i++ ) {
        if (i <= nb_it/4) {
        x_init(i,0) = 3.*double(i)/double(nb_it/4);
        x_init(i,1) = 0.*double(i)/double(nb_it/4);
        x_init(i,2) = 3.*double(i)/double(nb_it/4);
        }
        if (i > nb_it/4 && i <= nb_it/2) {
        x_init(i,0) = 3. + -3.*(double(i)-nb_it/4)/double(nb_it/4);
        x_init(i,1) = 0.*(double(i)-nb_it/4)/double(nb_it/4);
        x_init(i,2) = 3. + 3.*(double(i)-nb_it/4)/double(nb_it/4);
        }
        if (i > nb_it/2) {
        x_init(i,0) = 0. + 1.*(double(i)-2*nb_it/4)/double(nb_it/2);
        x_init(i,1) = 0.*(double(i)-2*nb_it/4)/double(nb_it/2);
        x_init(i,2) = 6. + 9.*(double(i)-2*nb_it/4)/double(nb_it/2);
        }
        x_init(i,3) = 0.;
        x_init(i,4) = 0.;
        x_init(i,5) = 0.;
        x_init(i,6) = 0.;
        x_init(i,7) = 0.;
        x_init(i,8) = 0.;
        x_init(i,9) = 0.;
        x_init(i,10) = 0.;
        x_init(i,11) = 0.;
        x_init(i,12) = 58.;
        x_init(i,13) = 58;
        x_init(i,14) = 58;
        x_init(i,15) = 58;
    }
#endif

#if INIT==3      // init with static
    for (int i = 0 ; i<nb_it+1 ; i++ ) {
        x_init(i,0) = x0(0);
        x_init(i,1) = x0(1);
        x_init(i,2) = x0(2);
        x_init(i,3) = 0.;
        x_init(i,4) = 0.;
        x_init(i,5) = 0.;
        x_init(i,6) = 0.;
        x_init(i,7) = 0.;
        x_init(i,8) = 0.;
        x_init(i,9) = 0.;
        x_init(i,10) = 0.;
        x_init(i,11) = 0.;
        x_init(i,12) = 58.;
        x_init(i,13) = 58.;
        x_init(i,14) = 58.;
        x_init(i,15) = 58.;
    }
#endif

//    x_init.print();
//    GnuplotWindow windowXinit;
//    windowXinit.addSubplot( x_init(0), "x " );
//    windowXinit.addSubplot( x_init(1), "y " );
//    windowXinit.addSubplot( x_init(2), "z " );
//    windowXinit.plot();
 // ----------------------------------------------------------------------------------- //

    // ---------------------------- DEFINE CONTRAINTES --------------------------------- //


    // Min/Max time
#if TIME_OPT==1
    ocp.subjectTo(0.1 <= T <= 20);
#endif

#if ALGO==0 || ALGO==2
    //Start conditions
    ocp.subjectTo( AT_START, x == 0 );
    ocp.subjectTo( AT_START, y == 0  );
    ocp.subjectTo( AT_START, z ==  0 );
    ocp.subjectTo( AT_START, vx == 0  );
    ocp.subjectTo( AT_START, vy ==  0 );
    ocp.subjectTo( AT_START, vz ==  0 );
    ocp.subjectTo( AT_START, phi ==  0.0 );
//    ocp.subjectTo( AT_START, theta ==  -0. );
//    ocp.subjectTo( AT_START, psi ==  -0.);
    ocp.subjectTo( AT_START, p ==  0.0 );
    ocp.subjectTo( AT_START, q ==  0.0 );
    ocp.subjectTo( AT_START, r ==  0.0 );
    ocp.subjectTo( AT_START, (d*Cf*(u1*u1-u2*u2)+(Jy-Jz)*q*r)/Jx == 0 );
    ocp.subjectTo( AT_START, (d*Cf*(u4*u4-u3*u3)+(Jz-Jx)*p*r)/Jy == 0 );
    ocp.subjectTo( AT_START, (c*(u1*u1+u2*u2-u3*u3-u4*u4)+(Jx-Jy)*p*q)/Jz ==  0.0 );
    ocp.subjectTo( AT_START, Cf*(u1*u1+u2*u2+u3*u3+u4*u4)*sin(theta)/m ==  0);
    ocp.subjectTo( AT_START, -Cf*(u1*u1+u2*u2+u3*u3+u4*u4)*sin(psi)*cos(theta)/m ==   0 );
    ocp.subjectTo( AT_START, Cf*(u1*u1+u2*u2+u3*u3+u4*u4)*cos(psi)*cos(theta)/m - g ==  0 );
#endif

    //end conditions
//    ocp.subjectTo( AT_END  , (x-xf)*(x-xf) <= 100);//(x_init(nb_it,0)-xf)*(x_init(nb_it,0)-xf) );
//    ocp.subjectTo( AT_END  , (y-yf)*(y-yf) <= 10);//(x_init(nb_it,1)-yf)*(x_init(nb_it,1)-yf) );
//    ocp.subjectTo( AT_END  , (z-zf)*(z-zf) <= 10);//(x_init(nb_it,2)-zf)*(x_init(nb_it,2)-zf) );

    ocp.subjectTo( AT_END  , x ==  xf );
    ocp.subjectTo( AT_END  , y ==  yf );
    ocp.subjectTo( AT_END  , z ==  zf );
    ocp.subjectTo( AT_END  , vx ==  0 );
    ocp.subjectTo( AT_END  , vy ==   0);
    ocp.subjectTo( AT_END  , vz ==  0 );
////    ocp.subjectTo( AT_END  , phi ==  0.0 );
//    ocp.subjectTo( AT_END  , theta ==  -0. );
//    ocp.subjectTo( AT_END  , psi == -0.0 );
    ocp.subjectTo( AT_END  , p ==  0.0 );
    ocp.subjectTo( AT_END  , q ==  0.0 );
    ocp.subjectTo( AT_END  , r ==  0.0 );
    ocp.subjectTo( AT_END, (d*Cf*(u1*u1-u2*u2)+(Jy-Jz)*q*r)/Jx ==  0.0 );
    ocp.subjectTo( AT_END, (d*Cf*(u4*u4-u3*u3)+(Jz-Jx)*p*r)/Jy ==  0.0 );
    ocp.subjectTo( AT_END, (c*(u1*u1+u2*u2-u3*u3-u4*u4)+(Jx-Jy)*p*q)/Jz ==  0.0);
    ocp.subjectTo( AT_END, Cf*(u1*u1+u2*u2+u3*u3+u4*u4)*sin(theta)/m ==  0 );
    ocp.subjectTo( AT_END, -Cf*(u1*u1+u2*u2+u3*u3+u4*u4)*sin(psi)*cos(theta)/m == 0.  );
    ocp.subjectTo( AT_END, Cf*(u1*u1+u2*u2+u3*u3+u4*u4)*cos(psi)*cos(theta)/m - g ==  0 );

#if RELAX_END_CONST==1
    ocp.subjectTo( AT_END  , 0 <=  100*((x-xf)*(x-xf) + (y-yf)*(y-yf) + (z-zf)*(z-zf)) - vx*vx );
    ocp.subjectTo( AT_END  , 0 <=  100*((x-xf)*(x-xf) + (y-yf)*(y-yf) + (z-zf)*(z-zf)) - vy*vy );
    ocp.subjectTo( AT_END  , 0 <=  100*((x-xf)*(x-xf) + (y-yf)*(y-yf) + (z-zf)*(z-zf)) - vz*vz );
    ocp.subjectTo( AT_END  , 0 <=  100*((x-xf)*(x-xf) + (y-yf)*(y-yf) + (z-zf)*(z-zf)) - p*p );
    ocp.subjectTo( AT_END  , 0 <=  100*((x-xf)*(x-xf) + (y-yf)*(y-yf) + (z-zf)*(z-zf)) - q*q );
    ocp.subjectTo( AT_END  , 0 <=  100*((x-xf)*(x-xf) + (y-yf)*(y-yf) + (z-zf)*(z-zf)) - r*r );
    ocp.subjectTo( AT_END  , 0 <=  100*((x-xf)*(x-xf) + (y-yf)*(y-yf) + (z-zf)*(z-zf)) - psi*psi );
    ocp.subjectTo( AT_END  , 0 <=  100*((x-xf)*(x-xf) + (y-yf)*(y-yf) + (z-zf)*(z-zf)) - theta*theta );
#endif

    //Command constraints
    //on each motor
    ocp.subjectTo( 16 <= u1 <= 95 );
    ocp.subjectTo( 16 <= u2 <= 95 );
    ocp.subjectTo( 16 <= u3 <= 95 );
    ocp.subjectTo( 16 <= u4 <= 95 );
    ocp.subjectTo( -100 <= vu1 <= 100 );
    ocp.subjectTo( -100 <= vu2 <= 100 );
    ocp.subjectTo( -100 <= vu3 <= 100 );
    ocp.subjectTo( -100 <= vu4 <= 100 );

#if AVOID_SINGULARITIES == 1
    //Constraint to avoid singularity
    ocp.subjectTo( -1. <= theta <= 1.);
#endif


    //Obstacle constraints
#if OBS==1
    ocp.subjectTo( 16 <= ((x+3)*(x+3)+2*(z-5)*(z-5)) );
    ocp.subjectTo( 16 <= ((x-3)*(x-3)+2*(z-9)*(z-9)) );
    ocp.subjectTo( 16 <= ((x+3)*(x+3)+2*(z-15)*(z-15)) );
#endif

#if OBS==2
    ocp.subjectTo( 4 <= (x*x+4*(z-3)*(z-3)) );
    ocp.subjectTo( 4 <= ((x-2)*(x-2)+4*(z-6)*(z-6)) );
#endif

#if ALGO==2
    ocpMPC.subjectTo( 16 <= u1 <= 95 );
    ocpMPC.subjectTo( 16 <= u2 <= 95 );
    ocpMPC.subjectTo( 16 <= u3 <= 95 );
    ocpMPC.subjectTo( 16 <= u4 <= 95 );
    ocpMPC.subjectTo( -100 <= vu1 <= 100 );
    ocpMPC.subjectTo( -100 <= vu2 <= 100 );
    ocpMPC.subjectTo( -100 <= vu3 <= 100 );
    ocpMPC.subjectTo( -100 <= vu4 <= 100 );

#if AVOID_SINGULARITIES_MPC == 1
    //Constraint to avoid singularity
    ocpMPC.subjectTo( -1. <= theta <= 1. );
#endif

#if OBS==1
    ocpMPC.subjectTo( 18 <= ((x+3)*(x+3)+4*(z-5)*(z-5)) );
    ocpMPC.subjectTo( 18 <= ((x-2)*(x-2)+4*(z-9)*(z-9)) );
    ocpMPC.subjectTo( 18 <= ((x+2)*(x+2)+4*(z-15)*(z-15)) );
#endif

#endif
     // ----------------------------------------------------------------------------------- //






#if ALGO == 0 || ALGO==2

    // DEFINE A PLOT WINDOW:
    // ---------------------
    GnuplotWindow window2(PLOT_AT_EACH_ITERATION);
        window2.addSubplot( z,"DifferentialState z" );
        window2.addSubplot( x,"DifferentialState x" );
        window2.addSubplot( y,"DifferentialState y" );
        window2.addSubplot( theta,"DifferentialState theta" );
        window2.addSubplot( psi,"psi" );
        window2.addSubplot( u1,"u1" );
        window2.addSubplot( u2,"u2" );
        window2.addSubplot( u3,"u3" );
        window2.addSubplot( u4,"u4" );
//        window2.addSubplot( 16./((x+3)*(x+3)+4*(z-5)*(z-5)),"Dist obs1" );
//        window2.addSubplot( 16./((x-3)*(x-3)+4*(z-9)*(z-9)),"Dist obs2" );
//        window2.addSubplot( 16./((x+3)*(x+3)+4*(z-15)*(z-15)),"Dist obs3" );
//        window2.addSubplot( vx,"DifferentialState vx" );
//        window2.addSubplot( vz,"DifferentialState vz" );
//        window2.addSubplot( Cf*(u1*u1+u2*u2+u3*u3+u4*u4)*cos(phi)*cos(theta)/m - g,"acc z" );

        //window2.addSubplot( vx,"DifferentialState vx" );
//        window2.addSubplot( -Cf*(u1*u1+u2*u2+u3*u3+u4*u4)*sin(theta)/m,"acc x" );
//        window2.addSubplot( acc(0),"acc x libquadspline" );

        //window2.addSubplot( vy,"DifferentialState vy" );
        //window2.addSubplot( Cf*(u1*u1+u2*u2+u3*u3+u4*u4)*sin(phi)*cos(theta)/m,"acc y" );

        //window2.addSubplot( u4+u1+u2+u3,"f tot" );
        //window2.addSubplot( vu1,"vu1" );
       //window2.addSubplot( vu2,"vu2" );
        //window2.addSubplot( vu3,"vu3" );
        //window2.addSubplot( vu4,"vu4" );
//        window2.addSubplot( phi,"phi" );
//        window2.addSubplot( theta,"theta" );


//        window2.addSubplot( q,"theta" );
//        window2.addSubplot( x_init(10),"theta" );
//        window2.addSubplot(x_init(7), "pith libquad");
//        window2.addSubplot( sqrt((y-yf)*(y-yf)),"|y-yf|" );
        //window2.addSubplot( y,"y" );

//        window2.addSubplot( PLOT_KKT_TOLERANCE,"KKT Tolerance" );


    // ------------------------- INITIALIZE OCP PROBLEM ------------------------------- //
       OptimizationAlgorithm algorithm(ocp);
#if INIT==0 || INIT==1 || INIT==2 || INIT==3
       algorithm.initializeDifferentialStates(x_init);
#endif

       VariablesGrid param(1,timeGrid);
       param.setAll(10);

       algorithm.initializeParameters(param);

//    algorithm.set(DISCRETIZATION_TYPE, SINGLE_SHOOTING);
//    algorithm.set( DYNAMIC_SENSITIVITY,  FORWARD_SENSITIVITY );
//            algorithm.set( HESSIAN_APPROXIMATION, GAUSS_NEWTON );
//        algorithm.set( HESSIAN_APPROXIMATION, EXACT_HESSIAN );
//        algorithm.set( GLOBALIZATION_STRATEGY, GS_LINESEARCH );
//               algorithm.set( HESSIAN_APPROXIMATION, BLOCK_BFGS_UPDATE );
    algorithm.set( MAX_NUM_ITERATIONS, ocp_nb_it );
    algorithm.set( KKT_TOLERANCE, 1e-16 );
// 	algorithm.set( MAX_NUM_INTEGRATOR_STEPS, 4 );

#if PLOT==0
    algorithm << window2;
#endif
        LogRecord logdiff(LOG_AT_END , PS_DEFAULT);
    logdiff << LOG_DIFFERENTIAL_STATES;
//logdiff << LOG_KKT_TOLERANCE;

#if PLOT==1
        algorithm << logdiff;
#endif

        t = clock();

    algorithm.solve();

    t = clock() - t;
    std::cout << "total time : " << (((float)t)/CLOCKS_PER_SEC)<<std::endl;

    algorithm.getLogRecord(logdiff);

//#if ALGO==0
    logdiff.print(file);
//#endif

    VariablesGrid control(4,timeGrid);
    VariablesGrid state(16,timeGrid);
    algorithm.getDifferentialStates(state);
    algorithm.getControls(control);

    // -------------------- Export Results ------------------ //
//    VariablesGrid states, controls;
//    algorithm.getDifferentialStates(x_init);
//    x_init.print();

//    algorithm.getDifferentialStates(states);
//    algorithm.getControls("/tmp/controls.txt");
//    algorithm.getControls(controls);

#endif

#if ALGO == 1 || ALGO == 2
    // SETTING UP THE (SIMULATED) PROCESS:
    // -----------------------------------
    OutputFcn identity;
    DynamicSystem dynamicSystem( f,identity );
    Process process( dynamicSystem,INT_RK45 );

    // SETTING UP THE MPC CONTROLLER:
    // ------------------------------
#if ALGO==1
    RealTimeAlgorithm alg( ocp,tmpc );
#endif
#if ALGO==2
    RealTimeAlgorithm alg( ocpMPC,tmpc );
#endif
    alg.set( MAX_NUM_ITERATIONS, mpc_nb_it );
    StaticReferenceTrajectory zeroReference;
    Controller controller( alg,zeroReference );

#if ALGO==2
//    alg.initializeControls(control);
    alg.initializeDifferentialStates(state);
//    GnuplotWindow windowx;
//    windowx.addSubplot( state(0), "x " );
//    windowx.addSubplot( state(1), "y " );
//    windowx.addSubplot( state(2), "z " );
//    windowx.addSubplot( state(6),"phi" );
//        windowx.addSubplot( state(7),"theta" );
//            windowx.addSubplot( state(8),"psi" );
//                windowx.plot( );
#endif

#if ALGO==1 && (INIT==0 || INIT==1 || INIT==2 || INIT==3)
    alg.initializeDifferentialStates(x_init);
#endif

    // SET OPTION AND PLOTS WINDOW
    // ---------------------------
//    alg.set(DISCRETIZATION_TYPE, SINGLE_SHOOTING);
//        alg.set( HESSIAN_APPROXIMATION, BLOCK_BFGS_UPDATE );
//        alg.set( HESSIAN_APPROXIMATION, GAUSS_NEWTON_WITH_BLOCK_BFGS );
//            alg.set( HESSIAN_APPROXIMATION, GAUSS_NEWTON );
//    alg.set( HESSIAN_APPROXIMATION, FULL_BFGS_UPDATE );
//    alg.set( HESSIAN_APPROXIMATION, CONSTANT_HESSIAN );
//        alg.set( HESSIAN_APPROXIMATION, EXACT_HESSIAN );
        alg.set( GLOBALIZATION_STRATEGY, GS_LINESEARCH );
//        alg.set(INTEGRATOR_TYPE, INT_RK78);
//        alg.set(INTEGRATOR_TOLERANCE, 10e-6);

    GnuplotWindow window1(PLOT_AT_EACH_ITERATION);
    window1.addSubplot( z,"DifferentialState z" );
    window1.addSubplot( x,"DifferentialState x" );
    window1.addSubplot( theta,"DifferentialState theta" );
    window1.addSubplot( 16./((x+3)*(x+3)+4*(z-5)*(z-5)),"Dist obs1" );
    window1.addSubplot( 16./((x-3)*(x-3)+4*(z-9)*(z-9)),"Dist obs2" );
    window1.addSubplot( 16./((x+2)*(x+2)+4*(z-15)*(z-15)),"Dist obs3" );
//    alg<<window1;

    LogRecord logdiff2(LOG_AT_END, PS_DEFAULT);
//logdiff2 << LOG_;
//alg << logdiff2;

    // SETTING UP THE SIMULATION ENVIRONMENT,  RUN THE EXAMPLE...
    // ----------------------------------------------------------
    SimulationEnvironment sim( 0.0,7.,process,controller );

    t = clock();
    if (sim.init( x0 ) != SUCCESSFUL_RETURN)
        exit( EXIT_FAILURE );
    if (sim.run( ) != SUCCESSFUL_RETURN)
        exit( EXIT_FAILURE );
    t = clock() - t;
    std::cout << "total time : " << (((float)t)/CLOCKS_PER_SEC)<<std::endl;

    // ...AND PLOT THE RESULTS
    // ----------------------------------------------------------
    VariablesGrid sampledProcessOutput;
    sim.getSampledProcessOutput( sampledProcessOutput );
    sampledProcessOutput.print(file);

    VariablesGrid feedbackControl;
    sim.getFeedbackControl( feedbackControl );
    feedbackControl.print(file2);


    GnuplotWindow window;
    window.addSubplot( sampledProcessOutput(0), "x " );
    window.addSubplot( sampledProcessOutput(1), "y " );
    window.addSubplot( sampledProcessOutput(2), "z " );
    window.addSubplot( sampledProcessOutput(6),"phi" );
        window.addSubplot( sampledProcessOutput(7),"theta" );
            window.addSubplot( sampledProcessOutput(8),"psi" );

    //               window.addSubplot( sampledProcessOutput(3), "Wheel Velocity [m/s]" );
    //               window.addSubplot( feedbackControl(1),      "Damping Force [N]" );
    //               window.addSubplot( feedbackControl(0),      "Road Excitation [m]" );
    window.plot( );
    logdiff2.print();

#endif

#if ALGO==3
    // ------------------------------- CODe GENERATION TOOL -------------- //
//    OCPexport mpc( ocp );

//    mpc.set( HESSIAN_APPROXIMATION,       GAUSS_NEWTON    );
//    mpc.set( DISCRETIZATION_TYPE,         SINGLE_SHOOTING );
//    mpc.set( INTEGRATOR_TYPE,             INT_RK4         );
//    mpc.set( NUM_INTEGRATOR_STEPS,        30              );

//    mpc.set( QP_SOLVER,                   QP_QPOASES      );
//    mpc.set( HOTSTART_QP,                 YES             );
//// 	mpc.set( LEVENBERG_MARQUARDT,         1.0e-4          );
//    mpc.set( GENERATE_TEST_FILE,          YES             );
//    mpc.set( GENERATE_MAKE_FILE,          YES             );
////    mpc.set( GENERATE_MATLAB_INTERFACE,   YES             );
////    mpc.set( GENERATE_SIMULINK_INTERFACE, YES             );
//// 	mpc.set( USE_SINGLE_PRECISION,        YES             );

//    if (mpc.exportCode( "getting_started_export" ) != SUCCESSFUL_RETURN)
//        exit( EXIT_FAILURE );

//    mpc.printDimensionsQP( );
#endif

    return 0;
}
/* <<< end tutorial code <<< */

