/**
 * @file
 * @brief Implementation of reading 3D scans
 * @author Andreas Nuechter. Jacobs University Bremen gGmbH
 * @author Dorit Borrmann. Smart Systems Group, Jacobs University Bremen gGmbH, Germany. 
 */

#include "slam6d/scan_io_riegl_rgb.h"
#include "slam6d/globals.icc"
#include <fstream>
using std::ifstream;
#include <iostream>
using std::cerr;
using std::endl;

#include <algorithm>
using std::swap;

#ifdef _MSC_VER
#include <windows.h>
#endif

/**
 * Reads specified scans from given directory in
 * the file format Riegl Laser Measurement GmbH 
 * uses including color information . It will be compiled as shared lib.
 *
 * Scan poses will NOT be initialized after a call
 * to this function. Initial pose estimation works 
 * only with the -p switch, i.e., trusting the initial
 * estimations by Riegl.
 *
 * The scans have to be exported from the Riegl software
 * as follows:
 * 1. Export point cloud data to ASCII
 *    Use Scanners own Coordinate System (SOCS)
 *    X Y Z Range Theta Phi RGB Reflectance
 *    Name the files scanXXX.rgb where XXX determines the number of the scan
 * 2. Export acqusition location (after you have registered
 *    with the Riegl software)
 *    Export SOP
 *    Write out as .dat file 
 * 
 * @param start Starts to read with this scan
 * @param end Stops with this scan
 * @param dir The directory from which to read
 * @param maxDist Reads only Points up to this Distance
 * @param minDist Reads only Points from this Distance
 * @param euler Initital pose estimates (will not be applied to the points
 * @param ptss Vector containing the read points
 */
int ScanIO_riegl_rgb::readScans(int start, int end, string &dir, int maxDist, int mindist,
						  double *euler, vector<Point> &ptss)
{
  static int fileCounter = start;
  string scanFileName;
  string poseFileName;

  ifstream scan_in, pose_in;

  if (end > -1 && fileCounter > end) return -1; // 'nuf read

  
  poseFileName = dir + "scan" + to_string(fileCounter,3) + ".dat";
  scanFileName = dir + "scan" + to_string(fileCounter,3) + ".rgb";
    
  scan_in.open(scanFileName.c_str());
  pose_in.open(poseFileName.c_str());
  // read 3D scan

  if (!pose_in.good() && !scan_in.good()) return -1; // no more files in the directory
  if (!pose_in.good()) { cerr << "ERROR: Missing file " << poseFileName << endl; exit(1); }
  if (!scan_in.good()) { cerr << "ERROR: Missing file " << scanFileName << endl; exit(1); }
  cout << "Processing Scan " << scanFileName;
  cout.flush();
  
  double rPos[3], rPosTheta[16];
  double inMatrix[16], tMatrix[16];
  for (unsigned int i = 0; i < 16; pose_in >> inMatrix[i++]);
 
  M4transpose(inMatrix, tMatrix);
  Matrix4ToEuler(tMatrix, rPosTheta, rPos);
    
  rPos[0] *= 100;
  rPos[1] *= 100;
  rPos[2] *= 100;

  //  double t3 = (double)(rand() % 10);
    
  swap(rPos[1], rPos[2]);
  swap(rPosTheta[1], rPosTheta[2]);
  euler[0] = rPos[0];
  euler[1] = rPos[1];
  euler[2] = rPos[2];
  euler[3] = rPosTheta[0];
  euler[4] = rPosTheta[1];
  euler[5] = rPosTheta[2];

  double test[16], test1[16];
  EulerToMatrix4(rPos, rPosTheta, test);
  M4inv(test, test1);
  /*
  // transform input pose
  tMatrix[0] = inMatrix[5];
  tMatrix[1] = -inMatrix[9];
  tMatrix[2] = -inMatrix[1];
  tMatrix[3] = -inMatrix[13];
  tMatrix[4] = -inMatrix[6];
  tMatrix[5] = inMatrix[10];
  tMatrix[6] = inMatrix[2];
  tMatrix[7] = inMatrix[14];
  tMatrix[8] = -inMatrix[4];
  tMatrix[9] = inMatrix[8];
  tMatrix[10] = inMatrix[0];
  tMatrix[11] = inMatrix[12];
  tMatrix[12] = -inMatrix[7];
  tMatrix[13] = inMatrix[11];
  tMatrix[14] = inMatrix[3];
  tMatrix[15] = inMatrix[15];
  
  Matrix4ToEuler(tMatrix, rPosTheta, rPos);
 
  cout << endl;
  for(int i = 0; i < 16; i++) {
    cout << tMatrix[i] << " ";
  }
  cout << endl;
  euler[0] = 100*rPos[0];
  euler[1] = 100*rPos[1];
  euler[2] = 100*rPos[2];
  euler[3] = rPosTheta[0];
  euler[4] = rPosTheta[1];
  euler[5] = rPosTheta[2];
  */

  long num_pts = 0;
   
  if (scan_in.good()) {
    scan_in >> num_pts;
    cout << " with " << num_pts << " Points";
    cout.flush();
    ptss.reserve(num_pts);
  }
  
  // read point data and transform into slam6D coordinate system 
  /*
  EulerToMatrix4(rPos, rPosTheta, tMatrix); 
  cout << endl;
  for(int i = 0; i < 16; i++) {
    cout << tMatrix[i] << " ";
  }
  cout << endl;
  //M4inv(tMatrix, inMatrix); 
  */
  while (scan_in.good()) {
    Point p;
    double range, theta, phi, reflectance;
    int r, g, b;
    //scan_in >> p.z >> p.x >> p.y >> range >> theta >> phi >> r >> g >> b >> reflectance;
    scan_in >> p.x >> p.z >> p.y >> range >> theta >> phi >> r >> g >> b >> reflectance;
    
    p.x *= 100;
    p.y *= 100;
    p.z *= 100;
    
    p.transform(test1);
/*
    p2.x = p.z;
    p2.z = -p.x;
    p2.y = p.y;
    */
    p.rgb[0] = (char)r;
    p.rgb[1] = (char)g;
    p.rgb[2] = (char)b;
    
    p.reflectance = reflectance;

    ptss.push_back(p);
  }
  
  cout << " done" << endl;

  scan_in.close();
  scan_in.clear();
  pose_in.close();
  pose_in.clear();
  fileCounter++;
  
  return fileCounter-1;
}


/**
 * class factory for object construction
 *
 * @return Pointer to new object
 */
#ifdef _MSC_VER
extern "C" __declspec(dllexport) ScanIO* create()
#else
extern "C" ScanIO* create()
#endif
{
  return new ScanIO_riegl_rgb;
}


/**
 * class factory for object construction
 *
 * @return Pointer to new object
 */
#ifdef _MSC_VER
extern "C" __declspec(dllexport) void destroy(ScanIO *sio)
#else
extern "C" void destroy(ScanIO *sio)
#endif
{
  delete sio;
}

#ifdef _MSC_VER
BOOL APIENTRY DllMain(HANDLE hModule, DWORD dwReason, LPVOID lpReserved)
{
	return TRUE;
}
#endif
