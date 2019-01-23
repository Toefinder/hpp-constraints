// Copyright (c) 2016, Joseph Mirabel
// Authors: Joseph Mirabel (joseph.mirabel@laas.fr)
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

#include <hpp/constraints/generic-transformation.hh>

#include <hpp/util/indent.hh>

#include <hpp/pinocchio/configuration.hh>
#include <hpp/pinocchio/device.hh>

#include <hpp/constraints/macros.hh>

#include <../src/generic-transformation/helper.hh>

namespace hpp {
  namespace constraints {
    void GenericTransformationModel<true>::setJoint1(const JointConstPtr_t& j)
    {
      if (j && j->index() > 0)
        joint1 = j;
      else joint1.reset();
      assert (!joint1 || joint1->index());
    }

    template <int _Options> std::ostream&
      GenericTransformation<_Options>::print (std::ostream& os) const
    {
      os << (IsRelative ? "Relative" : "") <<
            (ComputePosition ? (ComputeOrientation ? "Transformation" : "Position")
                   : "Orientation") << ": " << name()
        << ", active dof: "
        << pretty_print (BlockIndex::fromLogicalExpression (activeParameters_)) << incindent
        << iendl << "Joint1: "         << ((IsRelative && joint1()) ? joint1()->name() : "World")
        << iendl << "Frame in joint 1" << incindent << iendl; pinocchio::display(os, frame1InJoint1()) << decindent
        << iendl << "Joint2: "         << joint2()->name()
        << iendl << "Frame in joint 2" << incindent << iendl; pinocchio::display(os, frame2InJoint2()) << decindent
        << iendl << "mask: ";
      for (size_type i=0; i<ValueSize; ++i) os << mask_ [i] << ", ";
      return os << decindent;
    }

    template <int _Options> typename GenericTransformation<_Options>::Ptr_t
      GenericTransformation<_Options>::create
    (const std::string& name, const DevicePtr_t& robot,
     const JointConstPtr_t& joint2,
     const Transform3f& reference, std::vector <bool> mask)
    {
      GenericTransformation<_Options>* ptr =
        new GenericTransformation<_Options> (name, robot, mask);
      ptr->joint1 (JointConstPtr_t());
      ptr->joint2 (joint2);
      ptr->reference (reference);
      Ptr_t shPtr (ptr);
      ptr->init (shPtr);
      return shPtr;
    }

    template <int _Options> typename GenericTransformation<_Options>::Ptr_t
      GenericTransformation<_Options>::create
    (const std::string& name, const DevicePtr_t& robot,
     /* World frame          */ const JointConstPtr_t& joint2,
     const Transform3f& frame2, const Transform3f& frame1,
     std::vector <bool> mask)
    {
      GenericTransformation<_Options>* ptr =
        new GenericTransformation<_Options> (name, robot, mask);
      ptr->joint1 (JointConstPtr_t());
      ptr->joint2 (joint2);
      ptr->frame1InJoint1 (frame1);
      ptr->frame2InJoint2 (frame2);
      Ptr_t shPtr (ptr);
      return shPtr;
    }

    template <int _Options> typename GenericTransformation<_Options>::Ptr_t
      GenericTransformation<_Options>::create
    (const std::string& name, const DevicePtr_t& robot,
     const JointConstPtr_t& joint1, const JointConstPtr_t& joint2,
     const Transform3f& reference, std::vector <bool> mask)
    {
      GenericTransformation<_Options>* ptr =
        new GenericTransformation<_Options> (name, robot, mask);
      ptr->joint1 (joint1);
      ptr->joint2 (joint2);
      ptr->reference (reference);
      Ptr_t shPtr (ptr);
      ptr->init (shPtr);
      return shPtr;
    }

    template <int _Options> typename GenericTransformation<_Options>::Ptr_t
      GenericTransformation<_Options>::create
    (const std::string& name, const DevicePtr_t& robot,
     const JointConstPtr_t& joint1, const JointConstPtr_t& joint2,
     const Transform3f& frame1, const Transform3f& frame2,
     std::vector <bool> mask)
    {
      GenericTransformation<_Options>* ptr =
        new GenericTransformation<_Options> (name, robot, mask);
      ptr->joint1 (joint1);
      ptr->joint2 (joint2);
      ptr->frame1InJoint1 (frame1);
      ptr->frame2InJoint2 (frame2);
      Ptr_t shPtr (ptr);
      return shPtr;
    }

