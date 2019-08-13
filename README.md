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
its IDL preprocessor, and so it is probably advisable to check its README for details.  On an Ubuntu
18.04 system, ``sudo apt-get install maven default-jdk`` will likely address that.

If you want to use a pre-existing installation of Cyclone DDS, you don't need to clone it, but you
may have to tell CMake where to look for it using the CycloneDDS\_DIR variable.  That also appears
to be the case if there are other packages in the ROS2 workspace that you would like to use Cyclone
DDS directly instead of via the ROS2 abstraction.

## Known limitations

There are a number of known limitations:

* Cyclone DDS does not yet implement DDS Security.  Consequently, there is no support for security
  in this RMW implementation either.

* Cyclone DDS does not allow creating a waitset or a guard condition outside a participant, and this
  forces the creation of an additional participant.  It can be fixed in the RMW layer, or it can be
  dealt with in Cyclone DDS, but the trouble with the latter is that there are solid reasons for not
  allowing it, even if it is easy to support it today.  (E.g., a remote procedure call interface
  ...)
    
* Cyclone DDS does not currently support multiple domains simultaneously (waiting in a PR for the
  final polish), and so this RMW implementation ignores the domain\_id parameter in create\_node,
  instead creating all nodes/participants (including the special participant mentioned above) in the
  default domain, which can be controlled via CYCLONEDDS\_URI.
    
* Deserialization only handles native format (it doesn't do any byte swapping).  This is pure
  laziness, adding it is trivial.
    
* Deserialization assumes the input is valid and will do terrible things if it isn't.  Again, pure
  laziness, it's just adding some bounds checks and other validation code.
    
* There are some "oddities" with the way service requests and replies are serialized and what it
  uses as a "GUID".  (It actually uses an almost-certainly-unique 64-bit number, the Cyclone DDS
  instance id, instead of a real GUID.)  I'm pretty sure the format is wildly different from that in
  other RMW implementations, and so services presumably will not function cross-implementation.
    
* The name mangling seems to be compatibl-ish with the FastRTPS implementation and in some cases
  using the ros2 CLI for querying the system works cross-implementation, but not always.  The one in
  this implementation is reverse-engineered, so trouble may be lurking somewhere.  As a related
  point: the "no_demangle" option is currently ignored ... it causes a compiler warning.
