// Copyright (c) 2015, LAAS-CNRS
// Authors: Florent Lamiraux
//
// This file is part of hpp-constraints.
// hpp-constraints is free software: you can redistribute it
// and/or modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation, either version
// 3 of the License, or (at your option) any later version.
//
// hpp-constraints is distributed in the hope that it will be
// useful, but WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Lesser Public License for more details.  You should have
// received a copy of the GNU Lesser General Public License along with
// hpp-constraints. If not, see <http://www.gnu.org/licenses/>.

#include <hpp/pinocchio/device.hh>
#include <hpp/constraints/differentiable-function.hh>
#include <hpp/constraints/matrix-view.hh>
#include <hpp/constraints/explicit.hh>
#include <hpp/constraints/explicit/implicit-function.hh>

namespace hpp {
  namespace constraints {
    typedef constraints::Implicit Implicit;
    typedef constraints::ImplicitPtr_t ImplicitPtr_t;
    void complement (size_type size, const segments_t& intervals,
		     segments_t& result)
    {
      std::vector <bool> unionOfIntervals (size+1, false);
      for (segments_t::const_iterator it = intervals.begin ();
	   it != intervals.end (); ++it) {
	for (size_type i=it->first; i < it->first + it->second; ++i) {
	  unionOfIntervals [i] = true;
	}
      }
      unionOfIntervals [size] = true;
      unsigned int state = 0;
      segment_t interval;
      for (size_type i=0; i <= (size_type) unionOfIntervals.size (); ++i) {
	if ((state == 0) && (unionOfIntervals [i] == false)) {
	  // start a new interval
	  state = 1;
	  interval.first = i;
	} else if ((state == 1) && unionOfIntervals [i] == true) {
	  // finish current interval
	  state = 0;
	  interval.second = i - interval.first;
	  result.push_back (interval);
	}
      }
    }

    inline ComparisonTypes_t defaultCompTypes (
        const segments_t& outputVelocity,
        const ComparisonTypes_t& comp)
    {
      if (comp.size() == 0) {
        size_type n = Eigen::BlockIndex::cardinal (outputVelocity);
        if (n > 0)
          return ComparisonTypes_t(n, constraints::EqualToZero);
      }
      return comp;
    }

    ExplicitPtr_t Explicit::create
    (const LiegroupSpacePtr_t& configSpace,
     const DifferentiableFunctionPtr_t& function,
     const segments_t& inputConf,
     const segments_t& outputConf,
     const segments_t& inputVelocity,
     const segments_t& outputVelocity,
     const ComparisonTypes_t& comp)
    {
      Explicit* ptr = new Explicit
	(configSpace, function, inputConf, outputConf, inputVelocity,
         outputVelocity, defaultCompTypes(outputVelocity,comp));
      ExplicitPtr_t shPtr (ptr);
      ExplicitWkPtr_t wkPtr (shPtr);
      ptr->init (wkPtr);
      return shPtr;
    }

    ExplicitPtr_t Explicit::createCopy
    (const ExplicitPtr_t& other)
    {
      Explicit* ptr = new Explicit
	(*other);
      ExplicitPtr_t shPtr (ptr);
      ExplicitWkPtr_t wkPtr (shPtr);
      ptr->init (wkPtr);
      return shPtr;
    }

    ImplicitPtr_t Explicit::copy () const
    {
      return createCopy (weak_.lock ());
    }


    Explicit::Explicit
    (const LiegroupSpacePtr_t& configSpace,
     const DifferentiableFunctionPtr_t& explicitFunction,
     const segments_t& inputConf,
     const segments_t& outputConf,
     const segments_t& inputVelocity,
     const segments_t& outputVelocity,
     const ComparisonTypes_t& comp) :
      Implicit (explicit_::ImplicitFunction::create
                (configSpace, explicitFunction, inputConf,
                 outputConf, inputVelocity, outputVelocity),
                comp),
      inputToOutput_ (explicitFunction),
      inputConf_ (inputConf),
      outputConf_ (outputConf),
      inputVelocity_ (inputVelocity),
      outputVelocity_ (outputVelocity)
    {
    }

    Explicit::Explicit
    (const Explicit& other) :
      Implicit (other), inputToOutput_ (other.inputToOutput_),
      inputConf_ (other.inputConf_), outputConf_ (other.outputConf_),
      inputVelocity_ (other.inputVelocity_),
      outputVelocity_ (other.outputVelocity_)
    {
    }
    void Explicit::init (const ExplicitWkPtr_t& weak)
    {
      Implicit::init (weak);
      weak_ = weak;
    }
  } // namespace constraints
} // namespace hpp
