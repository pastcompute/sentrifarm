from platformio.platforms.base import BasePlatform

import os

# Copy this file into $PLATFORMIO_HOME_DIR/platforms (home_dir)

class Openwrtcarambola2Platform(BasePlatform):
    # This is a description of your platform.
    # Platformio uses it for the `platformio search / list` commands

    """
        My Test platform - openwrtcarambola2.py
    """

    PACKAGES = {
    }

    def get_build_script(self):
        """ Returns a path to build script """

        # You can return static path
        # return "/path/to/test-builder.py"

        # or detect dynamically if `test-builder.py` is located in the same
        # folder with `test.py`
        return os.path.join(
            os.path.dirname(os.path.realpath(__file__)),
            "openwrtcarambola2-builder.py"
        )

