/*!@file Channels/DirectionSpatioTemporalChannel.H */

// //////////////////////////////////////////////////////////////////// //
// The iLab Neuromorphic Vision C++ Toolkit - Copyright (C) 2000-2005   //
// by the University of Southern California (USC) and the iLab at USC.  //
// See http://iLab.usc.edu for information about this project.          //
// //////////////////////////////////////////////////////////////////// //
// Major portions of the iLab Neuromorphic Vision Toolkit are protected //
// under the U.S. patent ``Computation of Intrinsic Perceptual Saliency //
// in Visual Environments, and Applications'' by Christof Koch and      //
// Laurent Itti, California Institute of Technology, 2001 (patent       //
// pending; application number 09/912,225 filed July 23, 2001; see      //
// http://pair.uspto.gov/cgi-bin/final/home.pl for current status).     //
// //////////////////////////////////////////////////////////////////// //
// This file is part of the iLab Neuromorphic Vision C++ Toolkit.       //
//                                                                      //
// The iLab Neuromorphic Vision C++ Toolkit is free software; you can   //
// redistribute it and/or modify it under the terms of the GNU General  //
// Public License as published by the Free Software Foundation; either  //
// version 2 of the License, or (at your option) any later version.     //
//                                                                      //
// The iLab Neuromorphic Vision C++ Toolkit is distributed in the hope  //
// that it will be useful, but WITHOUT ANY WARRANTY; without even the   //
// implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR      //
// PURPOSE.  See the GNU General Public License for more details.       //
//                                                                      //
// You should have received a copy of the GNU General Public License    //
// along with the iLab Neuromorphic Vision C++ Toolkit; if not, write   //
// to the Free Software Foundation, Inc., 59 Temple Place, Suite 330,   //
// Boston, MA 02111-1307 USA.                                           //
// //////////////////////////////////////////////////////////////////// //
//
// Primary maintainer for this file:
// $HeadURL: svn://isvn.usc.edu/software/invt/trunk/saliency/src/Channels/DirectionSpatioTemporalChannel.H $
// $Id: $
//

#ifndef DIRECTIONSPATIOTEMPORALCHANNEL_H_DEFINED
#define DIRECTIONSPATIOTEMPORALCHANNEL_H_DEFINED

#include "Channels/SingleChannel.H"
#include "Robots/Beobot2/Navigation/FOE_Navigation/SpatioTemporalEnergy.H"


#include "rutz/shared_ptr.h"

// ######################################################################
//! a motion sensitive channel with direction selectivity
//! uses spatiotemporal filters
class DirectionSpatioTemporalChannel : public SingleChannel
{
public:
  //! Constructor with some parameteres
  /*! @param dirIndex is the channel's index, used in Param and Jet access
    @param direction is the preferred direction in degrees (0..360)
    @param type the type of the pyramid used for computing
    SpatioTemporal Energy. */
  DirectionSpatioTemporalChannel
  (OptionManager& mgr, 
   const uint dirIndex, 
   const uint speedIndex,
   const double direction, 
   const double speed,
   const PyramidType type);

  //! Destructor
  virtual ~DirectionSpatioTemporalChannel();

  //! Special version of computePyramid()
  virtual ImageSet<float> computePyramid
  (const Image<float>& bwimg,
   const rutz::shared_ptr<PyramidCache<float> >& cache);

  ImageSet<float> getSpatioTemporalEnergy();

  void setMTfeatureMap(Image<float> mtFeat);

  Image<float> getRawCSmap(const uint idx) const;

protected:

  //! DirectionSpatioTemporalChannel requires only luminance input.
  virtual void doInput(const InputFrame& inframe);

  void start1(); //!< get started
  void start2(); //!< get started

  NModelParam<uint> itsDirIndex;
  NModelParam<uint> itsSpeedIndex;
  NModelParam<double> itsDirection;
  NModelParam<double> itsSpeed;

  //! spatiotemporal energy maps
  ImageSet<float> itsSpatioTemporalEnergy;
  rutz::shared_ptr<SpatioTemporalEnergyPyrBuilder<float> > 
  itsSpatioTemporalPyrBuilder; 

  //! the final MT feature map
  //! this is after the directional competition
  Image<float> itsMTfeatureMap;
};

// ######################################################################
/* So things look consistent in everyone's emacs... */
/* Local Variables: */
/* indent-tabs-mode: nil */
/* End: */

#endif // DIRECTIONSPATIOTEMPORALCHANNEL_H_DEFINED