    template <int _Options>
      GenericTransformation<_Options>::GenericTransformation
      (const std::string& name, const DevicePtr_t& robot,
       std::vector <bool> mask) :
        DifferentiableFunction (robot->configSize (), robot->numberDof (),
                                LiegroupSpace::Rn (size (mask)), name),
        robot_ (robot),
        m_(robot->numberDof()-robot->extraConfigSpace().dimension()),
        mask_ (mask)
    {
      assert(mask.size()==ValueSize);
      std::size_t iOri = 0;
      m_.rowOri = 0;
      if (ComputePosition) {
        for (size_type i=0; i<3; ++i) if (mask_[i]) m_.rowOri++;
        m_.fullPos = (m_.rowOri==3);
        iOri = 3;
      } else m_.fullPos = false;
      if (ComputeOrientation)
        m_.fullOri = mask_[iOri + 0] && mask_[iOri + 1] && mask_[iOri + 2];
      else m_.fullOri = false;
    }

    template <int _Options>
    inline void GenericTransformation<_Options>::computeActiveParams ()
    {
      activeParameters_.setConstant (false);
      activeDerivativeParameters_.setConstant (false);

      setActiveParameters (robot_, joint1(), joint2(),
          activeParameters_, activeDerivativeParameters_);
    }

    template <int _Options>
    void GenericTransformation<_Options>::impl_compute
    (LiegroupElementRef result, ConfigurationIn_t argument) const throw ()
    {
      GTDataV<IsRelative, ComputePosition, ComputeOrientation> data (m_, robot_);

      data.device.currentConfiguration (argument);
      data.device.computeForwardKinematics ();
      compute<IsRelative, ComputePosition, ComputeOrientation>::error (data);

      size_type index=0;
      for (size_type i=0; i<ValueSize; ++i) {
	if (mask_ [i]) {
	  result.vector () [index] = data.value[i]; ++index;
	}
      }
    }

    template <int _Options>
    void GenericTransformation<_Options>::impl_jacobian
    (matrixOut_t jacobian, ConfigurationIn_t arg) const throw ()
    {
      // TODO there is still a little bit a memory which is dynamically
      // allocated in GTDataJ. At the moment, this allocation is necessary to
      // support multithreadind. To avoid it, DeviceData should provide some
      // a temporary buffer to pass to an Eigen::Map
      GTDataJ<IsRelative, ComputePosition, ComputeOrientation> data (m_, robot_);

      data.device.currentConfiguration (arg);
      data.device.computeForwardKinematics ();
      compute<IsRelative, ComputePosition, ComputeOrientation>::error (data);
      compute<IsRelative, ComputePosition, ComputeOrientation>::jacobian (data, jacobian, mask_);

#ifdef CHECK_JACOBIANS
      const value_type eps = std::sqrt(Eigen::NumTraits<value_type>::epsilon());
      matrix_t Jfd (outputDerivativeSize(), inputDerivativeSize());
      Jfd.setZero();
      finiteDifferenceCentral(Jfd, arg, robot_, eps);
      size_type row, col;
      value_type maxError = (jacobian - Jfd).cwiseAbs().maxCoeff(&row,&col);
      if (maxError > std::sqrt(eps)) {
        hppDout (error, "Jacobian of " << name() << " does not match central finite difference. "
            "DOF " << col << " at row " << row << ": "
            << maxError << " > " << /* HESSIAN_MAXIMUM_COEF << " * " << */ std::sqrt(eps)
            );
        hppDnum (error, "Jacobian is" << iendl << jacobian << iendl
            << "Finite diff is" << iendl << Jfd << iendl
            << "Difference is" << iendl << (jacobian - Jfd));
      }
#endif
    }

    /// Force instanciation of relevant classes
    template class GenericTransformation<               PositionBit | OrientationBit >;
    template class GenericTransformation<               PositionBit                  >;
    template class GenericTransformation<                             OrientationBit >;
    template class GenericTransformation< RelativeBit | PositionBit | OrientationBit >;
    template class GenericTransformation< RelativeBit | PositionBit                  >;
    template class GenericTransformation< RelativeBit |               OrientationBit >;
  } // namespace constraints
} // namespace hpp
