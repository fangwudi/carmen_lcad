/*               -*- mode: c++ -*-

   DO NOT EDIT THIS FILE -- CHANGES WILL BE LOST!

   [Wed Dec 12 08:54:17 BRST 2012] This file was automatically generated by
   applying the script "./devscripts/extract_templates.tcl" to the template
   declarations in source file "src/Image/CutPaste.H" for types [byte,
   float]
*/
#ifndef INST_CLASS
#  define INST_CLASS
#endif
#ifdef INST_NAMESPACE
namespace INST_NAMESPACE {
#endif
template Image<byte> INST_CLASS concatX(const Image<byte>& left, const Image<byte>& right);
template Image< PixRGB<byte> > INST_CLASS concatX(const Image< PixRGB<byte> >& left, const Image< PixRGB<byte> >& right);
template Image<float> INST_CLASS concatX(const Image<float>& left, const Image<float>& right);
template Image< PixRGB<float> > INST_CLASS concatX(const Image< PixRGB<float> >& left, const Image< PixRGB<float> >& right);
template Image<byte> INST_CLASS concatY(const Image<byte>& top, const Image<byte>& bottom);
template Image< PixRGB<byte> > INST_CLASS concatY(const Image< PixRGB<byte> >& top, const Image< PixRGB<byte> >& bottom);
template Image<float> INST_CLASS concatY(const Image<float>& top, const Image<float>& bottom);
template Image< PixRGB<float> > INST_CLASS concatY(const Image< PixRGB<float> >& top, const Image< PixRGB<float> >& bottom);
template Image<byte> INST_CLASS concatLooseY(const Image<byte>& topImg, const Image<byte>& bottomImg, const byte& bgColor );
template Image< PixRGB<byte> > INST_CLASS concatLooseY(const Image< PixRGB<byte> >& topImg, const Image< PixRGB<byte> >& bottomImg, const  PixRGB<byte> & bgColor );
template Image<float> INST_CLASS concatLooseY(const Image<float>& topImg, const Image<float>& bottomImg, const float& bgColor );
template Image< PixRGB<float> > INST_CLASS concatLooseY(const Image< PixRGB<float> >& topImg, const Image< PixRGB<float> >& bottomImg, const  PixRGB<float> & bgColor );
template Image<byte> INST_CLASS concatLooseX(const Image<byte>& leftImg, const Image<byte>& rightImg, const byte& bgColor );
template Image< PixRGB<byte> > INST_CLASS concatLooseX(const Image< PixRGB<byte> >& leftImg, const Image< PixRGB<byte> >& rightImg, const  PixRGB<byte> & bgColor );
template Image<float> INST_CLASS concatLooseX(const Image<float>& leftImg, const Image<float>& rightImg, const float& bgColor );
template Image< PixRGB<float> > INST_CLASS concatLooseX(const Image< PixRGB<float> >& leftImg, const Image< PixRGB<float> >& rightImg, const  PixRGB<float> & bgColor );
template Image<byte> INST_CLASS crop(const Image<byte>& src, const Point2D<int>& upperLeft, const Dims& size, const bool zerofill );
template Image< PixRGB<byte> > INST_CLASS crop(const Image< PixRGB<byte> >& src, const Point2D<int>& upperLeft, const Dims& size, const bool zerofill );
template Image<float> INST_CLASS crop(const Image<float>& src, const Point2D<int>& upperLeft, const Dims& size, const bool zerofill );
template Image< PixRGB<float> > INST_CLASS crop(const Image< PixRGB<float> >& src, const Point2D<int>& upperLeft, const Dims& size, const bool zerofill );
template Image<byte> INST_CLASS crop(const Image<byte>& src, const Rectangle& rect, const bool zerofill );
template Image< PixRGB<byte> > INST_CLASS crop(const Image< PixRGB<byte> >& src, const Rectangle& rect, const bool zerofill );
template Image<float> INST_CLASS crop(const Image<float>& src, const Rectangle& rect, const bool zerofill );
template Image< PixRGB<float> > INST_CLASS crop(const Image< PixRGB<float> >& src, const Rectangle& rect, const bool zerofill );
template Image<byte> INST_CLASS shift(const Image<byte>& srcImg, const int dx, const int dy);
template Image< PixRGB<byte> > INST_CLASS shift(const Image< PixRGB<byte> >& srcImg, const int dx, const int dy);
template Image<float> INST_CLASS shift(const Image<float>& srcImg, const int dx, const int dy);
template Image< PixRGB<float> > INST_CLASS shift(const Image< PixRGB<float> >& srcImg, const int dx, const int dy);
template Image<byte> INST_CLASS shiftImage(const Image<byte>& srcImg, const float dx, const float dy);
template Image< PixRGB<byte> > INST_CLASS shiftImage(const Image< PixRGB<byte> >& srcImg, const float dx, const float dy);
template Image<float> INST_CLASS shiftImage(const Image<float>& srcImg, const float dx, const float dy);
template Image< PixRGB<float> > INST_CLASS shiftImage(const Image< PixRGB<float> >& srcImg, const float dx, const float dy);
template Image<byte> INST_CLASS shiftClean(const Image<byte>& srcImg, const int dx, const int dy, const byte bgval );
template Image< PixRGB<byte> > INST_CLASS shiftClean(const Image< PixRGB<byte> >& srcImg, const int dx, const int dy, const  PixRGB<byte>  bgval );
template Image<float> INST_CLASS shiftClean(const Image<float>& srcImg, const int dx, const int dy, const float bgval );
template Image< PixRGB<float> > INST_CLASS shiftClean(const Image< PixRGB<float> >& srcImg, const int dx, const int dy, const  PixRGB<float>  bgval );
template void INST_CLASS inplacePaste(Image<byte>& dst, const Image<byte>& img, const Point2D<int>& pos);
template void INST_CLASS inplacePaste(Image< PixRGB<byte> >& dst, const Image< PixRGB<byte> >& img, const Point2D<int>& pos);
template void INST_CLASS inplacePaste(Image<float>& dst, const Image<float>& img, const Point2D<int>& pos);
template void INST_CLASS inplacePaste(Image< PixRGB<float> >& dst, const Image< PixRGB<float> >& img, const Point2D<int>& pos);
template void INST_CLASS inplaceClearRegion(Image<byte>& dst, const Rectangle& region, const byte& val);
template void INST_CLASS inplaceClearRegion(Image< PixRGB<byte> >& dst, const Rectangle& region, const  PixRGB<byte> & val);
template void INST_CLASS inplaceClearRegion(Image<float>& dst, const Rectangle& region, const float& val);
template void INST_CLASS inplaceClearRegion(Image< PixRGB<float> >& dst, const Rectangle& region, const  PixRGB<float> & val);
template void INST_CLASS inplaceEmbed(Image<byte>& embed, const Image<byte>& img, const Rectangle& r, const byte background, const bool keep_aspect );
template void INST_CLASS inplaceEmbed(Image< PixRGB<byte> >& embed, const Image< PixRGB<byte> >& img, const Rectangle& r, const  PixRGB<byte>  background, const bool keep_aspect );
template void INST_CLASS inplaceEmbed(Image<float>& embed, const Image<float>& img, const Rectangle& r, const float background, const bool keep_aspect );
template void INST_CLASS inplaceEmbed(Image< PixRGB<float> >& embed, const Image< PixRGB<float> >& img, const Rectangle& r, const  PixRGB<float>  background, const bool keep_aspect );
template Rectangle INST_CLASS findBoundingRect(const Image<byte>& src, const byte threshold);
template Rectangle INST_CLASS findBoundingRect(const Image<float>& src, const float threshold);
#ifdef INST_NAMESPACE
}
#endif
