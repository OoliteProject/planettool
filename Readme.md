# PlanetTool for Oolite
PlanetTool is a command-line utility and a Mac application for working with
planet textures, in particular converting between projections.

For documentation for the command-line version, use `planettool --help`.
For more information, see http://aegidian.org/bb/viewtopic.php?f=4&t=7843

## Building planettool
### From the repository
To build from the repository, planettool must be checked out within the Oolite
repository, at tools/planettool. This is because it uses Oolite’s maths
library, and (on Mac OS X) Oolite's libpng Xcode project.

* For Mac OS X, the Xcode project is the preferred way to build. The Xcode
  project is also the only supported way to build the GUI version of
  Planet Tool. The Xcode project assumes it's part of a full Oolite checkout;
  in particular, it expects Oolite’s libpng project to be in place, and uses
  the shared build directory.
* For other platforms, use `make inrepo=yes`.

### Stand-alone distribution
* Use `make`. (The Xcode project is not included with the stand-alone
  distribution.)

### Troubleshooting
* If you have build problems related to the `vasprintf` function, add the
  option `asprintf=no` to the `make` command.
* If you have build problems related to the file `<pthread.h>` or functions
  beginning with `pthread`, use the add the option `scheduer=SerialScheduler`
  to the `make` command. **Note:** if built this way, planettool will only use
  one processor core.
* If you are unsure whether planettool is functioning correctly, there is a
  [test suite](http://jens.ayton.se/oolite/tools/planettool/planettool-test-suite.zip)
  and [reference renderings](http://jens.ayton.se/oolite/tools/planettool/planettool-test-suite-reference-renderings.zip).
