# A ROS2 RMW implementation for Eclipse Cyclone DDS

With the code in this repository, it is possible to use [*ROS2*](https://index.ros.org/doc/ros2)
with [*Eclipse Cyclone DDS*](https://github.com/eclipse-cyclonedds/cyclonedds) as the underlying DDS
implementation.

## Getting, building and using it

All it takes to get Cyclone DDS support into ROS2 is to clone this repository and the Cyclone DDS
one in the ROS2 workspace source directory, and then run colcon build in the usual manner:

    cd ros2_ws/src
    git clone https://github.com/atolab/rmw_cyclonedds
    git clone https://github.com/eclipse-cyclonedds/cyclonedds
    cd ..
    colcon build --symlink-install --cmake-args -DCMAKE_BUILD_TYPE=RelWithDebInfo
    export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp

This seems to work fine on Linux with a binary ROS2 installation as well as when building ROS2 from
source.  On macOS it has only been tested in a source build on a machine in an "unsupported"
configuration (macOS 10.14 with SIP enabled, instead of 10.12 with SIP disabled), and apart from a
few details that are caused by the machine configuration, that works fine, too.  There is no reason
why it wouldn't work the same on Windows, but I haven't tried.

That said, Cyclone DDS has some prerequisites because it currently relies on Java and Maven to build
its IDL preprocessor, and so it is probably advisable to check its README for details. You can
install these with, ``rosdep install -i ros2_ws/src``.

If you want to use a pre-existing installation of Cyclone DDS, you don't need to clone it, but you
may have to tell CMake where to look for it using the CycloneDDS\_DIR variable.  That also appears
to be the case if there are other packages in the ROS2 workspace that you would like to use Cyclone
DDS directly instead of via the ROS2 abstraction.

## Known limitations

There are a number of known limitations:

* Cyclone DDS does not yet implement DDS Security.  Consequently, there is no support for security
  in this RMW implementation either.

* Deserialization only handles native format (it doesn't do any byte swapping).  This is pure
  laziness, adding it is trivial.
    
* Deserialization assumes the input is valid and will do terrible things if it isn't.  Again, pure
  laziness, it's just adding some bounds checks and other validation code.
