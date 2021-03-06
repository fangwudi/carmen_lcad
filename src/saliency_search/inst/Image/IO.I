/*               -*- mode: c++ -*-

   DO NOT EDIT THIS FILE -- CHANGES WILL BE LOST!

   [Wed Dec 12 08:54:30 BRST 2012] This file was automatically generated by
   applying the script "./devscripts/extract_templates.tcl" to the template
   declarations in source file "src/Image/IO.H" for types [byte, float]
*/
#ifndef INST_CLASS
#  define INST_CLASS
#endif
#ifdef INST_NAMESPACE
namespace INST_NAMESPACE {
#endif
template std::ostream& operator<<(std::ostream& os, const Image<byte>& img);
template std::ostream& operator<<(std::ostream& os, const Image< PixRGB<byte> >& img);
template std::ostream& operator<<(std::ostream& os, const Image<float>& img);
template std::ostream& operator<<(std::ostream& os, const Image< PixRGB<float> >& img);
template void INST_CLASS writeImageToStream(std::ostream& os, const Image<byte>& img);
template void INST_CLASS writeImageToStream(std::ostream& os, const Image<float>& img);
template void INST_CLASS readImageFromStream(std::istream& is, Image<byte>& img);
template void INST_CLASS readImageFromStream(std::istream& is, Image<float>& img);
#ifdef INST_NAMESPACE
}
#endif
