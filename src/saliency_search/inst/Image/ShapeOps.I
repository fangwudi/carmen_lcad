/*               -*- mode: c++ -*-

   DO NOT EDIT THIS FILE -- CHANGES WILL BE LOST!

   [Wed Dec 12 08:55:12 BRST 2012] This file was automatically generated by
   applying the script "./devscripts/extract_templates.tcl" to the template
   declarations in source file "src/Image/ShapeOps.H" for types [byte,
   float]
*/
#ifndef INST_CLASS
#  define INST_CLASS
#endif
#ifdef INST_NAMESPACE
namespace INST_NAMESPACE {
#endif
template Image<byte> INST_CLASS quickLocalAvg(const Image<byte>& array, const int scale);
template Image< PixRGB<byte> > INST_CLASS quickLocalAvg(const Image< PixRGB<byte> >& array, const int scale);
template Image<float> INST_CLASS quickLocalAvg(const Image<float>& array, const int scale);
template Image< PixRGB<float> > INST_CLASS quickLocalAvg(const Image< PixRGB<float> >& array, const int scale);
template Image<byte> INST_CLASS quickLocalAvg2x2(const Image<byte>& array);
template Image< PixRGB<byte> > INST_CLASS quickLocalAvg2x2(const Image< PixRGB<byte> >& array);
template Image<float> INST_CLASS quickLocalAvg2x2(const Image<float>& array);
template Image< PixRGB<float> > INST_CLASS quickLocalAvg2x2(const Image< PixRGB<float> >& array);
template Image<byte> INST_CLASS quickLocalMax(const Image<byte>& array, const int scale);
template Image< PixRGB<byte> > INST_CLASS quickLocalMax(const Image< PixRGB<byte> >& array, const int scale);
template Image<float> INST_CLASS quickLocalMax(const Image<float>& array, const int scale);
template Image< PixRGB<float> > INST_CLASS quickLocalMax(const Image< PixRGB<float> >& array, const int scale);
template Image<byte> INST_CLASS quickLocalMin(const Image<byte>& array, const int scale);
template Image< PixRGB<byte> > INST_CLASS quickLocalMin(const Image< PixRGB<byte> >& array, const int scale);
template Image<float> INST_CLASS quickLocalMin(const Image<float>& array, const int scale);
template Image< PixRGB<float> > INST_CLASS quickLocalMin(const Image< PixRGB<float> >& array, const int scale);
template Image<byte> INST_CLASS quickInterpolate(const Image<byte>& src, const int sfactor);
template Image< PixRGB<byte> > INST_CLASS quickInterpolate(const Image< PixRGB<byte> >& src, const int sfactor);
template Image<float> INST_CLASS quickInterpolate(const Image<float>& src, const int sfactor);
template Image< PixRGB<float> > INST_CLASS quickInterpolate(const Image< PixRGB<float> >& src, const int sfactor);
template Image<byte> INST_CLASS interpolate(const Image<byte>& src);
template Image< PixRGB<byte> > INST_CLASS interpolate(const Image< PixRGB<byte> >& src);
template Image<float> INST_CLASS interpolate(const Image<float>& src);
template Image< PixRGB<float> > INST_CLASS interpolate(const Image< PixRGB<float> >& src);
template Image<byte> INST_CLASS rescaleBilinear(const Image<byte>& src, const Dims& dims);
template Image< PixRGB<byte> > INST_CLASS rescaleBilinear(const Image< PixRGB<byte> >& src, const Dims& dims);
template Image<float> INST_CLASS rescaleBilinear(const Image<float>& src, const Dims& dims);
template Image< PixRGB<float> > INST_CLASS rescaleBilinear(const Image< PixRGB<float> >& src, const Dims& dims);
template Image<byte> INST_CLASS rescaleBilinear(const Image<byte>& src, int width, int height);
template Image< PixRGB<byte> > INST_CLASS rescaleBilinear(const Image< PixRGB<byte> >& src, int width, int height);
template Image<float> INST_CLASS rescaleBilinear(const Image<float>& src, int width, int height);
template Image< PixRGB<float> > INST_CLASS rescaleBilinear(const Image< PixRGB<float> >& src, int width, int height);
template Image<byte> INST_CLASS rescaleNI(const Image<byte>& src, const Dims& dims);
template Image< PixRGB<byte> > INST_CLASS rescaleNI(const Image< PixRGB<byte> >& src, const Dims& dims);
template Image<float> INST_CLASS rescaleNI(const Image<float>& src, const Dims& dims);
template Image< PixRGB<float> > INST_CLASS rescaleNI(const Image< PixRGB<float> >& src, const Dims& dims);
template Image<byte> INST_CLASS rescaleNI(const Image<byte>& src, int width, int height);
template Image< PixRGB<byte> > INST_CLASS rescaleNI(const Image< PixRGB<byte> >& src, int width, int height);
template Image<float> INST_CLASS rescaleNI(const Image<float>& src, int width, int height);
template Image< PixRGB<float> > INST_CLASS rescaleNI(const Image< PixRGB<float> >& src, int width, int height);
template Image<byte> INST_CLASS rescale(const Image<byte>& src, const Dims& newdims, RescaleType ftype );
template Image< PixRGB<byte> > INST_CLASS rescale(const Image< PixRGB<byte> >& src, const Dims& newdims, RescaleType ftype );
template Image<float> INST_CLASS rescale(const Image<float>& src, const Dims& newdims, RescaleType ftype );
template Image< PixRGB<float> > INST_CLASS rescale(const Image< PixRGB<float> >& src, const Dims& newdims, RescaleType ftype );
template Image<byte> INST_CLASS rescale(const Image<byte>& src, const int width, const int height, RescaleType ftype );
template Image< PixRGB<byte> > INST_CLASS rescale(const Image< PixRGB<byte> >& src, const int width, const int height, RescaleType ftype );
template Image<float> INST_CLASS rescale(const Image<float>& src, const int width, const int height, RescaleType ftype );
template Image< PixRGB<float> > INST_CLASS rescale(const Image< PixRGB<float> >& src, const int width, const int height, RescaleType ftype );
template Image<byte> INST_CLASS rescaleOpt(const Image<byte>& src, const Dims& dims, const bool interp);
template Image< PixRGB<byte> > INST_CLASS rescaleOpt(const Image< PixRGB<byte> >& src, const Dims& dims, const bool interp);
template Image<float> INST_CLASS rescaleOpt(const Image<float>& src, const Dims& dims, const bool interp);
template Image< PixRGB<float> > INST_CLASS rescaleOpt(const Image< PixRGB<float> >& src, const Dims& dims, const bool interp);
template Image<byte> INST_CLASS rescaleOpt(const Image<byte>& src, int width, int height, const bool interp);
template Image< PixRGB<byte> > INST_CLASS rescaleOpt(const Image< PixRGB<byte> >& src, int width, int height, const bool interp);
template Image<float> INST_CLASS rescaleOpt(const Image<float>& src, int width, int height, const bool interp);
template Image< PixRGB<float> > INST_CLASS rescaleOpt(const Image< PixRGB<float> >& src, int width, int height, const bool interp);
template Image<PixRGB<byte> > INST_CLASS downscaleFancy(const Image<PixRGB<byte> >& src, int width, int height, int weighting_slope, bool no_weight_black);
template Image<PixRGB<float> > INST_CLASS downscaleFancy(const Image<PixRGB<float> >& src, int width, int height, int weighting_slope, bool no_weight_black);
template Image<byte> INST_CLASS downSize(const Image<byte>& src, const Dims& dims, const int filterWidth );
template Image<float> INST_CLASS downSize(const Image<float>& src, const Dims& dims, const int filterWidth );
template Image<byte> INST_CLASS downSize(const Image<byte>& src, const int width, const int height, const int filterWidth );
template Image<float> INST_CLASS downSize(const Image<float>& src, const int width, const int height, const int filterWidth );
template Image<byte> INST_CLASS concatArray(const Image<byte> arr[], const int nbarr, const int Nx, const int destX , const int destY );
template Image< PixRGB<byte> > INST_CLASS concatArray(const Image< PixRGB<byte> > arr[], const int nbarr, const int Nx, const int destX , const int destY );
template Image<float> INST_CLASS concatArray(const Image<float> arr[], const int nbarr, const int Nx, const int destX , const int destY );
template Image< PixRGB<float> > INST_CLASS concatArray(const Image< PixRGB<float> > arr[], const int nbarr, const int Nx, const int destX , const int destY );
template Image<byte> INST_CLASS decXY(const Image<byte>& src, const int xfactor , const int yfactor );
template Image< PixRGB<byte> > INST_CLASS decXY(const Image< PixRGB<byte> >& src, const int xfactor , const int yfactor );
template Image<float> INST_CLASS decXY(const Image<float>& src, const int xfactor , const int yfactor );
template Image< PixRGB<float> > INST_CLASS decXY(const Image< PixRGB<float> >& src, const int xfactor , const int yfactor );
template Image<byte> INST_CLASS decX(const Image<byte>& src, const int factor );
template Image< PixRGB<byte> > INST_CLASS decX(const Image< PixRGB<byte> >& src, const int factor );
template Image<float> INST_CLASS decX(const Image<float>& src, const int factor );
template Image< PixRGB<float> > INST_CLASS decX(const Image< PixRGB<float> >& src, const int factor );
template Image<byte> INST_CLASS decY(const Image<byte>& src, const int factor );
template Image< PixRGB<byte> > INST_CLASS decY(const Image< PixRGB<byte> >& src, const int factor );
template Image<float> INST_CLASS decY(const Image<float>& src, const int factor );
template Image< PixRGB<float> > INST_CLASS decY(const Image< PixRGB<float> >& src, const int factor );
template Image<byte> INST_CLASS blurAndDecY(const Image<byte>& src, const int factor);
template Image<float> INST_CLASS blurAndDecY(const Image<float>& src, const int factor);
template Image<byte> INST_CLASS intXY(const Image<byte>& src, const bool dupli);
template Image< PixRGB<byte> > INST_CLASS intXY(const Image< PixRGB<byte> >& src, const bool dupli);
template Image<float> INST_CLASS intXY(const Image<float>& src, const bool dupli);
template Image< PixRGB<float> > INST_CLASS intXY(const Image< PixRGB<float> >& src, const bool dupli);
template Image<byte> INST_CLASS intX(const Image<byte>& src, const bool dupli);
template Image< PixRGB<byte> > INST_CLASS intX(const Image< PixRGB<byte> >& src, const bool dupli);
template Image<float> INST_CLASS intX(const Image<float>& src, const bool dupli);
template Image< PixRGB<float> > INST_CLASS intX(const Image< PixRGB<float> >& src, const bool dupli);
template Image<byte> INST_CLASS intY(const Image<byte>& src, const bool dupli);
template Image< PixRGB<byte> > INST_CLASS intY(const Image< PixRGB<byte> >& src, const bool dupli);
template Image<float> INST_CLASS intY(const Image<float>& src, const bool dupli);
template Image< PixRGB<float> > INST_CLASS intY(const Image< PixRGB<float> >& src, const bool dupli);
template Image<byte> INST_CLASS zoomXY(const Image<byte>& src, int xzoom , int yzoom );
template Image< PixRGB<byte> > INST_CLASS zoomXY(const Image< PixRGB<byte> >& src, int xzoom , int yzoom );
template Image<float> INST_CLASS zoomXY(const Image<float>& src, int xzoom , int yzoom );
template Image< PixRGB<float> > INST_CLASS zoomXY(const Image< PixRGB<float> >& src, int xzoom , int yzoom );
template Image<byte> INST_CLASS rotate(const Image<byte>& srcImg, const int x, const int y, const float ang);
template Image< PixRGB<byte> > INST_CLASS rotate(const Image< PixRGB<byte> >& srcImg, const int x, const int y, const float ang);
template Image<float> INST_CLASS rotate(const Image<float>& srcImg, const int x, const int y, const float ang);
template Image< PixRGB<float> > INST_CLASS rotate(const Image< PixRGB<float> >& srcImg, const int x, const int y, const float ang);
#ifdef INST_NAMESPACE
}
#endif
